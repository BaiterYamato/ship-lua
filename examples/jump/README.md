# Pulo

Mod de exemplo que combina a infraestrutura comum de hotkeys com a capability de
pulo do jogo ativo. Funciona em **ambos os jogos suportados**.

- Em **Ocarina of Time (Ship of Harkinian)**: usa `oot.player.jump`.
- Em **Majora's Mask (2Ship2Harkinian)**: usa `mm.player.jump`.

Copie a pasta para o diretório `mods` do SoH ou do 2Ship. Durante o jogo,
pressione `K` para pular; o atalho pode ser alterado nas configurações do
ShipLua. O pulo só é aplicado quando o jogador está no chão.

Declara `games = ["oot", "mm"]` e lista `oot.player.jump` / `mm.player.jump`
como capabilities **opcionais**. Em runtime, a função de pulo é resolvida pela
capability anunciada pelo host — assim o mesmo mod carrega nos dois jogos sem
simular uma ação incompatível.
