# dog-spawner

Mod de exemplo que funciona em **ambos os jogos suportados**: aperte **F**
(rebindável) e um cachorro aparece na sua frente.

- Em **Ocarina of Time (Ship of Harkinian)**: spawna o cachorro do `Hyrule Market`
  (`En_Dog`) via `ship.oot.spawn_dog()`.
- Em **Majora's Mask (2Ship2Harkinian)**: spawna o cachorro de `Clock Town`
  (`En_Dg`) via `ship.mm.spawn_dog()`.

Demonstra o padrão de **mod multi-jogo com branching por capability**: a mesma
hotkey é registrada nos dois hosts, mas a função de spawn é resolvida em runtime
conforme a capability anunciada (`oot.spawn_dog` ou `mm.spawn_dog`).

```lua
local function resolveSpawnDog()
    if ship.capabilities.has("oot.spawn_dog") and ship.oot and ship.oot.spawn_dog then
        return ship.oot.spawn_dog
    end
    if ship.capabilities.has("mm.spawn_dog") and ship.mm and ship.mm.spawn_dog then
        return ship.mm.spawn_dog
    end
    return nil
end
```

## Instalar

Copie esta pasta (ou empacote como `dog-spawner.shipmod`) para a pasta `mods/`
do 2Ship ou do SoH e abra o jogo. Em jogo, aperte **F**.

## Notas

- Declara `games = ["oot", "mm"]` e lista `oot.spawn_dog` / `mm.spawn_dog` como
  capabilities **opcionais** — carrega em ambos os hosts, mas só spawna quando o
  host oferece a capability correspondente.
- **OoT**: o cachorro (`ACTOR_EN_DOG`) é o NPC da side-quest da Dog Lady. Spawnar
  fora do `Hyrule Market` pode fazer o ator ficar parado ou se remover, pois o
  `En_Dog` valida cena/params no init. Use em `SCENE_MARKET_DAY`/`SCENE_MARKET_NIGHT`
  para resultado confiável.
- **MM**: o objeto `object_dog` precisa estar carregado na cena (Clock Town e áreas
  com cachorros) para o ator renderizar.
