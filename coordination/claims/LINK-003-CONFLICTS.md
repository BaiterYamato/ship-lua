# LINK-003-CONFLICTS

- Status: done
- Agent: Codex / Windows
- Branch: `agent/LINK-003-conflict-fix`
- Started: 2026-07-18
- Depends on: PR #39, PR #36, LINK-003, LINK-004, LINK-005
- Goal: reconciliar a PR #32 com o SDK atual sem perder launcher, gating de
  assets, pacote ROM-free, contratos gerados ou segurança MSVC dos callbacks.
- Guardrails:
  - não versionar ROMs, saves, logs ou archives derivados do usuário;
  - preservar worktrees antigas e alterações não relacionadas;
  - publicar e integrar somente após matriz local e CI remoto verdes.
