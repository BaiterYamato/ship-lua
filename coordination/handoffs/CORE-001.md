# Handoff — CORE-001

## Estado

review

## Resultado

- Repositório `ship-lua` iniciado localmente (git, branch `main`, sem remote ainda).
- Runtime Lua 5.4 isolado por mod (`LuaRuntime`): um `lua_State` por instância,
  alocador próprio com limite de memória configurável, RAII + move semantics.
- Sandbox inicial (CORE-003 parcial): abre apenas base/coroutine/table/string/
  math/utf8 + `os` sanitizado. `io`, `debug`, `package` nunca abertos;
  `dofile`, `loadfile` e `os.execute/exit/getenv/remove/rename/tmpname/setlocale`
  removidos.
- Logging estruturado por mod (CORE-004) via `Logger`/`LogSink`.
- Erros protegidos (CORE-005): `DoString` usa `lua_pcall` + `luaL_traceback`;
  nenhuma exceção C++ ou erro Lua cruza a fronteira; runtime segue usável após erro.
- Limite de memória por estado (CORE-006) implementado no alocador e testado.
- Lifecycle init/close (CORE-007 básico) via construtor/destrutor.

## Commits

- `b301723` — chore(repo): initialize ShipLua workspace scaffolding
- `3d56f20` — feat(runtime): add isolated Lua 5.4 runtime with sandbox (CORE-001)

## Arquivos alterados

- `CMakeLists.txt`
- `include/shiplua/runtime/Result.h`
- `include/shiplua/runtime/Logger.h`
- `include/shiplua/runtime/LuaRuntime.h`
- `src/runtime/LuaRuntime.cpp`
- `tests/CMakeLists.txt`
- `tests/unit/RuntimeTests.cpp`
- `.gitignore`

## Validação executada

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
```

## Resultado da validação

Windows 11 / MinGW g++ 15.2 / Ninja / Lua v5.4.7 (FetchContent).
`100% tests passed, 0 tests failed out of 1` (20 checks internos passam:
isolamento, libs perigosas ausentes, libs seguras presentes, erro protegido +
reuso, lifecycle, limite de memória).

## Pendências

- Sem remote/push (aguardando bootstrap GitHub GOV-001/GOV-002 autorizado).
- Falta build/test em Linux e macOS (matriz AGENTS §11).
- Sandbox ainda não expõe `require("ship")` nem loader customizado (fase API).
- CORE-002 (um estado por mod gerido por um Registry/ModHost) ainda não feito —
  hoje cada `LuaRuntime` já é isolado, mas falta o gerenciador multi-mod.

## Riscos

- `FetchContent` de Lua exige rede na configuração do CMake (build offline falha).
- Alocador não expõe métrica de uso por mod para fora (só limite interno).

## Próxima ação recomendada

Confirmar bootstrap GitHub (GOV-001/GOV-002) OU seguir local com CORE-002
(gerenciador multi-mod) e MOD-001/MOD-002 (schema + parser de manifesto TOML).
