# RFC 0009 — API genérica mínima de atores

- Status: aceita para implementação
- API alvo: 0.4.0
- Tarefa: MODSDK-005
- Dependências: MODSDK-001, MODSDK-002, MODSDK-003, MODSDK-004 e OOT-MODSDK-001

## Motivação

As funções específicas `ship.oot.spawn_dog` e `ship.mm.spawn_dog` provam o
bridge nativo, mas não formam um SDK reutilizável. Mods precisam criar e
controlar entidades por nomes lógicos estáveis, sem conhecer IDs, ponteiros,
estruturas ou unidades internas de OoT/MM.

Este primeiro recorte define somente criação, destruição e consulta de vida.
Transformação posterior, configuração, animação, comportamento e parâmetros
nativos exigem RFCs próprias.

## Contrato Lua

```lua
local actor, err = ship.actor.spawn("oot.en_dog", {
    position = { x = 10, y = 20, z = 30 },
    rotation = { x = 0, y = 90, z = 0 }, -- graus; opcional
})

local alive, exists_err = ship.actor.exists(actor)
local destroyed, destroy_err = ship.actor.destroy(actor)
```

As três funções retornam sempre dois valores:

- sucesso: `valor, nil`;
- falha esperada: `nil, { code = "...", message = "..." }`.

`spawn(actor_type, options)` exige um tipo lógico allowlisted pelo host e
posição absoluta finita. `rotation` é opcional, em graus, e assume zero.
`destroy(handle)` aceita apenas um ator pertencente ao mod chamador.
`exists(handle)` retorna `false, nil` para handle próprio já invalidado e erro
para handle estruturalmente inválido ou pertencente a outro mod.

O handle Lua contém somente:

```lua
{ kind = "actor", slot = 12, generation = 4, scene_generation = 9 }
```

Ele é opaco, válido apenas na sessão/cena corrente e não deve ser persistido.

## Provider nativo

O núcleo expõe `ActorProvider`, injetado em `LuaApiHostContext::actors`:

```cpp
struct ActorSpawnRequest {
    std::string actor;
    double x, y, z;
    double rotationX, rotationY, rotationZ; // graus
};

class ActorProvider {
  public:
    virtual Result<Handle> Spawn(const std::string& modId,
                                 const ActorSpawnRequest&) = 0;
    virtual Result<void> Destroy(const std::string& modId,
                                 const Handle&) = 0;
    virtual Result<bool> Exists(const std::string& modId,
                                const Handle&) const = 0;
    virtual Result<std::size_t> ReleaseMod(const std::string& modId) = 0;
};
```

O provider roda no thread do jogo e traduz o tipo lógico/transform em dados
nativos. Ele é responsável por allowlist, dependências de objeto, validação de
cena, ownership nativo e limpeza. O núcleo nunca inclui headers de OoT/MM.

## Capabilities e permissões

São adicionadas três capabilities experimentais comuns:

| Capability | Permissão obrigatória |
|---|---|
| `actor.spawn` | `world.entities.create` |
| `actor.destroy` | `world.entities.destroy` |
| `actor.exists` | `world.entities.read` |

Capability indica disponibilidade; permissão indica autorização. Ambas são
verificadas em cada chamada. O manifesto declara permissões genéricas:

```toml
[permissions]
grants = ["world.entities.create", "world.entities.destroy", "world.entities.read"]

[limits]
actors = 8
```

O limite padrão é 16 atores por mod, 0 desativa spawn e o máximo aceito pelo
parser é 256. O limite do manifesto só reduz o limite anunciado pelo provider.

## Lifecycle e ownership

- todo handle pertence ao mod que o criou;
- `destroy` e `exists` validam kind, slot, geração, cena e owner;
- unload chama `ReleaseMod` antes de fechar o estado Lua;
- mudança de cena é invalidada pelo provider do host;
- um handle stale nunca controla um ator novo (proteção ABA por geração);
- falha de spawn deve desfazer qualquer reserva parcial.

## Erros

Os códigos públicos usados neste recorte são:

- `invalid_argument`: tipos, campos, números ou handle malformado;
- `unsupported`: capability/provider/tipo lógico indisponível;
- `permission_denied`: permissão ausente ou handle de outro mod;
- `invalid_handle`: handle stale, destruído ou de cena anterior;
- `invalid_state`: jogo/cena/dependência ainda não permite a operação;
- `resource_limit`: limite de atores atingido;
- `host_failure`: provider nativo falhou de modo não recuperável.

Nenhuma exceção C++ cruza a fronteira C/Lua e as falhas acima não usam
`lua_error`, evitando `longjmp` sobre objetos C++ no MSVC.

## Compatibilidade e versionamento

Adicionar namespace, funções, tipos e capabilities é recurso compatível e
eleva a API de `0.3.0` para `0.4.0` (MINOR). APIs específicas existentes
continuam disponíveis; sua depreciação só poderá ocorrer em mudança futura.

## Mock e testes

O mock runtime fornece um `MockActorProvider` sem ROM, com allowlist lógica
para OoT/MM e inspeção determinística. A validação cobre:

- schemas/codegen e versão 0.4.0;
- parse de `permissions.grants` e `limits.actors`;
- capability ausente, permissão ausente e limite;
- spawn, exists, destroy, handle stale e ownership cruzado;
- limpeza no unload e rollback de spawn;
- ausência de ponteiro/ID nativo na API e no exemplo `actor-spawn`.

