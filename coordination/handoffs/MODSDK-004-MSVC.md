# MODSDK-004-MSVC

- Status: review
- Branch: `agent/MODSDK-004-msvc-crash-fix`
- PR alvo: https://github.com/BaiterYamato/link-span/pull/39

## Causa raiz

Os callbacks C++ chamavam `lua_error` diretamente em funções que também
possuíam `try/catch`, `std::string`, `Result`, `variant`, iteradores ou outros
objetos não triviais. No MSVC Release, o `longjmp` do Lua atravessava esses
frames e encerrava o teste com `0xc0000409` ou corrupção de heap.

## Correção

- `events`, `logging`, `timers` e `storage` agora executam o trabalho em helpers
  que retornam um ponteiro de erro estável.
- Apenas o wrapper Lua chama `Fail`/`lua_error`, depois que o helper terminou e
  seus objetos C++ foram destruídos.
- Nenhum `#pragma optimize`, handler SEH ou alteração manual de arquivo gerado
  faz parte da correção.

## Validação

- Reprodução anterior: `api_contract_tests` terminava com `0xc0000409` ao
  executar apenas `pcall(ship.storage.get)`.
- Release/MSVC 19.44 + Ninja: build completo aprovado.
- CTest Release/MSVC: 45/45 testes aprovados.
- `api_contract_tests`: aprovado para os contextos OoT e MM.
- `mock_runtime_unit_tests`: cobre `storage.get/set/delete` sem argumentos e
  confirma erro Lua protegido em vez de encerramento do host.
