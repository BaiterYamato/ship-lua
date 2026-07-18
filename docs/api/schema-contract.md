# Contrato dos schemas da API

Os arquivos `schema/api.yml`, `schema/events.yml` e `schema/capabilities.yml` são a fonte canônica da API pública ShipLua. Eles usam sintaxe JSON, que também é YAML 1.2 válido, para permitir validação sem dependências Python externas.

## Responsabilidades

- `api.yml`: tipos públicos, funções e códigos de erro;
- `events.yml`: nomes, tipos, payloads, suporte por host e fase;
- `capabilities.yml`: catálogo comum e específico, incluindo recursos apenas planejados.

Uma capability com status `planned` é documentação de roadmap e não pode ser referenciada por função ou evento público. Somente status `contract` integra a API `0.1.0`.

## IDL declarativa

Cada função de `api.yml` carrega, além de nome, argumentos, retorno e erros:

- `version`: SemVer da API em que a função entrou no contrato (nunca posterior à `api_version` do documento);
- `stability`: `experimental`, `preview`, `stable` ou `deprecated` (taxonomia do plan-sdk.md §15.5; símbolos `internal` não fazem parte da IDL pública).

## Artefatos gerados

| Ferramenta | Artefato |
|---|---|
| `tools/generate_cpp_api.py` | `generated/include/shiplua/generated/ApiBindings.h` (stubs/bindings C++) |
| `tools/generate_api_docs.py` | `generated/lua/shiplua.lua` (LuaDoc/autocomplete) e `generated/docs/api-reference.md` |
| `tools/generate_api_contracts.py` | `generated/lua/shiplua_validate.lua` (validadores), `generated/lua/shiplua_mock.lua` (mock runtime), `generated/tests/api_contract.lua` e `generated/tests/mock_contract.lua` (testes de contrato), `generated/docs/compatibility-matrix.md` (matriz OoT/MM) |

Todos possuem gate de drift no ctest (`--check`): editar os schemas sem regenerar quebra o build de testes.

## Invariantes automáticos

O comando abaixo valida os três documentos em conjunto:

```powershell
python tools/validate_api_schemas.py
```

O validador rejeita:

- versões divergentes;
- nomes duplicados ou inválidos;
- funções sem `version` SemVer ou com `stability` inválida;
- `version` de função posterior à `api_version`;
- tipos e códigos de erro inexistentes;
- ponteiros, endereços e layouts internos;
- capabilities órfãs ou ainda planejadas;
- recurso específico sem capability;
- suporte por host incompatível com a capability;
- evento `observe` marcado como cancelável.

Bindings C++, LuaDoc e documentação Markdown devem ser gerados somente destes arquivos. Alterações contratuais exigem RFC e incremento SemVer apropriado.
