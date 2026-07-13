# Contrato dos schemas da API

Os arquivos `schema/api.yml`, `schema/events.yml` e `schema/capabilities.yml` são a fonte canônica da API pública ShipLua. Eles usam sintaxe JSON, que também é YAML 1.2 válido, para permitir validação sem dependências Python externas.

## Responsabilidades

- `api.yml`: tipos públicos, funções e códigos de erro;
- `events.yml`: nomes, tipos, payloads, suporte por host e fase;
- `capabilities.yml`: catálogo comum e específico, incluindo recursos apenas planejados.

Uma capability com status `planned` é documentação de roadmap e não pode ser referenciada por função ou evento público. Somente status `contract` integra a API `0.1.0`.

## Invariantes automáticos

O comando abaixo valida os três documentos em conjunto:

```powershell
python tools/validate_api_schemas.py
```

O validador rejeita:

- versões divergentes;
- nomes duplicados ou inválidos;
- tipos e códigos de erro inexistentes;
- ponteiros, endereços e layouts internos;
- capabilities órfãs ou ainda planejadas;
- recurso específico sem capability;
- suporte por host incompatível com a capability;
- evento `observe` marcado como cancelável.

Bindings C++, LuaDoc e documentação Markdown devem ser gerados somente destes arquivos. Alterações contratuais exigem RFC e incremento SemVer apropriado.
