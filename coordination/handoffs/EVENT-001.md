# Handoff — EVENT-001 / EVENT-002 / EVENT-003

## Estado

review

## Resultado

- Dispatcher C++20 independente de Lua e dos hosts.
- Payload recursivo sem ponteiros ou layouts internos dos jogos.
- Ordem determinística por carga, prioridade do mod, prioridade do callback,
  ID do mod e ID de registro.
- Inscrições durante dispatch são diferidas; remoções passam a valer na
  iteração atual sem invalidá-la.
- Modos `observe`, `filter`, `transform` e `consume` implementados.
- Exceções e fluxos incompatíveis são isolados e identificados por mod.
- Operações mutáveis são restritas à thread proprietária.

## Commits

- `8f11491` — `feat(events): add deterministic dispatcher`

## Arquivos alterados

- `include/shiplua/events/EventDispatcher.h`
- `src/events/EventDispatcher.cpp`
- `tests/unit/EventDispatcherTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/event-dispatcher.md`
- `coordination/claims/EVENT-001.md`
- `coordination/handoffs/EVENT-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
rtk cmake --build build-verify
rtk ctest --test-dir build-verify --output-on-failure
rtk git diff --check
```

## Resultado da validação

- Windows 11, MinGW g++ 15.2, Ninja, C++20.
- Build completo concluído.
- 13/13 testes CTest passaram.
- Nenhum ativo protegido ou dependência de estruturas dos hosts foi encontrado.

## Testes não executados

- Linux e macOS.
- Integração em Shipwright e 2Ship, prevista para a Fase 4.
- Callback Lua real, que depende dos bindings e do bootstrap dos hosts.

## Pendências

- Converter callbacks Lua protegidos para o contrato `EventCallback`.
- Conectar a ordem resolvida de `DependencyResolver` às opções de inscrição.
- Implementar `TIMER-001` sobre eventos por frame.

## Riscos

- O limite configurável de execução por callback continua pertencendo ao
  runtime Lua e ainda não está conectado ao dispatcher.
- Prioridades menores executam primeiro; bindings e documentação gerada devem
  preservar essa convenção.

## Próxima ação recomendada

Implementar `TIMER-001` sobre o dispatcher antes de iniciar bindings ou
adaptadores dos hosts.
