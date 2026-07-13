# Handoff — CODEGEN-001

## Estado

review

## Resultado

- Gerador Python reutiliza o validador dos schemas canônicos.
- Header C++ versionado contém tipos públicos, erros, funções, eventos e
  capabilities.
- Argumentos, retornos, payloads e suporte por host são preservados como
  metadados para os bindings.
- Identificadores C++ colidentes são recusados explicitamente.
- Gate `--check` e CTest detectam drift do artefato gerado.
- Teste C++ prova que o header é consumível pelo toolchain do projeto.

## Commits

- `f1be716` — `feat(codegen): generate C++ API bindings`

## Arquivos alterados

- `tools/generate_cpp_api.py`
- `generated/include/shiplua/generated/ApiBindings.h`
- `tests/schema/test_generate_cpp_api.py`
- `tests/unit/GeneratedApiBindingsTests.cpp`
- `tests/CMakeLists.txt`
- `CMakeLists.txt`
- `docs/api/cpp-codegen.md`
- `coordination/claims/CODEGEN-001.md`
- `coordination/handoffs/CODEGEN-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
rtk python tools/generate_cpp_api.py --check
rtk python tests/schema/test_generate_cpp_api.py
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk git diff --check
```

## Resultado da validação

- Windows 11, Python 3.14, MinGW g++ 15.2, Ninja, C++20.
- 5/5 testes Python do gerador passaram.
- Build completo concluído.
- 17/17 testes CTest passaram.
- Header sincronizado e sem ponteiros ou dependências dos hosts.

## Testes não executados

- Linux e macOS.
- Uso pelo binding Lua real, ainda não implementado.

## Pendências

- `CODEGEN-002`: gerar LuaDoc da mesma fonte.
- `CODEGEN-003`: gerar referência Markdown da mesma fonte.
- Conectar `FunctionId` ao registrador de funções do runtime Lua.

## Riscos

- A ordem dos arrays gerados acompanha a ordem canônica dos schemas; mudanças
  de ordem produzem diff mesmo sem mudança semântica.
- Tipos dinâmicos `any` e `callback` permanecem metadados e serão validados no
  binding Lua, não materializados como campos C++.

## Próxima ação recomendada

Implementar `CODEGEN-002` e `CODEGEN-003` no mesmo gerador ou em módulos
compartilhados, mantendo gates de drift separados por artefato.
