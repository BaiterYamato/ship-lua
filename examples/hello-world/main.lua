local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info(
        "Hello from " ..
        ship.game.id() ..
        " host=" ..
        ship.game.host_version()
    )
end)
