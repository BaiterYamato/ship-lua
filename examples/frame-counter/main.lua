local ship = require("ship")

-- O entrypoint roda num host real ou no mock sem distinção: as mesmas
-- capabilities centrais (eventos, timers, storage, log, input) estão presentes.

ship.events.on("game.ready", function()
    ship.log.info("contador iniciado em " .. ship.game.id())
end)

if ship.capabilities.has("core.timers") then
    ship.timer.every(10, function()
        local ticks = tonumber(ship.storage.get("ticks")) or 0
        ship.storage.set("ticks", ticks + 1)
    end)
end

ship.hotkeys.register("count", { default = "C", label = "Incrementar contador" }, function()
    local count = tonumber(ship.storage.get("count")) or 0
    count = count + 1
    ship.storage.set("count", count)
    ship.log.info("count=" .. count)
end)
