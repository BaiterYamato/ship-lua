# Handoff — ARCH-001 / ARCH-002

## Estado

review

## Resultado

- Hooks dos dois hosts inventariados em commits identificados.
- Divergências de frame, cena, save, atores, texto e áudio registradas.
- Pontos reais de bootstrap localizados em `OTRGlobals.cpp` e `BenPort.cpp`.
- RFC 0001 define `IGameAdapter`, contrato Lua inicial, capabilities, lifecycle e segurança.
- Recursos exclusivos permanecem condicionados a capabilities; nenhuma simulação entre jogos.

## Commits

- `13ddf71` — docs(architecture): define host adapters

## Arquivos alterados

- `docs/architecture/host-hook-inventory.md`
- `rfcs/0001-shiplua-architecture.md`
- `coordination/claims/ARCH-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
rg "DEFINE_HOOK(...)" <Shipwright HookTable>
rg "DEFINE_HOOK(...)" <2Ship HookTable>
ctest --test-dir build-verify --output-on-failure
git diff --check
```

## Resultado da validação

- Assinaturas centrais reencontradas nos dois HookTable observados.
- 10/10 testes CTest passaram.
- `git diff --check` sem erros.

## Evidência dos hosts

- Shipwright: `3bed8cc2f3c1fe67b9fca15e6434551fcec57d0c`.
- 2Ship2Harkinian: `b3cc366288c0a3e7583810be816d5fed3cd54ac2`.

## Pendências

- Aprovação da RFC pelo integrador.
- Implementar schemas API-001/API-002/API-003 derivados da RFC aprovada.
- Revalidar SHAs e assinaturas imediatamente antes dos patches dos hosts.

## Riscos

- Os forks podem avançar antes da integração; drift check deve tratar os SHAs como baseline observada.
- Shutdown explícito do `GameInteractor` não foi encontrado; ShipLua precisa ownership/lifecycle próprio.
- Esta branch depende do PR #5 e deve permanecer empilhada até integração.

## Próxima ação recomendada

Revisar a RFC e iniciar API-001, API-002 e API-003 no núcleo.
