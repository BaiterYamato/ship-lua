#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "shiplua/manifest/ManifestParser.h"
#include "shiplua/manifest/ModDiscovery.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestManifestFixtures() {
    const std::filesystem::path fixtures =
        std::filesystem::path(SHIPLUA_TEST_SOURCE_DIR) / "fixtures" / "manifest";

    Check(ShipLua::ParseManifestFile((fixtures / "valid-minimal.toml").string()).isOk(),
          "minimal fixture should be valid");
    Check(ShipLua::ParseManifestFile((fixtures / "valid-complete.toml").string()).isOk(),
          "complete fixture should be valid");

    const auto missing =
        ShipLua::ParseManifestFile((fixtures / "invalid-missing-name.toml").string());
    Check(!missing.isOk() && missing.message.find("name") != std::string::npos,
          "missing-name fixture should fail with a readable field name");

    const auto syntax = ShipLua::ParseManifestFile((fixtures / "invalid-syntax.toml").string());
    Check(!syntax.isOk() && syntax.message.find("line") != std::string::npos,
          "syntax fixture should fail with a source location");
}

void WriteZipSignature(const std::filesystem::path& path) {
    const std::array<unsigned char, 4> signature{0x50, 0x4b, 0x03, 0x04};
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(signature.data()), signature.size());
}

void TestDiscovery() {
    const auto root = std::filesystem::temp_directory_path() / "shiplua-discovery-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "b-directory-mod");
    std::filesystem::create_directories(root / "ignored-directory");
    {
        std::ofstream manifest(root / "b-directory-mod" / "manifest.toml");
        manifest << "id = \"example.directory\"\n";
        std::ofstream fake(root / "fake.shipmod", std::ios::binary);
        fake << "not a zip";
    }
    WriteZipSignature(root / "a-package.shipmod");

    const auto result = ShipLua::DiscoverMods(root);
    Check(result.isOk(), "valid mods root should scan");
    if (result.isOk()) {
        Check(result.value->size() == 2, "only valid directory and package candidates should appear");
        if (result.value->size() == 2) {
            Check((*result.value)[0].kind == ShipLua::ModSourceKind::Package,
                  "results should be deterministically path-sorted");
            Check((*result.value)[1].kind == ShipLua::ModSourceKind::Directory,
                  "directory candidate should be identified");
        }
    }

    std::filesystem::remove_all(root);
}

void TestMissingRoot() {
    const auto missing = std::filesystem::temp_directory_path() / "shiplua-root-does-not-exist";
    std::filesystem::remove_all(missing);
    const auto result = ShipLua::DiscoverMods(missing);
    Check(!result.isOk(), "missing mods root should fail");
    Check(result.code == ShipLua::ErrorCode::InvalidArgument,
          "missing root should report invalid argument");
}

} // namespace

int main() {
    TestManifestFixtures();
    TestDiscovery();
    TestMissingRoot();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All manifest fixture and discovery checks passed\n";
    return 0;
}
