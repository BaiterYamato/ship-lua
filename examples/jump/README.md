# Pulo

Exemplo exclusivo de Majora's Mask que combina a infraestrutura comum de
hotkeys com a capability `mm.player.jump`.

Copie a pasta para o diretório `mods` do 2Ship2Harkinian. Durante o jogo,
pressione `K` para pular; o atalho pode ser alterado nas configurações do
ShipLua. O pulo só é aplicado quando o jogador está no chão.

O pacote declara `games = ["mm"]` e exige `mm.player.jump`, portanto é rejeitado
de forma explícita no OoT em vez de simular uma ação incompatível.
