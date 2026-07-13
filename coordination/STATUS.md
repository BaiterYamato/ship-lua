# STATUS.md

## Projeto

ShipLua

## Estado global

bootstrap-done + Fases 2 e 3 em review + carga de mods nos hosts em review

## Repositórios

| Repositório | Estado | Branch de integração | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | criado + push (origin) | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (fork de HarbourMasters/Shipwright) | OOT-001 a OOT-005 + hotkey + mundo/assets em review (PRs #5 a #12) | lua/main | `fb3259f7` |
| BaiterYamato/2ship2harkinian (fork de HarbourMasters/2ship2harkinian) | MM-001 a MM-005 + hotkey + pulo + mundo/assets em review (PRs #1 a #9) | lua/main | `b3cc3662` |

> Nota: o fork do Shipwright foi renomeado para `Shipwright-HyliaFoundry`.
> Ambos os forks têm default `develop` e branch de integração `lua/main` já criada.

## Tarefa ativa recomendada

Integrar a pilha WORLD-001 a WORLD-004 no núcleo e os adaptadores de mundo/assets
nos hosts. Em seguida, implementar o supervisor autenticado para ativar o host de
destino e validar o round-trip OoT/MM com o mesmo pacote de mod.

## Progresso

- `GOV-001` — done (forks já existiam; `lua/main` presente nos dois).
- `GOV-002` — done (BaiterYamato/ship-lua criado e com push).
- `CORE-001` — review (runtime Lua 5.4 isolado + sandbox; ctest verde no Windows/MinGW).
- `MOD-001` — review (struct C++ + JSON Schema do manifesto).
- `MOD-002` — review (parser TOML com erros estruturados).
- `CORE-002` — review (ModHost multi-mod isolado, rollback e unload).
- `TEST-002` — review (fixtures TOML válidas e inválidas).
- `MOD-003` — review (discovery + extração ZIP limitada/transacional + carga pelo ModHost).
- `MOD-004` — review (SemVer 2.0.0, ranges conjuntivos e validação no manifesto).
- `MOD-005` — review (grafo e validação de dependências).
- `MOD-006` — review (ordem determinística por grafo, prioridade e ID).
- `MOD-007` — review (ciclos, incompatibilidades e IDs duplicados).
- `TOOL-001` — review (validador CLI para manifesto, diretório e `.shipmod`).
- `ARCH-001` — review (inventário de hooks nos commits atuais dos dois hosts).
- `ARCH-002` — review (RFC 0001 da arquitetura núcleo/adaptadores).
- `API-001` — review (`schema/api.yml` com tipos, funções e erros).
- `API-002` — review (`schema/events.yml` com payloads, suporte e fases).
- `API-003` — review (`schema/capabilities.yml` com status e hosts).
- `EVENT-001` — review (dispatcher independente de host/Lua e thread proprietária).
- `EVENT-002` — review (ordem por carga, prioridades, mod e registro).
- `EVENT-003` — review (`observe`, `filter`, `transform`, `consume` e isolamento de falhas).
- `TIMER-001` — review (`after`/`every` por frame, cancelamento e isolamento de falhas).
- `CODEGEN-001` — review (tipos e metadados C++ gerados dos schemas com gate de drift).
- `CODEGEN-002` — review (LuaDoc gerado com tipos, assinaturas e nomes de eventos).
- `CODEGEN-003` — review (referência Markdown em pt-BR gerada dos schemas).
- `BIND-001` — review (`require("ship")`, jogo/versões, capabilities, eventos e log).
- `OOT-001` — review (submódulo/CMake; `soh.exe` gerado no Windows/MSVC; PR #5).
- `MM-001` — review (submódulo/CMake; `2ship.exe` gerado no Windows/MSVC; PR #1).
- `OOT-002` — review (bootstrap idempotente + logger + CRT estático; `soh.exe` gerado; PR #6).
- `MM-002` — review (bootstrap idempotente + logger + CRT estático; `2ship.exe` gerado; PR #2).
- `OOT-003` — review (shutdown idempotente antes dos serviços do host; `soh.exe` gerado; PR #7).
- `MM-003` — review (shutdown idempotente antes dos serviços do host; `2ship.exe` gerado; PR #3).
- `OOT-004` — review (`oot`, versão 9.1.2 e commit real injetados/detectados; PR #8).
- `MM-004` — review (`mm`, versão 4.0.2 e commit real injetados/detectados; PR #4).
- `MOD-008` — done (descoberta da raiz, compatibilidade, dependências, isolamento e cache de `.shipmod`; integrado em `main`).
- `OOT-005` — review (pasta gravável `mods`, carga comum e `game.ready`; `soh.exe` gerado; PR #9).
- `MM-005` — review (pasta gravável `mods`, carga comum e `game.ready`; `2ship.exe` gerado; PR #5).
- `EXAMPLE-001` — review (pacote comum reproduzível, conformidade OoT/MM e artefato de CI; PR #20).
- `HOTKEY-001` — review (`ship.hotkeys.register` comum, callbacks seguros no unload e 24/24 testes; PR #21).
- `OOT-HOTKEY-001` — review (registry/configuração/dispatch no thread principal; `soh.exe` gerado; PR #10).
- `MM-HOTKEY-001` — review (registry/configuração/dispatch no thread principal; `2ship.exe` gerado; PR #6).
- `MM-JUMP-001` — review (`ship.mm.player.jump`, capability e exemplo MM; 24/24 testes e `2ship.exe` gerado; PRs ship-lua #22 e 2Ship #7).
- `WORLD-001` — review (sessão transacional OoT/MM, inventário canônico, assets lógicos e round-trip sintético; 25/25 testes MinGW/MSVC).
- `WORLD-003` — review (catálogo canônico extensível, tradução explícita de equipamento e adiamento de itens exclusivos; 26/26 testes MinGW/MSVC).
- `STORE-003` — review (substituição atômica interna, sincronização e testes de concorrência; 25/25 testes verdes em MinGW e MSVC).
- `WORLD-002` — review (envelope v1 com HMAC-SHA-256, anti-replay e publicação atômica; 28/28 testes MinGW/MSVC).
- `WORLD-004` — review (catálogo de assets por owner/contrato, sondas autenticadas e 29/29 testes MinGW/MSVC).
- `OOT-WORLD-002` — review (sonda de três espadas nativas, resolução fail-closed; `soh.exe` gerado; PR #12).
- `MM-WORLD-002` — review (sonda de três espadas nativas, resolução fail-closed; `2ship.exe` gerado; PR #9).

## Bloqueios

Nenhum registrado.

## Última integração

ship-lua `main` publicado em origin (BaiterYamato/ship-lua).

## Próxima ação

Integrar as branches empilhadas do núcleo em `main`, atualizar os submódulos dos
hosts para esse topo e validar o pacote comum. Depois, implementar o supervisor
do handoff autenticado. Os builds Windows/MSVC estão verdes; execução com ativos
legítimos, Linux e macOS continuam pendentes.
