local ship = require("ship")

-- Cross-world de verdade no seu próprio Link: a display list do punho com a
-- Kokiri Sword vem do object_link_child do OoT, lido ao vivo do oot.o2r
-- vizinho. Os nomes de asset dos dois jogos não colidem (gLinkChild* vs
-- gLinkHuman*), então as texturas da espada resolvem sozinhas.

local using_oot = false

ship.events.on("game.ready", function()
    if not ship.capabilities.has("mm.player.sword_skin") then
        ship.log.warn("mm.player.sword_skin indisponível neste host")
        return
    end

    ship.hotkeys.register("oot_kokiri_sword", { default = "T", label = "Espada Kokiri: OoT/MM" }, function()
        local wanted = using_oot and "mm" or "oot"
        if not ship.mm.player.set_sword_skin(wanted) then
            ship.log.warn("não deu para trocar a espada — o oot.o2r do OoT está montado?")
            return
        end
        using_oot = not using_oot
        ship.log.info("espada Kokiri agora usa o visual " .. (using_oot and "do OoT" or "do MM"))
    end)

    ship.log.info("Espada Kokiri pronta — aperte T para alternar OoT/MM (empunhe a Kokiri para ver)")
end)
