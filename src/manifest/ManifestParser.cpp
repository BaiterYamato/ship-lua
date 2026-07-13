#include "shiplua/manifest/ManifestParser.h"

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <toml++/toml.hpp>

namespace ShipLua {

namespace {

// Reads an optional array-of-strings at `key` from `table` into `out`.
// Non-string elements and non-array values are ignored (forward compatible).
void ReadStringArray(const toml::table& table, std::string_view key, std::vector<std::string>& out) {
    if (const toml::array* arr = table[key].as_array()) {
        for (const auto& element : *arr) {
            if (auto str = element.value<std::string>()) {
                out.push_back(std::move(*str));
            }
        }
    }
}

// Reads a required top-level string field; appends to `missing` when absent
// or not a string.
std::string ReadRequiredString(const toml::table& table, std::string_view key,
                               std::vector<std::string>& missing) {
    if (auto str = table[key].value<std::string>()) {
        return std::move(*str);
    }
    missing.emplace_back(key);
    return {};
}

Result<Manifest> ParseTable(const toml::table& table, const std::string& sourceName) {
    Manifest manifest;

    std::vector<std::string> missing;
    manifest.id = ReadRequiredString(table, "id", missing);
    manifest.name = ReadRequiredString(table, "name", missing);
    manifest.version = ReadRequiredString(table, "version", missing);
    manifest.apiRange = ReadRequiredString(table, "api", missing);
    manifest.entrypoint = ReadRequiredString(table, "entrypoint", missing);

    if (!missing.empty()) {
        std::ostringstream oss;
        oss << sourceName << ": missing required field";
        if (missing.size() > 1) {
            oss << "s";
        }
        for (std::size_t i = 0; i < missing.size(); ++i) {
            oss << (i == 0 ? " '" : ", '") << missing[i] << "'";
        }
        return Result<Manifest>::err(ErrorCode::InvalidArgument, oss.str());
    }

    manifest.description = table["description"].value_or(std::string{});
    ReadStringArray(table, "authors", manifest.authors);
    ReadStringArray(table, "games", manifest.games);

    if (const toml::table* host = table["host"].as_table()) {
        if (auto v = (*host)["shipwright"].value<std::string>()) {
            manifest.hostShipwright = std::move(*v);
        }
        if (auto v = (*host)["two_ship"].value<std::string>()) {
            manifest.hostTwoShip = std::move(*v);
        }
    }

    if (const toml::table* caps = table["capabilities"].as_table()) {
        ReadStringArray(*caps, "required", manifest.capabilitiesRequired);
        ReadStringArray(*caps, "optional", manifest.capabilitiesOptional);
    }

    if (const toml::table* deps = table["dependencies"].as_table()) {
        for (const auto& [depId, node] : *deps) {
            if (auto range = node.value<std::string>()) {
                manifest.dependencies.emplace_back(std::string(depId.str()), std::move(*range));
            }
        }
    }

    if (const toml::table* load = table["load"].as_table()) {
        manifest.loadPriority = static_cast<int>((*load)["priority"].value_or(int64_t{50}));
        ReadStringArray(*load, "after", manifest.loadAfter);
        ReadStringArray(*load, "before", manifest.loadBefore);
    }

    if (const toml::table* perms = table["permissions"].as_table()) {
        manifest.permStorage = (*perms)["storage"].value_or(false);
        manifest.permNetwork = (*perms)["network"].value_or(false);
        manifest.permClipboard = (*perms)["clipboard"].value_or(false);
    }

    return Result<Manifest>::ok(std::move(manifest));
}

} // namespace

Result<Manifest> ParseManifestString(const std::string& toml, const std::string& sourceName) {
    try {
        toml::table table = toml::parse(toml, sourceName);
        return ParseTable(table, sourceName);
    } catch (const toml::parse_error& e) {
        std::ostringstream oss;
        oss << sourceName << ": TOML parse error at line " << e.source().begin.line
            << ", column " << e.source().begin.column << ": " << e.description();
        return Result<Manifest>::err(ErrorCode::InvalidArgument, oss.str());
    } catch (const std::exception& e) {
        return Result<Manifest>::err(ErrorCode::InvalidArgument,
                                     sourceName + ": unexpected error while parsing: " + e.what());
    } catch (...) {
        return Result<Manifest>::err(ErrorCode::InvalidArgument,
                                     sourceName + ": unexpected unknown error while parsing");
    }
}

Result<Manifest> ParseManifestFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return Result<Manifest>::err(ErrorCode::InvalidArgument,
                                     "cannot open manifest file '" + path + "'");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad()) {
        return Result<Manifest>::err(ErrorCode::InvalidArgument,
                                     "failed to read manifest file '" + path + "'");
    }
    return ParseManifestString(contents.str(), path);
}

} // namespace ShipLua
