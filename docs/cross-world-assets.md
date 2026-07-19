# Cross-World Assets — assets do MM dentro do OOT e vice-versa

> Desde o SDK 0.4 os hosts do Link-Span montam, em tempo de execução, o archive
> do jogo vizinho: o OOT lê `../MM/mm.o2r` e o MM lê `../OOT/oot.o2r`. Nenhum
> asset é copiado ou extraído — os mods referenciam os caminhos do outro jogo e
> o host resolve direto do archive dele.

## Layout de instalação

O mecanismo depende do layout de pastas irmãs criado pelo launcher:

```text
Link-Span/
├── link-span.exe
├── hosts/oot/
│   ├── soh.exe
│   ├── oot.o2r        ← gerado da SUA ROM no primeiro boot
│   └── mods/
└── hosts/mm/
    ├── 2ship.exe
    ├── mm.o2r         ← gerado da SUA ROM no primeiro boot
    └── mods/
```

Cada host procura o vizinho em `../MM` / `../OOT` relativo ao próprio
executável. Para layouts diferentes, use as variáveis de ambiente
`SHIPLUA_MM_ROOT` (no OOT) e `SHIPLUA_OOT_ROOT` (no MM).

No boot, o log confirma a montagem:

```text
ShipLua expôs 50496 assets do MM sob 'mm/' (18585 com alias direto, ...)
ShipLua montou o mm.o2r do MM em modo cross-world: ...\MM\mm.o2r
```

Se o vizinho não existir, o jogo continua normalmente — só os assets
cross-world ficam indisponíveis.

## Namespaces `mm/` e `oot/`

Todo asset do archive vizinho fica endereçável com um prefixo de namespace:

| Você está em | Prefixo | Exemplo |
|---|---|---|
| OOT | `mm/` | `mm/objects/gameplay_keep/gElegyShellHumanDL` |
| MM | `oot/` | `oot/objects/object_rl/object_rl_Skel_007B38` |

O prefixo evita colisões: os dois jogos têm milhares de caminhos com o mesmo
nome (`objects/gameplay_keep/...` existe em ambos, com conteúdos diferentes).
Cerca de 2.500 dos ~50.000 caminhos do MM colidem com caminhos do OOT — quase
todos de áudio.

## Alias de hash (por que ele existe)

Resources compostos referenciam seus filhos por **hash do caminho original**,
gravado dentro do binário: uma display list aponta para as próprias texturas
por hash de `objects/.../fooTex`, sem prefixo. Não dá para reescrever esses
hashes — então o host cria um **alias no hash original** para toda entrada de
`objects/` e `textures/` do vizinho que **não colide** com um caminho local.
Assim as referências internas (DL→textura, esqueleto→DL) resolvem sozinhas.

Entradas fora de `objects/`/`textures/` não ganham alias de propósito:
sistemas como o carregador de áudio **enumeram** o índice global, e dados do
outro jogo nessa enumeração derrubam o host (formatos de áudio diferem).
Continuam acessíveis pelo namespace, que os enumeradores não varrem.

## Sanitizador de dialeto de display lists

As DLs serializadas pelos dois ports não são 100% idênticas em dialeto:

- `G_DL_INDEX` (0x3D) salta para uma tabela de setup-DLs por convenção
  interna do 2ship — inexistente no contexto do outro jogo. É neutralizado
  (vira no-op); os comandos de material inline da própria DL permanecem e o
  host aplica o setup local antes do draw.
- Os opcodes de shader/interpolação a partir de 0x43 têm numeração divergente
  entre os dois LUS e são remapeados ou neutralizados no load.

Isso acontece de forma transparente quando a DL é carregada pelo archive
cross-world — o mod não precisa fazer nada.

## Walkthrough 1 — a estátua da Elegia no OOT

O host OOT registra o ator lógico `oot.mm_elegy_statue`: um ator leve cujo
draw desenha `mm/objects/gameplay_keep/gElegyShellHumanDL` — a forma humana
da estátua da Elegia do Vazio, lida ao vivo do `mm.o2r`. O mod inteiro:

```lua
local ship = require("ship")
local statue = nil

ship.hotkeys.register("elegy_statue", { default = "K", label = "Elegy Statue (MM)" }, function()
    if statue and ship.actor.exists(statue) then
        ship.actor.destroy(statue)
        statue = nil
        return
    end
    local actor, err = ship.actor.spawn("oot.mm_elegy_statue", {
        position = { x = 0, y = 0, z = 0 }, -- (0,0,0) = na frente do player
        rotation = { x = 0, y = 0, z = 0 },
    })
    if not actor then
        ship.log.warn("falhou [" .. err.code .. "]: " .. err.message)
        return
    end
    statue = actor
end)
```

## Walkthrough 2 — Rauru no MM

O reverso, com um upgrade: Rauru é um NPC **esquelético**. O host MM registra
`mm.oot_rauru`, que inicializa um SkelAnime com o esqueleto
(`oot/objects/object_rl/object_rl_Skel_007B38`) e a animação de espera
(`object_rl_Anim_000A3C`) do OOT — o alias de hash faz as display lists dos
membros e as texturas resolverem sozinhas. O mod Lua é idêntico ao da estátua,
trocando a chave por `"mm.oot_rauru"`.

## Limitações honestas

- **Assets esqueléticos** dependem da compatibilidade do formato de resource
  entre os dois ports (hoje compatível; pode divergir em versões futuras).
- **Setup-DLs do MM** viram no-op no OOT — diferenças sutis de material
  (modo de blend/fog) podem aparecer em relação ao visual original.
- **Áudio, cenas e sequências** do vizinho não têm alias e não são
  executáveis no outro engine — apenas dados de render cruzam de verdade.
- Os hosts **não** carregam o jogo vizinho inteiro: apenas leem o archive.

---

*(English version: [cross-world-assets.en.md](cross-world-assets.en.md))*
