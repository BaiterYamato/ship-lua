# RFC 0002 — Registro comum de hotkeys

- Status: proposta implementável
- API alvo: 0.2.0
- Tarefa: HOTKEY-001

## Motivação

Mods comuns precisam declarar atalhos sem conhecer scancodes, CVars ou APIs de
janela de cada port. O núcleo deve reter o callback Lua com segurança e delegar
somente persistência, conversão de tecla e disparo ao adaptador do host.

## Contrato Lua

```lua
local ship = require("ship")

local registrado = ship.hotkeys.register(
    "mostrar_estado",
    { default = "F8", label = "Mostrar estado" },
    function()
        ship.log.info("atalho acionado")
    end
)
```

`id` identifica o atalho dentro do mod e deve corresponder a
`[a-z][a-z0-9_.-]{0,63}`. Um novo registro com o mesmo `(mod_id, id)` substitui
atomicamente o anterior. `default` e `label` são opcionais; quando presentes,
devem ser strings com até 32 e 128 bytes, respectivamente. O retorno é `true`
quando o host aceitou o registro e `false` quando rejeitou uma tecla ou binding.

Em hosts oficiais compatíveis com API 0.2, a tabela `ship.hotkeys` existe nos
dois jogos. Um host em bootstrap que não injete o serviço retorna o erro
estruturado `unsupported` ao chamar `register`.

## Capabilities

O registro é parte comum da API 0.2 e, portanto, não exige capability nos hosts
oficiais OoT e MM. A ação executada pelo callback pode exigir capabilities ou
namespaces específicos; por exemplo, uma função `ship.mm.*` continua exclusiva
de Majora's Mask e deve ser verificada separadamente.

## Comportamento em OoT

O adaptador converte nomes físicos de tecla para `KbScancode`, persiste cada
binding em CVars sob namespace do mod e despacha keydowns no thread principal a
partir do loop de frame do Shipwright.

## Comportamento em MM

O adaptador aplica a mesma semântica, nomes de tecla, namespace de CVar e ordem
de disparo usando o loop de frame do 2Ship2Harkinian.

## Erros

- `invalid_argument`: ID, opções ou callback inválidos;
- `unsupported`: host sem registry injetado;
- retorno `false`: binding conhecido, porém rejeitado pelo host;
- exceção no callback: erro protegido com mod ID e stack trace; após três falhas
  consecutivas o callback fica inativo.

## Compatibilidade

A adição eleva a API de `0.1.0` para `0.2.0` sem remover contratos existentes.
Mods `>=0.1 <0.2` continuam corretamente recusando uma API fora do range; mods
que aceitam ambos os minors devem declarar `>=0.1 <0.3`.

O evento legado `input.hotkey` permanece disponível durante o ciclo 0.2, mas
novos mods devem preferir `ship.hotkeys.register`, que permite ownership e
rebind por mod.

## Segurança

- o núcleo não recebe scancode nem header do jogo;
- IDs e tamanhos são validados antes de formar namespaces do host;
- callbacks só executam no thread proprietário do jogo;
- unload marca callbacks inativos antes de remover bindings e referências Lua;
- o registry remove todos os bindings do mod no unload;
- uma falha de remoção no host não pode reativar ou acessar um `lua_State`
  destruído.

## Plano de testes

- registrar e disparar o mesmo callback com contextos `oot` e `mm`;
- substituir um ID sem duplicar binding;
- rejeitar ID e opções inválidos;
- descarregar e provar ausência de binding/callback residual;
- isolar e desativar callback com falhas repetidas;
- compilar os adaptadores OoT e MM e executar smoke com a mesma fixture.

## Depreciação

Nenhuma API é removida nesta RFC. Uma futura remoção de `input.hotkey` exige
marcação deprecated durante pelo menos um minor e só pode ocorrer em versão
major.
