#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shiplua/manifest/SemVersion.h"
#include "shiplua/runtime/Result.h"

namespace ShipLua {

// Maturidade declarada de uma capability (plan-sdk.md §15.5). Um release
// estável do runtime pode conter capabilities experimentais desde que isso
// esteja declarado no registry.
enum class CapabilityStability {
    Internal,
    Experimental,
    Preview,
    Stable,
    Deprecated,
};

Result<CapabilityStability> ParseCapabilityStability(const std::string& text);
std::string_view CapabilityStabilityName(CapabilityStability stability);

struct CapabilityLimits {
    // Máximo de instâncias que um único mod pode criar (ex.: 16 spawns).
    std::optional<std::uint64_t> perMod;
};

// Oferta de um provider para uma capability. Várias ofertas podem existir para
// o mesmo id; o host seleciona a provider compatível (plan-sdk.md §3.5).
struct CapabilityProvider {
    std::string name;                  // "shipwright-native", "2ship-native", "mock-runtime"
    SemVersion providerVersion;        // versão do próprio provider
    SemVersion capabilityVersion;      // versão da capability implementada
    std::vector<std::string> games;    // subconjunto não vazio de {"oot", "mm"}
    CapabilityStability stability = CapabilityStability::Preview;
    std::vector<std::string> permissions;
    CapabilityLimits limits;
    std::string description;
};

// Descritor público de uma capability: a oferta selecionada pelo registry para
// um jogo, pronta para inspeção por mods.
struct CapabilityDescriptor {
    std::string id;
    SemVersion version;
    std::string provider;
    SemVersion providerVersion;
    std::vector<std::string> games;
    CapabilityStability stability = CapabilityStability::Preview;
    std::vector<std::string> permissions;
    CapabilityLimits limits;
    std::string description;
};

// Registro consultável de capabilities. Pertence ao host e é compartilhado
// (somente leitura) com os bindings Lua dos mods. Uso single-threaded na
// thread do host, como os demais serviços do núcleo.
class CapabilityRegistry {
  public:
    // Registra a oferta de um provider. Erros:
    // - InvalidArgument: id/provider/jogos/permissões inválidos;
    // - InvalidState: oferta duplicada para o mesmo (id, provider).
    Result<void> Register(const std::string& capabilityId, CapabilityProvider provider);

    // Remove a oferta de um provider. InvalidHandle se inexistente.
    Result<void> Unregister(const std::string& capabilityId, const std::string& providerName);

    // Feature detection. `game` vazio desliga o filtro de jogo.
    bool Has(const std::string& id, const std::string& game = "") const;
    bool Has(const std::string& id, const VersionRange& range, const std::string& game = "") const;

    // Descritor da oferta selecionada para `game` (maior versão da capability,
    // desempate por maior versão do provider e nome lexicográfico).
    std::optional<CapabilityDescriptor> Info(const std::string& id, const std::string& game = "") const;

    // Nomes dos providers com oferta compatível com `game`, em ordem alfabética.
    std::vector<std::string> Providers(const std::string& id, const std::string& game = "") const;

    // Ids com ao menos uma oferta compatível com `game` (e com a estabilidade
    // pedida, quando informada), em ordem alfabética.
    std::vector<std::string> List(const std::string& game = "",
                                  std::optional<CapabilityStability> stability = std::nullopt) const;

    std::size_t Count() const;

  private:
    const CapabilityProvider* Select(const std::string& id, const std::string& game) const;

    // id -> (provider name -> oferta)
    std::map<std::string, std::map<std::string, CapabilityProvider>> mEntries;
};

} // namespace ShipLua
