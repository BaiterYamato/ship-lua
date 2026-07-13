# Handoff — MM-WORLD-002

## Estado

review

## Resultado

- O adaptador MM sonda Kokiri Sword, Razor Sword e Gilded Sword no archive ativo.
- Cada referência só resolve após `HasFile`, validação da versão do archive e carga real pelo `ResourceManager`.
- O adaptador deixou de aceitar qualquer string com prefixo `mm.`.

## Commits

- `4bf1c71` — feat(mm): validate native world assets

## Arquivos alterados

- `mm/2s2h/MmWorldAdapter.h`
- `mm/2s2h/MmWorldAdapter.cpp`
- `extern/ship-lua`

## Validação executada

```powershell
cmake -S . -B build-msvc-assets2 -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc-assets2 --config Release --target 2ship -- /m:1
```

## Resultado da validação

- Windows 11, MSVC 19.44, Release: `2ship.exe` gerado com sucesso.
- PR: https://github.com/BaiterYamato/2ship2harkinian/pull/9

## Pendências

- Confirmar em runtime, com `mm.o2r` legítimo, que os três paths permanecem válidos.
- Linux e macOS não foram executados localmente.

## Riscos

- Um path removido pelo host fica indisponível e registra warning; não há fallback inseguro.

## Próxima ação recomendada

Integrar após WORLD-004 e executar o mod de conformidade de troca de mundo.
