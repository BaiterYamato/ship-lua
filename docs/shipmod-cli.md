# CLI de desenvolvimento `shipmod`

O `shipmod` é o CLI de desenvolvimento de mods do ShipLua (plan-sdk.md §16,
release `v0.1.0-alpha.1` §29). É um script Python em `tools/shipmod.py` — sem
dependências externas — que orquestra as ferramentas compiladas do repositório.

```powershell
python tools/shipmod.py <subcomando> [opções]
```

Subcomandos disponíveis neste release: `new`, `validate`, `test` e `doctor`.

## `shipmod new <nome>`

Cria o scaffold de um mod novo nos moldes de `examples/`:

```powershell
python tools/shipmod.py new kafei-puppet --dir mods
```

Estrutura gerada em `mods/kafei-puppet/`:

```text
manifest.toml              # id community.kafei_puppet, api ">=0.1 <0.3"
main.lua                   # entrypoint mínimo com game.ready + log
README.md                  # instruções de validação e teste
tests/kafei_puppet_test.lua  # teste smoke na DSL describe/it/assert
```

Opções: `--dir` (diretório pai), `--id` (id do mod), `--autor` (padrão:
`user.name` do git), `--desc` (descrição curta).

O nome deve ser um slug em minúsculas (`kafei-puppet`); o id derivado usa
sublinhados (`community.kafei_puppet`). O scaffold se recusa a escrever em um
diretório não vazio.

## `shipmod validate <caminho> [...]`

Valida manifestos de mods — diretório com `manifest.toml`, arquivo TOML ou
pacote `.shipmod` — delegando ao `shiplua_manifest_validator` compilado:

```powershell
python tools/shipmod.py validate examples/hello-runtime
python tools/shipmod.py validate build/examples/hello-world.shipmod
```

Quando o validador canônico não está compilado, o CLI cai para uma verificação
estrutural básica em Python (campos obrigatórios, SemVer e `games`), marcada na
saída como "verificação básica". O executável é procurado na variável
`SHIPLUA_MANIFEST_VALIDATOR`, nos diretórios `build*` da raiz e no `PATH`.

## `shipmod test <caminho>`

Executa os testes do mod (`tests/*.lua`) no mock runtime, sem jogo ou ROM,
delegando ao `shiplua_mod_test_runner` (MODSDK-004):

```powershell
python tools/shipmod.py test examples/hello-runtime --game oot
python tools/shipmod.py test mods/kafei-puppet --capability core.input
```

As opções `--game` e `--capability` são repassadas ao runner. Enquanto o
MODSDK-004 não integra a `main`, o subcomando falha com código `3` e instruções
de compilação; um executável existente pode ser apontado pela variável
`SHIPLUA_MOD_TEST_RUNNER`.

## `shipmod doctor`

Diagnostica o ambiente de desenvolvimento e a saúde do repositório:

```powershell
python tools/shipmod.py doctor
```

Verificações:

- versões de Python, git, CMake (>= 3.20), Ninja e compilador C++;
- ferramentas Python do repo presentes em `tools/`;
- `shiplua_manifest_validator` e `shiplua_mod_test_runner` compilados;
- schemas da API válidos (`tools/validate_api_schemas.py`);
- codegen sem drift (`generate_cpp_api.py --check`, `generate_api_docs.py --check`);
- manifestos de `examples/` válidos.

Cada verificação resulta em `ok`, `aviso` ou `falha`; o comando sai com código
`1` se houver ao menos uma falha.

## Códigos de saída

| Código | Significado |
|---:|---|
| `0` | sucesso |
| `1` | falha (manifesto inválido, testes falharam, checks do doctor falharam) |
| `2` | uso inválido (argumentos) |
| `3` | ferramenta necessária ausente (ex.: `shiplua_mod_test_runner`) |
