#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "shiplua/mock/MockRuntime.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

struct ModTestCaseResult {
    std::string file;     // caminho do arquivo de teste (relativo ao mod)
    std::string test;     // nome completo: describe / it
    bool ok = false;
    std::string message;  // mensagem de erro quando ok == false
};

struct ModTestReport {
    std::string modId;
    std::size_t files = 0;                     // arquivos de teste executados
    std::vector<ModTestCaseResult> tests;      // resultado por it, na ordem de execução
    std::vector<std::string> errors;           // erros de infraestrutura (mod não carrega etc.)
    bool noTestsFound = false;

    std::size_t PassedCount() const;
    std::size_t FailedCount() const;
};

// Mod test runner (plan-sdk.md §16 'Mock runtime' / `shipmod test`): carrega um
// mod no MockRuntime e executa os testes do diretório <mod>/tests/*.lua.
//
// Convenção:
// - cada arquivo *.lua de <mod>/tests/ roda isolado: MockRuntime + instância do
//   mod novos por arquivo, em ordem lexicográfica de nome;
// - dentro de um arquivo, os blocos it executam em sequência no mesmo estado
//   (describe apenas agrupa nomes); mock.reset() limpa logs e storage;
// - após carregar o mod o runner emite game.ready; ao final, game.shutdown;
// - os testes usam a DSL global describe/it/assert e o módulo global mock.
class ModTestRunner {
  public:
    explicit ModTestRunner(MockHostOptions options = {});

    Result<ModTestReport> Run(const std::filesystem::path& modDir);

    // Lista <modDir>/tests/*.lua (arquivos regulares, sem symlinks), ordenados
    // por nome. Retorna vazio quando o diretório de testes não existe.
    static std::vector<std::filesystem::path> DiscoverTestFiles(
        const std::filesystem::path& modDir);

  private:
    void RunFile(const std::filesystem::path& modDir, const std::filesystem::path& testFile,
                 ModTestReport& report);

    MockHostOptions mOptions;
};

} // namespace ShipLua
