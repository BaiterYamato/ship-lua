# INTEGRATION.md — Log de integração (governança)

> Mantido apenas pelo agente de governança. Registra o que foi integrado em `main`,
> com validação de build/teste. Ver `AGENTS.md` §17 para o modelo operacional.

## Estado de `main`

- Tip integrado: `af85476` (núcleo + EXAMPLE-001; CI Linux+Windows verdes).
- Build/teste: **24/24 verdes** (rebuild limpo; inclui conformidade hello-world).
- **CI Linux + Windows + package-examples: verdes** (critério v0.1.0 §16 fechado).
- Lua v5.4.7 + toml++ v3.4.0 + miniz 3.1.2 (FetchContent).

### Fase 4 (bootstrap dos hosts nos forks) — coordenação integrada em `main`

| Task | Estado | Onde está o código |
|---|---|---|
| OOT-001 / MM-001 (submódulo + CMake) | review | forks (PRs #5 / #1) |
| OOT-002 / MM-002 (bootstrap) | review | forks |
| OOT-003 / MM-003 (shutdown) | review | forks |
| OOT-004 / MM-004 (identity) | review | forks |
| OOT-005 / MM-005 (localizar dir. de mods) | review | forks |

## PRs abertos nos forks (Fase 4) — pendentes de decisão do owner

| PR | Repo | O que faz | Estado |
|---|---|---|---|
| #5 | BaiterYamato/Shipwright-HyliaFoundry | `build(oot)`: submódulo `extern/ship-lua` + CMake, base `lua/main` | validado pelo builder (MSVC Release x64); **governança NÃO mergeou** — não é verificável neste ambiente (build completo do jogo) |
| #1 | BaiterYamato/2ship2harkinian | `build(mm)`: submódulo `extern/ship-lua` + CMake, base `lua/main` | idem |

- Pin do submódulo nos forks: `ad5aad6` (código do BIND-001) — já contém todo o
  núcleo (runtime, manifesto, eventos, timers, codegen, binding). Sugestão: antes
  do merge nos forks, bump do pin para o tip de `main` para incluir os docs de
  governança (opcional — não afeta o build).

## Critérios v0.1.0 (§16) — panorama de governança

| Critério | Estado |
|---|---|
| Shipwright e 2Ship compilam com ShipLua | parcial — submódulo nos forks validado em MSVC (PRs #5/#1), aguardando merge do owner |
| hello-world / scene-logger / player rodam nos dois | parcial — hello-world provado por conformidade no núcleo; scene-logger/player pendentes |
| manifesto e dependências funcionam | ✅ |
| runtime isola mods | ✅ |
| storage bloqueia traversal | parcial — root loader valida entrypoint; VFS/quotas (STORE-*) pendentes |
| hot reload não duplica hooks | pendente (RELOAD-*) |
| documentação é gerada (codegen) | ✅ |
| API com SemVer | ✅ |
| CI Linux e Windows verde | ✅ (core-linux + core-windows + package-examples) |
| nenhuma ROM/asset protegido | ✅ (.gitignore + varredura; núcleo sem assets) |
| release com checksums | pendente (sem release ainda) |
| limitações conhecidas documentadas | parcial (INTEGRATION.md + handoffs) |

**Resumo:** o núcleo da API do modloader Lua está concluído e comprovado
(vertical slice §14, 24 testes, CI Linux+Windows verdes). O restante do v0.1.0 é
in-game (adaptadores nos forks) e fases posteriores (STORE/RELOAD/mais exemplos/
release), dependendo do builder e da decisão do owner sobre os PRs #5/#1.

## Histórico

### 2026-07-13 (9) — CI Windows verde (§16 fechado)

- `core-windows` corrigido (VS generator não achava o VS → `ilammy/msvc-dev-cmd`
  + Ninja): run **success**. Linux + Windows agora verdes.

### 2026-07-13 (8) — CI Windows/MSVC (builder ocioso)

- Adicionado `.github/workflows/core-windows.yml` (governança): windows-latest,
  gerador VS 2022 x64, `--config Release`, `ctest --timeout 120`. Sem actions de
  terceiros. De-risco: o CMake não usa flags GCC-específicas e o núcleo já compilou
  sob MSVC 19.44 na integração dos forks (handoff OOT-001).
- Objetivo: fechar o critério §16 "CI Windows verde". Resultado da 1ª run é
  verificado no próximo ciclo.
- Nada do builder para integrar neste ciclo (ocioso; `main` já em dia).

### 2026-07-13 (7) — EXAMPLE-001 hello-world + vertical slice (§14) provado no núcleo

- Fast-forward de `main` (`1adebb2` → `e6dcf6c`): EXAMPLE-001.
  - `examples/hello-world/` (manifest.toml + main.lua idênticos ao §14 do PLAN).
  - `tests/conformance/HelloWorldConformanceTests.cpp` + packaging Python:
    empacota `hello-world.shipmod`, carrega via `LoadModsFromRoot`, dispara
    `game.ready`, confere log "Hello from <jogo> host=<versão>" e unload sem
    callback vazado — **para oot e mm**.
  - `.github/workflows/package-examples.yml` — workflow de empacotamento de
    exemplos adicionado pelo builder (arquivo novo, não colide com `core-linux.yml`;
    CI de exemplos passa a ser co-mantido — ver nota abaixo).
- Validação: **rebuild limpo, 24/24 verdes**. Marco: o contrato da API do vertical
  slice (§14) está provado ponta-a-ponta no núcleo (parte in-game real depende
  dos adaptadores dos hosts nos forks).
- Nota de propriedade: `.github/**` era território de governança (§17); o builder
  contribuiu `package-examples.yml`. Aceito (sem colisão de arquivos); CI de
  núcleo (`core-linux.yml`) permanece com a governança.

### 2026-07-13 (6) — HOST-004 + MOD-008 root loader (código)

- Merge `--no-ff` de `agent/MOD-008-root-loader` (`ee0004f` → `265a200`), trazendo:
  - HOST-004 (OOT-004/MM-004 identity, coordenação fork-side);
  - **MOD-008** (código): `ModHost::LoadModsFromRoot` — descobre mods de um
    diretório raiz, valida compatibilidade host/API/jogo, resolve ordem de
    dependências e isola falhas por mod (retorna `ModLoadReport` com
    `loadedIds`/`rejected`); `LoadModFromPackage` (.shipmod); `DispatchEvent`
    conectando o EventDispatcher ao host.
- Validação: **rebuild limpo**, 21/21 verdes (novo `mod_root_loader_tests`:
  ordem determinística, rejeição por incompatibilidade, isolamento de falhas,
  ownership/cache de pacote). Sem headers de jogo.
- Nota operacional: um `taskkill cmake.exe` global (para destravar reconfigure)
  pode ter atingido um cmake do builder — evitar kill por nome; usar PID.

### 2026-07-13 (5) — CI Linux verde + HOST-002

- CI `core-linux` (run 29228265961): **success**. `main` verificado em Ubuntu.
- Merge `--no-ff` de `agent/HOST-002-bootstrap` (`1a1856a` → `18c9397`):
  coordenação OOT-002/MM-002 (bootstrap dos hosts, em review nos forks). Sem código.
- PR ship-lua#15 fechado (merged) pelo push.
- HOST-003 (shutdown OOT-003/MM-003) ainda em claim — não integrado.

### 2026-07-13 (4) — CI Linux do núcleo

- Adicionado `.github/workflows/core-linux.yml` (governança / Build-CI).
  Espelha o build/teste verificado: Ubuntu + CMake/Ninja + FetchContent
  (Lua 5.4.7, toml++ 3.4.0, miniz 3.1.2) + ctest, `--timeout 120`.
  Dispara em push/PR para `main` e por `workflow_dispatch`.
- Testes Python: só stdlib; CMake usa `find_package(Python3)` + `Python3_EXECUTABLE`
  (portável, sem `python` hardcoded). Sem `pip install` necessário.
- Windows/macOS CI ficam para um passo seguinte após o Linux ficar verde.
- Nada da branch em andamento do builder (HOST-002, claims OOT-002/MM-002) foi
  integrado: ainda sem handoff/código.

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
