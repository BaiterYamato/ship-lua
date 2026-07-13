# Olá, mundo

Este é o primeiro mod comum do ShipLua. O mesmo pacote funciona no Shipwright e no
2Ship2Harkinian porque usa apenas a API compartilhada.

## Gerar o pacote

```powershell
cmake --build build --target package_examples
```

O arquivo será criado em `build/examples/hello-world.shipmod`. A geração fixa ordem,
timestamp e permissões das entradas para produzir o mesmo conteúdo em builds iguais.

## Instalar

Copie `hello-world.shipmod` para a pasta `mods` do diretório gravável do jogo. Na
inicialização, o log deve registrar uma das mensagens:

```text
Hello from oot host=<versão>
Hello from mm host=<versão>
```

O mod não solicita permissões e não acessa arquivos, rede, DLLs ou ativos dos jogos.
