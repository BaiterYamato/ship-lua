local ship = require("ship")

ship.events.on("game.ready", function()
    if ship.hotkeys == nil then
        ship.log.warn("host sem suporte a ship.hotkeys — pulo desativado")
        return
    end
    ship.hotkeys.register("jump", { default = "K", label = "Pulo" }, function()
        if ship.mm == nil or ship.mm.jump == nil then
            ship.log.warn("ship.mm.jump indisponivel neste host")
            return
        end
        ship.mm.jump()
    end)
    ship.log.info("Jump pronto — aperte K (rebindável em Settings → ShipLua Hotkeys)")
end)
