local ship = require("ship")

-- Demo das três primitivas que destravam conteúdo NOVO:
--
--   1. assets de mod   -> qualquer .o2r na pasta mods vira "mod/<nome>/..."
--   2. attach_model    -> desenha uma DL arbitrária no player
--   3. player.get/set  -> lê e escreve campos do jogador por nome
--
-- Para uma máscara própria (ex.: uma Rito Mask que não existe em nenhum dos
-- dois jogos), empacote o modelo num .o2r, ponha na pasta mods e aponte:
--
--   MODEL = "mod/rito_mask/objects/rito_mask/gRitoMaskDL"
--
-- Enquanto você não tiver um modelo próprio, a demo usa um asset do MM lido
-- pelo cross-world — o que já prova o caminho inteiro.

local MODEL = "mm/objects/gameplay_keep/gElegyShellHumanDL"
local SPEED = 1.3

local wearing = false

ship.events.on("game.ready", function()
    if not ship.capabilities.has("oot.player.attach_model") then
        ship.log.warn("host sem oot.player.attach_model")
        return
    end

    ship.hotkeys.register("custom_mask", { default = "M", label = "Máscara customizada" }, function()
        if wearing then
            ship.oot.player.attach_model("head", nil)
            if ship.capabilities.has("player.speed") then
                ship.player.set_speed_multiplier(1.0)
            end
            wearing = false
            ship.log.info("máscara removida")
            return
        end

        if not ship.oot.player.attach_model("head", MODEL) then
            ship.log.warn("não deu para anexar '" .. MODEL .. "' — o asset existe?")
            return
        end
        if ship.capabilities.has("player.speed") then
            ship.player.set_speed_multiplier(SPEED)
        end
        wearing = true

        -- Passo 3 em ação: ler e escrever estado do jogador sem função dedicada.
        if ship.capabilities.has("player.fields") then
            local hp = ship.player.get("health")
            local rupees = ship.player.get("rupees")
            ship.log.info(("máscara vestida — vida %s, rupees %s"):format(tostring(hp), tostring(rupees)))
        end
    end)

    ship.log.info("Máscara customizada pronta — aperte M")
end)
