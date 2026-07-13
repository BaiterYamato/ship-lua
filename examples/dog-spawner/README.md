# dog-spawner

Mod de exemplo para **Majora's Mask (2Ship)**: aperte **F** e um cachorro da Clock
Town (`En_Dg`) aparece na sua frente.

Demonstra o padrão recomendado para mods que reagem a input do jogador:

- o **host** publica o evento comum `input.hotkey` quando a tecla configurada é
  pressionada (padrão `F`, rebindável via CVar `gShipLua.DogHotkey.Scancode`);
- o **mod** (Lua) reage e chama uma função específica do jogo, `ship.mm.spawn_dog()`,
  fornecida pelo adaptador do MM.

```lua
ship.events.on("input.hotkey", function(event)
    if event.action == "spawn_dog" and ship.mm and ship.mm.spawn_dog then
        ship.mm.spawn_dog()
    end
end)
```

## Instalar

Copie esta pasta (ou empacote como `dog-spawner.shipmod`) para a pasta `mods/` do
2Ship e abra o jogo. Em jogo, aperte **F**.

## Notas

- Só funciona no MM (`games = ["mm"]`); em outros hosts `ship.mm` não existe e o mod
  simplesmente não spawna nada.
- O cachorro (`ACTOR_EN_DG`) é spawnado com um **path válido** (params `0x03E0`) — sem
  isso o ator se auto-remove ao entrar em idle (ex.: como Link humano).
- O objeto `object_dog` precisa estar carregado na cena para o cachorro renderizar
  (Clock Town e áreas com cachorros).
