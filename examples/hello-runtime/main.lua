-- hello-runtime: o mod mínimo do release v0.1.0-alpha.1 (plan-sdk.md §29).
-- Usa apenas a API comum: logging, feature detection, eventos e hotkeys.
-- O mesmo arquivo roda nos dois hosts e no mock runtime (MODSDK-004).
local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("hello-runtime carregado em " .. ship.game.id())

    -- Feature detection: a hotkey só é registrada quando o host suporta input.
    if ship.capabilities.has("core.input") then
        ship.hotkeys.register("hello", { default = "H", label = "Hello runtime" }, function()
            ship.log.info("Hello from " .. ship.game.id() .. "!")
        end)
        ship.log.info("hello-runtime pronto — aperte H (rebindável)")
    else
        ship.log.warn("capability core.input indisponível — hotkey do hello-runtime desativada")
    end
end)
