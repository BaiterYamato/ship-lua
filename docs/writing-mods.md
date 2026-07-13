# Escrevendo mods para ShipLua

Este guia mostra como criar um mod em Lua que roda no Shipwright (OoT) e no 2Ship (MM).

> Referência completa e sempre atualizada da API (gerada dos schemas):
> [`generated/docs/api-reference.md`](../generated/docs/api-reference.md).

## 1. Anatomia de um mod

Um mod é uma pasta (ou um `.shipmod`, que é só essa pasta em ZIP) com no mínimo:

```text
meu-mod/
├── manifest.toml   # metadados: id, versão, compatibilidade, dependências
└── main.lua        # ponto de entrada
```

Opcionalmente: `scripts/`, `assets/`, `locale/`, `README.md`.

## 2. O manifesto (`manifest.toml`)

```toml
id = "community.meu_mod"      # obrigatório, único, estilo domínio-invertido
name = "Meu Mod"              # obrigatório
version = "0.1.0"             # obrigatório, SemVer
api = ">=0.1 <0.2"           # obrigatório, faixa da API que o mod suporta
entrypoint = "main.lua"       # obrigatório
description = "O que faz."
authors = ["Seu Nome"]

games = ["oot", "mm"]         # em quais jogos roda (vazio = ambos)

[host]                        # faixas de versão do host (opcional)
shipwright = ">=9.0"
two_ship = ">=4.0"

[capabilities]                # recursos que o mod precisa/usa (opcional)
required = ["scene.events"]   # se ausentes no host, o mod não carrega
optional = ["mm.cycle"]       # detecte em runtime com ship.capabilities.has

[dependencies]                # outros mods (opcional)
"community.core_utils" = ">=1.0 <2.0"

[load]                        # ordem de carga (opcional)
priority = 50
after = ["community.core_utils"]

[permissions]                 # opcional
storage = true
network = false
```

Só `id`, `name`, `version`, `api` e `entrypoint` são obrigatórios. O loader valida
`games`, `host` e `api` contra o jogo atual e **recusa** o mod (sem derrubar os outros)
se for incompatível.

## 3. O ponto de entrada (`main.lua`)

O `main.lua` roda uma vez quando o mod carrega. Aqui você registra callbacks de eventos.

```lua
local ship = require("ship")

ship.log.info("meu-mod carregado")

ship.events.on("game.ready", function()
    ship.log.info("Rodando em " .. ship.game.id() .. " host=" .. ship.game.host_version())
end)
```

Cada mod roda num **estado Lua isolado** com um sandbox: `io`, `os.execute`,
`debug`, `package`, `dofile`/`loadfile` **não** estão disponíveis. Um erro num mod
não afeta os outros.

## 4. A API `ship`

### Log
```lua
ship.log.debug(msg)   ship.log.info(msg)
ship.log.warn(msg)    ship.log.error(msg)
```

### Identidade e versões
```lua
ship.game.id()            -- "oot" ou "mm"
ship.game.host_version()  -- versão do jogo, ex. "4.0.2"
ship.runtime.version()    -- versão do runtime ShipLua
ship.api.version()        -- versão da API
```

### Capacidades (recursos detectáveis)
```lua
if ship.capabilities.has("mm.cycle") then
    -- só executa onde a capacidade existe
end
for _, cap in ipairs(ship.capabilities.list()) do ... end
```

### Eventos
```lua
local sub = ship.events.on("game.ready", function(payload) ... end)
ship.events.off(sub)   -- cancela a inscrição

-- com opções (prioridade menor executa primeiro):
ship.events.on("game.frame", { priority = 10 }, function(payload)
    -- payload.frame
end)
```

O callback recebe o **payload** do evento como uma tabela Lua (campos conforme o
schema de cada evento).

### Eventos comuns disponíveis

| Evento | Payload | Observação |
|---|---|---|
| `game.ready` | `game_id, host_version, runtime_version, api_version` | disparado no boot, após carregar os mods |
| `game.frame` | `frame` | por frame (quando o host publica) |
| `game.shutdown` | — | ao encerrar |
| `input.hotkey` | `action, key` | atalho de teclado configurado pelo host |
| `scene.enter` | `scene_id` | requer capability `scene.events` |
| `actor.init` / `actor.update` / `actor.destroy` | `actor` / `handle` | requer `actor.events` |
| `save.loaded` | `slot` | requer `save.events` |
| `text.open` | `text_id` | requer `text.events` |
| `audio.sequence_started` | `player_index, sequence_id` | requer `audio.sequence.events` |

Nem todo host publica todos os eventos — use `ship.capabilities.has(...)` para os
condicionais.

### Namespaces específicos de jogo

Recursos exclusivos ficam em `ship.mm.*` (Majora's Mask) e `ship.oot.*` (Ocarina).
Eles são instalados pelo adaptador do host, então **cheque se existem** antes de usar:

```lua
if ship.mm ~= nil and ship.mm.spawn_dog ~= nil then
    ship.mm.spawn_dog()   -- spawna um cachorro da Clock Town (MM)
end
```

## 5. Empacotando e instalando

Durante o desenvolvimento, copie a **pasta** do mod direto para `mods/`:

```text
<pasta-do-jogo>/mods/meu-mod/{manifest.toml, main.lua}
```

Para distribuir, empacote como `.shipmod` (ZIP com `manifest.toml` + `main.lua` na
raiz do arquivo). Exemplo com Python:

```python
import zipfile
with zipfile.ZipFile("meu-mod.shipmod", "w", zipfile.ZIP_DEFLATED) as z:
    z.write("meu-mod/manifest.toml", "manifest.toml")
    z.write("meu-mod/main.lua", "main.lua")
```

Coloque o `.shipmod` (ou a pasta) em `mods/` e abra o jogo. Confira o carregamento
no log do jogo (`logs/...log`). Se um mod for recusado, o log diz o motivo
(incompatibilidade de jogo, versão ou API).

## 6. Exemplo completo: hotkey que spawna um cachorro (MM)

Veja [`examples/dog-spawner`](../examples/dog-spawner/). O host publica `input.hotkey`
quando a tecla configurada (padrão **F**) é pressionada; o mod reage e chama
`ship.mm.spawn_dog()`:

```lua
local ship = require("ship")

ship.events.on("input.hotkey", function(event)
    if event.action == "spawn_dog" and ship.mm and ship.mm.spawn_dog then
        ship.mm.spawn_dog()
    end
end)
```

Isso mostra o padrão recomendado: **eventos comuns** (`input.hotkey`) do núcleo +
uma **função específica do jogo** (`ship.mm.*`) fornecida pelo adaptador.

## 7. Validando o manifesto (opcional)

O núcleo traz um validador CLI:

```bash
shiplua_manifest_validator caminho/para/meu-mod/manifest.toml
shiplua_manifest_validator caminho/para/meu-mod          # pasta
shiplua_manifest_validator meu-mod.shipmod               # pacote
```

## Boas práticas

- Verifique `ship.capabilities.has(...)` antes de usar recursos não universais.
- Não assuma que `ship.mm`/`ship.oot` existem — cheque com `~= nil`.
- Um mod que usa só a API comum deve rodar nos dois jogos sem alteração.
- Prefira `priority` explícita se a ordem entre callbacks importar.
