local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("fixture failing carregada")
end)
