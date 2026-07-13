# Handoff — WORLD-003

## Estado

review

## Resultado

- O núcleo ganhou um catálogo de itens sem dependência de enums ou layouts dos hosts.
- Itens comuns resolvem com o mesmo ID nos dois jogos.
- Espadas do recorte inicial usam traduções explícitas e bidirecionais.
- Itens exclusivos sem tradução, como a máscara Deku, permanecem adiados.
- Mods podem registrar IDs namespaced sem ampliar o catálogo embutido.
- Quantidades, equipamento e referências lógicas de assets são validados antes do adaptador.
- O preview distingue equipamento traduzido para equivalente nativo de equipamento que mantém asset estrangeiro.

## Commits

- `4a7c389` — Add portable item catalog.
- `f02f393` — Distinguish translated equipment from foreign-asset equipment.

## Arquivos alterados

- `include/shiplua/world/PortableItemCatalog.h`
- `src/world/PortableItemCatalog.cpp`
- `tests/unit/PortableItemCatalogTests.cpp`
- `tests/CMakeLists.txt`
- `rfcs/0005-portable-item-catalog.md`
- `coordination/claims/WORLD-003.md`
- `coordination/STATUS.md`
- `coordination/handoffs/WORLD-003.md`

## Validação executada

```powershell
cmake -S . -B build-world003 -G Ninja
cmake --build build-world003
ctest --test-dir build-world003 --output-on-failure

cmake -S . -B build-world003-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-world003-msvc --config Release
ctest --test-dir build-world003-msvc -C Release --output-on-failure
```

## Resultado da validação

- Windows 11, MinGW GCC 15.2.0, Ninja: 26/26 testes passaram.
- Windows 11, MSVC 19.44.35213, Release x64: 26/26 testes passaram.

## Pendências

- Implementar tabelas de `ItemId`, slots e valores de equipamento dentro de cada host.
- Ligar o catálogo aos adaptadores transacionais de snapshot e restauração.
- Validar os assets reais antes de aceitar equipamento estrangeiro no destino.
- Executar Linux, macOS e smoke com ativos legítimos fora da CI pública.

## Riscos

- As equivalências de tiers de espada são uma política inicial e precisam de revisão de gameplay.
- O catálogo não torna os recursos `.o2r` interoperáveis; WORLD-004 continua obrigatório.
- A viagem real entre executáveis ainda depende de WORLD-002 ou de um supervisor combinado.

## Próxima ação recomendada

Implementar OOT-WORLD-001 e MM-WORLD-001 em branches separadas, usando este catálogo
e mantendo todos os enums brutos nos respectivos adaptadores.
