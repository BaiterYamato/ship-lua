#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "shiplua/host/ModHost.h"

namespace {

int failures = 0;

struct CapturedLog {
  ShipLua::LogLevel level;
  std::string modId;
  std::string message;
};

void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

bool ContainsInfo(const std::vector<CapturedLog> &logs,
                  const std::string &message) {
  return std::any_of(logs.begin(), logs.end(), [&](const CapturedLog &log) {
    return log.level == ShipLua::LogLevel::Info &&
           log.modId == "example.hello_world" && log.message == message;
  });
}

void RunForHost(const std::filesystem::path &root,
                const std::filesystem::path &package, const std::string &game,
                const std::string &hostVersion) {
  const auto hostRoot = root / game;
  const auto modsRoot = hostRoot / "mods";
  const auto cacheRoot = hostRoot / "cache";
  std::filesystem::create_directories(modsRoot);
  std::filesystem::copy_file(package, modsRoot / "hello-world.shipmod");

  std::vector<CapturedLog> logs;
  ShipLua::LuaApiHostContext context{game, hostVersion, "0.1.0", {}};
  ShipLua::ModHost host(
      context,
      ShipLua::Logger([&](ShipLua::LogLevel level, const std::string &modId,
                          const std::string &message) {
        logs.push_back({level, modId, message});
      }));

  const auto loaded = host.LoadModsFromRoot(modsRoot, cacheRoot);
  Check(loaded.isOk(),
        "o pacote deve carregar em " + game + ": " + loaded.message);
  Check(loaded.isOk() && loaded.value->loadedIds ==
                             std::vector<std::string>{"example.hello_world"},
        "o relatório deve conter somente hello-world em " + game);
  Check(host.Count() == 1 && host.SubscriptionCount() == 1,
        "hello-world deve registrar um mod e um callback em " + game);

  ShipLua::EventPayload payload{
      {"game_id", game},
      {"host_version", hostVersion},
      {"runtime_version", "0.1.0"},
      {"api_version", "0.1.0"},
  };
  const auto dispatched = host.DispatchEvent("game.ready", payload);
  Check(dispatched.isOk() && dispatched.value->callbacksInvoked == 1 &&
            dispatched.value->failures.empty(),
        "game.ready deve executar sem falhas em " + game);
  Check(ContainsInfo(logs, "Hello from " + game + " host=" + hostVersion),
        "hello-world deve registrar jogo e versão em " + game);

  host.UnloadAll();
  Check(host.Count() == 0 && host.SubscriptionCount() == 0,
        "unload deve remover mod e callback em " + game);
  Check(std::filesystem::exists(cacheRoot) &&
            std::filesystem::is_empty(cacheRoot),
        "unload deve limpar a extração do pacote em " + game);
}

} // namespace

int main() {
  const std::filesystem::path package = SHIPLUA_HELLO_WORLD_PACKAGE;
  Check(std::filesystem::is_regular_file(package),
        "package_examples deve gerar hello-world.shipmod");

  const auto root = std::filesystem::temp_directory_path() /
                    "shiplua-hello-world-conformance";
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);
  std::filesystem::create_directories(root);

  if (std::filesystem::is_regular_file(package)) {
    RunForHost(root, package, "oot", "9.1.2");
    RunForHost(root, package, "mm", "4.0.2");
  }

  std::filesystem::remove_all(root, ignored);
  if (failures != 0) {
    std::cerr << failures << " verificação(ões) falharam\n";
    return 1;
  }
  std::cout << "hello-world.shipmod passou em OoT e MM\n";
  return 0;
}
