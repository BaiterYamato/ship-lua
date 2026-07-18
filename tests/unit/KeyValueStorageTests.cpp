#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "shiplua/storage/KeyValueStorage.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestRoundtripAndTypes() {
    ShipLua::KeyValueStorage storage;
    Check(storage.Set("mod.a", "flag", true).isOk(), "set bool should succeed");
    Check(storage.Set("mod.a", "count", std::int64_t{7}).isOk(), "set int should succeed");
    Check(storage.Set("mod.a", "ratio", 2.5).isOk(), "set double should succeed");
    Check(storage.Set("mod.a", "label", std::string("valor")).isOk(),
          "set string should succeed");

    const auto flag = storage.Get("mod.a", "flag");
    Check(flag.isOk() && flag.value->has_value() &&
              std::holds_alternative<bool>(**flag.value) && std::get<bool>(**flag.value),
          "bool roundtrip should preserve type and value");
    const auto count = storage.Get("mod.a", "count");
    Check(count.isOk() && count.value->has_value() &&
              std::holds_alternative<std::int64_t>(**count.value) &&
              std::get<std::int64_t>(**count.value) == 7,
          "int roundtrip should preserve type and value");
    const auto ratio = storage.Get("mod.a", "ratio");
    Check(ratio.isOk() && ratio.value->has_value() &&
              std::holds_alternative<double>(**ratio.value) &&
              std::get<double>(**ratio.value) == 2.5,
          "double roundtrip should preserve type and value");
    const auto label = storage.Get("mod.a", "label");
    Check(label.isOk() && label.value->has_value() &&
              std::get<std::string>(**label.value) == "valor",
          "string roundtrip should preserve value");

    const auto missing = storage.Get("mod.a", "missing");
    Check(missing.isOk() && !missing.value->has_value(), "missing key should return nullopt");

    // Valores podem conter bytes NUL (somente chaves são validadas contra NUL).
    const std::string binary("com\0byte", 8);
    Check(storage.Set("mod.a", "bin", binary).isOk(), "binary value should succeed");
    const auto bin = storage.Get("mod.a", "bin");
    Check(bin.isOk() && bin.value->has_value() && std::get<std::string>(**bin.value) == binary,
          "binary value should roundtrip");
}

void TestDeleteClearAndKeys() {
    ShipLua::KeyValueStorage storage;
    storage.Set("mod.a", "b", std::int64_t{1});
    storage.Set("mod.a", "a", std::int64_t{2});
    storage.Set("mod.a", "c", std::int64_t{3});

    const auto keys = storage.Keys("mod.a");
    Check(keys.isOk() && *keys.value == std::vector<std::string>({"a", "b", "c"}),
          "keys should be deterministic and sorted");

    const auto removed = storage.Delete("mod.a", "b");
    Check(removed.isOk() && *removed.value, "delete should report existing key");
    const auto removedAgain = storage.Delete("mod.a", "b");
    Check(removedAgain.isOk() && !*removedAgain.value, "delete should report missing key");

    Check(storage.KeyCount("mod.a") == 2, "key count should track deletions");

    const auto cleared = storage.Clear("mod.a");
    Check(cleared.isOk() && *cleared.value == 2, "clear should remove remaining keys");
    Check(storage.KeyCount("mod.a") == 0 && storage.TotalBytes("mod.a") == 0,
          "clear should reset accounting");
    const auto clearedAgain = storage.Clear("mod.a");
    Check(clearedAgain.isOk() && *clearedAgain.value == 0, "clear should be idempotent");
}

void TestNamespacing() {
    ShipLua::KeyValueStorage storage;
    storage.Set("mod.a", "key", std::string("a"));
    storage.Set("mod.b", "key", std::string("b"));

    const auto fromA = storage.Get("mod.a", "key");
    const auto fromB = storage.Get("mod.b", "key");
    Check(fromA.isOk() && std::get<std::string>(**fromA.value) == "a",
          "mod A should read its own namespace");
    Check(fromB.isOk() && std::get<std::string>(**fromB.value) == "b",
          "mod B should read its own namespace");

    storage.Clear("mod.a");
    Check(storage.KeyCount("mod.b") == 1, "clearing one mod must not affect another");
}

void TestValidationAndLimits() {
    ShipLua::KeyValueStorage::Limits limits;
    limits.maxKeysPerMod = 2;
    limits.maxKeyBytes = 4;
    limits.maxValueBytes = 8;
    limits.maxTotalBytesPerMod = 24;
    ShipLua::KeyValueStorage storage(limits);

    Check(!storage.Set("", "k", std::int64_t{1}).isOk(), "empty mod id should fail");
    Check(!storage.Set("mod", "", std::int64_t{1}).isOk(), "empty key should fail");
    Check(!storage.Set("mod", "chave-longa", std::int64_t{1}).isOk(),
          "oversized key should fail");
    Check(!storage.Set("mod", std::string("k\0", 2), std::int64_t{1}).isOk(),
          "key with NUL should fail");

    Check(storage.Set("mod", "k1", std::string("12345678")).isOk(),
          "value at the size limit should succeed");
    Check(!storage.Set("mod", "k2", std::string("123456789")).isOk(),
          "value above the size limit should fail with resource limit");

    // k1 ocupa 2 + 8 = 10 bytes; k2 com inteiro ocuparia 2 + 8 = 10 (total 20 <= 24)
    Check(storage.Set("mod", "k2", std::int64_t{1}).isOk(), "second key should fit the quota");
    Check(!storage.Set("mod", "k3", std::int64_t{1}).isOk(),
          "third key should hit the key count limit");

    // Atualizar chave existente não conta como chave nova.
    Check(storage.Set("mod", "k2", std::int64_t{2}).isOk(),
          "updating an existing key should not hit the key count limit");
}

} // namespace

int main() {
    TestRoundtripAndTypes();
    TestDeleteClearAndKeys();
    TestNamespacing();
    TestValidationAndLimits();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "all key_value_storage tests passed\n";
    return 0;
}
