-- Testes do frame-counter rodando no mock runtime (sem jogo/ROM).
-- O runner emite game.ready antes de executar este arquivo.
local ship = require("ship")

describe("contador", function()
    it("registra início no game.ready", function()
        local encontrado = false
        for _, entry in ipairs(mock.log.entries()) do
            if string.find(entry.message, "contador iniciado", 1, true) ~= nil then
                encontrado = true
                break
            end
        end
        assert.is_true(encontrado, "esperava log de início do mod")
    end)

    it("detecta as capabilities centrais do mock", function()
        assert.is_true(ship.capabilities.has("core.events"))
        assert.is_true(ship.capabilities.has("core.timers"))
        assert.is_true(ship.capabilities.has("core.storage"))
        assert.is_true(ship.capabilities.has("core.input"))
    end)

    it("incrementa ao pressionar C", function()
        mock.reset()
        assert.equals(1, mock.input.press("C"), "uma hotkey deve disparar")
        assert.equals(1, tonumber(ship.storage.get("count")))
        mock.input.press("C")
        assert.equals(2, tonumber(ship.storage.get("count")))
    end)

    it("emite input.hotkey ao pressionar a tecla", function()
        mock.reset()
        local eventos = 0
        ship.events.on("input.hotkey", function(event)
            if event.key == "C" then
                eventos = eventos + 1
            end
        end)
        mock.input.press("C")
        assert.equals(1, eventos)
    end)

    it("conta um tick a cada 10 frames", function()
        mock.reset()
        mock.timer.advance(10)
        assert.equals(1, tonumber(ship.storage.get("ticks")))
        mock.timer.advance(9)
        assert.equals(1, tonumber(ship.storage.get("ticks")))
        mock.timer.advance(1)
        assert.equals(2, tonumber(ship.storage.get("ticks")))
    end)

    it("storage persiste durante a sessão e delete remove a chave", function()
        mock.reset()
        ship.storage.set("anotacao", "valor")
        assert.equals("valor", ship.storage.get("anotacao"))
        assert.is_true(ship.storage.delete("anotacao"))
        assert.is_nil(ship.storage.get("anotacao"))
        assert.equals("padrao", ship.storage.get("anotacao", "padrao"))
    end)
end)
