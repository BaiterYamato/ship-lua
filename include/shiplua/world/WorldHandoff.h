#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "shiplua/runtime/Result.h"
#include "shiplua/world/WorldSession.h"

namespace ShipLua {

struct WorldHandoff {
  std::array<std::byte, 16> sessionId{};
  std::uint64_t sequence = 0;
  WorldId source = WorldId::Oot;
  WorldDestination destination;
  PortablePlayerState player;

  bool operator==(const WorldHandoff &) const = default;
};

class WorldHandoffCodec {
public:
  static constexpr std::uint16_t FormatVersion = 1;
  static constexpr std::size_t AuthenticationTagSize = 32;
  static constexpr std::size_t MinimumKeySize = 32;
  static constexpr std::size_t MaximumEnvelopeSize = 4 * 1024 * 1024;

  static Result<std::vector<std::byte>>
  Serialize(const WorldHandoff &handoff,
            std::span<const std::byte> authenticationKey);

  static Result<WorldHandoff>
  Deserialize(std::span<const std::byte> envelope,
              std::span<const std::byte> authenticationKey,
              std::optional<std::uint64_t> lastAcceptedSequence = std::nullopt);

  static Result<void> WriteFile(const std::filesystem::path &destination,
                                const WorldHandoff &handoff,
                                std::span<const std::byte> authenticationKey);

  static Result<WorldHandoff>
  ReadFile(const std::filesystem::path &source,
           std::span<const std::byte> authenticationKey,
           std::optional<std::uint64_t> lastAcceptedSequence = std::nullopt);
};

} // namespace ShipLua
