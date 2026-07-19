#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "shiplua/actor/ActorProvider.h"
#include "shiplua/handles/HandleRegistry.h"

namespace ShipLua {

struct MockActorRecord {
  Handle handle;
  std::string ownerModId;
  ActorSpawnRequest request;
};

// Deterministic, ROM-free actor provider used by tests and the CLI mock host.
class MockActorProvider final : public ActorProvider {
public:
  explicit MockActorProvider(std::string gameId,
                             HandleLimits limits = {256, 1024});

  Result<Handle> Spawn(const std::string &ownerModId,
                       const ActorSpawnRequest &request) override;
  Result<void> Destroy(const std::string &ownerModId,
                       const Handle &handle) override;
  Result<bool> Exists(const std::string &ownerModId,
                      const Handle &handle) const override;
  Result<std::size_t> ReleaseMod(const std::string &modId) override;

  std::size_t InvalidateScene();
  std::size_t Count() const noexcept { return mRecords.size(); }
  std::size_t CountForMod(const std::string &modId) const;
  std::vector<MockActorRecord> Records() const;
  const HandleRegistry &Handles() const noexcept { return mHandles; }

private:
  bool IsAllowed(const std::string &actor) const;

  std::string mGameId;
  HandleRegistry mHandles;
  std::map<std::uint32_t, MockActorRecord> mRecords;
};

} // namespace ShipLua
