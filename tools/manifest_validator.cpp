#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "shiplua/manifest/ManifestParser.h"
#include "shiplua/manifest/PackageExtractor.h"

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

std::string MensagemEmPortugues(ShipLua::ErrorCode code, const std::string& detalhe) {
    const auto missing = detalhe.find("missing required field");
    if (missing != std::string::npos) {
        const auto fields = detalhe.find('\'', missing);
        return "campo obrigatório ausente" +
               (fields == std::string::npos ? std::string{} : ": " + detalhe.substr(fields));
    }
    const auto invalidField = detalhe.find("invalid field '");
    if (invalidField != std::string::npos) {
        const auto begin = invalidField + 15;
        const auto end = detalhe.find('\'', begin);
        return "campo inválido '" + detalhe.substr(begin, end - begin) + "'";
    }
    const auto dependency = detalhe.find("invalid dependency range for '");
    if (dependency != std::string::npos) {
        const auto begin = dependency + 30;
        const auto end = detalhe.find('\'', begin);
        return "range inválido para a dependência '" + detalhe.substr(begin, end - begin) + "'";
    }
    if (detalhe.find("TOML parse error") != std::string::npos) {
        const auto line = detalhe.find("line ");
        const auto column = detalhe.find("column ");
        if (line != std::string::npos && column != std::string::npos) {
            const auto lineEnd = detalhe.find(',', line);
            const auto columnEnd = detalhe.find(':', column);
            return "erro de sintaxe TOML na linha " +
                   detalhe.substr(line + 5, lineEnd - line - 5) + ", coluna " +
                   detalhe.substr(column + 7, columnEnd - column - 7);
        }
        return "erro de sintaxe TOML";
    }
    switch (code) {
        case ShipLua::ErrorCode::PermissionDenied:
            return "o pacote contém um caminho inseguro ou não permitido";
        case ShipLua::ErrorCode::ResourceLimit:
            return "o pacote excede os limites de segurança";
        case ShipLua::ErrorCode::HostFailure:
            return "falha do sistema ao acessar ou preparar o pacote";
        default:
            return "estrutura ou conteúdo inválido";
    }
}

ShipLua::Result<ShipLua::Manifest> ValidarEntrada(const std::filesystem::path& entrada) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(entrada, error);
    if (error || std::filesystem::is_symlink(status)) {
        return ShipLua::Result<ShipLua::Manifest>::err(
            ShipLua::ErrorCode::PermissionDenied, "entrada ausente, inacessível ou simbólica");
    }
    if (std::filesystem::is_directory(status)) {
        return ShipLua::ParseManifestFile((entrada / "manifest.toml").string());
    }
    if (!std::filesystem::is_regular_file(status)) {
        return ShipLua::Result<ShipLua::Manifest>::err(
            ShipLua::ErrorCode::InvalidArgument, "entrada não é arquivo nem diretório regular");
    }
    if (entrada.extension() != ".shipmod") {
        return ShipLua::ParseManifestFile(entrada.string());
    }

    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    TempDirectory temp(std::filesystem::temp_directory_path() /
                       ("shiplua-validator-" + std::to_string(stamp)));
    const auto extracted = temp.Path() / "conteudo";
    auto extraction = ShipLua::ExtractShipmod(entrada, extracted);
    if (!extraction.isOk()) {
        return ShipLua::Result<ShipLua::Manifest>::err(extraction.code, extraction.message);
    }
    return ShipLua::ParseManifestFile((extracted / "manifest.toml").string());
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: shiplua_manifest_validator <manifesto|diretório|pacote.shipmod> [...]\n";
        return 2;
    }

    bool todosValidos = true;
    for (int i = 1; i < argc; ++i) {
        const std::filesystem::path entrada(argv[i]);
        auto resultado = ValidarEntrada(entrada);
        if (resultado.isOk()) {
            std::cout << "Válido: " << entrada.string() << " — id=" << resultado.value->id
                      << ", versão=" << resultado.value->version << '\n';
        } else {
            todosValidos = false;
            std::cerr << "Inválido: " << entrada.string() << " — "
                      << MensagemEmPortugues(resultado.code, resultado.message) << '\n';
        }
    }
    return todosValidos ? 0 : 1;
}
