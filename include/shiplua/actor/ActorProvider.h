#pragma once

#include <cstddef>
#include <string>

#include "shiplua/handles/HandleRegistry.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

// Game-neutral actor creation request. Names are logical allowlist keys and
// rotations are degrees; native ids, pointers, params and engine units never
// cross this boundary.
struct ActorSpawnRequest {
  std::string actor;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double rotationX = 0.0;
  double rotationY = 0.0;
  double rotationZ = 0.0;
};

// Main-thread provider implemented by each game host. The provider owns the
// native record and validates ownership on every operation.
class ActorProvider {
public:
  virtual ~ActorProvider() = default;

  virtual Result<Handle> Spawn(const std::string &ownerModId,
                               const ActorSpawnRequest &request) = 0;
  virtual Result<void> Destroy(const std::string &ownerModId,
                               const Handle &handle) = 0;
  virtual Result<bool> Exists(const std::string &ownerModId,
                              const Handle &handle) const = 0;
  virtual Result<std::size_t> ReleaseMod(const std::string &modId) = 0;
};

} // namespace ShipLua
