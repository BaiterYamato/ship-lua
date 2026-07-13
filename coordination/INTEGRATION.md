# INTEGRATION.md — Log de integração (governança)

> Mantido apenas pelo agente de governança. Registra o que foi integrado em `main`,
> com validação de build/teste. Ver `AGENTS.md` §17 para o modelo operacional.

## Estado de `main`

- Tip integrado: `a16df3b` (EVENT-001 + TIMER-001).
- Validação: Windows 11 / MinGW g++ 15.2 / Ninja / C++20.
  `100% tests passed, 0 tests failed out of 14` (ctest, timeout 60s).
- Lua v5.4.7 (FetchContent) + toml++ v3.4.0 (FetchContent).

## Histórico

### 2026-07-13 — Integração da Fase 2/3 + timers

- Fast-forward de `main` `abbd687` → `82a6d55`: integrada a pilha linear de PRs #1–#7
  (MOD-001/CORE-002, MOD-003 discovery+extração, MOD-004 SemVer, MOD-005 grafo de
  dependências, TOOL-001 validador, ARCH-001 inventário de hooks, ARCH-002 RFC 0001,
  API-001/002/003 schemas).
  - PR#1 marcado MERGED automaticamente; PR#2–#7 fechados com nota (diff vazio contra
    `main` após o fast-forward — commits já presentes). Branches integradas removidas.
- Fast-forward de `main` `82a6d55` → `a16df3b`: EVENT-001 (dispatcher determinístico)
  e TIMER-001 (frame scheduler). Sem PR associado; integrados direto após validação.
- Build/teste isolados: 14/14 verdes.

## Notas de robustez (backlog de CI)

- Os executáveis de teste MinGW dependem de `libstdc++-6.dll`/`libgcc_s_seh-1.dll`/
  `libwinpthread-1.dll`. Sem elas no `PATH`, o teste **trava num diálogo do Windows**
  em vez de falhar. Recomendação para CI/GOV: linkar estático
  (`-static -static-libgcc -static-libstdc++`) nos alvos de teste, ou garantir o
  runtime no `PATH`/`--timeout` no ctest. Aplicado `--timeout 60` como salvaguarda.

## Pendências de integração

- CODEGEN-001 (bindings C++) em andamento pelo builder — integrar quando concluído.
- CI-001..004 (builds dos jogos) e Fase 4 (submódulo `extern/ship-lua` nos forks)
  dependem dos forks e ficam para depois.
