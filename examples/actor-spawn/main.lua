local ship = require("ship")

local logical_actor = ship.game.id() == "oot" and "oot.en_dog" or "mm.en_dg"
local active_actor = nil

function spawn_demo_actor()
    if active_actor then
        local alive = ship.actor.exists(active_actor)
        if alive then
            ship.actor.destroy(active_actor)
        end
        active_actor = nil
    end

    local actor, err = ship.actor.spawn(logical_actor, {
        position = { x = 0, y = 0, z = 0 },
        rotation = { x = 0, y = 0, z = 0 },
    })
    if not actor then
        ship.log.warn("actor spawn failed [" .. err.code .. "]: " .. err.message)
        return nil, err
    end
    active_actor = actor
    ship.log.info("spawned " .. logical_actor .. " as safe actor handle")
    return actor, nil
end

ship.hotkeys.register("spawn_actor", {
    default = "K",
    label = "Spawn generic actor",
}, function()
    spawn_demo_actor()
end)
