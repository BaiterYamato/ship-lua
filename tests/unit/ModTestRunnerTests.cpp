#include <iostream>
#include <string>

#include "shiplua/testing/ModTestRunner.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::filesystem::path Fixture(const std::string& name) {
    return std::filesystem::path(SHIPLUA_TEST_SOURCE_DIR) / "fixtures" / "mod-runner" / name;
}

std::filesystem::path Example(const std::string& name) {
    return std::filesystem::path(SHIPLUA_TEST_SOURCE_DIR).parent_path() / "examples" / name;
}

bool AllTestsPassed(const ShipLua::ModTestReport& report) {
    return report.FailedCount() == 0 && !report.tests.empty();
}

void TestPassingMod() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Fixture("passing-mod"));
    Check(report.isOk(), "running the passing fixture should succeed");
    if (!report.isOk()) {
        return;
    }
    Check(report.value->modId == "fixture.passing", "report should carry the mod id");
    Check(report.value->files == 2, "both test files should run");
    Check(report.value->errors.empty(), "no infrastructure errors expected");
    Check(AllTestsPassed(*report.value), "every fixture test should pass");
    Check(report.value->tests.size() == 8, "expected 8 test cases across both files");

    bool sawNestedName = false;
    for (const ShipLua::ModTestCaseResult& test : report.value->tests) {
        if (test.test == "basico / interno / aninha nomes") {
            sawNestedName = true;
        }
    }
    Check(sawNestedName, "nested describe should compose the test name");
}

void TestFailingMod() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Fixture("failing-mod"));
    Check(report.isOk(), "running the failing fixture should succeed");
    if (!report.isOk()) {
        return;
    }
    Check(report.value->PassedCount() == 1 && report.value->FailedCount() == 1,
          "failing fixture should pass one test and fail one test");
    bool sawMessage = false;
    for (const ShipLua::ModTestCaseResult& test : report.value->tests) {
        if (!test.ok && test.message.find("esperado") != std::string::npos) {
            sawMessage = true;
        }
    }
    Check(sawMessage, "failure should keep the assertion message");
}

void TestModWithoutTests() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Fixture("no-tests-mod"));
    Check(report.isOk(), "running a mod without tests should succeed");
    if (!report.isOk()) {
        return;
    }
    Check(report.value->noTestsFound, "mod without tests/ should be reported as no-tests");
    Check(report.value->tests.empty() && report.value->errors.empty(),
          "no-tests report should be empty");
}

void TestBrokenMod() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Fixture("broken-mod"));
    Check(report.isOk(), "running a broken mod should still produce a report");
    if (!report.isOk()) {
        return;
    }
    Check(!report.value->errors.empty(), "broken manifest should surface as an error");
    Check(report.value->PassedCount() == 0, "broken mod should not pass any test");
}

void TestMissingDirectory() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Fixture("does-not-exist"));
    Check(!report.isOk(), "missing directory should fail to run");
}

void TestFrameCounterExample() {
    ShipLua::ModTestRunner runner;
    const auto report = runner.Run(Example("frame-counter"));
    Check(report.isOk(), "running the frame-counter example should succeed");
    if (!report.isOk()) {
        return;
    }
    Check(report.value->modId == "example.frame_counter", "example mod id should be reported");
    Check(report.value->errors.empty(), "example should not produce infrastructure errors");
    Check(AllTestsPassed(*report.value),
          "example mod should pass all of its tests in the mock");
    Check(report.value->tests.size() == 6, "example should run 6 test cases");
}

} // namespace

int main() {
    TestPassingMod();
    TestFailingMod();
    TestModWithoutTests();
    TestBrokenMod();
    TestMissingDirectory();
    TestFrameCounterExample();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all mod_test_runner tests passed\n";
    return 0;
}
