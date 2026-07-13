#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <miniz.h>

#include "shiplua/host/ModHost.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

void WriteText(const std::filesystem::path &path, const std::string &contents) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << contents;
}

std::string Manifest(const std::string &id, const std::string &extra = {}) {
  return "id = \"" + id + "\"\nname = \"" + id +
         "\"\nversion = \"1.0.0\"\n"
         "api = \">=0.1 <0.2\"\nentrypoint = \"main.lua\"\n" +
         extra;
}

void WriteDirectoryMod(const std::filesystem::path &root,
                       const std::string &folder, const std::string &manifest,
                       const std::string &source = "loaded = true\n") {
  WriteText(root / folder / "manifest.toml", manifest);
  WriteText(root / folder / "main.lua", source);
}

bool WritePackage(const std::filesystem::path &path,
                  const std::string &manifest, const std::string &source) {
  mz_zip_archive archive{};
  if (!mz_zip_writer_init_file(&archive, path.string().c_str(), 0)) {
    return false;
  }
  bool ok = mz_zip_writer_add_mem(&archive, "manifest.toml", manifest.data(),
                                  manifest.size(), MZ_BEST_COMPRESSION) != 0;
  ok = mz_zip_writer_add_mem(&archive, "main.lua", source.data(), source.size(),
                             MZ_BEST_COMPRESSION) != 0 &&
       ok;
  ok = mz_zip_writer_finalize_archive(&archive) != 0 && ok;
  mz_zip_writer_end(&archive);
  return ok;
}

ShipLua::ModHost MakeHost(const std::string &game = "oot",
                          const std::string &version = "9.1.2") {
  ShipLua::LuaApiHostContext context{game, version, "0.1.0", {}};
  return ShipLua::ModHost(std::move(context));
}

void TestDeterministicDependencies(const std::filesystem::path &root) {
  const auto mods = root / "dependencies";
  std::filesystem::create_directories(mods);
  WriteDirectoryMod(
      mods, "feature",
      Manifest("example.feature",
               "[dependencies]\n\"example.core\" = \">=1.0 <2.0\"\n"));
  WriteDirectoryMod(mods, "core", Manifest("example.core"));

  auto host = MakeHost();
  auto result = host.LoadModsFromRoot(mods, root / "cache-dependencies");
  Check(result.isOk(), "dependency root should load: " + result.message);
  Check(result.isOk() &&
            result.value->loadedIds ==
                std::vector<std::string>{"example.core", "example.feature"},
        "dependencies should determine load order");
  Check(host.Count() == 2, "both dependency mods should remain loaded");
}

void TestCompatibilityAndFailureIsolation(const std::filesystem::path &root) {
  const auto mods = root / "compatibility";
  std::filesystem::create_directories(mods);
  WriteDirectoryMod(mods, "healthy",
                    Manifest("example.healthy", "games = [\"oot\"]\n"));
  WriteDirectoryMod(mods, "wrong-game",
                    Manifest("example.wrong_game", "games = [\"mm\"]\n"));
  WriteDirectoryMod(
      mods, "wrong-host",
      Manifest("example.wrong_host", "[host]\nshipwright = \">=10.0\"\n"));
  WriteDirectoryMod(
      mods, "wrong-api",
      "id = \"example.wrong_api\"\nname = \"example.wrong_api\"\n"
      "version = \"1.0.0\"\napi = \">=1.0 <2.0\"\n"
      "entrypoint = \"main.lua\"\n");
  WriteDirectoryMod(mods, "broken", Manifest("example.broken"),
                    "error('boom')\n");
  WriteDirectoryMod(
      mods, "dependent",
      Manifest("example.dependent",
               "[dependencies]\n\"example.broken\" = \">=1.0 <2.0\"\n"));

  auto host = MakeHost();
  auto result = host.LoadModsFromRoot(mods, root / "cache-compatibility");
  Check(result.isOk(), "per-mod failures should not abort the root");
  Check(result.isOk() && result.value->loadedIds ==
                             std::vector<std::string>{"example.healthy"},
        "only compatible healthy mod should load");
  Check(result.isOk() && result.value->rejected.contains("example.wrong_game"),
        "wrong game should be reported");
  Check(result.isOk() && result.value->rejected.contains("example.wrong_host"),
        "wrong host version should be reported");
  Check(result.isOk() && result.value->rejected.contains("example.wrong_api"),
        "wrong API version should be reported");
  Check(result.isOk() && result.value->rejected.contains("example.broken"),
        "script failure should be reported");
  Check(result.isOk() && result.value->rejected.contains("example.dependent"),
        "dependent of a failed script should be rejected");
}

void TestPackageOwnership(const std::filesystem::path &root) {
  const auto mods = root / "packages";
  const auto cache = root / "cache-packages";
  std::filesystem::create_directories(mods);
  Check(WritePackage(mods / "example.shipmod", Manifest("example.package"),
                     "loaded = true\n"),
        "package fixture should be created");

  auto host = MakeHost();
  auto result = host.LoadModsFromRoot(mods, cache);
  Check(result.isOk() && result.value->loadedIds ==
                             std::vector<std::string>{"example.package"},
        "package should load from root");
  Check(std::filesystem::exists(cache), "package cache should exist");
  Check(host.UnloadMod("example.package").isOk(), "package should unload");
  Check(std::filesystem::is_empty(cache),
        "unload should remove owned extraction directory");
}

void TestInvalidRoot(const std::filesystem::path &root) {
  auto host = MakeHost();
  auto result = host.LoadModsFromRoot(root / "missing", root / "cache-missing");
  Check(!result.isOk() && result.code == ShipLua::ErrorCode::InvalidArgument,
        "missing root should be a top-level error");
}

} // namespace

int main() {
  const auto root =
      std::filesystem::temp_directory_path() / "shiplua-mod-root-loader-tests";
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);
  std::filesystem::create_directories(root);

  TestDeterministicDependencies(root);
  TestCompatibilityAndFailureIsolation(root);
  TestPackageOwnership(root);
  TestInvalidRoot(root);

  std::filesystem::remove_all(root, ignored);
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "all mod root loader tests passed\n";
  return 0;
}
