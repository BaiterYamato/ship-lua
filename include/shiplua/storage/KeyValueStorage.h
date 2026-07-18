#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "shiplua/runtime/Result.h"

namespace ShipLua {

// Armazenamento chave-valor por mod, em memória, com namespace automático por
// mod e limites de segurança (plan-sdk.md §8.18: namespace por mod, limite de
// tamanho, tipos serializáveis restritos). Valores aceitos são os tipos
// serializáveis do contrato: booleano, inteiro, número e texto.
//
// Determinístico (std::map) e confinado à thread dona, seguindo o padrão de
// EventDispatcher e FrameTimerScheduler. O mock runtime usa esta implementação
// diretamente; hosts reais podem persistir o mesmo conteúdo via AtomicFile.
class KeyValueStorage {
  public:
    using Value = std::variant<bool, std::int64_t, double, std::string>;

    struct Limits {
        std::size_t maxKeysPerMod = 1024;
        std::size_t maxKeyBytes = 128;
        std::size_t maxValueBytes = 64 * 1024;
        std::size_t maxTotalBytesPerMod = 256 * 1024; // plan-sdk.md: storage_kb = 256
    };

    KeyValueStorage() = default;
    explicit KeyValueStorage(Limits limits);

    KeyValueStorage(const KeyValueStorage&) = delete;
    KeyValueStorage& operator=(const KeyValueStorage&) = delete;

    const Limits& GetLimits() const;

    // Retorna o valor da chave ou std::nullopt quando ausente.
    Result<std::optional<Value>> Get(const std::string& modId, const std::string& key) const;

    // Grava o valor. ResourceLimit quando os limites do mod são excedidos.
    Result<void> Set(const std::string& modId, const std::string& key, Value value);

    // Remove a chave; retorna true quando ela existia.
    Result<bool> Delete(const std::string& modId, const std::string& key);

    // Remove todas as chaves do mod; retorna quantas foram removidas.
    Result<std::size_t> Clear(const std::string& modId);

    // Lista as chaves do mod em ordem lexicográfica determinística.
    Result<std::vector<std::string>> Keys(const std::string& modId) const;

    std::size_t KeyCount(const std::string& modId) const;
    std::size_t TotalBytes(const std::string& modId) const;

  private:
    Result<void> ValidateKey(const std::string& modId, const std::string& key) const;
    static std::size_t ValueBytes(const Value& value);

    Limits mLimits;
    std::map<std::string, std::map<std::string, Value>> mMods;
    std::map<std::string, std::size_t> mBytesPerMod;
};

} // namespace ShipLua
