# Handoff — HOTKEY-001

## Estado

review

## Resultado

- `ship.hotkeys.register` foi definido como API comum 0.2.0.
- Registros são isolados por mod, substituíveis e removidos no unload.
- Callbacks ficam inativos antes do fechamento do estado Lua e são desativados após três falhas consecutivas.
- Erros de validação são convertidos em erro Lua somente após sair do bloco C++ `try`, evitando corrupção de heap pelo `longjmp` no MSVC.
- O exemplo `dog-spawner` usa a API comum; o exemplo de pulo foi retirado deste escopo porque depende de uma ação MM ainda sem contrato.

## Commits

- `1d034c2` — Registry SPI comum.
- `9d6563f` — Contrato da API 0.2.0.
- `6065b5d` — Binding Lua.
- `110641a` — Testes de registro e fallback.
- `c04f1cc` — Lifecycle seguro no unload.

## Arquivos centrais alterados

- `include/shiplua/input/HotkeyRegistry.h`
- `src/input/HotkeyRegistry.cpp`
- `include/shiplua/api/LuaApiBinding.h`
- `src/api/LuaApiBinding.cpp`
- `schema/api.yml`
- `rfcs/0002-common-hotkey-registration.md`
- `tests/unit/LuaApiBindingTests.cpp`

## Validação executada

```powershell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

## Resultado da validação

- Windows 11, Ninja/MinGW: build concluído.
- CTest: 24/24 testes passaram.
- Windows 11, Visual Studio 2022/v143, Release: 24/24 testes passaram.
- Validações focais de schema, codegen e bindings passaram.

## Pendências

- CI Linux e macOS.
- Smoke com os executáveis e ativos legítimos nos dois hosts.

## Riscos

- O menu de configuração depende da persistência CVar fornecida por cada host.

## Próxima ação recomendada

Revisar o PR #21 e, depois, os bridges OOT-HOTKEY-001 e MM-HOTKEY-001.
