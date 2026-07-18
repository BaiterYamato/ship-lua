# RFC 0008 — Contrato das adições do PR #31: pulo e cachorro em OoT, `mm.spawn_dog` formalizado

- Status: aceito (cobre retroativamente o PR #31)
- API alvo: 0.3.0
- Tarefa: LINK-007
- PR relacionado: BaiterYamato/link-span#31 (`agent/OOT-001-player-jump-and-dog`)

## Motivação

O PR #31 portou os mods de exemplo `jump` e `dog-spawner` para Ocarina of Time
e formalizou três adições à API pública, mas sem RFC prévio e com bump de
versão `0.2.0 → 0.2.1` (PATCH). Adição de funções e capabilities ao contrato é
mudança contratual compatível e, pela política de versionamento do projeto
(AGENTS.md §8), exige bump `MINOR`. Este RFC registra o contrato das três
adições e a decisão de versionamento, destravando o merge do PR #31.

## Mudanças de contrato

Três funções novas, todas sem argumentos, retornando `boolean` e sem códigos de
erro estruturados (`errors: []`) — a falha é sinalizada pelo retorno `false`:

| Função | Disponibilidade | Capability | Hosts |
|---|---|---|---|
| `ship.oot.player.jump` | `oot` | `oot.player.jump` (contract) | oot |
| `ship.oot.spawn_dog` | `oot` | `oot.spawn_dog` (contract) | oot |
| `ship.mm.spawn_dog` | `mm` | `mm.spawn_dog` (contract) | mm |

Além disso, o namespace Lua `ship.oot` (e `ship.oot.player`) passa a existir no
contrato, espelhando `ship.mm`.

`mm.spawn_dog` já existia como binding host-only fora do schema (invisível ao
contrato). Este RFC a formaliza: tornar pública uma binding existente é adição
de contrato, não correção.

## Contrato Lua

```lua
-- OoT
if ship.capabilities.has("oot.player.jump") then
    local pulou = ship.oot.player.jump()
end
if ship.capabilities.has("oot.spawn_dog") then
    local spawnou = ship.oot.spawn_dog()
end

-- MM
if ship.capabilities.has("mm.spawn_dog") then
    local spawnou = ship.mm.spawn_dog()
end
```

As três funções retornam `true` somente quando o host executou a ação.
Retornam `false` fora de gameplay, sem jogador válido ou em cena/estado
incompatível. Nenhuma recebe parâmetros arbitrários (força, ponteiro, cena);
o adaptador de cada host controla os valores seguros.

## Padrão de mod multi-jogo

Os exemplos `jump` e `dog-spawner` passam a declarar `games = ["oot", "mm"]` e
migram as capabilities de `required` para `optional`, resolvendo a função em
runtime pela capability anunciada:

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

Este é o padrão recomendado para mods com equivalente funcional nos dois jogos:
`optional` + branching em runtime, em vez de `required` + rejeição de carga.
`required` continua correto para mods sem equivalente no outro host.

## Comportamento em OoT

- `ship.oot.player.jump()`: o adaptador valida estado de gameplay, jogador e
  contato com o chão, e aplica um impulso vertical estável equivalente ao do
  host. Fora dessas condições retorna `false`. Mesmo contrato de
  `mm.player.jump` (RFC 0003).
- `ship.oot.spawn_dog()`: spawna o `En_Dog` (cachorro do Hyrule Market, NPC da
  side-quest da Dog Lady) perto do jogador. O ator valida cena/params no init;
  fora de `SCENE_MARKET_DAY`/`SCENE_MARKET_NIGHT` pode ficar parado ou se
  auto-remover. O adaptador retorna `false` quando a cena/estado não permite o
  spawn confiável.
- A implementação das bindings no host SoH fica no repositório
  Shipwright-HyliaFoundry, espelhando o bootstrap do MM; o núcleo não inclui
  headers de OoT.

## Comportamento em MM

- `ship.mm.spawn_dog()`: spawna o `En_Dg` de Clock Town com path válido
  (params `0x03E0`) — sem path o ator se auto-remove ao entrar em idle. O
  objeto `object_dog` precisa estar carregado na cena (Clock Town e áreas com
  cachorros) para renderização. Retorna `false` fora de gameplay ou em cena
  incompatível.
- Hosts que não anunciam as capabilities não expõem os namespaces/funções; não
  há fallback artificial nem simulação pelo núcleo.

## Erros

- `false`: estado do jogo incompatível com a ação naquele instante;
- erro Lua interno: contido pelo callback protegido (hotkey/evento);
- nenhuma exceção C++ atravessa a fronteira C/Lua;
- `errors: []` nas três funções — nenhum código estruturado novo é introduzido.

## Versionamento (decisão SemVer)

A política escrita do projeto (AGENTS.md §8) define:

- `MAJOR`: quebra intencional de compatibilidade;
- `MINOR`: novo recurso compatível;
- `PATCH`: correção sem mudança contratual.

As três funções, as três capabilities e o namespace `ship.oot` são **novos
recursos compatíveis**: nada é removido, renomeado ou tem semântica alterada, e
mods existentes continuam carregando. Portanto o bump correto é
**`0.2.0 → 0.3.0` (MINOR)**, não `0.2.1` (PATCH). Um bump PATCH sinalizaria
incorretamente "apenas correção" e esconderia dos consumidores a existência de
contrato novo detectável.

Alternativas consideradas:

1. **PATCH (`0.2.1`)** — rejeitada: viola a definição escrita de PATCH
   ("correção sem mudança contratual"); há mudança contratual aditiva.
2. **Manter `0.2.0`** (tratar como ciclo ainda aberto, como fez o RFC 0003) —
   rejeitada: `0.2.0` já é contrato vigente consumido por mods
   (`api = ">=0.2 <0.3"` nos manifestos), por testes que fixam
   `ship.api.version()` e pelos hosts que anunciam a versão. O ciclo 0.2.0
   está fechado para adições.
3. **MAJOR (`1.0.0`)** — rejeitada: não há quebra; e o marco de 1.0.0
   permanece vinculado aos critérios de release do plano mestre.

Consequência prática do MINOR: manifestos com range `api = ">=0.2 <0.3"` não
carregam num host `0.3.0`. Os mods de exemplo deste PR devem ampliar o teto
para `">=0.2 <0.4"`. Até `1.0.0`, mods devem revisar o teto do range a cada
MINOR; após `1.0.0`, o teto natural passa a ser a próxima MAJOR.

Regra processual adotada: todo PR que altera `schema/api.yml`,
`schema/capabilities.yml` ou `schema/events.yml` precisa referenciar um RFC
aceito que cubra a mudança e o bump de `api_version`. Este RFC cobre
retroativamente o PR #31.

## Compatibilidade

Adição estritamente compatível: nenhuma função, evento, capability ou tipo
existente é alterado. Mods comuns e mods MM existentes executam sem mudança.
Mods que quiserem as novas funções declaram as capabilities como `optional`
(ou `required`, se exclusivos de um host) e ampliam o range de `api`.

## Segurança

- o núcleo não inclui headers de OoT nem de MM;
- Lua não recebe ponteiro, força, cena ou acesso a flags internas;
- as mutações ocorrem no thread principal, chamadas por callbacks protegidos;
- cada host valida o estado do jogo antes de aplicar impulso ou spawnar ator.

## Plano de testes

- validar schemas (`validate_api_schemas.py`) e consistência do codegen
  (`api_cpp_codegen_check`, `api_docs_codegen_check`);
- testes gerados: `generated_api_bindings_tests`, `lua_api_binding_tests`,
  `hello_world_conformance_tests`, incluindo contagens (17 funções,
  16 capabilities), ordem de schema e a string `0.3.0`;
- provar que manifesto multi-jogo com capabilities `optional` carrega nos dois
  hosts e que a resolução por capability desativa o mod com aviso quando
  nenhuma capability existe;
- smoke em jogo (OoT): pulo no chão/recusa no ar; spawn de `En_Dog` confiável
  no Hyrule Market e recusa fora dele;
- smoke em jogo (MM): regressão de `mm.player.jump` e `mm.spawn_dog`.

## Depreciação

Nenhuma API é depreciada. Substituições futuras destas funções devem manter o
contrato funcional por pelo menos um ciclo `MINOR` e só removê-lo em versão
`MAJOR`.
