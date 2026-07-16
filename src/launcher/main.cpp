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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
    MessageBoxW(nullptr, message.c_str(), L"Link-Span", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

std::optional<Game> ChooseGame(const AssetSet& assets) {
    if (!assets.HasBoth()) {
        return assets.oot ? std::optional<Game>(Game::Oot) : std::optional<Game>(Game::Mm);
    }
    const int answer = MessageBoxW(
        nullptr,
        L"Os assets de Ocarina of Time e Majora's Mask foram encontrados.\n\n"
        L"Sim: Ocarina of Time\nNão: Majora's Mask\nCancelar: sair",
        L"Link-Span — escolher jogo", MB_YESNOCANCEL | MB_ICONQUESTION | MB_SETFOREGROUND);
    if (answer == IDYES) {
        return Game::Oot;
    }
    if (answer == IDNO) {
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
    if (assets.Empty()) {
        ShowError(
            L"Nenhum jogo foi encontrado ao lado de link-span.exe.\n\n"
            L"Adicione oot.o2r/oot.otr ou uma ROM legítima de Ocarina of Time; "
            L"e/ou mm.o2r ou uma ROM legítima de Majora's Mask.");
        return 2;
    }
    const auto dualMods = DiscoverDualWorldMods(root);
    if (!dualMods.empty() && !assets.HasBoth()) {
        // Exit code 8 = a dual-world mod (requires_both_games = true) is
        // installed but only one game's assets are present. The launcher never
        // inspects packaged .shipmod files (see docs/link-span-process.md), so
        // only unpacked development mods in mods/ are listed here. The host's
        // own modloader validates packaged mods at load time.
        std::wstring message =
            L"Mods que exigem os dois jogos (requires_both_games = true) estão "
            L"instalados, mas apenas um asset foi encontrado:\n\n";
        for (const auto& mod : dualMods) {
            message += L"  • ";
            message += std::wstring(mod.begin(), mod.end());
            message += L"\n";
        }
        message +=
            L"\nAdicione o asset (oot.o2r/oot.otr e mm.o2r) do jogo ausente ou "
            L"remova estes mods de mods/ para continuar.";
        ShowError(message);
        return 8;
    }

    std::optional<Game> game = ChooseGame(assets);
    if (!game.has_value()) {
        return 0;
    }
    const SessionSecrets secrets = CreateSessionSecrets();
    for (int switches = 0; switches <= kMaximumSwitches; ++switches) {
        if (!assets.Has(*game)) {
            ShowError(L"O jogo solicitado pelo handoff não possui asset disponível.");
            return 3;
        }
        const auto host = FindHost(root, *game);
        if (!host.has_value()) {
            ShowError(*game == Game::Oot
                ? L"O host hosts/oot/soh.exe não foi encontrado. Execute build-linkspan.ps1."
                : L"O host hosts/mm/2ship.exe não foi encontrado. Execute build-linkspan.ps1.");
            return 4;
        }
        const unsigned long exitCode = RunHost(root, *host, *game, assets, secrets,
                                               static_cast<std::uint64_t>(switches + 1));
        if (exitCode == static_cast<unsigned long>(-1)) {
            ShowError(L"O Windows não conseguiu iniciar o executável do host.");
            return 5;
        }
        if (exitCode != kSwitchWorldExitCode) {
            return static_cast<int>(exitCode);
        }
        game = ConsumeNextWorld(root);
        if (!game.has_value()) {
            ShowError(L"O host solicitou troca, mas state/next-world é inválido ou está ausente.");
            return 6;
        }
    }
    ShowError(L"A sessão excedeu o limite de 16 trocas consecutivas entre jogos.");
    return 7;
}
#else
int main() {
    return 1;
}
#endif
