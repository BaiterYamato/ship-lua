# Handoff — MM-WORLD-001

## Estado

review

## Resultado

- O adaptador MM captura vida, capacidade, rupees, arco, bombas, bombchu, hookshot, ocarina, espada ativa e máscara Deku em IDs canônicos.
- A importação valida o catálogo, traduz equipamento nativo e adia itens exclusivos sem mutar o save durante a preparação.
- Quando o snapshot OoT contém várias espadas, MM materializa a equipada ou a mais forte e mantém as demais diferidas para o round-trip.
- O commit aplica o snapshot validado e resolve `mm.clock_town` e `mm.woodfall` para entradas nativas.
- Arco e bombas criam apenas a capacidade mínima ausente, sem reduzir upgrades já existentes.
- O bootstrap cria e mantém uma instância do adaptador ao lado do runtime, das hotkeys e do binding `ship.mm.player.jump`.

## Commits

- `8ca2ab7` — feat(mm): add portable save adapter
- `6c15c7f` — docs(mm): claim portable save adapter
- `bd591a0` — chore(world): merge portable session core

## Arquivos alterados

- `mm/2s2h/MmWorldAdapter.h`
- `mm/2s2h/MmWorldAdapter.cpp`
- `mm/2s2h/ShipLuaBootstrap.h`
- `mm/2s2h/ShipLuaBootstrap.cpp`
- `coordination/claims/MM-WORLD-001.md`
- `coordination/handoffs/MM-WORLD-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
cmake -S . -B build-msvc-world -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc-world --config Release --target 2ship -- /m:1
cmake -S . -B build-mm-world -G Ninja
cmake --build build-mm-world
ctest --test-dir build-mm-world --output-on-failure
```

## Resultado da validação

- Windows 11, Visual Studio 2022 v143, Release: `2ship.exe` gerado com sucesso.
- Windows 11, MinGW: 26/26 testes do núcleo combinado passaram.
- O rebuild incremental após materializar upgrades mínimos também gerou `2ship.exe`.

## Pendências

- Executar smoke com ativos legítimos e save descartável.
- Conectar os adaptadores OoT/MM ao supervisor e ao handoff entre processos.
- Persistir e autenticar o snapshot antes de encerrar um host.
- Validar Linux e macOS.

## Riscos

- O adaptador ainda não persiste o save em disco por conta própria; ele altera o `SaveContext` vivo e dispara a transição.
- A máscara Deku é preservada no inventário, mas o adaptador não força transformação durante o commit.
- O bridge entre processos e a resolução de assets do outro host ainda não estão implementados.

## Próxima ação recomendada

Implementar o supervisor transacional que serializa o snapshot, inicia o host de destino e só confirma a troca após o destino validar a importação.
