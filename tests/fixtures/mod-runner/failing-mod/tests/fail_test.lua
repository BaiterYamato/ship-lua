describe("falhas", function()
    it("este passa", function()
        assert.equals(1, 1)
    end)

    it("este falha de propósito", function()
        assert.equals(42, 1 + 1, "demonstração de falha do runner")
    end)
end)
