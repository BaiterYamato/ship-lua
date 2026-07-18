# Handoff — MODSDK-001

## Estado

review

## Resultado

- `CapabilityRegistry` no núcleo: registro consultável de capabilities com
  descritores completos (id, version, provider, provider_version, games,
  stability, permissions, limits.per_mod, description) conforme plan-sdk.md
  §3.5/§15 e RFC 0008.
- Múltiplos providers por capability com seleção determinística: maior versão
  da capability, desempate por maior versão do provider e nome lexicográfico.
- Feature detection: `Has(id, range, game)` usa `VersionRange` (mesma gramática
  do manifesto); filtros de jogo (`oot`/`mm`) aplicados em todas as consultas.
- API Lua: `ship.capabilities.has(id[, range])`, `list([filter])` (filtros
  `game` e `stability`), `info(id)` (descritor ou nil), `providers(id)`.
- Host injeta `std::shared_ptr<CapabilityRegistry>` via
  `LuaApiHostContext::capabilityRegistry`; sem ele, o binding sintetiza
  descritores da lista plana legada via `Generated::kCapabilities`
  (provider `"legacy-host"`, versão = API, estabilidade derivada do status),
  preservando `has`/`list` existentes (retrocompatibilidade verificada por
  testes).
- Estabilidade `internal|experimental|preview|stable|deprecated` (§15.5);
  `deprecated` permanece detectável.
- Correção de robustez: `Fail` (`lua_error`) nunca é chamado dentro de `try`
  nem com locais C++ não triviais vivos — seguia causar segfault em MSVC
  Release; reestruturado para o padrão `EventsOn`.

## Commits

- (ver `git log` da branch `agent/MODSDK-001-capability-registry`)

## Arquivos alterados

- `include/shiplua/capability/CapabilityRegistry.h` (novo)
- `src/capability/CapabilityRegistry.cpp` (novo)
- `include/shiplua/api/LuaApiBinding.h` (registry no contexto + 2 funções Lua)
- `src/api/LuaApiBinding.cpp` (has com range, list com filtro, info, providers,
  síntese legada)
- `tests/unit/CapabilityRegistryTests.cpp` (novo; 8 grupos de teste)
- `tests/unit/LuaApiBindingTests.cpp` (+3 testes de integração Lua)
- `tests/CMakeLists.txt` (alvo `capability_registry_tests`)
- `rfcs/0008-capability-registry.md` (novo)
- `coordination/claims/MODSDK-001.md`, `coordination/handoffs/MODSDK-001.md`,
  `coordination/STATUS.md`

## Validação executada

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  # MinGW g++ 15.2.0
cmake --build build
ctest --test-dir build --output-on-failure
```

## Resultado da validação

30/30 testes passaram em Windows 11 x86_64 (MinGW g++ 15.2.0, Release),
incluindo `capability_registry_tests` e `lua_api_binding_tests`. Gates de
schema/codegen (`api_cpp_codegen_check`, `api_docs_codegen_check`) verdes —
nenhum arquivo gerado alterado.

## Pendências

- MSVC Release (VS 2022, /O2) apresentou segfault intermitente nos caminhos de
  erro Lua durante o desenvolvimento; após a reestruturação dos Fails, MSVC
  Debug passa 100% e MinGW Release passa 100%. Recomenda-se revalidar MSVC
  Release na CI/governança antes do merge.
- Metadados de descritor no catálogo `schema/capabilities.yml` (versão,
  estabilidade, permissões das capabilities oficiais) ficam para follow-up —
  não exigidos por este PR.
- `ship.capability.define/remove/polyfill` (capabilities definidas em Lua,
  §8.19) fora do escopo: MODSDK futuro.
- Registro nativo pela ABI C (`ship_register_capability`, §8.20): MODSDK
  futuro (Native Extension SDK).

## Riscos

- Hosts que não injetarem o registry seguem no caminho legado; descritores
  sintetizados têm versão da API (0.2.0), não versões reais por capability.
- A seleção de provider não considera estabilidade (apenas versões); hosts que
  quiserem ocultar ofertas experimentais devem filtrar por jogo/registro.

## Próxima ação recomendada

MODSDK-002 — handles seguros, ownership e lifecycle (usa o registry para
declarar limites `per_mod` por capability).
