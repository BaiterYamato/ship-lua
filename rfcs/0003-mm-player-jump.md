# RFC 0003 — Pulo do jogador em Majora's Mask

- Status: proposta implementável
- API alvo: 0.2.0
- Tarefa: MM-JUMP-001

## Motivação

Mods de Majora's Mask precisam poder solicitar um pulo discreto sem receber
ponteiros, layouts de `Player` ou acesso genérico à memória do jogo. O recurso
não existe como contrato comum em OoT e não deve ser simulado pelo núcleo.

## Contrato Lua

```lua
if ship.capabilities.has("mm.player.jump") then
    local pulou = ship.mm.player.jump()
end
```

`ship.mm.player.jump()` retorna `true` somente quando o host aplicou o impulso.
Retorna `false` fora de gameplay, sem jogador válido ou quando o jogador não
está no chão. A função não recebe força arbitrária; o adaptador controla um
impulso seguro e estável.

## Capabilities

O contrato exige `mm.player.jump`, anunciada apenas pelo host `mm`. Mods que
dependem da ação devem declará-la em `capabilities.required` e `games = ["mm"]`.

## Comportamento em OoT

O namespace e a capability não são expostos. O mod é rejeitado antes da carga
quando declara a capability como obrigatória. Não há fallback artificial.

## Comportamento em MM

O adaptador valida `PlayState`, jogador e contato com o chão. Em sucesso, aplica
ao ator do jogador o mesmo impulso vertical `6.34375` usado pelo Moon Jump do
host. Todas as formas jogáveis usam o mesmo contrato.

## Erros

- `false`: estado do jogo incompatível com o pulo naquele instante;
- erro Lua interno: permanece contido pelo callback protegido da hotkey;
- nenhuma exceção C++ atravessa a fronteira C/Lua.

## Compatibilidade

A função é uma adição opcional ao ciclo ainda não lançado da API 0.2.0. Mods
comuns continuam independentes dela e podem executar nos dois hosts sem mudança.

## Segurança

- o núcleo não inclui headers de MM;
- Lua não recebe ponteiro, força ou acesso a flags internas;
- a mutação ocorre no thread principal, chamada pelo callback de hotkey;
- o host valida o estado antes de escrever a velocidade vertical.

## Plano de testes

- validar schema, capability, disponibilidade e artefatos gerados;
- provar que manifesto MM exige `mm.player.jump`;
- compilar o adaptador no 2Ship2Harkinian em Windows/MSVC;
- smoke em jogo: pulo no chão, recusa no ar e funcionamento nas formas jogáveis.

## Depreciação

Uma substituição futura deve manter este contrato funcional por pelo menos um
ciclo minor e só removê-lo em versão major.
