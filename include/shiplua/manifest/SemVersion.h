#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

class SemVersion {
  public:
    std::uint64_t major = 0;
    std::uint64_t minor = 0;
    std::uint64_t patch = 0;
    std::vector<std::string> prerelease;
    std::vector<std::string> build;

    static Result<SemVersion> Parse(const std::string& text, bool allowPartial = false);
    int Compare(const SemVersion& other) const;

    bool operator==(const SemVersion& other) const { return Compare(other) == 0; }
    bool operator<(const SemVersion& other) const { return Compare(other) < 0; }
};

class VersionRange {
  public:
    static Result<VersionRange> Parse(const std::string& text);
    bool Contains(const SemVersion& version) const;

  private:
    enum class Operator { Equal, Greater, GreaterEqual, Less, LessEqual };
    struct Comparator {
        Operator operation;
        SemVersion version;
    };
    std::vector<Comparator> mComparators;
};

} // namespace ShipLua
