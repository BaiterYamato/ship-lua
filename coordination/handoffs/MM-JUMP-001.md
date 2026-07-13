# Handoff — MM-JUMP-001

## Estado

review

## Resultado

- `ship.mm.player.jump()` foi definido no schema público e na RFC 0003.
- A capability `mm.player.jump` existe apenas para o host MM.
- O exemplo usa `ship.hotkeys.register`, exige a capability e declara `games = ["mm"]`.
- O adaptador valida gameplay, jogador vivo e chão antes de aplicar o impulso vertical.
- OoT não anuncia nem simula o recurso; o hotkey comum continua equivalente nos dois hosts.

## Commits

- `6aa220679` — Contrato, capability, exemplo, geração e testes.
- `034ad946a` — Bridge da ação no 2Ship2Harkinian.

## Arquivos centrais alterados

- `rfcs/0003-mm-player-jump.md`
- `schema/api.yml`
- `schema/capabilities.yml`
- `generated/**`
- `examples/jump/**`
- `tools/validate_api_schemas.py`
- `tests/schema/test_validate_api_schemas.py`
- `tests/unit/GeneratedApiBindingsTests.cpp`
- `mm/2s2h/ShipLuaBootstrap.cpp`
- `extern/ship-lua`

## Validação executada

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
.\build\shiplua_manifest_validator.exe examples\jump

cmake -S . -B build-msvc-shiplua -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=OFF
cmake --build build-msvc-shiplua --config Release --target 2ship
```

## Resultado da validação

- Núcleo, Windows 11, Ninja/MinGW: build concluído e 24/24 testes passaram.
- Manifesto `community.jump`: válido.
- Host MM, Visual Studio 2022/v143, Release: `x64/Release/2ship.exe` gerado.
- Rebuild incremental do target `2ship`: concluído.

## Pendências

- Smoke em jogo com ativos legítimos: pulo no chão, recusa no ar e todas as formas jogáveis.
- CI Linux e macOS do núcleo; build Linux/macOS do host.

## Riscos

- O valor do impulso replica o Moon Jump atual do host; mudanças futuras na física podem exigir calibração do adaptador sem alterar a assinatura Lua.

## Próxima ação recomendada

Revisar ship-lua PR #22 e 2Ship PR #7 depois de HOTKEY-001 e MM-HOTKEY-001.
