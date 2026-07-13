# Timers por frame

O `FrameTimerScheduler` implementa a base interna de `ship.timer.after` e
`ship.timer.every`. Ele não conhece Lua nem estruturas dos jogos; cada host
avança o scheduler uma vez ao publicar seu evento comum `game.frame`.

## Semântica

- `after(mod, frames, callback)` executa uma vez;
- `every(mod, frames, callback)` executa novamente no mesmo período;
- `frames` deve ser maior que zero;
- `frames = 1` significa o próximo tick;
- o período recorrente parte do vencimento anterior, evitando deriva;
- cancelamento por handle e cancelamento de todos os timers de um mod são
  idempotentes no ciclo de vida, mas um handle já inativo retorna
  `InvalidHandle` quando cancelado outra vez.

## Ordem

Timers vencidos no mesmo tick são ordenados por:

1. frame de vencimento;
2. ordem de carregamento do mod;
3. prioridade do mod;
4. ID do mod;
5. ID crescente do timer.

Valores menores executam primeiro. Um timer criado durante um tick nunca entra
na lista já coletada para aquele tick.

## Falhas e thread

Exceções não atravessam o scheduler. A falha registra handle, ID do mod e
mensagem; um timer único com falha é removido e um timer recorrente é desativado
após três falhas consecutivas por padrão. Uma execução bem-sucedida zera o
contador consecutivo.

O scheduler pertence à thread em que foi criado. `Tick`, agendamento e
cancelamento em outra thread retornam `InvalidState`; ticks reentrantes também
são recusados.
