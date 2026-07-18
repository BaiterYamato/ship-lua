# Handoff — LINK-005

## Estado

review

## Resultado

- O pacote público preserva `soh.o2r` e `2ship.o2r`, archives redistribuíveis
  necessários pelos ports.
- O scanner continua bloqueando ROMs e archives derivados do jogo:
  `.z64`, `.n64`, `.v64`, `oot.o2r`, `oot.otr` e `mm.o2r`.
- O empacotamento falha cedo quando o archive runtime do host está ausente.
- A documentação diferencia archives dos ports e archives privados do usuário.
- Os dois arquivos runtime foram aplicados em `D:\Link-Span-Windows-x64` sem
  alterar `oot.o2r`, `mm.o2r`, saves ou mods do usuário.

## Causa raiz

`-RomFree` tratava toda extensão `.o2r` como conteúdo derivado de ROM. Isso
removeu também `soh.o2r` e `2ship.o2r`, embora os executáveis procurem esses
archives próprios antes de carregar `oot.o2r` e `mm.o2r`.

## Validação executada

- MSVC Release build: sucesso.
- CTest: 32/32 testes passaram.
- `V-LINK-7`: archive do port permitido; ROM/archive base rejeitado.
- `V-LINK-8`: pacote público sem archive do port é rejeitado.
- Scanner do pacote final: nenhum arquivo privado/protegido.
- Smoke real em `D:\Link-Span-Windows-x64`:
  - OoT abriu até a tela Nintendo 64 sem `Missing soh.o2r`.
  - MM abriu a tela inicial sem `Missing 2ship.o2r`.

## Arquivos alterados

- `tools/LinkSpanPackaging.psm1`
- `tests/tools/BuildLinkSpanPackagingTests.ps1`
- `build-linkspan.ps1`
- `README.md`
- `docs/link-span-process.md`
- `coordination/claims/LINK-005.md`
- `coordination/handoffs/LINK-005.md`
- `coordination/STATUS.md`
- `docs/agents/Plans.md`

## Release

- PR: `https://github.com/BaiterYamato/link-span/pull/33`.
- Release: `https://github.com/BaiterYamato/link-span/releases/tag/v0.1.0-alpha.2`.
- `v0.1.0-alpha.1` não é funcional sem adicionar manualmente os dois archives
  runtime dos ports e deve ser tratada como obsoleta.
