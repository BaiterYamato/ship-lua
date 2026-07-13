# Handoff — WORLD-001

## Estado

review

## Resultado

- Uma `WorldSession` registra adaptadores OoT e MM simultaneamente.
- O estado canônico transporta vida, rupees, itens, equipamento e assets lógicos sem structs de host.
- A viagem usa prepare/validate/commit e mantém a fonte ativa quando o destino falha.
- Itens diferidos permanecem no estado canônico e reaparecem em um mundo compatível.
- Equipamento aceito exige que o destino resolva o asset do mundo de origem.
- O teste sintético percorre OoT → Clock Town → Kakariko preservando hookshot, máscara e sword equipada.

## Commits

- `162316970` — Sessão cross-world transacional, RFC, plano e testes.

## Arquivos alterados

- `PLAN.md`
- `rfcs/0004-cross-world-session.md`
- `include/shiplua/world/WorldSession.h`
- `src/world/WorldSession.cpp`
- `tests/unit/WorldSessionTests.cpp`
- `tests/CMakeLists.txt`

## Validação executada

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

## Resultado da validação

- Windows 11, MinGW/Ninja: 25/25 testes passaram.
- Windows 11, Visual Studio 2022/v143, Release: 25/25 testes passaram.
- O primeiro build MSVC encontrou dois `.obj` temporariamente bloqueados; não havia processo dono persistente e a repetição determinística passou.

## Pendências

- `WORLD-002`: envelope entre processos.
- `WORLD-003`: catálogo canônico e tradutores de IDs.
- `WORLD-004`: catálogo/montagem validada de assets OoT/MM.
- Adaptadores reais e smoke com ativos legítimos.

## Riscos

- `CommitImport()` depende de atomicidade implementada pelo adaptador; testes dos hosts devem provar rollback.
- Resolver um arquivo no archive não prova compatibilidade de factory/runtime; WORLD-004 deve validar ambos.

## Próxima ação recomendada

Implementar WORLD-003 e então os adaptadores de snapshot OoT/MM sobre tabelas de tradução explícitas.
