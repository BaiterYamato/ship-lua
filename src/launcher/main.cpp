#include "linkspan/launcher/AssetDiscovery.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <span>
#include <string>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#ifdef _MSC_VER
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

namespace LinkSpan::Launcher {
namespace {

constexpr unsigned long kSwitchWorldExitCode = 73;
constexpr int kMaximumSwitches = 16;

struct SessionSecrets {
    std::array<std::byte, 16> id{};
    std::array<std::byte, 32> key{};
};

template <std::size_t Size>
void FillRandom(std::array<std::byte, Size>& output, std::random_device& random) {
    for (std::byte& value : output) {
        value = static_cast<std::byte>(random() & 0xffU);
    }
}

SessionSecrets CreateSessionSecrets() {
    std::random_device random;
    SessionSecrets secrets;
    FillRandom(secrets.id, random);
    FillRandom(secrets.key, random);
    return secrets;
}

template <std::size_t Size>
std::wstring Hex(const std::array<std::byte, Size>& value) {
    static constexpr wchar_t digits[] = L"0123456789abcdef";
    std::wstring encoded;
    encoded.reserve(Size * 2);
    for (const std::byte byte : value) {
        const unsigned int number = std::to_integer<unsigned int>(byte);
        encoded.push_back(digits[number >> 4U]);
        encoded.push_back(digits[number & 0x0fU]);
    }
    return encoded;
}

#ifdef _WIN32
std::filesystem::path ExecutableDirectory() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return std::filesystem::current_path();
    }
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}

void ShowError(const std::wstring& message) {
    MessageBoxW(nullptr, message.c_str(), L"Link-Span Error",
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

std::optional<Game> ChooseGame(const AssetSet& assets) {
    if (!assets.HasBoth()) {
        return assets.oot ? std::optional<Game>(Game::Oot) : std::optional<Game>(Game::Mm);
    }
    constexpr int kChooseOot = 100;
    constexpr int kChooseMm = 101;
    constexpr int kExit = 102;
    const TASKDIALOG_BUTTON buttons[] = {
        {kChooseOot, L"Ocarina of Time"},
        {kChooseMm, L"Majora's Mask"},
        {kExit, L"Exit"},
    };
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof(dialog);
    dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SIZE_TO_CONTENT;
    dialog.pszWindowTitle = L"Link-Span - Choose Game";
    dialog.pszMainInstruction = L"Choose a game";
    dialog.pszContent = L"Both Ocarina of Time and Majora's Mask were found.";
    dialog.pszMainIcon = TD_INFORMATION_ICON;
    dialog.cButtons = static_cast<UINT>(std::size(buttons));
    dialog.pButtons = buttons;
    dialog.nDefaultButton = kChooseOot;

    int answer = kExit;
    const HRESULT result = TaskDialogIndirect(&dialog, &answer, nullptr, nullptr);
    if (FAILED(result)) {
        ShowError(L"Windows could not create the game selection dialog.");
        return std::nullopt;
    }
    if (answer == kChooseOot) {
        return Game::Oot;
    }
    if (answer == kChooseMm) {
        return Game::Mm;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> FindHost(const std::filesystem::path& root, Game game) {
    const std::filesystem::path nested = game == Game::Oot
        ? root / "hosts" / "oot" / "soh.exe"
        : root / "hosts" / "mm" / "2ship.exe";
    const std::filesystem::path flat = game == Game::Oot ? root / "soh.exe" : root / "2ship.exe";
    if (std::filesystem::is_regular_file(nested)) {
        return nested;
    }
    if (std::filesystem::is_regular_file(flat)) {
        return flat;
    }
    return std::nullopt;
}

unsigned long RunHost(const std::filesystem::path& root, const std::filesystem::path& executable,
                      Game game, const AssetSet& assets, const SessionSecrets& secrets,
                      std::uint64_t sequence) {
    const std::filesystem::path session = root / "state";
    std::filesystem::create_directories(session);
    std::filesystem::create_directories(root / "mods");
    _wputenv_s(L"LINKSPAN_ROOT", root.c_str());
    _wputenv_s(L"LINKSPAN_WORLD", game == Game::Oot ? L"oot" : L"mm");
    _wputenv_s(L"LINKSPAN_SESSION_DIR", session.c_str());
    _wputenv_s(L"LINKSPAN_HANDOFF_PATH", (session / "handoff.bin").c_str());
    _wputenv_s(L"LINKSPAN_SESSION_ID", Hex(secrets.id).c_str());
    _wputenv_s(L"LINKSPAN_AUTH_KEY", Hex(secrets.key).c_str());
    _wputenv_s(L"LINKSPAN_SEQUENCE", std::to_wstring(sequence).c_str());
    _wputenv_s(L"LINKSPAN_AVAILABLE_GAMES", assets.HasBoth()
        ? L"oot,mm"
        : (assets.oot ? L"oot" : L"mm"));

    std::wstring command = L"\"" + executable.wstring() + L"\"";
    command.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(
        executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr,
        root.c_str(), &startup, &process);
    if (!created) {
        return static_cast<unsigned long>(-1);
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exitCode;
}

std::optional<Game> ConsumeNextWorld(const std::filesystem::path& root) {
    const std::filesystem::path request = root / "state" / "next-world";
    std::ifstream stream(request);
    std::string value;
    std::getline(stream, value);
    std::error_code ignored;
    std::filesystem::remove(request, ignored);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (value == "oot") {
        return Game::Oot;
    }
    if (value == "mm") {
        return Game::Mm;
    }
    return std::nullopt;
}
#endif

} // namespace
} // namespace LinkSpan::Launcher

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    using namespace LinkSpan::Launcher;
    const std::filesystem::path root = ExecutableDirectory();
    const AssetSet assets = DiscoverAssets(root);
    const auto dualMods = DiscoverDualWorldMods(root);
    const StartupDecision startup = DecideStartup(assets, !dualMods.empty());
    if (startup == StartupDecision::MissingAssets) {
        ShowError(
            L"No supported game was found next to link-span.exe.\n\n"
            L"Add oot.o2r/oot.otr or a legally obtained Ocarina of Time ROM, "
            L"and/or mm.o2r or a legally obtained Majora's Mask ROM.");
        return 2;
    }
    if (startup == StartupDecision::DualGameAssetsRequired) {
        // Exit code 8 = a dual-world mod (requires_both_games = true) is
        // installed but only one game's assets are present.
        std::wstring message =
            L"The following mods require both games (requires_both_games = true), "
            L"but assets for only one game were found:\n\n";
        for (const auto& mod : dualMods) {
            message += L"  - ";
            message += std::wstring(mod.begin(), mod.end());
            message += L"\n";
        }
        message +=
            L"\nAdd the missing game's ROM or extracted assets, or remove these "
            L"mods from mods/ to continue.";
        ShowError(message);
        return 8;
    }

    std::optional<Game> game;
    if (startup == StartupDecision::ChooseGame) {
        game = ChooseGame(assets);
    } else {
        game = startup == StartupDecision::LaunchOot ? Game::Oot : Game::Mm;
    }
    if (!game.has_value()) {
        return 0;
    }
    const SessionSecrets secrets = CreateSessionSecrets();
    for (int switches = 0; switches <= kMaximumSwitches; ++switches) {
        if (!assets.Has(*game)) {
            ShowError(L"The game requested by the handoff has no available ROM or asset.");
            return 3;
        }
        const auto host = FindHost(root, *game);
        if (!host.has_value()) {
            ShowError(*game == Game::Oot
                ? L"The host hosts/oot/soh.exe was not found. Run build-linkspan.ps1."
                : L"The host hosts/mm/2ship.exe was not found. Run build-linkspan.ps1.");
            return 4;
        }
        const unsigned long exitCode = RunHost(root, *host, *game, assets, secrets,
                                               static_cast<std::uint64_t>(switches + 1));
        if (exitCode == static_cast<unsigned long>(-1)) {
            ShowError(L"Windows could not start the game host executable.");
            return 5;
        }
        if (exitCode != kSwitchWorldExitCode) {
            return static_cast<int>(exitCode);
        }
        game = ConsumeNextWorld(root);
        if (!game.has_value()) {
            ShowError(L"The host requested a world switch, but state/next-world is missing or invalid.");
            return 6;
        }
    }
    ShowError(L"The session exceeded the limit of 16 consecutive world switches.");
    return 7;
}
#else
int main() {
    return 1;
}
#endif
