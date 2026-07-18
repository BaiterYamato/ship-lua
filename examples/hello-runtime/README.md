# Hello, runtime

Exemplo mínimo do release `v0.1.0-alpha.1` do SDK (plan-sdk.md §29): demonstra a
superfície central da API comum — logging, detecção de capabilities
(`ship.capabilities.has`), eventos (`ship.events.on`) e hotkeys
(`ship.hotkeys.register`) — sem depender de recursos exclusivos de um host.

## Validar

```powershell
python tools/shipmod.py validate examples/hello-runtime
```

## Testar sem jogo (mock runtime)

Os testes em `tests/` usam a DSL `describe`/`it`/`assert` do
`shiplua_mod_test_runner` (MODSDK-004) e rodam sem jogo ou ROM:

```powershell
python tools/shipmod.py test examples/hello-runtime
```

## Instalar

Empacote o diretório como `hello-runtime.shipmod` (ou copie a pasta para o
diretório `mods` do jogo). Na inicialização, o log deve registrar:

```text
hello-runtime carregado em oot
hello-runtime pronto — aperte H (rebindável)
```

Ao pressionar `H` (rebindável nas configurações do ShipLua):

```text
Hello from oot!
```

O mod não solicita permissões e não acessa arquivos, rede, DLLs ou ativos dos jogos.
