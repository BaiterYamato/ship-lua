# Handoff — CODEGEN-002 / CODEGEN-003

## Estado

review

## Resultado

- LuaDoc gerado com aliases, classes, payloads, namespaces e assinaturas.
- Alias de nomes de evento usado por `ship.events.on` para autocomplete.
- Referência Markdown em português do Brasil cobre todos os contratos.
- Os dois artefatos derivam dos três schemas após validação canônica.
- Gate `--check` relata drift de cada saída separadamente.

## Commits

- `6a718f6` — `feat(codegen): generate LuaDoc and API docs`

## Arquivos alterados

- `tools/generate_api_docs.py`
- `generated/lua/shiplua.lua`
- `generated/docs/api-reference.md`
- `tests/schema/test_generate_api_docs.py`
- `tests/CMakeLists.txt`
- `docs/api/generated-docs.md`
- `coordination/claims/CODEGEN-002-003.md`
- `coordination/handoffs/CODEGEN-002-003.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
rtk python tools/generate_api_docs.py --check
rtk python tests/schema/test_generate_api_docs.py
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk git diff --check
```

## Resultado da validação

- Windows 11, Python 3.14, MinGW g++ 15.2, Ninja.
- 5/5 testes Python do gerador passaram.
- Build completo concluído.
- 19/19 testes CTest passaram.
- Saídas UTF-8 sincronizadas e documentação visível em pt-BR.

## Testes não executados

- Linux e macOS.
- Validação dentro de uma instalação real do Lua Language Server.
- Publicação em site de documentação.

## Pendências

- Empacotar `generated/lua/shiplua.lua` na distribuição de SDK.
- Conectar as assinaturas ao binding Lua real.
- Definir pipeline de site em `DOC-001` quando a referência crescer.

## Riscos

- O schema atual não possui descrição individual de funções; o LuaDoc expõe
  disponibilidade, capability e erros, mas não uma narrativa específica.
- `callback` permanece tipo genérico até os schemas modelarem assinaturas de
  callbacks por evento.

## Próxima ação recomendada

Implementar o binding mínimo de `ship.game`, `ship.api`, `ship.runtime`,
`ship.capabilities`, `ship.events` e `ship.log` para o primeiro vertical slice.
