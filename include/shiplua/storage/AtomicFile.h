#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string_view>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

class AtomicFile {
  public:
    static Result<void> Write(const std::filesystem::path& destination,
                              std::span<const std::byte> contents);

    static Result<void> Write(const std::filesystem::path& destination,
                              std::string_view contents) {
        return Write(destination,
                     std::as_bytes(std::span(contents.data(), contents.size())));
    }
};

} // namespace ShipLua
