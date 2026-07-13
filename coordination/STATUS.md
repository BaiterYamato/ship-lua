# STATUS.md

## Projeto

ShipLua

## Estado global

bootstrap-done (repos criados) + core-runtime em review

## Repositórios

| Repositório | Estado | Branch de integração | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | criado + push (origin) | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (fork de HarbourMasters/Shipwright) | fork ok, lua/main existe | lua/main | registrar SHA |
| BaiterYamato/2ship2harkinian (fork de HarbourMasters/2ship2harkinian) | fork ok, lua/main existe | lua/main | registrar SHA |

> Nota: o fork do Shipwright foi renomeado para `Shipwright-HyliaFoundry`.
> Ambos os forks têm default `develop` e branch de integração `lua/main` já criada.

## Tarefa ativa recomendada

`CORE-002` (gerenciador multi-mod) + `MOD-001`/`MOD-002` (manifesto TOML).

## Progresso

- `GOV-001` — done (forks já existiam; `lua/main` presente nos dois).
- `GOV-002` — done (BaiterYamato/ship-lua criado e com push).
- `CORE-001` — review (runtime Lua 5.4 isolado + sandbox; ctest verde no Windows/MinGW).

## Bloqueios

Nenhum registrado.

## Última integração

ship-lua `main` publicado em origin (BaiterYamato/ship-lua).

## Próxima ação

Seguir com CORE-002 e MOD-001/MOD-002. Baseline de build dos jogos
(CI-001/CI-002) e submódulo `extern/ship-lua` nos forks ficam para a Fase 4.
