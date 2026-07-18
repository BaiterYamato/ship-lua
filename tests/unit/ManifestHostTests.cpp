#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "shiplua/host/ModHost.h"
#include "shiplua/manifest/ManifestParser.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::Manifest MakeManifest(std::string id) {
    ShipLua::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = "Test mod";
    manifest.version = "0.1.0";
    manifest.apiRange = ">=0.1 <0.2";
    manifest.entrypoint = "main.lua";
    return manifest;
}

void TestCompleteManifest() {
    const std::string source = R"toml(
id = "community.example"
name = "Example"
version = "1.2.3"
api = ">=0.1 <1.0"
entrypoint = "main.lua"
description = "Example mod"
authors = ["Ada", "Linus"]
games = ["oot", "mm"]

[host]
shipwright = ">=9.0"
two_ship = ">=4.0"

[capabilities]
required = ["scene.events"]
optional = ["mm.cycle"]

[dependencies]
"community.core" = ">=1.0 <2.0"

[load]
priority = 10
after = ["community.core"]
before = ["community.ui"]

[permissions]
storage = true
)toml";

    auto result = ShipLua::ParseManifestString(source, "complete.toml");
    Check(result.isOk(), "complete manifest should parse");
    if (!result.isOk()) {
        return;
    }
    const auto& manifest = *result.value;
    Check(manifest.id == "community.example", "id should be preserved");
    Check(manifest.authors.size() == 2, "authors should parse");
    Check(manifest.games.size() == 2, "games should parse");
    Check(manifest.hostShipwright == ">=9.0", "Shipwright constraint should parse");
    Check(manifest.capabilitiesRequired.size() == 1, "required capabilities should parse");
    Check(manifest.dependencies.size() == 1, "dependencies should parse");
    Check(manifest.loadPriority == 10, "load priority should parse");
    Check(manifest.permStorage, "storage permission should parse");
    Check(!manifest.permNetwork, "permissions should default to false");
    Check(!manifest.requiresBothGames, "requires_both_games should default to false");
}

void TestRequiresBothGamesField() {
    // Explicit `requires_both_games = true` must flip the flag. This is the
    // contract the launcher gates on (exit code 8 when a dual-world mod has
    // only one host asset available).
    {
        const std::string source = R"toml(
id = "example.dual"
name = "Dual"
version = "0.2.0"
api = ">=0.3 <1.0"
entrypoint = "main.lua"
games = ["oot", "mm"]
requires_both_games = true
)toml";
        auto result = ShipLua::ParseManifestString(source, "dual.toml");
        Check(result.isOk(), "manifest with requires_both_games=true should parse");
        if (result.isOk()) {
            Check(result.value->requiresBothGames,
                  "requires_both_games=true should set the flag");
        }
    }
    // Explicit `requires_both_games = false` keeps compatibility with either
    // host (the historical semantics of games=["oot","mm"]).
    {
        const std::string source = R"toml(
id = "example.either"
name = "Either"
version = "0.2.0"
api = ">=0.3 <1.0"
entrypoint = "main.lua"
games = ["oot", "mm"]
requires_both_games = false
)toml";
        auto result = ShipLua::ParseManifestString(source, "either.toml");
        Check(result.isOk(), "manifest with requires_both_games=false should parse");
        if (result.isOk()) {
            Check(!result.value->requiresBothGames,
                  "requires_both_games=false must clear the flag");
        }
    }
    // Omitting the field keeps the default false — a mod that only lists
    // `games` stays runnable on whichever host is present.
    {
        const std::string source = R"toml(
id = "example.implicit"
name = "Implicit"
version = "0.2.0"
api = ">=0.3 <1.0"
entrypoint = "main.lua"
games = ["oot", "mm"]
)toml";
        auto result = ShipLua::ParseManifestString(source, "implicit.toml");
        Check(result.isOk(), "manifest without requires_both_games should parse");
        if (result.isOk()) {
            Check(!result.value->requiresBothGames,
                  "absent requires_both_games must default to false");
        }
    }
}

void TestManifestErrors() {
    auto missing = ShipLua::ParseManifestString("id = \"only.id\"", "missing.toml");
    Check(!missing.isOk(), "missing required fields should fail");
    Check(missing.message.find("name") != std::string::npos,
          "missing-field error should name the field");

    auto invalid = ShipLua::ParseManifestString("id = [", "broken.toml");
    Check(!invalid.isOk(), "invalid TOML should fail");
    Check(invalid.message.find("line") != std::string::npos,
          "syntax error should include source location");
}

void TestIsolatedHostLifecycle() {
    ShipLua::ModHost host;
    auto first = host.LoadModFromManifestAndSource(MakeManifest("mod.first"), "shared = 42");
    auto second = host.LoadModFromManifestAndSource(
        MakeManifest("mod.second"), "assert(shared == nil); shared = 7");

    Check(first.isOk() && second.isOk(), "two isolated mods should load");
    Check(host.Count() == 2, "host should track both mods");
    Check(host.GetRuntime("mod.first") != host.GetRuntime("mod.second"),
          "mods should own different runtimes");

    auto duplicate = host.LoadModFromManifestAndSource(MakeManifest("mod.first"), "return true");
    Check(!duplicate.isOk(), "duplicate mod id should fail");
    Check(host.Count() == 2, "duplicate failure should not change loaded mods");

    auto broken = host.LoadModFromManifestAndSource(MakeManifest("mod.broken"), "error('boom')");
    Check(!broken.isOk(), "script failure should fail the affected mod");
    Check(!host.IsLoaded("mod.broken"), "failed mod should roll back");
    Check(host.Count() == 2, "script failure should not unload healthy mods");

    Check(host.UnloadMod("mod.first").isOk(), "loaded mod should unload");
    Check(!host.IsLoaded("mod.first"), "unloaded mod should disappear");
    host.UnloadAll();
    Check(host.Count() == 0, "UnloadAll should clear the host");
}

void TestDirectoryLoad() {
    const auto root = std::filesystem::temp_directory_path() / "shiplua-manifest-host-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    {
        std::ofstream manifest(root / "manifest.toml");
        manifest << "id = \"mod.directory\"\n"
                    "name = \"Directory mod\"\n"
                    "version = \"0.1.0\"\n"
                    "api = \">=0.1 <0.2\"\n"
                    "entrypoint = \"main.lua\"\n";
        std::ofstream script(root / "main.lua");
        script << "directory_loaded = true\n";
    }

    ShipLua::ModHost host;
    auto loaded = host.LoadModFromDirectory(root.string());
    Check(loaded.isOk(), "directory mod should load");
    Check(host.IsLoaded("mod.directory"), "directory mod should be registered");
    std::filesystem::remove_all(root);
}

void TestDirectoryTraversalIsRejected() {
    const auto base = std::filesystem::temp_directory_path() / "shiplua-traversal-test";
    const auto root = base / "mod";
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(root);
    {
        std::ofstream manifest(root / "manifest.toml");
        manifest << "id = \"mod.traversal\"\n"
                    "name = \"Traversal mod\"\n"
                    "version = \"0.1.0\"\n"
                    "api = \">=0.1 <0.2\"\n"
                    "entrypoint = \"../outside.lua\"\n";
        std::ofstream outside(base / "outside.lua");
        outside << "escaped = true\n";
    }

    ShipLua::ModHost host;
    auto loaded = host.LoadModFromDirectory(root.string());
    Check(!loaded.isOk(), "parent traversal entrypoint should be rejected");
    Check(loaded.code == ShipLua::ErrorCode::PermissionDenied,
          "parent traversal should report permission denied");
    Check(host.Count() == 0, "rejected traversal must not register a mod");
    std::filesystem::remove_all(base);
}

} // namespace

int main() {
    TestCompleteManifest();
    TestRequiresBothGamesField();
    TestManifestErrors();
    TestIsolatedHostLifecycle();
    TestDirectoryLoad();
    TestDirectoryTraversalIsRejected();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All manifest and host checks passed\n";
    return 0;
}
