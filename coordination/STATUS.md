# STATUS.md

## Projeto

ShipLua

## Estado global

bootstrap-done (repos criados) + core-runtime/manifesto/mod-host em review

## Repositórios

| Repositório | Estado | Branch de integração | Baseline |
|---|---|---|---|
| BaiterYamato/ship-lua | criado + push (origin) | main | — |
| BaiterYamato/Shipwright-HyliaFoundry (fork de HarbourMasters/Shipwright) | fork ok, lua/main existe | lua/main | registrar SHA |
| BaiterYamato/2ship2harkinian (fork de HarbourMasters/2ship2harkinian) | fork ok, lua/main existe | lua/main | registrar SHA |

> Nota: o fork do Shipwright foi renomeado para `Shipwright-HyliaFoundry`.
> Ambos os forks têm default `develop` e branch de integração `lua/main` já criada.

## Tarefa ativa recomendada

Revisar e integrar `CORE-002` + `MOD-001`/`MOD-002`.

## Progresso

- `GOV-001` — done (forks já existiam; `lua/main` presente nos dois).
- `GOV-002` — done (BaiterYamato/ship-lua criado e com push).
- `CORE-001` — review (runtime Lua 5.4 isolado + sandbox; ctest verde no Windows/MinGW).
- `MOD-001` — review (struct C++ + JSON Schema do manifesto).
- `MOD-002` — review (parser TOML com erros estruturados).
- `CORE-002` — review (ModHost multi-mod isolado, rollback e unload).

## Bloqueios

Nenhum registrado.

## Última integração

ship-lua `main` publicado em origin (BaiterYamato/ship-lua).

## Próxima ação

Revisar o PR de CORE-002/MOD-001/MOD-002. Depois, seguir para TEST-002 e
MOD-003. Baseline de build dos jogos (CI-001/CI-002) e submódulo
`extern/ship-lua` nos forks ficam para a Fase 4.
