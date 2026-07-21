local ship = require("ship")

-- Antes este mod dependia de um verbo dedicado no host (set_bunny_hood).
-- Agora ele é composto de duas primitivas genéricas — a mesma dupla serve
-- para qualquer máscara com qualquer efeito, sem tocar em C++:
--
--   ship.oot.player.set_mask(nome)          -- keaton, skull, spooky,
--                                              bunny_hood, goron, zora,
--                                              gerudo, truth, none
--   ship.player.set_speed_multiplier(fator) -- 0.1 a 5.0; 1.0 restaura
--
-- Se o host for antigo (alpha.3) e só tiver o verbo dedicado, caímos nele.

local MASK = "bunny_hood"
local SPEED = 1.5

local wearing = false

local function set(enabled)
    if ship.capabilities.has("oot.player.mask") and ship.capabilities.has("player.speed") then
        if not ship.oot.player.set_mask(enabled and MASK or "none") then
            return false
        end
        return ship.player.set_speed_multiplier(enabled and SPEED or 1.0)
    end
    if ship.capabilities.has("oot.player.bunny_hood") then
        return ship.oot.player.set_bunny_hood(enabled)
    end
    return false
end

ship.events.on("game.ready", function()
    local ok = ship.capabilities.has("oot.player.bunny_hood")
        or (ship.capabilities.has("oot.player.mask") and ship.capabilities.has("player.speed"))
    if not ok then
        ship.log.warn("host sem as capabilities de máscara/velocidade")
        return
    end

    ship.hotkeys.register("bunny_hood", { default = "U", label = "Bunny Hood (MM speed)" }, function()
        if not set(not wearing) then
            ship.log.warn("não deu para trocar a Bunny Hood — precisa estar em jogo")
            return
        end
        wearing = not wearing
        ship.log.info(wearing and "Bunny Hood equipada — corra!" or "Bunny Hood guardada")
    end)

    ship.log.info("Bunny Hood pronta — aperte U para equipar/remover")
end)
