# Handoff — EXAMPLE-001

## Estado

review

## Resultado

- `examples/hello-world` contém manifesto, entrypoint e instruções em pt-BR.
- O alvo `package_examples` gera `hello-world.shipmod` sem versionar o ZIP.
- A geração fixa ordem, timestamp e permissões e é byte a byte reproduzível.
- O mesmo pacote carrega pelos contextos `oot` e `mm`, recebe `game.ready` e registra a identidade do host.
- `UnloadAll` remove o mod, o callback e a extração temporária.

## Commits

- `6e42d36` — feat(example): add reproducible hello-world package

## Arquivos alterados

- `CMakeLists.txt`
- `examples/hello-world/manifest.toml`
- `examples/hello-world/main.lua`
- `examples/hello-world/README.md`
- `tests/CMakeLists.txt`
- `tests/conformance/HelloWorldConformanceTests.cpp`
- `tests/conformance/PackageHelloWorld.py`
- `tests/conformance/test_package_hello_world.py`

## Validação executada

```powershell
cmake -S . -B build -G Ninja
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

## Resultado da validação

- Windows 11, MinGW g++ 15.2, Ninja, C++20.
- 24 de 24 testes passaram.
- Dois empacotamentos independentes produziram SHA-256 idêntico.
- O validador aceitou o `.shipmod` gerado.

## Pendências

- Copiar o pacote para os diretórios reais dos dois hosts e observar os logs com ativos legítimos.
- Validar Linux e macOS.

## Riscos

- O teste atual usa os contextos reais de identidade, mas não inicializa os executáveis dos jogos.

## Próxima ação recomendada

Após integrar OOT-005 e MM-005, executar o pacote gerado nos dois hosts e registrar a evidência do log.
