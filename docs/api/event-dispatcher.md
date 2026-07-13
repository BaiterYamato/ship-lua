# Dispatcher de eventos

O `EventDispatcher` é o contrato interno, independente de Lua e dos hosts, que
ordena e isola callbacks antes de os bindings públicos serem conectados.

## Ordem determinística

As inscrições são executadas por:

1. ordem de carregamento já resolvida pelo grafo de mods;
2. prioridade do mod;
3. prioridade do callback;
4. ID do mod;
5. ID crescente de registro.

Valores menores executam primeiro. A ordem de carregamento incorpora
dependências e as restrições `load.after` e `load.before`.

## Tipos de evento

- `observe`: observa uma cópia do payload; alterações são descartadas;
- `filter`: retorna `Block` para interromper a propagação sem alterar o payload;
- `transform`: altera o payload compartilhado e continua a propagação;
- `consume`: retorna `Consume` para encerrar a propagação sem alterar o payload.

Um fluxo incompatível com o tipo do evento é registrado como falha do callback,
mas não derruba o dispatcher.

## Segurança durante o dispatch

- exceções são convertidas em `CallbackFailure` com assinatura e ID do mod;
- callbacks seguintes continuam após uma falha;
- uma remoção passa a valer imediatamente para callbacks ainda não chamados;
- uma inscrição criada durante o dispatch só participa do próximo dispatch;
- eventos não podem ser definidos durante um dispatch;
- todas as operações mutáveis devem ocorrer na thread que criou o dispatcher.

O payload aceita `nil`, booleano, inteiro de 64 bits, número, texto, array e
objeto recursivo. Ele não contém ponteiros, referências do host nem layouts de
estruturas internas dos jogos.
