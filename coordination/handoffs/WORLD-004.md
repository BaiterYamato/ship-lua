# Handoff — WORLD-004

## Estado

review

## Resultado

- Catálogo central descreve assets por mundo proprietário, ID lógico, tipo e contrato de factory.
- Resolução exige compatibilidade de host, versão exata do contrato, archive validado, namespace isolado e bundle carregado.
- Uma nova sonda inválida remove uma resolução antiga do mesmo host.
- A sessão rejeita referências cujo namespace não pertence ao mundo declarado.
- RFC 0007 proíbe montar archives completos de outro jogo e define a evolução por bundles allowlisted.

## Commits

- `a22d89c` — feat(world): validate cross-world asset bundles
- `ff2a996` — docs(world): claim host asset probes

## Arquivos alterados

- `rfcs/0007-cross-world-assets.md`
- `include/shiplua/world/WorldAssetCatalog.h`
- `include/shiplua/world/WorldSession.h`
- `src/world/WorldAssetCatalog.cpp`
- `src/world/WorldSession.cpp`
- `tests/unit/WorldAssetCatalogTests.cpp`
- `tests/CMakeLists.txt`

## Validação executada

```powershell
cmake --build build-world004-msvc --config Release
ctest --test-dir build-world004-msvc -C Release --output-on-failure
```

## Resultado da validação

- Windows 11, Visual Studio 2022 v143, Release: 29/29 testes passaram.
- O catálogo também foi validado anteriormente no MinGW/Ninja: 29/29 testes passaram.

## Pendências

- A execução com archives legítimos deve confirmar os paths lógicos sondados pelos hosts.
- Bundles estrangeiros ainda não são montados; isso requer pacote isolado, allowlist e prova do contrato de factory.

## Riscos

- Mudanças de nomes internos nos archives do host tornam o asset indisponível de forma segura, com warning, em vez de aceitar um path incompatível.

## Próxima ação recomendada

Implementar o supervisor que consome o handoff autenticado, ativa o host de destino e executa o round-trip OoT/MM.
