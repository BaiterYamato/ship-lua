#include "shiplua/world/WorldHandoff.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>

#include "shiplua/storage/AtomicFile.h"

namespace ShipLua {
namespace {

constexpr std::array<std::byte, 8> kMagic = {
    std::byte{'S'}, std::byte{'H'}, std::byte{'L'}, std::byte{'W'},
    std::byte{'R'}, std::byte{'L'}, std::byte{'D'}, std::byte{0},
};
constexpr std::size_t kHeaderSize = 16;
constexpr std::size_t kMaximumStringSize = 4096;
constexpr std::size_t kMaximumItems = 256;

class Sha256 {
public:
  void Update(std::span<const std::byte> input) {
    for (const std::byte value : input) {
      mBuffer[mBuffered++] = std::to_integer<std::uint8_t>(value);
      ++mTotalBytes;
      if (mBuffered == mBuffer.size()) {
        Transform(mBuffer.data());
        mBuffered = 0;
      }
    }
  }

  std::array<std::byte, 32> Final() {
    const std::uint64_t bitLength = mTotalBytes * 8;
    mBuffer[mBuffered++] = 0x80;
    if (mBuffered > 56) {
      std::fill(mBuffer.begin() + static_cast<std::ptrdiff_t>(mBuffered),
                mBuffer.end(), 0);
      Transform(mBuffer.data());
      mBuffered = 0;
    }
    std::fill(mBuffer.begin() + static_cast<std::ptrdiff_t>(mBuffered),
              mBuffer.begin() + 56, 0);
    for (std::size_t index = 0; index < 8; ++index) {
      mBuffer[56 + index] =
          static_cast<std::uint8_t>(bitLength >> ((7 - index) * 8));
    }
    Transform(mBuffer.data());

    std::array<std::byte, 32> digest{};
    for (std::size_t word = 0; word < mState.size(); ++word) {
      for (std::size_t byte = 0; byte < 4; ++byte) {
        digest[word * 4 + byte] =
            static_cast<std::byte>(mState[word] >> ((3 - byte) * 8));
      }
    }
    return digest;
  }

private:
  static constexpr std::array<std::uint32_t, 64> kRoundConstants = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
  };

  static std::uint32_t LoadBigEndian(const std::uint8_t *input) {
    return (static_cast<std::uint32_t>(input[0]) << 24) |
           (static_cast<std::uint32_t>(input[1]) << 16) |
           (static_cast<std::uint32_t>(input[2]) << 8) |
           static_cast<std::uint32_t>(input[3]);
  }

  void Transform(const std::uint8_t *block) {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
      words[index] = LoadBigEndian(block + index * 4);
    }
    for (std::size_t index = 16; index < words.size(); ++index) {
      const std::uint32_t s0 = std::rotr(words[index - 15], 7) ^
                               std::rotr(words[index - 15], 18) ^
                               (words[index - 15] >> 3);
      const std::uint32_t s1 = std::rotr(words[index - 2], 17) ^
                               std::rotr(words[index - 2], 19) ^
                               (words[index - 2] >> 10);
      words[index] = words[index - 16] + s0 + words[index - 7] + s1;
    }

    std::uint32_t a = mState[0];
    std::uint32_t b = mState[1];
    std::uint32_t c = mState[2];
    std::uint32_t d = mState[3];
    std::uint32_t e = mState[4];
    std::uint32_t f = mState[5];
    std::uint32_t g = mState[6];
    std::uint32_t h = mState[7];
    for (std::size_t index = 0; index < words.size(); ++index) {
      const std::uint32_t sum1 =
          std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
      const std::uint32_t choose = (e & f) ^ (~e & g);
      const std::uint32_t temp1 =
          h + sum1 + choose + kRoundConstants[index] + words[index];
      const std::uint32_t sum0 =
          std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
      const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = sum0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }
    mState[0] += a;
    mState[1] += b;
    mState[2] += c;
    mState[3] += d;
    mState[4] += e;
    mState[5] += f;
    mState[6] += g;
    mState[7] += h;
  }

  std::array<std::uint32_t, 8> mState = {
      0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
  };
  std::array<std::uint8_t, 64> mBuffer{};
  std::size_t mBuffered = 0;
  std::uint64_t mTotalBytes = 0;
};

std::array<std::byte, 32> Hash(std::span<const std::byte> input) {
  Sha256 hash;
  hash.Update(input);
  return hash.Final();
}

std::array<std::byte, 32> Authenticate(std::span<const std::byte> key,
                                       std::span<const std::byte> message) {
  std::array<std::byte, 64> normalizedKey{};
  if (key.size() > normalizedKey.size()) {
    const auto digest = Hash(key);
    std::copy(digest.begin(), digest.end(), normalizedKey.begin());
  } else {
    std::copy(key.begin(), key.end(), normalizedKey.begin());
  }

  std::array<std::byte, 64> innerPad{};
  std::array<std::byte, 64> outerPad{};
  for (std::size_t index = 0; index < normalizedKey.size(); ++index) {
    innerPad[index] = normalizedKey[index] ^ std::byte{0x36};
    outerPad[index] = normalizedKey[index] ^ std::byte{0x5c};
  }
  Sha256 inner;
  inner.Update(innerPad);
  inner.Update(message);
  const auto innerDigest = inner.Final();
  Sha256 outer;
  outer.Update(outerPad);
  outer.Update(innerDigest);
  return outer.Final();
}

bool ConstantTimeEqual(std::span<const std::byte> left,
                       std::span<const std::byte> right) noexcept {
  if (left.size() != right.size()) {
    return false;
  }
  std::uint8_t difference = 0;
  for (std::size_t index = 0; index < left.size(); ++index) {
    difference |= std::to_integer<std::uint8_t>(left[index] ^ right[index]);
  }
  return difference == 0;
}

class Writer {
public:
  void U8(std::uint8_t value) {
    bytes.push_back(static_cast<std::byte>(value));
  }
  void U16(std::uint16_t value) {
    U8(static_cast<std::uint8_t>(value));
    U8(static_cast<std::uint8_t>(value >> 8));
  }
  void U32(std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index)
      U8(static_cast<std::uint8_t>(value >> (index * 8)));
  }
  void U64(std::uint64_t value) {
    for (std::size_t index = 0; index < 8; ++index)
      U8(static_cast<std::uint8_t>(value >> (index * 8)));
  }
  void Raw(std::span<const std::byte> value) {
    bytes.insert(bytes.end(), value.begin(), value.end());
  }
  void String(const std::string &value) {
    U32(static_cast<std::uint32_t>(value.size()));
    Raw(std::as_bytes(std::span(value.data(), value.size())));
  }
  std::vector<std::byte> bytes;
};

class Reader {
public:
  explicit Reader(std::span<const std::byte> input) : mInput(input) {}

  bool U8(std::uint8_t &value) {
    if (Remaining() < 1)
      return false;
    value = std::to_integer<std::uint8_t>(mInput[mOffset++]);
    return true;
  }
  bool U16(std::uint16_t &value) {
    std::uint8_t low = 0, high = 0;
    if (!U8(low) || !U8(high))
      return false;
    value = static_cast<std::uint16_t>(low |
                                       (static_cast<std::uint16_t>(high) << 8));
    return true;
  }
  bool U32(std::uint32_t &value) {
    value = 0;
    for (std::size_t index = 0; index < 4; ++index) {
      std::uint8_t byte = 0;
      if (!U8(byte))
        return false;
      value |= static_cast<std::uint32_t>(byte) << (index * 8);
    }
    return true;
  }
  bool U64(std::uint64_t &value) {
    value = 0;
    for (std::size_t index = 0; index < 8; ++index) {
      std::uint8_t byte = 0;
      if (!U8(byte))
        return false;
      value |= static_cast<std::uint64_t>(byte) << (index * 8);
    }
    return true;
  }
  bool Raw(std::span<std::byte> output) {
    if (Remaining() < output.size())
      return false;
    std::copy_n(mInput.begin() + static_cast<std::ptrdiff_t>(mOffset),
                output.size(), output.begin());
    mOffset += output.size();
    return true;
  }
  bool String(std::string &value) {
    std::uint32_t size = 0;
    if (!U32(size) || size > kMaximumStringSize || Remaining() < size)
      return false;
    const char *begin = reinterpret_cast<const char *>(mInput.data() + mOffset);
    value.assign(begin, begin + size);
    mOffset += size;
    return true;
  }
  std::size_t Remaining() const { return mInput.size() - mOffset; }

private:
  std::span<const std::byte> mInput;
  std::size_t mOffset = 0;
};

bool IsValidWorld(WorldId world) noexcept {
  return world == WorldId::Oot || world == WorldId::Mm;
}

std::uint8_t EncodeWorld(WorldId world) { return world == WorldId::Mm ? 1 : 0; }

bool DecodeWorld(std::uint8_t encoded, WorldId &world) {
  if (encoded > 1)
    return false;
  world = encoded == 0 ? WorldId::Oot : WorldId::Mm;
  return true;
}

Result<void> ValidateHandoff(const WorldHandoff &handoff) {
  if (handoff.sequence == 0) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "world handoff sequence must be positive");
  }
  if (std::all_of(handoff.sessionId.begin(), handoff.sessionId.end(),
                  [](std::byte value) { return value == std::byte{0}; })) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "world handoff session id cannot be zero");
  }
  if (!IsValidWorld(handoff.source) ||
      !IsValidWorld(handoff.destination.world)) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "world handoff contains an unknown world");
  }
  const auto destination = ValidateWorldDestination(handoff.destination);
  if (!destination.isOk())
    return destination;
  const auto player = ValidatePortablePlayerState(handoff.player);
  if (!player.isOk())
    return player;
  for (const PortableItem &item : handoff.player.items) {
    if (!IsValidWorld(item.origin) ||
        (item.visualAsset && !IsValidWorld(item.visualAsset->owner))) {
      return Result<void>::err(ErrorCode::InvalidArgument,
                               "world handoff item contains an unknown world");
    }
    if (item.id.size() > kMaximumStringSize ||
        (item.visualAsset &&
         item.visualAsset->id.size() > kMaximumStringSize)) {
      return Result<void>::err(ErrorCode::ResourceLimit,
                               "world handoff string exceeds limit");
    }
  }
  if (handoff.destination.id.size() > kMaximumStringSize) {
    return Result<void>::err(ErrorCode::ResourceLimit,
                             "world handoff string exceeds limit");
  }
  return Result<void>::ok();
}

Result<void> ValidateKey(std::span<const std::byte> key) {
  if (key.size() < WorldHandoffCodec::MinimumKeySize) {
    return Result<void>::err(ErrorCode::InvalidArgument,
                             "world handoff authentication key is too short");
  }
  return Result<void>::ok();
}

Result<WorldHandoff>
ParsePayload(std::span<const std::byte> payload,
             std::optional<std::uint64_t> lastAcceptedSequence) {
  Reader reader(payload);
  WorldHandoff handoff;
  std::uint8_t source = 0, destination = 0;
  if (!reader.Raw(handoff.sessionId) || !reader.U64(handoff.sequence) ||
      !reader.U8(source) || !reader.U8(destination) ||
      !DecodeWorld(source, handoff.source) ||
      !DecodeWorld(destination, handoff.destination.world) ||
      !reader.String(handoff.destination.id) ||
      !reader.U16(handoff.player.health) ||
      !reader.U16(handoff.player.healthCapacity) ||
      !reader.U32(handoff.player.rupees)) {
    return Result<WorldHandoff>::err(ErrorCode::InvalidArgument,
                                     "world handoff payload is malformed");
  }
  std::uint32_t itemCount = 0;
  if (!reader.U32(itemCount) || itemCount > kMaximumItems) {
    return Result<WorldHandoff>::err(ErrorCode::ResourceLimit,
                                     "world handoff item count exceeds limit");
  }
  handoff.player.items.reserve(itemCount);
  for (std::uint32_t index = 0; index < itemCount; ++index) {
    PortableItem item;
    std::uint8_t origin = 0, equipped = 0, hasAsset = 0;
    if (!reader.String(item.id) || !reader.U8(origin) ||
        !DecodeWorld(origin, item.origin) || !reader.U32(item.quantity) ||
        !reader.U8(equipped) || equipped > 1 || !reader.U8(hasAsset) ||
        hasAsset > 1) {
      return Result<WorldHandoff>::err(ErrorCode::InvalidArgument,
                                       "world handoff item is malformed");
    }
    item.equipped = equipped != 0;
    if (hasAsset != 0) {
      AssetReference asset;
      std::uint8_t owner = 0;
      if (!reader.U8(owner) || !DecodeWorld(owner, asset.owner) ||
          !reader.String(asset.id)) {
        return Result<WorldHandoff>::err(ErrorCode::InvalidArgument,
                                         "world handoff asset is malformed");
      }
      item.visualAsset = std::move(asset);
    }
    handoff.player.items.push_back(std::move(item));
  }
  if (reader.Remaining() != 0) {
    return Result<WorldHandoff>::err(
        ErrorCode::InvalidArgument, "world handoff payload has trailing bytes");
  }
  const auto valid = ValidateHandoff(handoff);
  if (!valid.isOk())
    return Result<WorldHandoff>::err(valid.code, valid.message);
  if (lastAcceptedSequence && handoff.sequence <= *lastAcceptedSequence) {
    return Result<WorldHandoff>::err(
        ErrorCode::InvalidState, "world handoff sequence was already accepted");
  }
  return Result<WorldHandoff>::ok(std::move(handoff));
}

} // namespace

Result<std::vector<std::byte>>
WorldHandoffCodec::Serialize(const WorldHandoff &handoff,
                             std::span<const std::byte> authenticationKey) {
  const auto key = ValidateKey(authenticationKey);
  if (!key.isOk())
    return Result<std::vector<std::byte>>::err(key.code, key.message);
  const auto valid = ValidateHandoff(handoff);
  if (!valid.isOk())
    return Result<std::vector<std::byte>>::err(valid.code, valid.message);

  Writer payload;
  payload.Raw(handoff.sessionId);
  payload.U64(handoff.sequence);
  payload.U8(EncodeWorld(handoff.source));
  payload.U8(EncodeWorld(handoff.destination.world));
  payload.String(handoff.destination.id);
  payload.U16(handoff.player.health);
  payload.U16(handoff.player.healthCapacity);
  payload.U32(handoff.player.rupees);
  payload.U32(static_cast<std::uint32_t>(handoff.player.items.size()));
  for (const PortableItem &item : handoff.player.items) {
    payload.String(item.id);
    payload.U8(EncodeWorld(item.origin));
    payload.U32(item.quantity);
    payload.U8(item.equipped ? 1 : 0);
    payload.U8(item.visualAsset ? 1 : 0);
    if (item.visualAsset) {
      payload.U8(EncodeWorld(item.visualAsset->owner));
      payload.String(item.visualAsset->id);
    }
  }
  if (payload.bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
    return Result<std::vector<std::byte>>::err(
        ErrorCode::ResourceLimit, "world handoff payload exceeds limit");
  }

  Writer envelope;
  envelope.Raw(kMagic);
  envelope.U16(FormatVersion);
  envelope.U16(0);
  envelope.U32(static_cast<std::uint32_t>(payload.bytes.size()));
  envelope.Raw(payload.bytes);
  if (envelope.bytes.size() + AuthenticationTagSize > MaximumEnvelopeSize) {
    return Result<std::vector<std::byte>>::err(
        ErrorCode::ResourceLimit, "world handoff envelope exceeds limit");
  }
  const auto tag = Authenticate(authenticationKey, envelope.bytes);
  envelope.Raw(tag);
  return Result<std::vector<std::byte>>::ok(std::move(envelope.bytes));
}

Result<WorldHandoff> WorldHandoffCodec::Deserialize(
    std::span<const std::byte> envelope,
    std::span<const std::byte> authenticationKey,
    std::optional<std::uint64_t> lastAcceptedSequence) {
  const auto key = ValidateKey(authenticationKey);
  if (!key.isOk())
    return Result<WorldHandoff>::err(key.code, key.message);
  if (envelope.size() < kHeaderSize + AuthenticationTagSize ||
      envelope.size() > MaximumEnvelopeSize) {
    return Result<WorldHandoff>::err(ErrorCode::ResourceLimit,
                                     "world handoff envelope size is invalid");
  }
  const auto authenticated =
      envelope.first(envelope.size() - AuthenticationTagSize);
  const auto receivedTag = envelope.last(AuthenticationTagSize);
  const auto expectedTag = Authenticate(authenticationKey, authenticated);
  if (!ConstantTimeEqual(receivedTag, expectedTag)) {
    return Result<WorldHandoff>::err(ErrorCode::PermissionDenied,
                                     "world handoff authentication failed");
  }

  Reader reader(authenticated);
  std::array<std::byte, 8> magic{};
  std::uint16_t version = 0, flags = 0;
  std::uint32_t payloadSize = 0;
  if (!reader.Raw(magic) || magic != kMagic || !reader.U16(version) ||
      !reader.U16(flags) || !reader.U32(payloadSize)) {
    return Result<WorldHandoff>::err(ErrorCode::InvalidArgument,
                                     "world handoff header is malformed");
  }
  if (version != FormatVersion || flags != 0) {
    return Result<WorldHandoff>::err(ErrorCode::Unsupported,
                                     "world handoff format is unsupported");
  }
  if (payloadSize != reader.Remaining()) {
    return Result<WorldHandoff>::err(
        ErrorCode::InvalidArgument,
        "world handoff payload size does not match envelope");
  }
  return ParsePayload(authenticated.subspan(kHeaderSize, payloadSize),
                      lastAcceptedSequence);
}

Result<void>
WorldHandoffCodec::WriteFile(const std::filesystem::path &destination,
                             const WorldHandoff &handoff,
                             std::span<const std::byte> authenticationKey) {
  auto serialized = Serialize(handoff, authenticationKey);
  if (!serialized.isOk())
    return Result<void>::err(serialized.code, serialized.message);
  return AtomicFile::Write(destination, *serialized.value);
}

Result<WorldHandoff>
WorldHandoffCodec::ReadFile(const std::filesystem::path &source,
                            std::span<const std::byte> authenticationKey,
                            std::optional<std::uint64_t> lastAcceptedSequence) {
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(source, error);
  if (error) {
    return Result<WorldHandoff>::err(ErrorCode::HostFailure,
                                     "cannot inspect world handoff file");
  }
  if (size > MaximumEnvelopeSize) {
    return Result<WorldHandoff>::err(ErrorCode::ResourceLimit,
                                     "world handoff file exceeds limit");
  }
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  std::ifstream input(source, std::ios::binary);
  if (!input || (!bytes.empty() &&
                 !input.read(reinterpret_cast<char *>(bytes.data()),
                             static_cast<std::streamsize>(bytes.size())))) {
    return Result<WorldHandoff>::err(ErrorCode::HostFailure,
                                     "cannot read world handoff file");
  }
  return Deserialize(bytes, authenticationKey, lastAcceptedSequence);
}

} // namespace ShipLua
