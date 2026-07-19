local ship = require("ship")

-- Demo de compatibilidade entre jogos: a display list gElegyShellHumanDL vem
-- do mm.o2r (2S2H), transplantada para o OOT pelo pacote mm-elegy-assets.o2r
-- na pasta mods. O host resolve o asset por caminho __OTR__, igual faria no MM.

local statue = nil

local function toggle_statue()
    if statue and ship.actor.exists(statue) then
        ship.actor.destroy(statue)
        statue = nil
        ship.log.info("estátua removida")
        return
    end

    -- Posição (0,0,0) = na frente do player.
    local actor, err = ship.actor.spawn("oot.mm_elegy_statue", {
        position = { x = 0, y = 0, z = 0 },
        rotation = { x = 0, y = 0, z = 0 },
    })
    if not actor then
        ship.log.warn("estátua falhou [" .. err.code .. "]: " .. err.message)
        return
    end
    statue = actor
    ship.log.info("estátua da Elegia (MM) spawnada no OOT — compatibilidade entre jogos OK")
end

ship.events.on("game.ready", function()
    ship.log.info("Elegy Statue pronta — aperte K para spawnar/remover a estátua do MM")
end)

ship.hotkeys.register("elegy_statue", {
    default = "K",
    label = "Elegy Statue (MM)",
}, toggle_statue)
