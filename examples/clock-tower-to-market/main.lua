local ship = require("ship")

-- Contraparte de link-home-to-clock-tower: a viagem de volta, de MM para OoT.
-- O host do MM não exige uma cena específica (o de OoT exige a casa do Link),
-- então a volta pode ser pedida de qualquer lugar do jogo.
--
-- Destinos aceitos pelo host OoT:
--   "oot.market"    -> saída sul do Mercado de Hyrule
--   "oot.kakariko"  -> portão da Vila Kakariko

local DESTINATION = "oot.market"

ship.events.on("game.ready", function()
    if ship.game.id() ~= "mm" then
        return
    end
    if not ship.capabilities.has("world.travel") then
        ship.log.warn("world.travel indisponível — abra pelo link-span.exe com os dois jogos instalados")
        return
    end

    ship.hotkeys.register(
        "clock_tower_to_market",
        { default = "Y", label = "Voltar para o Mercado (OoT)" },
        function()
            local ok, err = ship.world.travel("oot", DESTINATION)
            if not ok then
                ship.log.warn("viagem falhou [" .. err.code .. "]: " .. err.message)
            end
        end
    )
    ship.log.info("Volta pronta: aperte Y para atravessar para o Mercado de Hyrule")
end)
