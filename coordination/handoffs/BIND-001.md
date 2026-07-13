# Handoff — BIND-001

## Estado

review

## Resultado

- `require("ship")` interno funciona sem abrir `package` ou módulos externos.
- Jogo, versões, capabilities, eventos e logging seguem o schema 0.1.0.
- O mesmo `hello-world` executa em memória com contextos OoT e MM.
- Payloads recursivos são convertidos sem ponteiros ou layouts dos jogos.
- Callbacks Lua usam chamada protegida, traceback e desativação após três falhas.
- `off`, rollback e unload liberam inscrições e referências do registry.
- Instalação do módulo ocorre dentro de `lua_pcall`, inclusive sob memória baixa.
- `UnloadAll` copia IDs antes de apagar o mapa, eliminando dangling reference.

## Commits

- `ad5aad6` — `feat(api): bind Lua runtime services`

## Arquivos alterados

- `include/shiplua/api/LuaApiBinding.h`
- `src/api/LuaApiBinding.cpp`
- `include/shiplua/host/ModHost.h`
- `src/host/ModHost.cpp`
- `tests/unit/LuaApiBindingTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/lua-binding.md`
- `coordination/claims/BIND-001.md`
- `coordination/handoffs/BIND-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk cmake -S . -B build-warnings -G Ninja `
  "-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion"
rtk cmake --build build-warnings --target lua_api_binding_tests manifest_host_tests
rtk ctest --test-dir build-warnings `
  -R "lua_api_binding_tests|manifest_host_tests" --output-on-failure
rtk git diff --check
```

## Resultado da validação

- Windows 11, MinGW g++ 15.2, Ninja, C++20.
- Build normal completo concluído.
- 20/20 testes CTest passaram.
- Build com warnings ampliados compilou o binding sem warnings novos.
- 2/2 testes focais passaram também no build com warnings.
- Nenhum ativo protegido ou header/global dos jogos foi introduzido.

## Testes não executados

- Linux e macOS.
- Shipwright e 2Ship reais; o slice atual usa contextos sintéticos sem ROM.
- Limite de instruções por callback; permanece responsabilidade do runtime.

## Pendências

- Conectar `LuaApiHostContext` aos adaptadores reais.
- Publicar `game.ready`, `game.frame` e `game.shutdown` pelo bootstrap dos hosts.
- Integrar `FrameTimerScheduler` ao evento de frame.
- Empacotar o exemplo `hello-world.shipmod` para conformance nos dois ports.

## Riscos

- O binding atual trata os eventos contratados como observação; semânticas Lua
  de filter/transform/consume ainda exigem contrato específico no schema.
- O build com warnings também apontou avisos preexistentes em
  `PackageExtractor.cpp` e headers miniz, fora deste claim.
- Contexto de host vazio mantém API/versão/log/eventos disponíveis, mas
  `ship.game.*` falha até o bootstrap fornecer identidade.

## Próxima ação recomendada

Após integrar os PRs de codegen e binding, iniciar `OOT-001` e `MM-001` para
adicionar ShipLua como submódulo e conectar o bootstrap mínimo nos dois hosts.
