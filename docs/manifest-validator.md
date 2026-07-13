# Validador de manifestos

O executável `shiplua_manifest_validator` verifica manifestos sem executar o código Lua do mod.

## Entradas aceitas

- caminho direto para um `manifest.toml`;
- diretório contendo `manifest.toml`;
- pacote `.shipmod` em formato ZIP.

Pacotes são extraídos em um diretório temporário com os mesmos bloqueios de segurança do runtime. O conteúdo temporário é removido ao final da validação.

## Uso

```powershell
shiplua_manifest_validator caminho\para\manifest.toml
shiplua_manifest_validator caminho\para\mod
shiplua_manifest_validator caminho\para\mod.shipmod
```

Várias entradas podem ser verificadas na mesma execução.

## Códigos de saída

| Código | Significado |
|---:|---|
| `0` | todas as entradas são válidas |
| `1` | uma ou mais entradas são inválidas |
| `2` | nenhum caminho foi informado |

O validador não carrega o entrypoint, não cria um estado Lua e não resolve dependências instaladas. Ele verifica somente o contrato local de cada manifesto e a segurança estrutural do pacote.
