# Handoff — MM-HOTKEY-001

## Estado

review

## Resultado

- O 2Ship2Harkinian injeta o registry comum no bootstrap ShipLua.
- Keydowns são despachados no thread principal e a configuração é persistida por mod/hotkey.
- A descoberta CMake acompanha novas fontes com `CONFIGURE_DEPENDS`.
- O unload remove registros antes de destruir o estado Lua.

## Commits

- `24093bc` — Bridge comum de hotkeys no MM.

## Arquivos alterados

- `extern/ship-lua`
- `mm/2s2h/MmHotkeyRegistry.h`
- `mm/2s2h/MmHotkeyRegistry.cpp`
- `mm/2s2h/ShipLuaBootstrap.h`
- `mm/2s2h/ShipLuaBootstrap.cpp`
- `mm/2s2h/BenPort.cpp`
- `mm/CMakeLists.txt`

## Validação executada

```powershell
cmake --build build-msvc-shiplua --config Release --target 2ship
```

## Resultado da validação

- Windows 11, Visual Studio 2022/v143, Release: `2ship.exe` gerado.
- Rebuild incremental concluído em 4,4 s.

## Pendências

- Smoke em jogo com ativos legítimos.
- Builds Linux e macOS.

## Riscos

- A lista inicial de nomes de teclas é intencionalmente limitada e rejeita defaults desconhecidos.

## Próxima ação recomendada

Revisar o PR #6 após integrar HOTKEY-001 e a pilha MM-001 a MM-005.
