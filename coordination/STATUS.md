# STATUS.md

## Projeto

ShipLua

## Estado global

bootstrap-done + Fase 2, arquitetura e base da Fase 3 em review

## Repositórios

| Repositório | Estado | Branch de integração | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | criado + push (origin) | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (fork de HarbourMasters/Shipwright) | fork ok, lua/main existe | lua/main | registrar SHA |
| BaiterYamato/2ship2harkinian (fork de HarbourMasters/2ship2harkinian) | fork ok, lua/main existe | lua/main | registrar SHA |

> Nota: o fork do Shipwright foi renomeado para `Shipwright-HyliaFoundry`.
> Ambos os forks têm default `develop` e branch de integração `lua/main` já criada.

## Tarefa ativa recomendada

Revisar e integrar a pilha de PRs da Fase 2 em ordem.

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

## Bloqueios

Nenhum registrado.

## Última integração

ship-lua `main` publicado em origin (BaiterYamato/ship-lua).

## Próxima ação

Integrar os PRs empilhados em ordem. Depois, iniciar o binding Lua do primeiro vertical slice.
Baseline de build dos jogos (CI-001/CI-002) e
submódulo `extern/ship-lua` nos forks ficam para a Fase 4.
