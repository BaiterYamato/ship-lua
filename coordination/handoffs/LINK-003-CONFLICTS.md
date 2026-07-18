# LINK-003-CONFLICTS handoff

- Status: review
- Branch local: `agent/LINK-003-conflict-fix`
- PR head: `agent/LINK-003-consolidate`
- PR: https://github.com/BaiterYamato/link-span/pull/32

## Entregue

- PR #32 reconciliada sobre `main` após as integrações das PRs #39 e #36.
- Launcher dual e fluxo em inglês preservados.
- Seleção automática com um jogo, chooser com dois jogos e bloqueio de mods
  `requires_both_games` preservados.
- `ship.world.travel` integrado à capability registry `0.3.0` e protegido da
  corrupção de pilha causada por `lua_error`/`longjmp` no MSVC.
- Contratos C++, Lua, documentação e mock runtime regenerados da IDL.

## Validação

- CMake/Ninja Release com MSVC 19.44: aprovado.
- CTest: 49/49 aprovados.
- `build_game_script_tests`: aprovado.
- `build_linkspan_packaging_tests`: aprovado.
- Codegen C++, docs e contracts em modo `--check`: aprovado.
- Scanner de arquivos rastreados/protegidos: nenhum ROM, O2R/OTR derivado do
  usuário ou save encontrado.
- Submódulos publicados:
  - OoT `f25e52084a17e2c146e9f90f7e60498cd325a3c9`;
  - MM `50f132f40b925ccc795177fe7fbc8649df0104a2`.

## Gate restante

- Push do merge ao head da PR #32.
- CI remoto verde antes de retirar o draft e integrar.
