# Handoff — MODSDK-006

- Status: review
- Branch: `agent/MODSDK-006-dev-cli-integration`

## Resultado

- `tools/shipmod.py` oferece `new`, `validate`, `test` e `doctor` usando apenas
  a biblioteca padrão do Python.
- `new` gera manifesto API `>=0.1 <0.4`, Lua, README e teste mock; valores
  fornecidos pelo usuário são codificados com segurança para TOML/Lua.
- `validate` delega ao validador C++ e mantém fallback estrutural identificado.
- `test` delega ao mod test runner integrado, com jogo e capabilities.
- `doctor` verifica toolchain, ferramentas, schemas, três geradores e exemplos.
- `examples/hello-runtime` passa em OoT e MM sem jogo ou ROM.

## Validação

- 19/19 testes Python.
- `shipmod doctor`: 0 falhas.
- Release/MSVC: build aprovado.
- CTest: 53/53 aprovados.

## Próximo

- Publicar PR, confirmar CI Linux/Windows/package e integrar.
- Depois iniciar `OOT-MODSDK-001`, o provider genérico de atores OoT.
