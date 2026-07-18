local ship = require("ship")

describe("isolamento", function()
    it("não vê globais de outros arquivos de teste", function()
        assert.is_nil(rawget(_G, "global_do_arquivo_01"))
    end)

    it("não vê storage escrito por outros arquivos de teste", function()
        assert.is_nil(ship.storage.get("marcador_do_arquivo_01"))
    end)

    it("ainda vê o storage do entrypoint (instância nova do mod)", function()
        assert.is_true(ship.storage.get("loaded"))
    end)
end)
