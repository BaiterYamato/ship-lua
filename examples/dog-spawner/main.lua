local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("Dog Spawner pronto — aperte F na Clock Town para spawnar um cachorro")
end)

ship.events.on("input.hotkey", function(event)
    if event.action ~= "spawn_dog" then
        return
    end
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
