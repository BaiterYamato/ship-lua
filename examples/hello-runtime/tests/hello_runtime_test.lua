-- Testes do hello-runtime rodando no mock runtime (MODSDK-004), sem jogo/ROM.
-- O runner executa o entrypoint e emite game.ready antes deste arquivo.
local ship = require("ship")

describe("hello-runtime", function()
    it("registra o carregamento no game.ready", function()
        local encontrado = false
        for _, entry in ipairs(mock.log.entries()) do
            if string.find(entry.message, "hello-runtime carregado", 1, true) ~= nil then
                encontrado = true
                break
            end
        end
        assert.is_true(encontrado, "esperava log de carregamento do mod")
    end)

    it("detecta as capabilities centrais do mock", function()
        assert.is_true(ship.capabilities.has("core.events"))
        assert.is_true(ship.capabilities.has("core.input"))
    end)

    it("responde à hotkey H com um log", function()
        mock.reset()
        assert.equals(1, mock.input.press("H"), "uma hotkey deve disparar")
        local encontrado = false
        for _, entry in ipairs(mock.log.entries()) do
            if string.find(entry.message, "Hello from", 1, true) ~= nil then
                encontrado = true
                break
            end
        end
        assert.is_true(encontrado, "esperava log da hotkey H")
    end)
end)
