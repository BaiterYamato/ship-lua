#include "shiplua/manifest/SemVersion.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>
#include <string_view>

namespace ShipLua {

namespace {

bool IsIdentifierCharacter(unsigned char value) {
    return (value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z') ||
           (value >= 'a' && value <= 'z') || value == '-';
}

Result<std::vector<std::string>> ParseIdentifiers(std::string_view text, bool prerelease) {
    std::vector<std::string> identifiers;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('.', start);
        const std::string identifier(text.substr(start, end == std::string_view::npos
                                                            ? text.size() - start
                                                            : end - start));
        if (identifier.empty() ||
            !std::all_of(identifier.begin(), identifier.end(),
                         [](unsigned char value) { return IsIdentifierCharacter(value); })) {
            return Result<std::vector<std::string>>::err(
                ErrorCode::InvalidArgument, "semantic version contains an invalid identifier");
        }
        const bool numeric = std::all_of(identifier.begin(), identifier.end(),
                                         [](unsigned char value) { return std::isdigit(value) != 0; });
        if (prerelease && numeric && identifier.size() > 1 && identifier.front() == '0') {
            return Result<std::vector<std::string>>::err(
                ErrorCode::InvalidArgument,
                "numeric prerelease identifiers cannot contain leading zeroes");
        }
        identifiers.push_back(identifier);
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return Result<std::vector<std::string>>::ok(std::move(identifiers));
}

Result<std::uint64_t> ParseCoreNumber(std::string_view text) {
    if (text.empty() || (text.size() > 1 && text.front() == '0')) {
        return Result<std::uint64_t>::err(
            ErrorCode::InvalidArgument, "semantic version core numbers cannot contain leading zeroes");
    }
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return Result<std::uint64_t>::err(ErrorCode::InvalidArgument,
                                          "semantic version core contains a non-numeric value");
    }
    return Result<std::uint64_t>::ok(value);
}

int ComparePrereleaseIdentifier(const std::string& left, const std::string& right) {
    const bool leftNumeric = std::all_of(left.begin(), left.end(),
                                         [](unsigned char value) { return std::isdigit(value) != 0; });
    const bool rightNumeric = std::all_of(right.begin(), right.end(),
                                          [](unsigned char value) { return std::isdigit(value) != 0; });
    if (leftNumeric != rightNumeric) {
        return leftNumeric ? -1 : 1;
    }
    if (leftNumeric && left.size() != right.size()) {
        return left.size() < right.size() ? -1 : 1;
    }
    if (left == right) {
        return 0;
    }
    return left < right ? -1 : 1;
}

} // namespace

Result<SemVersion> SemVersion::Parse(const std::string& text, bool allowPartial) {
    if (text.empty() || text.find_first_of(" \t\r\n") != std::string::npos) {
        return Result<SemVersion>::err(ErrorCode::InvalidArgument,
                                       "semantic version is empty or contains whitespace");
    }

    std::string_view remaining(text);
    std::string_view buildText;
    if (const std::size_t plus = remaining.find('+'); plus != std::string_view::npos) {
        if (remaining.find('+', plus + 1) != std::string_view::npos) {
            return Result<SemVersion>::err(ErrorCode::InvalidArgument,
                                           "semantic version contains multiple build separators");
        }
        buildText = remaining.substr(plus + 1);
        remaining = remaining.substr(0, plus);
    }

    std::string_view prereleaseText;
    if (const std::size_t dash = remaining.find('-'); dash != std::string_view::npos) {
        prereleaseText = remaining.substr(dash + 1);
        remaining = remaining.substr(0, dash);
    }

    std::vector<std::string_view> core;
    std::size_t start = 0;
    while (start <= remaining.size()) {
        const std::size_t end = remaining.find('.', start);
        core.push_back(remaining.substr(start, end == std::string_view::npos
                                                  ? remaining.size() - start
                                                  : end - start));
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    if ((!allowPartial && core.size() != 3) || (allowPartial && (core.empty() || core.size() > 3))) {
        return Result<SemVersion>::err(
            ErrorCode::InvalidArgument,
            allowPartial ? "range version must contain one to three core numbers"
                         : "semantic version must contain major, minor, and patch numbers");
    }

    SemVersion version;
    std::uint64_t* fields[] = {&version.major, &version.minor, &version.patch};
    for (std::size_t i = 0; i < core.size(); ++i) {
        auto number = ParseCoreNumber(core[i]);
        if (!number.isOk()) {
            return Result<SemVersion>::err(number.code, number.message);
        }
        *fields[i] = *number.value;
    }
    if (!prereleaseText.empty()) {
        auto identifiers = ParseIdentifiers(prereleaseText, true);
        if (!identifiers.isOk()) {
            return Result<SemVersion>::err(identifiers.code, identifiers.message);
        }
        version.prerelease = std::move(*identifiers.value);
    } else if (remaining.size() != text.size() && text.find('-') != std::string::npos) {
        return Result<SemVersion>::err(ErrorCode::InvalidArgument,
                                       "semantic version prerelease cannot be empty");
    }
    if (!buildText.empty()) {
        auto identifiers = ParseIdentifiers(buildText, false);
        if (!identifiers.isOk()) {
            return Result<SemVersion>::err(identifiers.code, identifiers.message);
        }
        version.build = std::move(*identifiers.value);
    } else if (text.find('+') != std::string::npos) {
        return Result<SemVersion>::err(ErrorCode::InvalidArgument,
                                       "semantic version build metadata cannot be empty");
    }
    return Result<SemVersion>::ok(std::move(version));
}

int SemVersion::Compare(const SemVersion& other) const {
    if (major != other.major) return major < other.major ? -1 : 1;
    if (minor != other.minor) return minor < other.minor ? -1 : 1;
    if (patch != other.patch) return patch < other.patch ? -1 : 1;
    if (prerelease.empty() != other.prerelease.empty()) return prerelease.empty() ? 1 : -1;
    for (std::size_t i = 0; i < std::min(prerelease.size(), other.prerelease.size()); ++i) {
        const int compared = ComparePrereleaseIdentifier(prerelease[i], other.prerelease[i]);
        if (compared != 0) return compared;
    }
    if (prerelease.size() == other.prerelease.size()) return 0;
    return prerelease.size() < other.prerelease.size() ? -1 : 1;
}

Result<VersionRange> VersionRange::Parse(const std::string& text) {
    std::istringstream stream(text);
    std::string token;
    VersionRange range;
    while (stream >> token) {
        Operator operation = Operator::Equal;
        std::size_t prefix = 0;
        if (token.starts_with(">=")) {
            operation = Operator::GreaterEqual;
            prefix = 2;
        } else if (token.starts_with("<=")) {
            operation = Operator::LessEqual;
            prefix = 2;
        } else if (token.starts_with(">")) {
            operation = Operator::Greater;
            prefix = 1;
        } else if (token.starts_with("<")) {
            operation = Operator::Less;
            prefix = 1;
        } else if (token.starts_with("=")) {
            prefix = 1;
        }
        auto version = SemVersion::Parse(token.substr(prefix), true);
        if (!version.isOk()) {
            return Result<VersionRange>::err(
                version.code, "invalid version range comparator '" + token + "': " + version.message);
        }
        range.mComparators.push_back({operation, std::move(*version.value)});
    }
    if (range.mComparators.empty()) {
        return Result<VersionRange>::err(ErrorCode::InvalidArgument, "version range is empty");
    }
    return Result<VersionRange>::ok(std::move(range));
}

bool VersionRange::Contains(const SemVersion& version) const {
    for (const Comparator& comparator : mComparators) {
        const int compared = version.Compare(comparator.version);
        const bool matches =
            (comparator.operation == Operator::Equal && compared == 0) ||
            (comparator.operation == Operator::Greater && compared > 0) ||
            (comparator.operation == Operator::GreaterEqual && compared >= 0) ||
            (comparator.operation == Operator::Less && compared < 0) ||
            (comparator.operation == Operator::LessEqual && compared <= 0);
        if (!matches) return false;
    }
    return true;
}

} // namespace ShipLua
