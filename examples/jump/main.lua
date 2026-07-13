local ship = require("ship")

ship.events.on("game.ready", function()
    if not ship.capabilities.has("mm.player.jump") then
        ship.log.warn("capability mm.player.jump indisponível — mod de pulo desativado")
        return
    end

    ship.hotkeys.register("jump", { default = "K", label = "Pulo" }, function()
        if not ship.mm.player.jump() then
            ship.log.debug("pulo ignorado: o jogador precisa estar no chão")
        end
    end)
    ship.log.info("Pulo pronto — aperte K ou altere o atalho nas configurações do ShipLua")
end)
