local ship = require("ship")

-- Item NOVO, não um replacer: o modelo vive em fox_mask.o2r (pasta mods) sob
-- o namespace próprio "objects/fox_mask", e o host o monta como
-- "mod/fox_mask/...". Nada do jogo é substituído — a Máscara da Verdade
-- continua sendo a Máscara da Verdade.
--
-- É o caminho completo das três primitivas:
--   assets de mod -> attach_model -> efeito via primitiva de gameplay

local MODEL = "mod/fox_mask/objects/fox_mask/fox_mask"
local SPEED = 1.25

local wearing = false

ship.events.on("game.ready", function()
    if not ship.capabilities.has("oot.player.attach_model") then
        ship.log.warn("host sem oot.player.attach_model")
        return
    end

    ship.hotkeys.register("fox_mask", { default = "M", label = "Fox Mask" }, function()
        if wearing then
            ship.oot.player.attach_model("head", nil)
            if ship.capabilities.has("player.speed") then
                ship.player.set_speed_multiplier(1.0)
            end
            wearing = false
            ship.log.info("Fox Mask guardada")
            return
        end

        if not ship.oot.player.attach_model("head", MODEL) then
            ship.log.warn("Fox Mask indisponível — o fox_mask.o2r está na pasta mods?")
            return
        end
        if ship.capabilities.has("player.speed") then
            ship.player.set_speed_multiplier(SPEED)
        end
        wearing = true
        ship.log.info("Fox Mask vestida — item novo, modelo próprio, nada substituído")
    end)

    ship.log.info("Fox Mask pronta — aperte M")
end)
