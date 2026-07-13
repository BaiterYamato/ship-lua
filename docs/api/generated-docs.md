# Documentação gerada da API

`tools/generate_api_docs.py` produz dois artefatos versionados a partir dos
schemas canônicos:

- `generated/lua/shiplua.lua`: definições LuaDoc para autocomplete e análise
  estática no Lua Language Server;
- `generated/docs/api-reference.md`: referência humana em português do Brasil.

O LuaDoc inclui aliases, objetos, payloads de evento, namespaces e assinaturas.
O parâmetro de evento usa um alias com todos os nomes canônicos. O Markdown
lista tipos, funções, eventos, suporte por host, capabilities e erros.

## Atualização

```powershell
rtk python tools/generate_api_docs.py
```

## Verificação de drift

```powershell
rtk python tools/generate_api_docs.py --check
```

O CTest executa o gate e os testes do gerador. Os artefatos não devem ser
editados manualmente; descrições duráveis pertencem aos schemas.
