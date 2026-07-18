#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "shiplua/manifest/PackageExtractor.h"
#include "shiplua/testing/ModTestRunner.h"

namespace {

class TempDirectory {
  public:
    explicit TempDirectory(std::filesystem::path path) : mPath(std::move(path)) {}
    ~TempDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(mPath, ignored);
    }
    const std::filesystem::path& Path() const { return mPath; }

  private:
    std::filesystem::path mPath;
};

void PrintUsage() {
    std::cerr << "Uso: shiplua_mod_test_runner <diretório-do-mod|pacote.shipmod> "
                 "[--game oot|mm] [--capability <nome>]...\n"
                 "\n"
                 "Executa os testes do mod (<mod>/tests/*.lua) no mock runtime, sem jogo/ROM.\n"
                 "Saída: 0 = todos passaram; 1 = falhas ou erros; 2 = uso inválido.\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage();
        return 2;
    }

    ShipLua::MockHostOptions options;
    std::filesystem::path input;
    bool haveInput = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--game") {
            if (i + 1 >= argc) {
                PrintUsage();
                return 2;
            }
            options.gameId = argv[++i];
            if (options.gameId != "oot" && options.gameId != "mm") {
                std::cerr << "--game deve ser 'oot' ou 'mm'\n";
                return 2;
            }
        } else if (argument == "--capability") {
            if (i + 1 >= argc) {
                PrintUsage();
                return 2;
            }
            options.extraCapabilities.emplace_back(argv[++i]);
        } else if (!argument.empty() && argument.front() == '-') {
            std::cerr << "Opção desconhecida: " << argument << '\n';
            PrintUsage();
            return 2;
        } else if (!haveInput) {
            input = argument;
            haveInput = true;
        } else {
            std::cerr << "Apenas um caminho de mod é aceito por execução.\n";
            PrintUsage();
            return 2;
        }
    }
    if (!haveInput) {
        PrintUsage();
        return 2;
    }

    std::filesystem::path modDir = input;
    std::unique_ptr<TempDirectory> temporary;
    std::error_code error;
    const auto status = std::filesystem::symlink_status(input, error);
    if (error || std::filesystem::is_symlink(status)) {
        std::cerr << "Entrada ausente, inacessível ou simbólica: " << input.string() << '\n';
        return 2;
    }
    if (std::filesystem::is_regular_file(status)) {
        if (input.extension() != ".shipmod") {
            std::cerr << "Arquivo de entrada não é um pacote .shipmod: " << input.string() << '\n';
            return 2;
        }
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temporary = std::make_unique<TempDirectory>(
            std::filesystem::temp_directory_path() /
            ("shiplua-mod-test-" + std::to_string(stamp)));
        const std::filesystem::path extracted = temporary->Path() / "conteudo";
        const auto extraction = ShipLua::ExtractShipmod(input, extracted);
        if (!extraction.isOk()) {
            std::cerr << "Falha ao extrair o pacote: " << extraction.message << '\n';
            return 1;
        }
        modDir = extracted;
    } else if (!std::filesystem::is_directory(status)) {
        std::cerr << "Entrada não é diretório nem arquivo regular: " << input.string() << '\n';
        return 2;
    }

    ShipLua::ModTestRunner runner(std::move(options));
    const auto report = runner.Run(modDir);
    if (!report.isOk()) {
        std::cerr << "Falha ao executar os testes: " << report.message << '\n';
        return 1;
    }
    const ShipLua::ModTestReport& result = *report.value;

    if (result.noTestsFound) {
        std::cout << "Nenhum teste encontrado em " << (modDir / "tests").string()
                  << " — nada a executar.\n";
        return 0;
    }

    std::cout << "Mod: " << (result.modId.empty() ? "(desconhecido)" : result.modId) << '\n';
    for (const ShipLua::ModTestCaseResult& test : result.tests) {
        if (test.ok) {
            std::cout << "[ok] " << test.file << " :: " << test.test << '\n';
        } else {
            std::cout << "[falha] " << test.file << " :: " << test.test;
            if (!test.message.empty()) {
                const auto newline = test.message.find('\n');
                std::cout << " — " << test.message.substr(0, newline);
            }
            std::cout << '\n';
        }
    }
    for (const std::string& infraError : result.errors) {
        std::cout << "[erro] " << infraError << '\n';
    }

    const std::size_t passed = result.PassedCount();
    const std::size_t failed = result.FailedCount();
    std::cout << "Resumo: " << passed << " passaram, " << failed << " falharam ("
              << result.tests.size() << " testes em " << result.files << " arquivo(s))\n";
    if (!result.errors.empty()) {
        std::cout << "Erros de infraestrutura: " << result.errors.size() << '\n';
    }
    return failed == 0 && result.errors.empty() ? 0 : 1;
}
