# Writing Your First Mod

A mod is a folder with two files, zipped into a `.shipmod`:

```text
my-mod/
├── manifest.toml
└── main.lua
```

## 1. The manifest

```toml
id = "yourname.my_mod"
name = "My Mod"
version = "0.1.0"
api = ">=0.4 <0.5"
entrypoint = "main.lua"
description = "My first Link-Span mod."
authors = ["You"]
games = ["oot", "mm"]
```

## 2. Hello world

```lua
local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("Hello from " .. ship.game.id())
end)
```

## 3. Add a hotkey

```lua
ship.hotkeys.register("greet", { default = "G", label = "Greet" }, function()
    ship.log.info("G pressed in " .. ship.game.id())
end)
```

Hotkeys are rebindable by the player in the ShipLua settings.

## 4. Spawn something

```lua
ship.hotkeys.register("spawn", { default = "K", label = "Spawn" }, function()
    -- By curated name…
    local actor, err = ship.actor.spawn(
        ship.game.id() == "oot" and "oot.en_dog" or "mm.en_dg",
        { position = { x = 0, y = 0, z = 0 } })  -- (0,0,0) = in front of you
    -- …or by raw id, e.g. ship.actor.spawn("mm.id.0x021", …)
    if not actor then
        ship.log.warn(err.code .. ": " .. err.message)
    end
end)
```

## 5. Test without the game (optional)

```powershell
python tools/shipmod.py validate my-mod
python tools/shipmod.py test my-mod --game oot
```

## 6. Package and install

```powershell
Compress-Archive -Path my-mod\manifest.toml, my-mod\main.lua -DestinationPath my-mod.zip
Rename-Item my-mod.zip my-mod.shipmod
```

Copy it to the game's `mods/` folder and boot. The log tells you if the mod
loaded — or exactly why it was rejected.

Full guide: [docs/writing-mods.en.md](https://github.com/BaiterYamato/link-span/blob/main/docs/writing-mods.en.md)

---

**Resumo (pt-BR):** crie `manifest.toml` (com `api = ">=0.4 <0.5"`) e
`main.lua`, registre eventos e hotkeys pela API `ship.*`, teste sem ROM com o
CLI `shipmod`, zipe os dois arquivos como `.shipmod` e jogue na pasta `mods/`.
Guia completo em `docs/writing-mods.md` (português).
