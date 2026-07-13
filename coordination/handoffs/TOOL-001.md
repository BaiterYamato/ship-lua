# Handoff — TOOL-001

## Estado

review

## Resultado

- CLI valida arquivo TOML, diretório de mod ou pacote `.shipmod`.
- Pacotes usam o extrator seguro e diretório temporário removido por RAII.
- Nenhum entrypoint Lua é executado durante validação.
- Exit codes: `0` válido, `1` inválido, `2` uso incorreto.
- Mensagens e documentação são apresentadas em português do Brasil.

## Commits

- `c6705a9` — feat(tool): validate mod manifests

## Arquivos alterados

- `tools/manifest_validator.cpp`
- `docs/manifest-validator.md`
- `tests/fixtures/validator/valid-mod/manifest.toml`
- `tests/CMakeLists.txt`
- `CMakeLists.txt`
- `coordination/claims/TOOL-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
cmake --build build-verify
ctest --test-dir build-verify --output-on-failure
shiplua_manifest_validator valid-minimal.toml invalid-version.toml
git diff --check
rg --files | rg "\.(z64|n64|v64|o2r)$"
```

## Resultado da validação

- Windows 11 / MinGW g++ 15.2 / Ninja: build concluído.
- 10/10 testes CTest passaram.
- Saída manual exibida em português com resultado válido e inválido.
- `git diff --check` sem erros e nenhum ativo protegido encontrado.

## Pendências

- Empacotar/instalar o executável junto das releases em TOOL-003/TOOL-004.
- Executar testes em Linux e macOS.

## Riscos

- Diagnósticos internos são reduzidos a mensagens seguras em português; detalhes técnicos permanecem no `Result` do núcleo.
- Esta branch depende do PR #4 e deve permanecer empilhada até sua integração.

## Próxima ação recomendada

Integrar a pilha em ordem e iniciar ARCH-001/ARCH-002 antes da API pública.
