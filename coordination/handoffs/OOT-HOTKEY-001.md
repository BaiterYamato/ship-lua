# Handoff — OOT-HOTKEY-001

## Estado

review

## Resultado

- O Shipwright injeta o registry comum no bootstrap ShipLua.
- Keydowns são despachados no thread principal e a configuração é persistida por mod/hotkey.
- O unload remove registros antes de destruir o estado Lua.

## Commits

- `24ebcc161` — Bridge comum de hotkeys no OoT.

## Arquivos alterados

- `extern/ship-lua`
- `soh/soh/OotHotkeyRegistry.h`
- `soh/soh/OotHotkeyRegistry.cpp`
- `soh/soh/ShipLuaBootstrap.h`
- `soh/soh/ShipLuaBootstrap.cpp`
- `soh/soh/OTRGlobals.cpp`

## Validação executada

```powershell
cmake --build build-msvc-shiplua --config Release --target soh
```

## Resultado da validação

- Windows 11, Visual Studio 2022/v143, Release: `soh.exe` gerado.
- Rebuild incremental concluído em 11,2 s.

## Pendências

- Smoke em jogo com ativos legítimos.
- Builds Linux e macOS.

## Riscos

- A lista inicial de nomes de teclas é intencionalmente limitada e rejeita defaults desconhecidos.

## Próxima ação recomendada

Revisar o PR #10 após integrar HOTKEY-001 e a pilha OOT-001 a OOT-005.
