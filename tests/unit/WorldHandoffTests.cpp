#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "shiplua/world/WorldHandoff.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

std::array<std::byte, 32> Key(std::uint8_t offset = 0) {
  std::array<std::byte, 32> key{};
  for (std::size_t index = 0; index < key.size(); ++index) {
    key[index] = static_cast<std::byte>(index + offset);
  }
  return key;
}

ShipLua::WorldHandoff Example(std::uint64_t sequence = 42) {
  ShipLua::WorldHandoff handoff;
  for (std::size_t index = 0; index < handoff.sessionId.size(); ++index) {
    handoff.sessionId[index] = static_cast<std::byte>(index + 1);
  }
  handoff.sequence = sequence;
  handoff.source = ShipLua::WorldId::Oot;
  handoff.destination = {ShipLua::WorldId::Mm, "mm.clock_town"};
  handoff.player.health = 32;
  handoff.player.healthCapacity = 48;
  handoff.player.rupees = 125;
  handoff.player.items = {
      {"shared.bow", ShipLua::WorldId::Oot, 20, true,
       ShipLua::AssetReference{ShipLua::WorldId::Oot, "oot.item.bow"}},
      {"mm.mask.deku", ShipLua::WorldId::Mm, 1, false, std::nullopt},
  };
  return handoff;
}

std::string Hex(std::span<const std::byte> bytes) {
  constexpr char digits[] = "0123456789abcdef";
  std::string output;
  output.reserve(bytes.size() * 2);
  for (const std::byte value : bytes) {
    const std::uint8_t byte = std::to_integer<std::uint8_t>(value);
    output.push_back(digits[byte >> 4]);
    output.push_back(digits[byte & 0x0f]);
  }
  return output;
}

void TestRoundTripAndKnownTag() {
  const auto key = Key();
  const auto handoff = Example();
  auto encoded = ShipLua::WorldHandoffCodec::Serialize(handoff, key);
  Check(encoded.isOk(), "envelope válido deve serializar");
  if (!encoded.isOk())
    return;

  const auto tag = std::span(*encoded.value)
                       .last(ShipLua::WorldHandoffCodec::AuthenticationTagSize);
  Check(Hex(tag) ==
            "a62af9c11a07ce1d23596913d6cad4904c77671fecc4220e0a9f42f89045b136",
        "tag HMAC-SHA-256 deve coincidir com vetor determinístico");

  auto decoded = ShipLua::WorldHandoffCodec::Deserialize(*encoded.value, key);
  Check(decoded.isOk(), "envelope autenticado deve desserializar");
  Check(decoded.isOk() && *decoded.value == handoff,
        "round-trip deve preservar todo o handoff");
}

void TestAuthenticationAndReplay() {
  const auto key = Key();
  auto encoded = ShipLua::WorldHandoffCodec::Serialize(Example(), key);
  Check(encoded.isOk(), "fixture de autenticação deve serializar");
  if (!encoded.isOk())
    return;

  auto wrongKey =
      ShipLua::WorldHandoffCodec::Deserialize(*encoded.value, Key(1));
  Check(!wrongKey.isOk() &&
            wrongKey.code == ShipLua::ErrorCode::PermissionDenied,
        "chave incorreta deve falhar autenticação");

  auto tampered = *encoded.value;
  tampered[24] ^= std::byte{0x01};
  auto changed = ShipLua::WorldHandoffCodec::Deserialize(tampered, key);
  Check(!changed.isOk() && changed.code == ShipLua::ErrorCode::PermissionDenied,
        "payload adulterado deve falhar autenticação antes do parse");

  auto truncated = *encoded.value;
  truncated.pop_back();
  auto shortEnvelope = ShipLua::WorldHandoffCodec::Deserialize(truncated, key);
  Check(!shortEnvelope.isOk(), "envelope truncado deve ser rejeitado");

  auto replay =
      ShipLua::WorldHandoffCodec::Deserialize(*encoded.value, key, 42);
  Check(!replay.isOk() && replay.code == ShipLua::ErrorCode::InvalidState,
        "sequência já aceita deve ser rejeitada como replay");
  Check(ShipLua::WorldHandoffCodec::Deserialize(*encoded.value, key, 41).isOk(),
        "sequência estritamente maior deve ser aceita");
}

void TestValidation() {
  auto handoff = Example();
  handoff.sessionId.fill(std::byte{0});
  Check(!ShipLua::WorldHandoffCodec::Serialize(handoff, Key()).isOk(),
        "id de sessão zero deve ser rejeitado");

  const std::array<std::byte, 16> shortKey{};
  Check(!ShipLua::WorldHandoffCodec::Serialize(Example(), shortKey).isOk(),
        "chave curta deve ser rejeitada");
}

void TestAtomicFileRoundTrip() {
  const auto root =
      std::filesystem::temp_directory_path() / "shiplua-world-handoff-tests";
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);
  std::filesystem::create_directories(root);
  const auto path = root / "handoff.bin";
  const auto key = Key();

  Check(ShipLua::WorldHandoffCodec::WriteFile(path, Example(7), key).isOk(),
        "publicação atômica inicial deve funcionar");
  Check(ShipLua::WorldHandoffCodec::WriteFile(path, Example(8), key).isOk(),
        "publicação atômica deve substituir envelope anterior");
  auto loaded = ShipLua::WorldHandoffCodec::ReadFile(path, key, 7);
  Check(loaded.isOk() && loaded.value->sequence == 8,
        "leitura deve observar somente o envelope novo completo");

  std::filesystem::remove_all(root, ignored);
}

} // namespace

int main() {
  TestRoundTripAndKnownTag();
  TestAuthenticationAndReplay();
  TestValidation();
  TestAtomicFileRoundTrip();
  return failures == 0 ? 0 : 1;
}
