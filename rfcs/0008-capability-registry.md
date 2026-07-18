# RFC 0008 — Capability registry, descritores e feature detection

- Status: proposta implementável
- API alvo: 0.2.0 (sem elevação; extensão retrocompatível do módulo `ship.capabilities`)
- Tarefa: MODSDK-001
- Referências: plan-sdk.md §3.5, §7, §8.19, §15

## Motivação

O módulo `ship.capabilities` atual expõe apenas `has(name)` e `list()` sobre uma
lista plana de nomes. O plano SDK exige um registro consultável em que a
**presença da capability é a fonte de verdade** (e não a versão geral do host),
com descritores completos, múltiplos providers por capability e feature
detection por intervalo SemVer:

```lua
if ship.capabilities.has("actor.visual", ">=1.1") then ... end
```

Um host moderno (ou o mock runtime de MODSDK-004) deve registrar descritores
reais — id, versão, provider, jogos, estabilidade, permissões e limites — e
mods precisam inspecioná-los antes de compor comportamentos.

## Contrato Lua

```lua
local ship = require("ship")

ship.capabilities.has("actor.spawn")               --> boolean (presença)
ship.capabilities.has("actor.spawn", ">=1.0 <2.0") --> boolean (presença + range)
ship.capabilities.list()                           --> array de ids (ordenado)
ship.capabilities.list({ stability = "preview" })  --> array filtrado
ship.capabilities.list({ game = "mm" })            --> array filtrado por jogo
ship.capabilities.info("actor.spawn")              --> tabela descritora ou nil
ship.capabilities.providers("actor.spawn")         --> array de nomes de provider
```

Descritor retornado por `info`:

```lua
{
    id = "actor.spawn",
    version = "1.1.0",
    provider = "shipwright-native",
    provider_version = "0.4.0",
    games = { "oot" },
    stability = "preview",          -- internal|experimental|preview|stable|deprecated
    permissions = { "world.entities.create" },
    limits = { per_mod = 16 },      -- chaves ausentes quando sem limite
    description = "...",
}
```

Regras:

- `has`, `list` (sem filtro explícito de jogo), `info` e `providers` consideram
  apenas ofertas compatíveis com o jogo do host (`game.id()`); um host sem jogo
  configurado não aplica filtro.
- `has(id, range)` analisa o range com a gramática de `VersionRange` do
  manifesto (`>=1.0`, `<2.0`, `=1.1.0`, combináveis por espaço); range inválido
  é erro de script (`invalid_argument`), não `false`.
- `info(id)` resolve o provider selecionado pelo host: entre as ofertas
  compatíveis com o jogo, vence a maior `version` da capability; empates são
  decididos pela maior `provider_version` e, por fim, pelo nome do provider em
  ordem lexicográfica (determinístico).
- `providers(id)` retorna os nomes dos providers compatíveis com o jogo, em
  ordem alfabética; capability desconhecida retorna array vazio (não `nil`).
- Capabilities `deprecated` continuam presentes e detectáveis; a estabilidade é
  visível em `info` para o mod decidir.
- IDs de capability seguem `[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+`. Capabilities
  oficiais usam namespaces reservados (`core.*`, `actor.*`, `mm.*`, `oot.*`,
  ...); capabilities de terceiros usam o namespace do autor ou pacote
  (§8.19 do plano). O registro de capabilities definidas em Lua
  (`ship.capability.define`) fica para tarefa futura.

## API nativa (núcleo)

```cpp
struct CapabilityProvider {
    std::string name;                 // "shipwright-native"
    SemVersion providerVersion;       // versão do provider
    SemVersion capabilityVersion;     // versão da capability implementada
    std::vector<std::string> games;   // subconjunto não vazio de {"oot","mm"}
    CapabilityStability stability;    // internal..deprecated
    std::vector<std::string> permissions;
    CapabilityLimits limits;          // per_mod opcional
    std::string description;
};

class CapabilityRegistry {
    Result<void> Register(const std::string& capabilityId, CapabilityProvider provider);
    Result<void> Unregister(const std::string& capabilityId, const std::string& providerName);
    bool Has(...);                                        // com range/jogo opcionais
    std::optional<CapabilityDescriptor> Info(...);        // provider selecionado
    std::vector<std::string> Providers(...);
    std::vector<std::string> List(...);
};
```

O host injeta `std::shared_ptr<CapabilityRegistry>` em
`LuaApiHostContext::capabilityRegistry`. Quando ausente, o binding sintetiza um
registry a partir da lista plana legada `capabilities` usando
`Generated::kCapabilities` (provider `"legacy-host"`, versão da capability =
versão da API, estabilidade derivada do status do catálogo), preservando o
comportamento de `has`/`list` existentes.

## Capabilities

Esta RFC é a infraestrutura de registro; ela não introduz capabilities novas de
jogo. O catálogo oficial permanece em `schema/capabilities.yml` (API-003),
reutilizado pelo caminho legado. Metadados de descritor no catálogo (versão,
estabilidade) são um follow-up possível, não exigido aqui.

## Comportamento em OoT

O adaptador Shipwright registra ofertas com `games = {"oot"}` (ou ambos) no
bootstrap, antes de carregar mods. Mods OoT observam somente ofertas
compatíveis com `oot`.

## Comportamento em MM

Idêntico, com `games = {"mm"}`. Capabilities exclusivas (`mm.cycle`, ...)
registradas pelo host MM não aparecem para mods em host OoT, e vice-versa.

## Erros

- `invalid_argument`: id, provider, jogos ou permissões inválidos no registro;
  range SemVer inválido em `has`; filtro com tipos errados em `list`.
- `invalid_state`: registro duplicado do mesmo `(capability, provider)`.
- `invalid_handle`: `Unregister` de oferta inexistente.
- Consultas (`has`/`list`/`info`/`providers`) nunca lançam por capability
  ausente: retornam `false`, array vazio ou `nil`.

## Compatibilidade

- `has(name)` e `list()` sem argumentos preservam o comportamento atual sobre o
  contexto legado (mesmos resultados, mesma ordenação).
- A validação do contexto legado contra `Generated::kCapabilities` permanece.
- Nenhum contrato existente é removido ou alterado; a API segue `0.2.0`.

## Segurança

- O registry é somente leitura para mods: a superfície Lua não registra nem
  remove ofertas (registro nativo ocorre no bootstrap do host; providers
  externos virão pela ABI versionada de MODSDK futuro).
- Toda string vinda de Lua é validada antes de tocar o registry.
- Nenhuma exceção C++ atravessa a fronteira C/Lua (mesmo padrão dos demais
  bindings: funções `noexcept` com `try/catch` total).
- Sem ponteiros, endereços ou handles crus expostos a Lua.

## Plano de testes

- `CapabilityRegistryTests` (unitário puro): registro, validações, duplicatas,
  seleção multi-provider determinística, filtros de jogo/estabilidade, ranges
  SemVer, `deprecated` presente, unregister.
- `LuaApiBindingTests` (integração): `has` com/sem range, `info` completo
  (campos, limites, permissões), `providers` multi-provider, `list` com filtros
  `game`/`stability`, range inválido como erro protegido, capability ausente,
  e o caminho legado sintetizando descritores equivalentes.

## Depreciação

Nenhuma API é removida. A lista plana `LuaApiHostContext::capabilities` passa a
ser o caminho legado; sua remoção, se ocorrer, exigirá marcação deprecated por
pelo menos um minor e só poderá acontecer em versão major.
