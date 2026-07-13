# INTEGRATION.md — Log de integração (governança)

> Mantido apenas pelo agente de governança. Registra o que foi integrado em `main`,
> com validação de build/teste. Ver `AGENTS.md` §17 para o modelo operacional.

## Estado de `main`

- Tip integrado: `c92d0e7` (merge de HOST-001 — coordenação OOT-001/MM-001).
- Build/teste: idêntico a `64a76fa` (HOST-001 não alterou código): 20/20 verdes.
- Lua v5.4.7 (FetchContent) + toml++ v3.4.0 (FetchContent).

## PRs abertos nos forks (Fase 4) — pendentes de decisão do owner

| PR | Repo | O que faz | Estado |
|---|---|---|---|
| #5 | BaiterYamato/Shipwright-HyliaFoundry | `build(oot)`: submódulo `extern/ship-lua` + CMake, base `lua/main` | validado pelo builder (MSVC Release x64); **governança NÃO mergeou** — não é verificável neste ambiente (build completo do jogo) |
| #1 | BaiterYamato/2ship2harkinian | `build(mm)`: submódulo `extern/ship-lua` + CMake, base `lua/main` | idem |

- Pin do submódulo nos forks: `ad5aad6` (código do BIND-001) — já contém todo o
  núcleo (runtime, manifesto, eventos, timers, codegen, binding). Sugestão: antes
  do merge nos forks, bump do pin para o tip de `main` para incluir os docs de
  governança (opcional — não afeta o build).

## Histórico

### 2026-07-13 (3) — Coordenação HOST-001 (bootstrap dos forks)

- Merge `--no-ff` de `agent/HOST-001-submodules` em `main` (`8a702c1` → `c92d0e7`).
  Conteúdo real: só coordenação (STATUS + claims/handoffs de OOT-001 e MM-001).
  Sem código ship-lua → build inalterado. Merge limpo.
- PR ship-lua#14 (branch HOST-001) fechado como merged pelo push.
- Fork PRs #5 (Shipwright) e #1 (2ship) abertos pelo builder — ver tabela acima.
  Não integrados pela governança (não verificáveis aqui).

### 2026-07-13 (2) — Codegen + binding Lua

- Merge `--no-ff` de `agent/BIND-001-lua-api` em `main` (`a62a0de` → `64a76fa`),
  trazendo a cadeia CODEGEN-001 (bindings C++), CODEGEN-002/003 (LuaDoc + Markdown)
  e BIND-001 (binding do runtime Lua aos serviços `ship.*`).
- Merge limpo — nenhuma sobreposição de arquivos com os docs de governança
  (a exclusividade de §17 funcionou).
- Núcleo verificado: nenhum header de jogo em `src/**` ou `include/**`.
- Build/teste isolados: 20/20 verdes.
- Marcados no PLAN §13: CODEGEN-001/002/003.

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
