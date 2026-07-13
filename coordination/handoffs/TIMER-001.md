# Handoff — TIMER-001

## Estado

review

## Resultado

- Scheduler C++20 independente de Lua e dos hosts.
- Timers únicos e recorrentes medidos em frames.
- Ordem determinística por vencimento, carga, prioridade, mod e registro.
- Cancelamento por handle e por proprietário, inclusive durante callback.
- Timers criados durante tick só entram em ticks futuros.
- Exceções isoladas e recorrentes desativados após falhas consecutivas.
- Thread proprietária e rejeição de tick reentrante.

## Commits

- `b2ce9df` — `feat(timer): add frame scheduler`

## Arquivos alterados

- `include/shiplua/timer/FrameTimerScheduler.h`
- `src/timer/FrameTimerScheduler.cpp`
- `tests/unit/FrameTimerSchedulerTests.cpp`
- `tests/CMakeLists.txt`
- `docs/api/frame-timers.md`
- `coordination/claims/TIMER-001.md`
- `coordination/handoffs/TIMER-001.md`
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
- 14/14 testes CTest passaram.
- Nenhum ativo protegido, marcador pendente ou dependência dos hosts encontrado.

## Testes não executados

- Linux e macOS.
- Integração com o evento `game.frame` dos hosts.
- Callback Lua real e limites de instrução do runtime.
- Compilação avulsa com warnings pelo proxy `rtk g++`; o wrapper não traduziu
  caminhos Windows, enquanto o alvo CMake compilou normalmente.

## Pendências

- Binding de `ship.timer.after` e `ship.timer.every`.
- Avançar o scheduler a partir do evento comum `game.frame` no bootstrap.
- Cancelar todos os timers no unload do mod pelo ciclo de vida do host.

## Riscos

- O máximo de três falhas consecutivas é configurável no C++, mas ainda não
  possui configuração pública.
- Prioridades menores executam primeiro, como no dispatcher.

## Próxima ação recomendada

Iniciar `CODEGEN-001` para gerar contratos C++ a partir dos schemas canônicos,
sem duplicar manualmente a API pública nos futuros bindings.
