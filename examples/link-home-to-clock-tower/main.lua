local ship = require("ship")

ship.events.on("game.ready", function()
    if not ship.capabilities.has("world.travel") then
        ship.log.warn("world.travel indisponível; teleporte cross-world desativado")
        return
    end
    if ship.game.id() ~= "oot" then
        return
    end

    ship.hotkeys.register(
        "link_home_to_clock_tower",
        { default = "Y", label = "Ir para a entrada da Torre do Relógio" },
        function()
            ship.world.travel("mm", "mm.clock_tower.entrance")
        end
    )
    ship.log.info("Teleporte pronto: aperte Y na casa do Link")
end)
