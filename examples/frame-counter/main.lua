local ship = require("ship")

-- O entrypoint roda num host real ou no mock sem distinção. Storage é uma
-- capability opcional (core.storage): quando o host não a oferece, o contador
-- continua funcionando apenas em memória, sem persistir entre sessões.

local hasStorage = ship.capabilities.has("core.storage")
local memoryCount = 0
local memoryTicks = 0

local function loadNumber(key)
    if hasStorage then
        return tonumber(ship.storage.get(key)) or 0
    end
    return nil
end

ship.events.on("game.ready", function()
    local persistence = hasStorage and "persistente" or "somente em memória"
    ship.log.info("contador iniciado em " .. ship.game.id() .. " (" .. persistence .. ")")
end)

if ship.capabilities.has("core.timers") then
    ship.timer.every(10, function()
        if hasStorage then
            ship.storage.set("ticks", (loadNumber("ticks") or 0) + 1)
        else
            memoryTicks = memoryTicks + 1
        end
    end)
end

ship.hotkeys.register("count", { default = "C", label = "Incrementar contador" }, function()
    local count
    if hasStorage then
        count = (loadNumber("count") or 0) + 1
        ship.storage.set("count", count)
    else
        memoryCount = memoryCount + 1
        count = memoryCount
    end
    ship.log.info("count=" .. count)
end)
