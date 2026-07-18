# MODSDK-006

- Status: done
- Agent: Codex / Windows
- Branch: `agent/MODSDK-006-dev-cli-integration`
- Started: 2026-07-18
- Depends on: MODSDK-003, MODSDK-004
- Goal: integrar o CLI `shipmod` do release inicial com `new`, `validate`,
  `test` e `doctor`, além do exemplo testável `hello-runtime`.
- Guardrails:
  - não depender de jogo, ROM ou archive extraído;
  - delegar validação e testes às ferramentas canônicas quando disponíveis;
  - manter o fallback Python explicitamente identificado como básico.
