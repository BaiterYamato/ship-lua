local ship = require("ship")

ship.events.on("game.ready", function()
    if ship.hotkeys == nil then
        ship.log.warn("host sem suporte a ship.hotkeys — dog spawner desativado")
        return
    end
    ship.hotkeys.register("spawn_dog", { default = "F", label = "Spawn Dog" }, function()
        if ship.mm == nil or ship.mm.spawn_dog == nil then
            ship.log.warn("ship.mm.spawn_dog indisponivel neste host")
            return
        end
        if ship.mm.spawn_dog() then
            ship.log.info("Au au! cachorro spawnado")
        else
            ship.log.warn("nao deu pra spawnar aqui — tente estando em jogo na Clock Town")
        end
    end)
    ship.log.info("Dog Spawner pronto — aperte F (rebindável) na Clock Town")
end)
