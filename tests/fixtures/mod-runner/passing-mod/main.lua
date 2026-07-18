local ship = require("ship")

ship.storage.set("loaded", true)

ship.hotkeys.register("go", { default = "G", label = "Executar" }, function()
    ship.log.info("foi pressionado")
end)
