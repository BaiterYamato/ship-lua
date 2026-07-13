#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "shiplua/manifest/DependencyResolver.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

ShipLua::Manifest Mod(std::string id, std::string version = "1.0.0", int priority = 50) {
    ShipLua::Manifest manifest;
    manifest.id = std::move(id);
    manifest.name = manifest.id;
    manifest.version = std::move(version);
    manifest.apiRange = ">=0.1 <1.0";
    manifest.entrypoint = "main.lua";
    manifest.loadPriority = priority;
    return manifest;
}

void TestDependenciesAndIsolation() {
    auto core = Mod("core", "1.5.0", 100);
    auto feature = Mod("feature", "1.0.0", 0);
    feature.dependencies.push_back({"core", ">=1.0 <2.0"});
    auto broken = Mod("broken");
    broken.dependencies.push_back({"missing", ">=1.0"});
    auto independent = Mod("independent", "1.0.0", 10);

    auto result = ShipLua::ResolveMods({feature, broken, core, independent});
    Check(result.isOk(), "dependency graph should resolve");
    if (!result.isOk()) return;
    Check(result.value->rejected.contains("broken"), "missing dependency should reject affected mod");
    Check(!result.value->rejected.contains("independent"), "independent mod should remain loadable");
    const auto& order = result.value->orderedIds;
    const auto corePos = std::find(order.begin(), order.end(), "core");
    const auto featurePos = std::find(order.begin(), order.end(), "feature");
    Check(corePos != order.end() && featurePos != order.end() && corePos < featurePos,
          "dependency must load before dependent despite priority");
}

void TestVersionMismatchPropagation() {
    auto core = Mod("core", "2.0.0");
    auto feature = Mod("feature");
    feature.dependencies.push_back({"core", ">=1.0 <2.0"});
    auto addon = Mod("addon");
    addon.dependencies.push_back({"feature", ">=1.0"});

    auto result = ShipLua::ResolveMods({core, feature, addon});
    Check(result.isOk(), "mismatch graph should produce a partial resolution");
    if (!result.isOk()) return;
    Check(result.value->rejected.contains("feature"), "incompatible dependency should reject mod");
    Check(result.value->rejected.contains("addon"), "rejection should propagate to dependents");
    Check(result.value->orderedIds == std::vector<std::string>{"core"},
          "healthy dependency should remain loadable");
}

void TestDeterministicOrderAndHints() {
    auto alpha = Mod("alpha", "1.0.0", 20);
    auto beta = Mod("beta", "1.0.0", 10);
    auto gamma = Mod("gamma", "1.0.0", 10);
    alpha.loadBefore.push_back("beta");
    gamma.loadAfter.push_back("beta");

    auto result = ShipLua::ResolveMods({gamma, alpha, beta});
    Check(result.isOk(), "hint graph should resolve");
    if (result.isOk()) {
        Check(result.value->orderedIds == std::vector<std::string>({"alpha", "beta", "gamma"}),
              "hints should override priority and IDs should break equal-priority ties");
    }
    auto permuted = ShipLua::ResolveMods({beta, gamma, alpha});
    Check(permuted.isOk() && result.isOk() &&
              permuted.value->orderedIds == result.value->orderedIds,
          "load order must not depend on manifest enumeration order");
}

void TestCyclesAndDuplicates() {
    auto a = Mod("a");
    auto b = Mod("b");
    a.dependencies.push_back({"b", ">=1.0"});
    b.dependencies.push_back({"a", ">=1.0"});
    auto healthy = Mod("healthy");

    auto cycle = ShipLua::ResolveMods({a, b, healthy});
    Check(cycle.isOk(), "cycle should be reported as partial resolution");
    if (cycle.isOk()) {
        Check(cycle.value->rejected.contains("a") && cycle.value->rejected.contains("b"),
              "all cycle members should be rejected");
        Check(cycle.value->orderedIds == std::vector<std::string>{"healthy"},
              "independent mod should survive a cycle");
    }

    auto duplicate = ShipLua::ResolveMods({Mod("same"), Mod("same"), Mod("other")});
    Check(duplicate.isOk() && duplicate.value->rejected.contains("same"),
          "duplicate IDs should be rejected");
    Check(duplicate.isOk() && duplicate.value->orderedIds == std::vector<std::string>{"other"},
          "duplicate conflict should not reject unrelated mods");
}

} // namespace

int main() {
    TestDependenciesAndIsolation();
    TestVersionMismatchPropagation();
    TestDeterministicOrderAndHints();
    TestCyclesAndDuplicates();
    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All dependency resolution checks passed\n";
    return 0;
}
