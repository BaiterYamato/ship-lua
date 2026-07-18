local ship = require("ship")

-- Marca globais e storage para provar que arquivos de teste são isolados.
global_do_arquivo_01 = true
ship.storage.set("marcador_do_arquivo_01", true)

describe("basico", function()
    it("soma simples", function()
        assert.equals(4, 2 + 2)
    end)

    it("assert chamável preserva o builtin", function()
        assert(true)
        assert(1 == 1, "mensagem")
    end)

    it("entrypoint gravou storage", function()
        assert.is_true(ship.storage.get("loaded"))
    end)

    it("hotkey registra log", function()
        assert.equals(1, mock.input.press("G"))
        local encontrado = false
        for _, entry in ipairs(mock.log.entries()) do
            if string.find(entry.message, "foi pressionado", 1, true) ~= nil then
                encontrado = true
                break
            end
        end
        assert.is_true(encontrado)
    end)

    describe("interno", function()
        it("aninha nomes", function()
            assert.not_nil(mock)
        end)
    end)
end)
