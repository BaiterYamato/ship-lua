# STATUS.md

## Projeto

ShipLua

## Estado global

núcleo estável em `main`; API genérica `ship.actor` 0.4.0 concluída e validada em branch de review, aguardando integração no provider OoT

## Repositórios

| Repositório | Estado | Branch de integração | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | núcleo estável; sem PRs abertos | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (fork de HarbourMasters/Shipwright) | OOT-001 a OOT-005 + hotkey + mundo/assets em PRs abertos (#5 a #12) | lua/main | `fb3259f7` |
| BaiterYamato/2ship2harkinian (fork de HarbourMasters/2ship2harkinian) | MM-001 a MM-005 + hotkey + pulo + mundo/assets em PRs abertos (#1 a #9) | lua/main | `b3cc3662` |

> Nota: o fork do Shipwright foi renomeado para `Shipwright-HyliaFoundry`.
> Ambos os forks têm default `develop` e branch de integração `lua/main` já criada.

## Clones upstream puros (referência)

Clones somente-leitura do upstream HarbourMasters, rastreando `develop`. Uso:
comparar drift, sincronizar atualizações upstream (PLAN.md §11) e auditar hooks
sem depender dos forks BaiterYamato.

| Clone local | Upstream | Branch | HEAD observado |
|---|---|---|---|
| `D:/Desenvolvimento/ship-lua-worktrees/upstream/Shipwright` | `HarbourMasters/Shipwright` | `develop` | `585530f68` |
| `D:/Desenvolvimento/ship-lua-worktrees/upstream/2ship2harkinian` | `HarbourMasters/2ship2harkinian` | `develop` | `b3cc36628` |

> Drift atual: Shipwright upstream está à frente do baseline do fork (`fb3259f7`);
> 2ship2harkinian upstream coincide com o baseline do fork (`b3cc3662`).
> Atualizar com `git -C <clone> pull --ff-only` (single-branch, só `develop`).

## Tarefa ativa recomendada

Mergear a pilha de PRs dos dois hosts (8 OoT + 8 MM = 16 PRs) em `lua/main`,
do mais antigo ao mais recente (submódulo → bootstrap → shutdown → identidade →
mods dir → hotkey → pulo/save → sondagem de assets). Após cada merge, atualizar
o submódulo `extern/ship-lua` para o topo de `main`.

## Progresso

### Núcleo (`ship-lua/main`) — done

- `GOV-001` — done (forks já existiam; `lua/main` presente nos dois).
- `GOV-002` — done (BaiterYamato/ship-lua criado e com push).
- `ARCH-001` — done (inventário de hooks nos commits atuais dos dois hosts).
- `ARCH-002` — done (RFC 0001 da arquitetura núcleo/adaptadores).
- `CORE-001` — done (runtime Lua 5.4 isolado + sandbox; ctest verde no Windows/MinGW).
- `CORE-002` — done (ModHost multi-mod isolado, rollback e unload).
- `MOD-001` — done (struct C++ + JSON Schema do manifesto).
- `MOD-002` — done (parser TOML com erros estruturados).
- `MOD-003` — done (discovery + extração ZIP limitada/transacional + carga pelo ModHost).
- `MOD-004` — done (SemVer 2.0.0, ranges conjuntivos e validação no manifesto).
- `MOD-005` — done (grafo e validação de dependências).
- `MOD-006` — done (ordem determinística por grafo, prioridade e ID).
- `MOD-007` — done (ciclos, incompatibilidades e IDs duplicados).
- `MOD-008` — done (descoberta da raiz, compatibilidade, dependências, isolamento e cache de `.shipmod`).
- `TEST-002` — done (fixtures TOML válidas e inválidas).
- `TOOL-001` — done (validador CLI para manifesto, diretório e `.shipmod`).
- `API-001` — done (`schema/api.yml` com tipos, funções e erros).
- `API-002` — done (`schema/events.yml` com payloads, suporte e fases).
- `API-003` — done (`schema/capabilities.yml` com status e hosts).
- `EVENT-001` — done (dispatcher independente de host/Lua e thread proprietária).
- `EVENT-002` — done (ordem por carga, prioridades, mod e registro).
- `EVENT-003` — done (`observe`, `filter`, `transform`, `consume` e isolamento de falhas).
- `TIMER-001` — done (`after`/`every` por frame, cancelamento e isolamento de falhas).
- `CODEGEN-001` — done (tipos e metadados C++ gerados dos schemas com gate de drift).
- `CODEGEN-002` — done (LuaDoc gerado com tipos, assinaturas e nomes de eventos).
- `CODEGEN-003` — done (referência Markdown em pt-BR gerada dos schemas).
- `BIND-001` — done (`require("ship")`, jogo/versões, capabilities, eventos e log).
- `HOTKEY-001` — done (`ship.hotkeys.register` comum, callbacks seguros no unload e testes; PR #21 merged).
- `EXAMPLE-001` — done (pacote comum reproduzível, conformidade OoT/MM e artefato de CI; PR #20 merged).
- `MM-JUMP-001` — done (`ship.mm.player.jump`, capability e exemplo MM; integrado em `main`).
- `WORLD-001` — done (sessão transacional OoT/MM, inventário canônico, assets lógicos e round-trip sintético; 25/25 testes MinGW/MSVC).
- `WORLD-003` — done (catálogo canônico extensível, tradução explícita de equipamento e adiamento de itens exclusivos; 26/26 testes MinGW/MSVC).
- `STORE-003` — done (substituição atômica interna, sincronização e testes de concorrência; 25/25 testes MinGW/MSVC; PR #27 merged).
- `WORLD-002` — done (envelope v1 com HMAC-SHA-256, anti-replay e publicação atômica; 28/28 testes MinGW/MSVC; PR #28 merged).
- `WORLD-004` — done (catálogo de assets por owner/contrato, sondas autenticadas e 29/29 testes MinGW/MSVC; PR #30 merged).
- `DOC-005` — done (conjunto completo de documentação em inglês; PR #29 merged).
- `MODSDK-001` — done (PR #36 integrada após correção das fronteiras Lua/MSVC;
  capability registry, Release/MSVC 46/46 e CI Linux/Windows/package verdes).
- `MODSDK-004` — done (mock runtime e mod test runner integrados pela PR #39;
  callbacks Lua sem `longjmp` sobre estado C++; Release/MSVC 45/45 e CI verde).
- `MODSDK-005` — review (PR #41; RFC 0009, `ActorProvider`, `ship.actor.spawn/destroy/exists`,
  permissões/limites por manifesto, mock ROM-free e exemplos OoT/MM; Release/MSVC
  56/56 e schemas/codegen 30/30 verdes).
- `TOOL-006` — review (`build-game.ps1` portátil entre raiz do host/submódulo,
  detecção testada e auditoria de integração registrada).
- `LINK-002` — review (gating de assets do launcher, parser de
  `requires_both_games` e mensagens de bloqueio cobertos por testes).
- `LINK-003` — done (PR #32 integrada; launcher dual, pacote ROM-free e
  `world.travel` preservados sobre o SDK 0.3.0).
- `LINK-003-CONFLICTS` — done (conflitos resolvidos; Release/MSVC, Ubuntu e
  package CI verdes, sem arquivos protegidos rastreados).
- `MODSDK-006` — review (CLI `shipmod` com `new`/`validate`/`test`/`doctor`,
  exemplo `hello-runtime` executável nos dois hosts e 53/53 testes MSVC).

> Observação: o núcleo estável, mas não há formalização de "review aceito" para
> CORE/MOD/API/EVENT/CODEGEN/BIND originais — estão em `main` e os testes
> passam, mas o aceite técnico formal (auditoria de specs, fuzzing, etc.)
> permanece pendente antes de marcar 0.1.0.

### Hosts — review (PRs abertos em `lua/main`, não merged)

#### Shipwright (OoT)

- `OOT-001` — review (submódulo/CMake; `soh.exe` gerado no Windows/MSVC; PR #5).
- `OOT-002` — review (bootstrap idempotente + logger + CRT estático; `soh.exe` gerado; PR #6).
- `OOT-003` — review (shutdown idempotente antes dos serviços do host; `soh.exe` gerado; PR #7).
- `OOT-004` — review (`oot`, versão 9.1.2 e commit real injetados/detectados; PR #8).
- `OOT-005` — review (pasta gravável `mods`, carga comum e `game.ready`; `soh.exe` gerado; PR #9).
- `OOT-HOTKEY-001` — review (registry/configuração/dispatch no thread principal; `soh.exe` gerado; PR #10).
- `OOT-WORLD-001` — review (adaptador de save portátil OoT, destinos Kakariko/Market e bootstrap; `soh.exe` gerado; PR #11).
- `OOT-WORLD-002` — review (sonda de três espadas nativas, resolução fail-closed; `soh.exe` gerado; PR #12).

#### 2Ship (MM)

- `MM-001` — review (submódulo/CMake; `2ship.exe` gerado no Windows/MSVC; PR #1).
- `MM-002` — review (bootstrap idempotente + logger + CRT estático; `2ship.exe` gerado; PR #2).
- `MM-003` — review (shutdown idempotente antes dos serviços do host; `2ship.exe` gerado; PR #3).
- `MM-004` — review (`mm`, versão 4.0.2 e commit real injetados/detectados; PR #4).
- `MM-005` — review (pasta gravável `mods`, carga comum e `game.ready`; `2ship.exe` gerado; PR #5).
- `MM-HOTKEY-001` — review (registry/configuração/dispatch no thread principal; `2ship.exe` gerado; PR #6).
- `MM-JUMP-001` — review (bridge do `ship.mm.player.jump` no host; `2ship.exe` gerado; PR #7).
- `MM-WORLD-001` — review (adaptador de save portátil MM, destinos Clock Town/Woodfall e bootstrap; `2ship.exe` gerado; PR #8).
- `MM-WORLD-002` — review (sonda de três espadas nativas, resolução fail-closed; `2ship.exe` gerado; PR #9).

### Não iniciado

- `OOT-006`/`MM-006` (menu/console mínimo).
- `OOT-EVT-001` a `OOT-EVT-004` / `MM-EVT-001` a `MM-EVT-004` (ponte de eventos do `GameInteractor`).
- `HANDLE-001/002`, `PLAYER-001` a `PLAYER-004`, `ACTOR-001` a `ACTOR-004`, `SECURITY-001`.
- `STORE-001/002/004`, `CONSOLE-001/002`, `RELOAD-001` a `RELOAD-003`, `SECURITY-002/003`.
- `OOT-OCARINA`, `OOT-EQUIPMENT`, etc.; `MM-CYCLE`, `MM-CLOCK`, `MM-MASKS`, etc. (Fase 8).
- `DOC-001` a `DOC-004`, `TOOL-002` a `TOOL-005`, `EXAMPLE-002` a `EXAMPLE-004` (Fase 9).
- `CI-001` a `CI-004` (baseline hosts Linux), workflows de drift, conformance.

## Bloqueios

Nenhum registrado.

## Última integração

ship-lua `main` em `fff5785` (MODSDK-001, MODSDK-004, MODSDK-006 e LINK-003 integrados). MODSDK-005 está em branch de review sobre esse baseline.

## Próxima ação

1. Integrar MODSDK-005 no provider OoT: atualizar `extern/ship-lua`, implementar `ActorProvider`, injetar em `LuaApiHostContext::actors` e rebuildar `soh.exe`.
2. Após aprovação do núcleo 0.4.0, atualizar os demais hosts/submódulos.
3. Sincronizar drift do Shipwright upstream (`585530f68`) para o fork `lua/main` quando pertinente (PLAN.md §11); o 2ship upstream já coincide com o baseline.
4. Validar runtime com ativos legítimos dos dois jogos (builds MSVC verdes, mas sem runtime testado com ROM).
5. Adicionar builds Linux/macOS (só Windows verificado).
6. Publicar `v0.1.0` após atender os critérios restantes do PLAN.md §16.
