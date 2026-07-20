local ship = require("ship")

-- Em OoT a Bunny Hood é só cosmética; em Majora's Mask ela deixa o Link mais
-- rápido. Este mod veste a máscara do próprio OoT e liga o comportamento de
-- MM (corrida mais rápida + pulo maior) enquanto ela estiver equipada.

local wearing = false

ship.events.on("game.ready", function()
    if not ship.capabilities.has("oot.player.bunny_hood") then
        ship.log.warn("oot.player.bunny_hood indisponível neste host")
        return
    end

    ship.hotkeys.register("bunny_hood", { default = "U", label = "Bunny Hood (MM speed)" }, function()
        if not ship.oot.player.set_bunny_hood(not wearing) then
            ship.log.warn("não deu para trocar a Bunny Hood — precisa estar em jogo")
            return
        end
        wearing = not wearing
        ship.log.info(wearing and "Bunny Hood equipada — corra!" or "Bunny Hood guardada")
    end)

    ship.log.info("Bunny Hood pronta — aperte U para equipar/remover")
end)
