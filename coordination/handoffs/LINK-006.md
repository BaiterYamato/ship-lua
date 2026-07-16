# Handoff — LINK-006

## Estado

review

## Causa raiz

O asset `alpha.2` foi criado por `tar.exe -a` como ZIP com host Unix e uma
entrada raiz `.`/`./`. O 7-Zip aceitava o arquivo, mas o extrator nativo do
Windows interrompia a extração e o Explorador reportava pasta compactada
inválida.

## Resultado

- `New-LinkSpanWindowsArchive` usa 7-Zip em modo ZIP/FAT Windows.
- `Test-LinkSpanWindowsArchive` rejeita raízes Unix e exige round-trip completo
  por `Expand-Archive`.
- `build-linkspan.ps1 -CreateZip` produz e valida o ZIP automaticamente.
- `V-LINK-9` cobre geração e extração nativa de um pacote sintético.
- O pacote real de 9.051 arquivos foi extraído integralmente pelo Windows.

## Validação

- CTest: 32/32 testes passaram.
- ZIP real: 35.639.538 bytes, 9.051 arquivos.
- SHA-256: `28ebeba4fe07a61a6563b35cae8365bbdff36205b680a30acd67f9115a5f64a6`.
- Arquivos obrigatórios após extração: `link-span.exe`, `soh.o2r`, `2ship.o2r`,
  `hosts/oot/soh.exe`, `hosts/mm/2ship.exe`.

## Publicação

- PR: `https://github.com/BaiterYamato/link-span/pull/34`.
- Assets substituídos em
  `https://github.com/BaiterYamato/link-span/releases/tag/v0.1.0-alpha.2`.
- Cópia verificada baixada em `D:\Link-Span-Windows-x64.zip`.
