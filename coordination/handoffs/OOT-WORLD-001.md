# Handoff — OOT-WORLD-001

## Estado

review

## Resultado

- O adaptador OoT captura vida, capacidade, rupees, arco, bombas, bombchu, hookshot, ocarina e espadas em IDs canônicos.
- A importação valida o catálogo, traduz equipamento nativo e adia itens exclusivos sem mutar o save durante a preparação.
- O commit aplica o snapshot validado e resolve `oot.kakariko` e `oot.market` para entradas nativas.
- O bootstrap cria e mantém uma instância do adaptador ao lado do runtime e das hotkeys.
- Munição portátil foi limitada a 99 para caber com segurança nos campos dos dois hosts.

## Commits

- `4d0c8c9df` — feat(oot): add portable save adapter
- `4fc821a` — docs(oot): claim portable save adapter
- `ad469cc` — fix(world): cap portable ammunition

## Arquivos alterados

- `soh/soh/OotWorldAdapter.h`
- `soh/soh/OotWorldAdapter.cpp`
- `soh/soh/ShipLuaBootstrap.h`
- `soh/soh/ShipLuaBootstrap.cpp`
- `coordination/claims/OOT-WORLD-001.md`
- `coordination/handoffs/OOT-WORLD-001.md`
- `coordination/STATUS.md`

## Validação executada

```powershell
cmake --build build-world003
ctest --test-dir build-world003 --output-on-failure
cmake --build build-world003-msvc --config Release
ctest --test-dir build-world003-msvc -C Release --output-on-failure
cmake --build build-msvc-world --config Release --target soh
```

## Resultado da validação

- Windows 11, MinGW: 26/26 testes do núcleo passaram.
- Windows 11, Visual Studio 2022 v143, Release: 26/26 testes do núcleo passaram.
- Windows 11, Visual Studio 2022 v143, Release: `soh.exe` gerado com sucesso.

## Pendências

- Executar smoke com ativos legítimos e save descartável.
- Implementar `MM-WORLD-001` com a mesma semântica transacional.
- Conectar os dois adaptadores ao supervisor/handoff entre processos.
- Validar Linux e macOS.

## Riscos

- O adaptador ainda não persiste o save em disco por conta própria; ele altera o `SaveContext` vivo e dispara a transição.
- O bridge entre processos e a resolução de assets do outro host ainda não estão implementados.
- Slots de botão para itens compartilhados ainda dependem da configuração nativa do host; somente a espada equipada atualiza o botão B.

## Próxima ação recomendada

Implementar `MM-WORLD-001`, mantendo os mesmos IDs canônicos e as traduções explícitas do catálogo.
