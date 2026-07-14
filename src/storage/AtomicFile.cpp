#include "shiplua/storage/AtomicFile.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ShipLua {
namespace {

std::atomic<std::uint64_t> gTemporaryCounter{0};

std::filesystem::path ParentOrCurrent(const std::filesystem::path& destination) {
    if (destination.parent_path().empty()) {
        return std::filesystem::current_path();
    }
    return destination.parent_path();
}

std::uint64_t ProcessId() {
#ifdef _WIN32
    return static_cast<std::uint64_t>(GetCurrentProcessId());
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

std::filesystem::path MakeTemporaryPath(const std::filesystem::path& destination) {
    const std::uint64_t sequence = gTemporaryCounter.fetch_add(1, std::memory_order_relaxed);
    std::filesystem::path name = destination.filename();
    name += ".shiplua-tmp-" + std::to_string(ProcessId()) + "-" + std::to_string(sequence);
    return ParentOrCurrent(destination) / name;
}

class TemporaryGuard {
  public:
    explicit TemporaryGuard(std::filesystem::path path) : mPath(std::move(path)) {
    }

    ~TemporaryGuard() {
        if (!mCommitted) {
            std::error_code ignored;
            std::filesystem::remove(mPath, ignored);
        }
    }

    void Commit() noexcept {
        mCommitted = true;
    }

  private:
    std::filesystem::path mPath;
    bool mCommitted = false;
};

Result<void> HostError(std::string message, std::uint64_t code) {
    return Result<void>::err(ErrorCode::HostFailure,
                             std::move(message) + " (code " + std::to_string(code) + ")");
}

#ifdef _WIN32
namespace {

// Reattempts the destination replace a bounded number of times when the host
// reports a transient sharing violation. Two writers committing to the same
// destination race inside MoveFileExW: the losing writer observes
// ERROR_ACCESS_DENIED while the winner's handle is still being released by the
// filesystem layer. This is a well-known Windows quirk for replace-on-rename
// under contention and clears within microseconds, so we back off briefly and
// retry instead of surfacing a spurious failure to the caller.
Result<void> ReplaceWithRetry(const std::filesystem::path& temporary,
                              const std::filesystem::path& destination) {
    constexpr int kMaxAttempts = 16;
    constexpr auto kBackoff = std::chrono::milliseconds(2);
    DWORD lastError = ERROR_SUCCESS;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        if (MoveFileExW(temporary.c_str(), destination.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return Result<void>::ok();
        }
        lastError = GetLastError();
        if (lastError != ERROR_ACCESS_DENIED) {
            break;
        }
        std::this_thread::sleep_for(kBackoff);
    }
    return HostError("cannot commit atomic file", lastError);
}

} // namespace

Result<void> WriteTemporary(const std::filesystem::path& path, std::span<const std::byte> contents) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return HostError("cannot create atomic temporary file", GetLastError());
    }

    std::size_t offset = 0;
    while (offset < contents.size()) {
        const std::size_t remaining = contents.size() - offset;
        const DWORD requested = static_cast<DWORD>(
            std::min<std::size_t>(remaining, static_cast<std::size_t>(MAXDWORD)));
        DWORD written = 0;
        if (!WriteFile(file, contents.data() + offset, requested, &written, nullptr) || written == 0) {
            const DWORD error = GetLastError();
            CloseHandle(file);
            return HostError("cannot write atomic temporary file", error);
        }
        offset += written;
    }
    if (!FlushFileBuffers(file)) {
        const DWORD error = GetLastError();
        CloseHandle(file);
        return HostError("cannot flush atomic temporary file", error);
    }
    if (!CloseHandle(file)) {
        return HostError("cannot close atomic temporary file", GetLastError());
    }
    return Result<void>::ok();
}

Result<void> ReplaceDestination(const std::filesystem::path& temporary,
                                const std::filesystem::path& destination) {
    return ReplaceWithRetry(temporary, destination);
}
#else
Result<void> WriteTemporary(const std::filesystem::path& path, std::span<const std::byte> contents) {
    const int file = open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (file < 0) {
        return HostError("cannot create atomic temporary file", errno);
    }

    std::size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t written = write(file, contents.data() + offset, contents.size() - offset);
        if (written <= 0) {
            const int error = written < 0 ? errno : EIO;
            close(file);
            return HostError("cannot write atomic temporary file", error);
        }
        offset += static_cast<std::size_t>(written);
    }
    if (fsync(file) != 0) {
        const int error = errno;
        close(file);
        return HostError("cannot flush atomic temporary file", error);
    }
    if (close(file) != 0) {
        return HostError("cannot close atomic temporary file", errno);
    }
    return Result<void>::ok();
}

Result<void> ReplaceDestination(const std::filesystem::path& temporary,
                                const std::filesystem::path& destination) {
    // rename(2) over an existing destination is atomic on POSIX local
    // filesystems, so there is no Windows-style sharing quirk to absorb. We
    // still retry EINTR defensively so a signal cannot turn a successful
    // replace into a reported failure.
    int lastError = 0;
    for (int attempt = 0; attempt < 16; ++attempt) {
        if (rename(temporary.c_str(), destination.c_str()) == 0) {
            lastError = 0;
            break;
        }
        lastError = errno;
        if (lastError != EINTR) {
            break;
        }
    }
    if (lastError != 0) {
        return HostError("cannot commit atomic file", static_cast<std::uint64_t>(lastError));
    }
    const std::filesystem::path parent = ParentOrCurrent(destination);
    const int directory = open(parent.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory < 0) {
        return HostError("cannot open atomic destination directory", static_cast<std::uint64_t>(errno));
    }
    if (fsync(directory) != 0) {
        const int error = errno;
        close(directory);
        return HostError("cannot flush atomic destination directory", static_cast<std::uint64_t>(error));
    }
    if (close(directory) != 0) {
        return HostError("cannot close atomic destination directory", static_cast<std::uint64_t>(errno));
    }
    return Result<void>::ok();
}
#endif

} // namespace

Result<void> AtomicFile::Write(const std::filesystem::path& destination,
                               std::span<const std::byte> contents) {
    if (destination.empty() || destination.filename().empty()) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "atomic destination must name a file");
    }

    std::error_code error;
    const std::filesystem::path parent = ParentOrCurrent(destination);
    if (!std::filesystem::is_directory(parent, error) || error) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "atomic destination parent must exist");
    }
    if (std::filesystem::is_directory(destination, error) && !error) {
        return Result<void>::err(ErrorCode::InvalidArgument,
                                 "atomic destination cannot be a directory");
    }
    error.clear();

    const std::filesystem::path temporary = MakeTemporaryPath(destination);
    TemporaryGuard guard(temporary);
    Result<void> written = WriteTemporary(temporary, contents);
    if (!written.isOk()) {
        return written;
    }
    Result<void> replaced = ReplaceDestination(temporary, destination);
    if (!replaced.isOk()) {
        return replaced;
    }
    guard.Commit();
    return Result<void>::ok();
}

} // namespace ShipLua
