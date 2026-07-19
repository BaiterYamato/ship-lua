# Escrevendo mods para ShipLua (API 0.4)

Este guia mostra como criar, testar, empacotar e instalar um mod Lua que roda no
Shipwright (OoT) e no 2Ship (MM) com a **API 0.4.0** do ShipLua.

> Referência completa gerada dos schemas:
> [`generated/docs/api-reference.md`](../generated/docs/api-reference.md).
> Instalação dos hosts e layout de pastas: [getting-started.md](getting-started.md).
> Assets de um jogo dentro do outro: [cross-world-assets.md](cross-world-assets.md).

## 1. Anatomia de um mod

Um mod é uma pasta (ou um `.shipmod`, que é essa pasta em ZIP) com no mínimo:

```text
meu-mod/
├── manifest.toml   # metadados: id, versão, faixa de API, capabilities, limites
└── main.lua        # ponto de entrada
```

Opcionalmente: `tests/` (testes no mock runtime), `README.md`.

## 2. Passo a passo: um mod do zero

### 2.1 Crie a pasta e o manifesto

```toml
id = "community.meu_mod"
name = "Meu Mod"
version = "0.1.0"
api = ">=0.4 <0.5"
entrypoint = "main.lua"
description = "Aperte K para spawnar um cachorro com a API genérica de atores."
authors = ["Seu Nome"]
games = ["oot", "mm"]

[capabilities]
required = ["actor.spawn", "actor.destroy", "actor.exists"]

[permissions]
grants = ["world.entities.create", "world.entities.destroy", "world.entities.read"]

[limits]
actors = 1
```

Sobre a faixa `api`:

- **`">=0.4 <0.5"`** — para mods que usam `ship.actor.*` (API genérica de atores,
  introduzida na 0.4);
- **`">=0.1 <0.5"`** — para mods básicos (log, eventos, hotkeys, capabilities),
  que rodam também em hosts antigos;
- faixas que **não cobrem a versão do host** são rejeitadas no load. Em especial,
  pacotes velhos com `api = ">=0.1 <0.2"` ou `<0.3` **não carregam** em hosts
  0.4 — reempacote com a faixa nova (o log diz o motivo da rejeição).

### 2.2 Escreva o `main.lua`

O `main.lua` roda uma vez quando o mod carrega. Este exemplo é o
[`examples/actor-spawn`](../examples/actor-spawn/) real, levemente comentado:

```lua
local ship = require("ship")

-- O mesmo mod roda nos dois jogos: escolhe o ator lógico pelo host.
local logical_actor = ship.game.id() == "oot" and "oot.en_dog" or "mm.en_dg"
local active_actor = nil

local function spawn_demo_actor()
    if active_actor then
        if ship.actor.exists(active_actor) then
            ship.actor.destroy(active_actor)
        end
        active_actor = nil
    end

    -- Posição (0,0,0) = "na frente do player" (convenção do host até existir
    -- ship.transform.relative_to_player, plan-sdk §8.4).
    local actor, err = ship.actor.spawn(logical_actor, {
        position = { x = 0, y = 0, z = 0 },
        rotation = { x = 0, y = 0, z = 0 },
    })
    if not actor then
        ship.log.warn("actor spawn failed [" .. err.code .. "]: " .. err.message)
        return
    end
    active_actor = actor
    ship.log.info("spawned " .. logical_actor)
end

ship.hotkeys.register("spawn_actor", {
    default = "K",
    label = "Spawn generic actor",
}, spawn_demo_actor)
```

Cada mod roda num **estado Lua isolado** com sandbox: `io`, `os.execute`,
`debug`, `package`, `dofile`/`loadfile` **não** estão disponíveis. Um erro num
mod não afeta os outros.

### 2.3 Valide e teste sem o jogo (opcional)

```powershell
python tools/shipmod.py validate meu-mod          # valida o manifesto
python tools/shipmod.py test meu-mod --game oot   # roda tests/*.lua no mock runtime
```

Detalhes em [shipmod-cli.md](shipmod-cli.md) e
[manifest-validator.md](manifest-validator.md).

### 2.4 Empacote como `.shipmod`

Um `.shipmod` é um ZIP com `manifest.toml` e `main.lua` **na raiz do arquivo**
(não dentro de uma subpasta). Com PowerShell:

```powershell
Compress-Archive -Path meu-mod\manifest.toml, meu-mod\main.lua `
  -DestinationPath meu-mod.shipmod
```

Ou com Python:

```python
import zipfile
with zipfile.ZipFile("meu-mod.shipmod", "w", zipfile.ZIP_DEFLATED) as z:
    z.write("meu-mod/manifest.toml", "manifest.toml")
    z.write("meu-mod/main.lua", "main.lua")
```

### 2.5 Instale e confira o log

Coloque o `.shipmod` (ou a pasta descompactada) em `mods/` ao lado do executável
do jogo e abra o jogo. No log (`logs/*.log`):

```text
ShipLua inicializando para oot 9.2.3 (...)
ShipLua carregou 5 mod(s) de '...mods'
```

Se um mod for recusado, o log diz o motivo, sem derrubar os outros:

```text
ShipLua rejeitou o mod 'community.meu_mod': api range ">=0.1 <0.2" não cobre 0.4.0
```

## 3. Referência do manifesto

Campos obrigatórios: `id`, `name`, `version`, `api`, `entrypoint`. Schema
completo: [`schema/manifest.schema.json`](../schema/manifest.schema.json).

| Campo | Tipo | Descrição |
|---|---|---|
| `id` | string | Único, estilo domínio-invertido (`community.meu_mod`) |
| `name` | string | Nome de exibição |
| `version` | string | SemVer do mod |
| `api` | string | Faixa da API ShipLua suportada (`">=0.4 <0.5"`) |
| `entrypoint` | string | Arquivo Lua de entrada (`main.lua`) |
| `description`, `authors` | string, array | Metadados opcionais |
| `games` | array | `["oot"]`, `["mm"]` ou ambos; vazio/omitido = ambos |
| `requires_both_games` | bool | Exclusivo do Link-Span: exige assets dos dois jogos |
| `[host]` | tabela | Faixas por host (`shipwright`, `two_ship`) |
| `[capabilities] required` | array | Ausente no host ⇒ o mod **não carrega** |
| `[capabilities] optional` | array | Detecte em runtime com `ship.capabilities.has` |
| `[dependencies]` | tabela | id de mod → faixa de versão |
| `[load]` | tabela | `priority` (menor carrega antes), `after`, `before` |
| `[permissions] grants` | array | Permissões pontilhadas (`world.entities.create`) |
| `[limits] actors` | int | Máximo de atores vivos do mod (padrão 16; 0 desativa) |

**Capability ≠ permissão** (plan-sdk §3.7): a capability (`actor.spawn`) diz que
o host *implementa* o recurso; a permissão (`world.entities.create` em
`grants`) diz que o mod está *autorizado* a usá-lo. Para `ship.actor.*` você
precisa dos dois — capability no host + grant no manifesto — senão a chamada
retorna `permission_denied`.

## 4. A API `ship`

Toda função abaixo existe na superfície 0.4.0
([`schema/api.yml`](../schema/api.yml)); nada aqui é planejado ou especulativo.

### Log e identidade

```lua
ship.log.debug(msg)  ship.log.info(msg)  ship.log.warn(msg)  ship.log.error(msg)

ship.game.id()            -- "oot" ou "mm"
ship.game.host_version()  -- ex.: "9.2.3" (Shipwright) ou "4.0.2" (2Ship)
ship.runtime.version()    -- versão do runtime ShipLua
ship.api.version()        -- "0.4.0"
```

### Capabilities (feature detection)

```lua
if ship.capabilities.has("core.storage") then ... end
for _, cap in ipairs(ship.capabilities.list()) do ship.log.debug(cap) end
```

### Eventos

```lua
local sub = ship.events.on("game.ready", function(payload) ... end)
ship.events.off(sub)

-- com opções (prioridade menor executa primeiro):
ship.events.on("game.frame", { priority = 10 }, function(payload)
    -- payload.frame
end)
```

| Evento | Payload | Capability |
|---|---|---|
| `game.ready` | `game_id, host_version, runtime_version, api_version` | — |
| `game.frame` | `frame` | — |
| `game.shutdown` | — | — |
| `input.hotkey` | `action, key` | — |
| `scene.enter` | `scene_id` | `scene.events` |
| `actor.init` / `actor.update` | `actor` (snapshot) | `actor.events` |
| `actor.destroy` | `handle` | `actor.events` |
| `save.loaded` | `slot` | `save.events` |
| `text.open` | `text_id` | `text.events` |
| `audio.sequence_started` | `player_index, sequence_id` | `audio.sequence.events` |

Nem todo host publica todos os eventos — cheque a capability correspondente.

### Hotkeys (rebindáveis)

```lua
ship.hotkeys.register("meu_atalho", { default = "K", label = "Meu atalho" }, function()
    ship.log.info("apertou!")
end)
```

A tecla `default` vale até o jogador remapear nas configurações do ShipLua no
host — por isso sempre informe um `label` legível. Registre condicionalmente
quando quiser rodar em hosts sem input:

```lua
if ship.capabilities.has("core.input") then
    ship.hotkeys.register(...)
end
```

### Atores genéricos (`ship.actor.*`)

Nomes **lógicos** allowlisted e **handles seguros** (nunca ponteiros nem IDs
nativos). As falhas esperadas retornam `nil, err` em vez de lançar erro:

```lua
local actor, err = ship.actor.spawn("oot.en_dog", {
    position = { x = 0, y = 0, z = 0 },   -- (0,0,0) = na frente do player
    rotation = { x = 0, y = 90, z = 0 },  -- graus; rotation é opcional
})
if not actor then
    ship.log.warn(err.code .. ": " .. err.message)
    return
end

local alive = ship.actor.exists(actor)     -- boolean
local ok    = ship.actor.destroy(actor)    -- boolean
```

Atores lógicos registrados hoje:

| Nome lógico | Host | O que é |
|---|---|---|
| `oot.en_dog` | OoT | Cachorro (En_Dog) |
| `oot.en_torch2` | OoT | Dark Link (En_Torch2) |
| `oot.link_child_puppet` | OoT | Puppet com esqueleto do Young Link; replacers `.otr` de `objects/object_link_child` (ex.: Kafei) trocam a aparência |
| `oot.mm_elegy_statue` | OoT | Estátua da Elegia do Vazio, lida do `mm.o2r` do MM ([cross-world](cross-world-assets.md)) |
| `mm.en_dg` | MM | Cachorro de Clock Town (En_Dg) |
| `mm.oot_rauru` | MM | Rauru esquelético, lido do `oot.o2r` do OoT ([cross-world](cross-world-assets.md)) |

**Spawn genérico por id** — qualquer ator do jogo, sem esperar entrar no
catálogo: use a chave `oot.id.<actorId>[.<params>]` (no OoT) ou
`mm.id.<actorId>[.<params>]` (no MM), com números em decimal ou `0x`-hex:

```lua
-- Estátua da Elegia NATIVA do MM (En_Torch2, actor 0x021), forma humana:
local statue = ship.actor.spawn("mm.id.0x021", { position = { x = 0, y = 0, z = 0 } })

-- Cucco do OoT (En_Niw, 0x0019):
local cucco = ship.actor.spawn("oot.id.0x0019", { position = { x = 0, y = 0, z = 0 } })
```

Regras do spawn genérico:

- o **engine valida o objeto da cena**: se o modelo do ator não está carregado
  na cena atual, o spawn falha com erro limpo (`invalid_state`) — nada de crash;
- `ACTOR_PLAYER` é proibido (um segundo player corrompe o jogo);
- `params` é o parâmetro nativo do ator (variante/comportamento) — consulte o
  `actor_table.h` do decomp para ids e o significado dos params;
- posição `{0,0,0}` = na frente do player, virado para ele.

Regras dos handles:

- o handle vale só na sessão/cena atual — **não** serialize nem guarde em storage;
- troca de cena invalida handles; `ship.actor.exists` responde `false` depois;
- o runtime destrói os atores do mod no unload;
- `[limits] actors` do manifesto limita quantos ficam vivos ao mesmo tempo
  (`resource_limit` ao exceder).

### Timers (capability `core.timers`)

```lua
if ship.capabilities.has("core.timers") then
    local t = ship.timer.every(10, function() ... end)  -- a cada 10 frames
    ship.timer.after(60, function() ... end)            -- uma vez, após 60 frames
    ship.timer.cancel(t)
end
```

### Storage (capability `core.storage`) e fallback em memória

`ship.storage.get/set/delete/clear` persiste chave-valor com namespace por mod.
**Atenção:** os hosts reais (Shipwright/2Ship) **ainda não expõem**
`core.storage` — só o mock runtime do `shipmod test` a oferece. Use o padrão de
fallback do [`examples/frame-counter`](../examples/frame-counter/):

```lua
local hasStorage = ship.capabilities.has("core.storage")
local memoryCount = 0

local function bump()
    if hasStorage then
        local count = (tonumber(ship.storage.get("count")) or 0) + 1
        ship.storage.set("count", count)
        return count
    end
    memoryCount = memoryCount + 1
    return memoryCount
end
```

Assim o mod persiste onde há storage e continua funcionando (sem persistir)
onde não há.

### Cross-world travel (capability `world.travel`)

```lua
ship.world.travel("mm", "mm.clock_tower.entrance")
```

Só é anunciada quando o processo roda sob o launcher Link-Span com os dois
jogos disponíveis — veja [link-span-process.md](link-span-process.md) e
[`examples/link-home-to-clock-tower`](../examples/link-home-to-clock-tower/).

### Namespaces específicos de jogo (`ship.oot.*` / `ship.mm.*`)

| Função | Capability | Host |
|---|---|---|
| `ship.oot.player.jump()` | `oot.player.jump` | OoT |
| `ship.oot.spawn_dog()` | `oot.spawn_dog` | OoT |
| `ship.mm.player.jump()` | `mm.player.jump` | MM |
| `ship.mm.spawn_dog()` | `mm.spawn_dog` | MM |

São instalados pelo adaptador do host — **cheque capability e existência** antes
de usar, como no [`examples/jump`](../examples/jump/):

```lua
local function resolveJump()
    if ship.capabilities.has("oot.player.jump") and ship.oot and ship.oot.player then
        return ship.oot.player.jump
    end
    if ship.capabilities.has("mm.player.jump") and ship.mm and ship.mm.player then
        return ship.mm.player.jump
    end
    return nil
end
```

Esse é o padrão recomendado de **feature detection**: capabilities opcionais no
manifesto + checagem em runtime + degradação com um `ship.log.warn` claro.

## 5. Erros comuns

| Sintoma | Causa | Correção |
|---|---|---|
| Log: `rejeitou o mod ... api` | Faixa `api` velha (`<0.3`) não cobre 0.4.0 | Atualize para `">=0.4 <0.5"` (ou `">=0.1 <0.5"` se o mod é básico) e reempacote |
| Mod não carrega, log cita capability | Capability em `[capabilities] required` ausente no host | Mova para `optional` e detecte em runtime, ou rode num host que a anuncie |
| `ship.actor.spawn` → `permission_denied` | Falta o grant em `[permissions] grants` | Adicione `world.entities.create/destroy/read` |
| `ship.actor.spawn` → `invalid_argument` | Nome lógico fora da allowlist do host | Use um nome da tabela acima (ou o equivalente do outro jogo) |
| `ship.actor.spawn` → `invalid_state` | Fora de jogo (menu/título) ou objeto do ator não carregável na cena | Spawne em jogo, numa cena válida |
| `ship.actor.spawn` → `resource_limit` | Estourou `[limits] actors` | Destrua atores antigos ou aumente o limite |
| `ship.storage.*` → erro `unsupported` | Host sem `core.storage` (hosts reais hoje) | Fallback em memória (ver §4) |
| Hotkey não responde | Tecla remapeada ou colisão de default com outro mod | Confira as configurações do ShipLua no host; defaults são só sugestões |

Códigos de erro estruturado (`err.code`): `unsupported`, `invalid_argument`,
`invalid_handle`, `invalid_state`, `permission_denied`, `resource_limit`,
`host_failure`, `script_failure`.

## Boas práticas

- Verifique `ship.capabilities.has(...)` antes de usar recursos não universais.
- Não assuma que `ship.mm`/`ship.oot` existem — cheque com `~= nil`.
- Um mod que usa só a API comum deve rodar nos dois jogos sem alteração
  (`games = ["oot", "mm"]` + nome lógico escolhido por `ship.game.id()`).
- Prefira `required` mínimo no manifesto e `optional` + detecção para o resto.
- Nunca guarde handles de ator entre cenas ou sessões.
