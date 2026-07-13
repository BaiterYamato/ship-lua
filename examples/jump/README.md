# Jump

Aperta **K** (rebindável) para fazer o Link pular no 2 Ship Hypes (Majora's Mask).

## Comportamento

- Impulso vertical aplicado a `actor.velocity.y`.
- Só atua com o Link no chão (`BGCHECKFLAG_GROUND`). Sem multi-jump no ar.
- Funciona em qualquer forma (humano, Deku, Zora, Goron) — usa impulso direto,
  sem animação dedicada, então pode parecer mais simples que um pulo nativo.

## Como usar

1. Copie a pasta `jump/` para `<pasta-do-2ship>/mods/`.
2. Inicie o jogo; entre em uma cena (ex.: Clock Town).
3. Aperte **K** para pular.
4. Para trocar a tecla: **Settings → ShipLua Hotkeys → Pulo** → clique e pressione a tecla desejada.

## Requer

- Host com suporte a `ship.hotkeys` (injeta `HotkeyRegistry`) e `ship.mm.jump`.
- API ShipLua `>=0.2 <0.3`.
