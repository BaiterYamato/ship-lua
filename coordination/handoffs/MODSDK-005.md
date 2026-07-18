# Handoff — MODSDK-005

## Estado

review

## Resultado

- API pública elevada para `0.4.0` pelo RFC 0009.
- `ship.actor.spawn`, `ship.actor.destroy` e `ship.actor.exists` expostos por
  `ActorProvider` injetável, sem headers, IDs ou ponteiros de OoT/MM no núcleo.
- Handle Lua alinhado ao `HandleRegistry`: `kind`, `slot`, `generation` e
  `scene_generation`, com ownership, proteção ABA e invalidação por cena.
- Erros esperados retornam `nil, { code, message }`; o IDL/codegen registra
  `error_mode = return`, evitando `lua_error` nesses caminhos no MSVC.
- Manifesto aceita `permissions.grants` e `limits.actors` (padrão 16, faixa
  0–256). Capability e permissão são verificadas separadamente por chamada.
- `ModHost` valida capabilities obrigatórias também na carga direta e passa a
  política individual do manifesto ao binding.
- `MockActorProvider` ROM-free integrado ao `MockRuntime`, com allowlist por
  jogo, ownership, limites, cleanup de unload e invalidação por cena.
- Exemplo bilíngue/documentado `examples/actor-spawn`, executável pelo mod test
  runner em OoT e MM sem ROM.

## Validação

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
py -X utf8 -m unittest discover -s tests/schema -p "test_*.py"
git diff --check
```

- MSVC 19.44 Release: 56/56 testes passaram.
- Testes Python de schema/codegen: 30/30 passaram.
- `actor_spawn_example_oot` e `actor_spawn_example_mm`: passaram sem ROM.
- Gates de schema e todos os artefatos gerados: sincronizados.

## Decisões

- O recorte 0.4.0 não inclui mudança de transform, configuração, animação,
  comportamento, spawn relativo ao jogador ou params nativos.
- Tipos lógicos de ator pertencem à allowlist do provider de cada host.
- `exists` retorna `false, nil` para handle próprio stale; handle malformado ou
  estrangeiro retorna erro estruturado.
- A limpeza do provider ocorre antes do fechamento do estado Lua no unload.

## Pendências

- Atualizar `extern/ship-lua` no host OoT e fazer `OotActorProvider` implementar
  a interface pública `ShipLua::ActorProvider`.
- Injetar o provider em `LuaApiHostContext::actors` e executar build MSVC do
  `soh.exe` + teste de contrato do adaptador.
- Criar o provider equivalente no host MM em bloco posterior.
- Smoke real com assets legítimos continua necessário; nenhum ROM/O2R/OTR
  privado foi usado ou rastreado neste bloco.

