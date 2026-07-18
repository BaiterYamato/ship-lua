#include "shiplua/storage/KeyValueStorage.h"

#include <utility>

namespace ShipLua {

KeyValueStorage::KeyValueStorage(Limits limits) : mLimits(limits) {
    if (mLimits.maxKeysPerMod == 0) {
        mLimits.maxKeysPerMod = 1;
    }
    if (mLimits.maxKeyBytes == 0) {
        mLimits.maxKeyBytes = 1;
    }
}

const KeyValueStorage::Limits& KeyValueStorage::GetLimits() const {
    return mLimits;
}

std::size_t KeyValueStorage::ValueBytes(const Value& value) {
    return std::visit(
        [](const auto& item) -> std::size_t {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return item.size();
            } else {
                return sizeof(item);
            }
        },
        value);
}

Result<void> KeyValueStorage::ValidateKey(const std::string& modId, const std::string& key) const {
    if (modId.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument, "storage requires a mod id");
    }
    if (key.empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument, "storage key cannot be empty");
    }
    if (key.size() > mLimits.maxKeyBytes) {
        return Result<void>::err(ErrorCode::InvalidArgument, "storage key exceeds the size limit");
    }
    if (key.find('\0') != std::string::npos) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "storage key cannot contain NUL bytes");
    }
    return Result<void>::ok();
}

Result<std::optional<KeyValueStorage::Value>> KeyValueStorage::Get(const std::string& modId,
                                                                   const std::string& key) const {
    const auto valid = ValidateKey(modId, key);
    if (!valid.isOk()) {
        return Result<std::optional<Value>>::err(valid.code, valid.message);
    }
    const auto mod = mMods.find(modId);
    if (mod == mMods.end()) {
        return Result<std::optional<Value>>::ok(std::nullopt);
    }
    const auto entry = mod->second.find(key);
    if (entry == mod->second.end()) {
        return Result<std::optional<Value>>::ok(std::nullopt);
    }
    return Result<std::optional<Value>>::ok(entry->second);
}

Result<void> KeyValueStorage::Set(const std::string& modId, const std::string& key, Value value) {
    const auto valid = ValidateKey(modId, key);
    if (!valid.isOk()) {
        return valid;
    }
    const std::size_t valueBytes = ValueBytes(value);
    if (valueBytes > mLimits.maxValueBytes) {
        return Result<void>::err(ErrorCode::ResourceLimit,
                                 "storage value exceeds the per-key size limit");
    }

    auto& entries = mMods[modId];
    std::size_t& totalBytes = mBytesPerMod[modId];
    const auto existing = entries.find(key);
    const std::size_t previousBytes =
        existing != entries.end() ? key.size() + ValueBytes(existing->second) : 0;
    const std::size_t newBytes = key.size() + valueBytes;

    if (existing == entries.end() && entries.size() >= mLimits.maxKeysPerMod) {
        return Result<void>::err(ErrorCode::ResourceLimit,
                                 "storage key count limit reached for this mod");
    }
    if (totalBytes - previousBytes + newBytes > mLimits.maxTotalBytesPerMod) {
        return Result<void>::err(ErrorCode::ResourceLimit,
                                 "storage total size limit reached for this mod");
    }

    totalBytes = totalBytes - previousBytes + newBytes;
    entries[key] = std::move(value);
    return Result<void>::ok();
}

Result<bool> KeyValueStorage::Delete(const std::string& modId, const std::string& key) {
    const auto valid = ValidateKey(modId, key);
    if (!valid.isOk()) {
        return Result<bool>::err(valid.code, valid.message);
    }
    const auto mod = mMods.find(modId);
    if (mod == mMods.end()) {
        return Result<bool>::ok(false);
    }
    const auto entry = mod->second.find(key);
    if (entry == mod->second.end()) {
        return Result<bool>::ok(false);
    }
    mBytesPerMod[modId] -= key.size() + ValueBytes(entry->second);
    mod->second.erase(entry);
    if (mod->second.empty()) {
        mMods.erase(mod);
        mBytesPerMod.erase(modId);
    }
    return Result<bool>::ok(true);
}

Result<std::size_t> KeyValueStorage::Clear(const std::string& modId) {
    if (modId.empty()) {
        return Result<std::size_t>::err(ErrorCode::InvalidArgument, "storage requires a mod id");
    }
    const auto mod = mMods.find(modId);
    if (mod == mMods.end()) {
        return Result<std::size_t>::ok(0);
    }
    const std::size_t removed = mod->second.size();
    mMods.erase(mod);
    mBytesPerMod.erase(modId);
    return Result<std::size_t>::ok(removed);
}

Result<std::vector<std::string>> KeyValueStorage::Keys(const std::string& modId) const {
    if (modId.empty()) {
        return Result<std::vector<std::string>>::err(ErrorCode::InvalidArgument,
                                                     "storage requires a mod id");
    }
    std::vector<std::string> keys;
    const auto mod = mMods.find(modId);
    if (mod != mMods.end()) {
        keys.reserve(mod->second.size());
        for (const auto& [key, value] : mod->second) {
            (void)value;
            keys.push_back(key);
        }
    }
    return Result<std::vector<std::string>>::ok(std::move(keys));
}

std::size_t KeyValueStorage::KeyCount(const std::string& modId) const {
    const auto mod = mMods.find(modId);
    return mod != mMods.end() ? mod->second.size() : 0;
}

std::size_t KeyValueStorage::TotalBytes(const std::string& modId) const {
    const auto entry = mBytesPerMod.find(modId);
    return entry != mBytesPerMod.end() ? entry->second : 0;
}

} // namespace ShipLua
