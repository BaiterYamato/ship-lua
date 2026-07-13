#include <iostream>
#include <string>
#include <vector>

#include "shiplua/manifest/SemVersion.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::SemVersion Parse(const std::string& text) {
    auto result = ShipLua::SemVersion::Parse(text);
    Check(result.isOk(), "version should parse: " + text + " (" + result.message + ")");
    return result.isOk() ? *result.value : ShipLua::SemVersion{};
}

void TestCanonicalVersions() {
    auto version = ShipLua::SemVersion::Parse("1.2.3-alpha.1+windows.x64");
    Check(version.isOk(), "complete SemVer should parse");
    if (version.isOk()) {
        Check(version.value->major == 1 && version.value->minor == 2 && version.value->patch == 3,
              "core version should parse");
        Check(version.value->prerelease.size() == 2, "prerelease should parse");
        Check(version.value->build.size() == 2, "build metadata should parse");
    }

    const std::vector<std::string> invalid = {
        "1", "1.2", "01.2.3", "1.02.3", "1.2.03", "1.2.3-", "1.2.3+",
        "1.2.3-alpha..1", "1.2.3-01", "1.2.3 extra", "1.2.3+one+two"};
    for (const auto& text : invalid) {
        Check(!ShipLua::SemVersion::Parse(text).isOk(), "invalid SemVer should fail: " + text);
    }
}

void TestPrecedence() {
    const std::vector<std::string> ordered = {
        "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta", "1.0.0-beta",
        "1.0.0-beta.2", "1.0.0-beta.11", "1.0.0-rc.1", "1.0.0"};
    for (std::size_t i = 1; i < ordered.size(); ++i) {
        Check(Parse(ordered[i - 1]) < Parse(ordered[i]),
              "SemVer precedence should order " + ordered[i - 1] + " before " + ordered[i]);
    }
    Check(Parse("1.2.3+one") == Parse("1.2.3+two"),
          "build metadata must not affect precedence");
}

void TestRanges() {
    auto range = ShipLua::VersionRange::Parse(">=0.1 <1.0");
    Check(range.isOk(), "manifest-style partial range should parse");
    if (range.isOk()) {
        Check(range.value->Contains(Parse("0.1.0")), "lower inclusive bound should match");
        Check(range.value->Contains(Parse("0.9.9")), "interior version should match");
        Check(!range.value->Contains(Parse("0.0.9")), "version below range should fail");
        Check(!range.value->Contains(Parse("1.0.0")), "upper exclusive bound should fail");
    }

    auto exact = ShipLua::VersionRange::Parse("=2.4.1");
    Check(exact.isOk() && exact.value->Contains(Parse("2.4.1")), "exact range should match");
    Check(exact.isOk() && !exact.value->Contains(Parse("2.4.2")), "exact range should reject others");

    const std::vector<std::string> invalid = {"", ">=", "^1.0", ">=01.0", ">=1.0 || <2.0"};
    for (const auto& text : invalid) {
        Check(!ShipLua::VersionRange::Parse(text).isOk(), "unsupported range should fail: " + text);
    }
}

} // namespace

int main() {
    TestCanonicalVersions();
    TestPrecedence();
    TestRanges();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All semantic version checks passed\n";
    return 0;
}
