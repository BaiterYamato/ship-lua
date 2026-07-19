#include "shiplua/mock/MockActorProvider.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ShipLua {

MockActorProvider::MockActorProvider(std::string gameId, HandleLimits limits)
    : mGameId(std::move(gameId)), mHandles(mGameId == "oot" ? 1 : 2, limits) {}

bool MockActorProvider::IsAllowed(const std::string &actor) const {
  if (mGameId == "oot") {
    return actor == "oot.en_dog" || actor == "oot.en_torch2";
  }
  if (mGameId == "mm") {
    return actor == "mm.en_dg";
  }
  return false;
}

Result<Handle> MockActorProvider::Spawn(const std::string &ownerModId,
                                        const ActorSpawnRequest &request) {
  if (ownerModId.empty()) {
    return Result<Handle>::err(ErrorCode::InvalidArgument,
                               "actor owner mod id cannot be empty");
  }
  if (!IsAllowed(request.actor)) {
    return Result<Handle>::err(ErrorCode::Unsupported,
                               "actor '" + request.actor +
                                   "' is not in the mock allowlist for " +
                                   mGameId);
  }
  if (!std::isfinite(request.x) || !std::isfinite(request.y) ||
      !std::isfinite(request.z) || !std::isfinite(request.rotationX) ||
      !std::isfinite(request.rotationY) || !std::isfinite(request.rotationZ)) {
    return Result<Handle>::err(ErrorCode::InvalidArgument,
                               "actor transform must contain finite numbers");
  }
  auto handle = mHandles.Create(HandleKind::Actor, ownerModId);
  if (!handle.isOk()) {
    return handle;
  }
  mRecords.emplace(handle.value->slot,
                   MockActorRecord{*handle.value, ownerModId, request});
  return handle;
}

Result<void> MockActorProvider::Destroy(const std::string &ownerModId,
                                        const Handle &handle) {
  auto valid = mHandles.Validate(handle, HandleKind::Actor, ownerModId);
  if (!valid.isOk()) {
    return valid;
  }
  auto record = mRecords.find(handle.slot);
  if (record == mRecords.end() || !(record->second.handle == handle)) {
    return Result<void>::err(ErrorCode::InvalidHandle,
                             "actor handle has no mock record");
  }
  mRecords.erase(record);
  return mHandles.Destroy(handle, ownerModId);
}

Result<bool> MockActorProvider::Exists(const std::string &ownerModId,
                                       const Handle &handle) const {
  auto valid = mHandles.Validate(handle, HandleKind::Actor, ownerModId);
  if (!valid.isOk()) {
    return Result<bool>::err(valid.code, valid.message);
  }
  const auto record = mRecords.find(handle.slot);
  if (record == mRecords.end() || !(record->second.handle == handle)) {
    return Result<bool>::err(ErrorCode::InvalidHandle,
                             "actor handle has no mock record");
  }
  return Result<bool>::ok(true);
}

Result<std::size_t> MockActorProvider::ReleaseMod(const std::string &modId) {
  if (modId.empty()) {
    return Result<std::size_t>::err(ErrorCode::InvalidArgument,
                                    "mod id cannot be empty");
  }
  for (auto record = mRecords.begin(); record != mRecords.end();) {
    if (record->second.ownerModId == modId) {
      record = mRecords.erase(record);
    } else {
      ++record;
    }
  }
  return Result<std::size_t>::ok(mHandles.ReleaseMod(modId));
}

std::size_t MockActorProvider::InvalidateScene() {
  mRecords.clear();
  return mHandles.InvalidateScene();
}

std::size_t MockActorProvider::CountForMod(const std::string &modId) const {
  return static_cast<std::size_t>(std::count_if(
      mRecords.begin(), mRecords.end(), [&modId](const auto &entry) {
        return entry.second.ownerModId == modId;
      }));
}

std::vector<MockActorRecord> MockActorProvider::Records() const {
  std::vector<MockActorRecord> records;
  records.reserve(mRecords.size());
  for (const auto &[slot, record] : mRecords) {
    (void)slot;
    records.push_back(record);
  }
  return records;
}

} // namespace ShipLua
