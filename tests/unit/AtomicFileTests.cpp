#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "shiplua/storage/AtomicFile.h"

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::string Read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::filesystem::path TestRoot() {
    const auto root = std::filesystem::temp_directory_path() / "shiplua-atomic-file-tests";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root);
    return root;
}

void TestCreatesAndReplaces(const std::filesystem::path& root) {
    const auto path = root / "state.bin";
    Check(ShipLua::AtomicFile::Write(path, "primeiro").isOk(),
          "escrita atômica deve criar arquivo novo");
    Check(Read(path) == "primeiro", "arquivo novo deve conter payload completo");
    Check(ShipLua::AtomicFile::Write(path, "segundo payload").isOk(),
          "escrita atômica deve substituir arquivo existente");
    Check(Read(path) == "segundo payload", "substituição deve publicar somente o novo payload");
}

void TestRejectsInvalidDestinationWithoutResidue(const std::filesystem::path& root) {
    const auto directory = root / "destino-diretorio";
    std::filesystem::create_directory(directory);
    std::ofstream(directory / "sentinela.txt") << "preservado";
    Check(!ShipLua::AtomicFile::Write(directory, "inválido").isOk(),
          "destino diretório deve ser rejeitado antes do commit");
    Check(Read(directory / "sentinela.txt") == "preservado",
          "falha anterior ao commit deve preservar dados existentes");

    const auto missing = root / "ausente" / "state.bin";
    Check(!ShipLua::AtomicFile::Write(missing, "inválido").isOk(),
          "pai inexistente deve ser rejeitado");
    Check(!std::filesystem::exists(missing), "falha não deve criar destino parcial");

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        Check(entry.path().filename().string().find(".shiplua-tmp-") == std::string::npos,
              "falha não deve deixar arquivo temporário");
    }
}

void TestConcurrentWritersPublishWholePayload(const std::filesystem::path& root) {
    const auto path = root / "concorrente.bin";
    const std::string a(128 * 1024, 'A');
    const std::string b(128 * 1024, 'B');
    ShipLua::Result<void> resultA;
    ShipLua::Result<void> resultB;
    std::thread first([&] { resultA = ShipLua::AtomicFile::Write(path, a); });
    std::thread second([&] { resultB = ShipLua::AtomicFile::Write(path, b); });
    first.join();
    second.join();

    Check(resultA.isOk() && resultB.isOk(), "escritores concorrentes devem concluir sem colisão temporária");
    const std::string contents = Read(path);
    Check(contents == a || contents == b,
          "destino concorrente deve conter exatamente um payload completo");
}

} // namespace

int main() {
    const std::filesystem::path root = TestRoot();
    TestCreatesAndReplaces(root);
    TestRejectsInvalidDestinationWithoutResidue(root);
    TestConcurrentWritersPublishWholePayload(root);
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return failures == 0 ? 0 : 1;
}
