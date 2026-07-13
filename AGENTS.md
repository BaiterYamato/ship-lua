# AGENTS.md — ShipLua

> Contrato operacional para agentes humanos e de IA trabalhando no modloader Lua compartilhado entre Shipwright e 2Ship2Harkinian.
>
> Nome provisório do projeto: **ShipLua**.  
> Owner padrão usado neste documento: **BaiterYamato**.  
> Data-base do plano: **2026-07-12**.

## 1. Finalidade deste arquivo

Este arquivo define como qualquer agente deve trabalhar no projeto, independentemente da plataforma usada: Codex, Claude Code, Gemini CLI, Copilot, agentes locais, automações de CI ou colaboradores humanos.

Leia este arquivo integralmente antes de alterar código. Em caso de conflito:

1. segurança e integridade do repositório;
2. este `AGENTS.md`;
3. `PLAN.md`;
4. documentação específica do diretório;
5. instrução da tarefa atribuída.

Nenhum agente deve alterar a arquitetura pública da API por conta própria.

---

## 2. Objetivo do projeto

Criar um modloader Lua modular para:

- `HarbourMasters/Shipwright` — Ocarina of Time;
- `HarbourMasters/2ship2harkinian` — Majora's Mask.

O projeto deve oferecer:

- um runtime Lua compartilhado;
- uma API comum e versionada;
- adaptadores específicos para cada jogo;
- manifesto, dependências, permissões e armazenamento por mod;
- instalação de mods sem recompilar o jogo;
- isolamento de erros;
- documentação e tipos gerados;
- compatibilidade contínua com futuras atualizações dos dois ports.

### Princípio central

Um mod que utilize apenas a API comum deve executar, sem alteração, nos dois jogos.

Recursos exclusivos devem ser expostos por capacidades e namespaces:

```lua
if ship.capabilities.has("mm.cycle") then
    ship.mm.cycle.on_reset(function(event)
        -- Majora's Mask
    end)
end
```

Não simule artificialmente um recurso inexistente no outro jogo.

---

## 3. Repositórios e branches

### Repositórios

```text
BaiterYamato/ship-lua
BaiterYamato/Shipwright
BaiterYamato/2ship2harkinian
```

O repositório `ship-lua` é a fonte canônica para:

- runtime compartilhado;
- esquema da API;
- manifesto;
- geradores;
- documentação;
- testes de conformidade;
- arquivos de coordenação.

Os dois forks contêm apenas:

- integração CMake;
- bootstrap do runtime;
- adaptador do jogo;
- hooks necessários;
- testes específicos do host.

### Branches protegidas conceitualmente

| Branch | Função |
|---|---|
| `develop` | espelho de `upstream/develop`; não recebe trabalho do projeto |
| `lua/main` | integração estável do modloader em cada fork |
| `agent/<TASK-ID>-<slug>` | trabalho isolado de uma tarefa |
| `release/<version>` | estabilização de versão |
| `hotfix/<TASK-ID>-<slug>` | correção urgente após release |

No repositório `ship-lua`, use `main` como integração estável e branches `agent/...` para tarefas.

### Regras Git

- Nunca faça commits diretamente em `develop`, `lua/main` ou `main`.
- Nunca use `git push --force` em branch compartilhada.
- Não rebase uma branch depois que outro agente passou a depender dela.
- Prefira PR pequeno, revisável e com uma única finalidade.
- Não misture refatoração não relacionada com implementação funcional.
- Não atualize submódulos sem registrar o motivo.
- Não faça merge de PR com CI falhando.
- Não inclua ROM, `.z64`, `.n64`, `.v64`, `oot.o2r`, `mm.o2r`, dumps, saves pessoais ou ativos protegidos.

---

## 4. Workspace recomendado

```text
ship-lua-workspace/
├── control/                 # clone de BaiterYamato/ship-lua
├── shipwright/              # clone do fork BaiterYamato/Shipwright
├── two-ship/                # clone do fork BaiterYamato/2ship2harkinian
└── worktrees/
    ├── CORE-001/
    ├── OOT-001/
    └── MM-001/
```

Quando vários agentes trabalham na mesma máquina, cada um deve usar um `git worktree` separado.

Exemplo:

```bash
git -C control fetch origin
git -C control worktree add \
  ../worktrees/CORE-001 \
  -b agent/CORE-001-runtime-bootstrap \
  origin/main
```

Não permita que dois agentes compartilhem o mesmo diretório de trabalho.

---

## 5. Protocolo obrigatório de coordenação

Antes de editar qualquer arquivo, o agente deve:

1. ler `AGENTS.md`;
2. ler `PLAN.md`;
3. ler `coordination/STATUS.md`;
4. verificar tarefas e dependências;
5. criar ou atualizar uma reivindicação em `coordination/claims/`;
6. declarar os caminhos que pretende alterar.

### Arquivo de reivindicação

Caminho:

```text
coordination/claims/<TASK-ID>.md
```

Formato:

```markdown
# CORE-001

- Status: claimed
- Agent: codex-windows-01
- Platform: Windows 11 / Codex
- Branch: agent/CORE-001-runtime-bootstrap
- Started: 2026-07-12T22:00:00-03:00
- Depends on: none
- Files:
  - src/runtime/**
  - include/shiplua/runtime/**
  - tests/runtime/**
- Goal:
  - Inicializar e encerrar um estado Lua isolado.
```

### Estados permitidos

```text
unclaimed
claimed
blocked
review
done
cancelled
```

### Exclusividade de arquivos

Um agente não deve editar um caminho reivindicado por outro agente, exceto quando:

- o primeiro agente atualizou formalmente a reivindicação;
- o coordenador autorizou;
- o trabalho ocorre em arquivos diferentes e não conflitantes.

Mudança emergencial fora do escopo deve ser registrada na reivindicação antes do commit.

---

## 6. Formato obrigatório de handoff

Ao interromper ou concluir uma tarefa, crie:

```text
coordination/handoffs/<TASK-ID>.md
```

Modelo:

```markdown
# Handoff — CORE-001

## Estado

review

## Resultado

- Runtime inicializa.
- Bibliotecas permitidas foram registradas.
- Bibliotecas perigosas permanecem indisponíveis.

## Commits

- `abc1234` — Add isolated Lua runtime
- `def5678` — Add runtime lifecycle tests

## Arquivos alterados

- `src/runtime/LuaRuntime.cpp`
- `include/shiplua/runtime/LuaRuntime.h`
- `tests/runtime/LuaRuntimeTests.cpp`

## Validação executada

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Resultado da validação

Todos os testes passaram em Linux x86_64.

## Pendências

- Ainda falta testar Windows.
- O limite de memória está configurável, mas sem teste de stress.

## Riscos

- O callback de alocação ainda não produz métrica por mod.

## Próxima ação recomendada

Executar `CORE-002` após merge.
```

Um handoff não pode dizer apenas “feito”. Deve permitir que outro agente continue sem reconstruir todo o contexto.

---

## 7. Divisão arquitetural obrigatória

```text
Lua mod
   ↓
API pública versionada
   ↓
runtime e serviços compartilhados
   ↓
IGameAdapter
   ├── ShipwrightAdapter
   └── TwoShipAdapter
       ↓
GameInteractor + host
```

### Núcleo compartilhado

O núcleo não pode incluir headers de OoT ou MM.

Proibido no núcleo:

```cpp
#include <z64.h>
#include "variables.h"
extern PlayState* gPlayState;
extern SaveContext gSaveContext;
```

O núcleo deve depender apenas de interfaces próprias e tipos estáveis.

### Adaptadores

Os adaptadores podem acessar estruturas internas, mas devem convertê-las em:

- snapshots;
- handles com geração;
- enums públicos;
- resultados validados;
- eventos comuns.

### API pública

A API pública não deve expor:

- ponteiros;
- endereços;
- layouts de structs;
- referências C++ com vida útil não controlada;
- enums internos sem tradução;
- funções arbitrárias da ABI C;
- FFI.

---

## 8. Regras de compatibilidade

### Versões separadas

Sempre diferencie:

```text
host_version
runtime_version
api_version
mod_version
```

Atualização do host não deve alterar automaticamente a versão da API.

### SemVer

A API pública usa SemVer:

- `MAJOR`: quebra intencional de compatibilidade;
- `MINOR`: novo recurso compatível;
- `PATCH`: correção sem mudança contratual.

### Capacidades

Toda função ou evento não universal deve possuir capacidade detectável:

```lua
if ship.capabilities.has("player.health.write") then
    ship.player.set_health(48)
end
```

### Depreciação

Uma API pública:

1. é marcada como deprecated;
2. continua funcional por pelo menos um ciclo `MINOR`;
3. ganha substituição documentada;
4. só é removida em versão `MAJOR`.

---

## 9. Segurança

### Bibliotecas Lua

Por padrão, não exponha:

- `io`;
- `os.execute`;
- `package.loadlib`;
- `debug`;
- FFI;
- carregamento de DLL/SO;
- acesso irrestrito ao sistema de arquivos;
- rede;
- subprocessos.

### Filesystem

Cada mod recebe armazenamento virtual próprio.

Requisitos mínimos:

- bloqueio de `..`;
- bloqueio de caminhos absolutos;
- normalização antes de validar;
- bloqueio de symlinks;
- limite de arquivos;
- limite de bytes;
- escrita atômica;
- namespace por ID de mod.

### Execução

Cada callback deve ter:

- tratamento protegido de erro;
- identificação do mod;
- stack trace;
- limite configurável;
- contador de falhas;
- desativação segura após falhas repetidas.

Nunca permita exceção C++ atravessar a fronteira C/Lua.

---

## 10. Padrões de código

### C e C++

- Siga a formatação e convenções do repositório host.
- O núcleo compartilhado usa C++20, salvo decisão registrada em RFC.
- Use RAII para recursos do runtime.
- Evite singletons globais quando uma dependência puder ser injetada.
- Não use exceções na fronteira pública.
- Retorne `Result<T>` ou erro estruturado.
- Evite alocação por frame em caminhos frequentes.
- Não faça logging por frame em build normal.
- Todo handle deve validar geração e host.

### Lua

- API principal em namespace `ship`.
- Não crie globais implícitas.
- Use LuaDoc nos exemplos.
- Scripts de exemplo devem funcionar nos dois hosts, salvo indicação explícita.
- Não distribua bytecode `.luac` como formato canônico.
- Um mod deve possuir `manifest.toml` e entrypoint explícito.

### Nomes

```text
C++ classes: PascalCase
C++ methods: PascalCase ou padrão do host
public Lua API: snake_case
events: dotted.snake_case
capabilities: dotted.snake_case
task IDs: PREFIX-NNN
```

---

## 11. Testes obrigatórios

Cada PR funcional deve incluir pelo menos um dos seguintes:

- teste unitário;
- teste de integração;
- mod de conformidade;
- teste de regressão;
- fixture de manifesto;
- validação de documentação gerada.

### Matriz mínima

| Camada | Linux | Windows | macOS |
|---|---:|---:|---:|
| núcleo `ship-lua` | obrigatório | obrigatório | recomendado inicialmente |
| Shipwright | obrigatório | obrigatório | recomendado |
| 2Ship | obrigatório | obrigatório | recomendado |
| segurança de caminhos | obrigatório | obrigatório | obrigatório |
| mod comum de conformidade | obrigatório | obrigatório | obrigatório antes de release |

Testes locais que exigem ativos legítimos não podem ser executados na CI pública com ROM incluída. Use fixtures sintéticas e testes sem conteúdo protegido.

---

## 12. Build e validação

### Shipwright — Linux

```bash
git submodule update --init --recursive
cmake -S . -B build-cmake -GNinja -DSUPPRESS_WARNINGS=0
cmake --build build-cmake --target GenerateSohOtr
cmake --build build-cmake
```

### 2Ship — Linux

```bash
git submodule update --init --recursive
cmake -S . -B build-cmake -GNinja
cmake --build build-cmake --target Generate2ShipOtr
cmake --build build-cmake
```

### Windows

Use Visual Studio 2022, toolset v143, CMake e Python 3. Os comandos completos permanecem em:

```text
Shipwright/docs/BUILDING.md
2ship2harkinian/docs/BUILDING.md
```

O agente deve registrar no handoff:

- sistema operacional;
- compilador;
- configuração;
- comando;
- resultado;
- testes não executados.

---

## 13. Política de commits e PRs

### Commit

Formato:

```text
<type>(<scope>): <resumo>
```

Exemplos:

```text
feat(runtime): add isolated Lua state
feat(oot): bridge scene enter hook
fix(storage): reject normalized parent traversal
test(api): add common player health conformance mod
docs(manifest): document optional capabilities
```

Tipos:

```text
feat fix refactor test docs build ci chore perf security
```

### PR

Todo PR deve informar:

- tarefa;
- problema;
- solução;
- arquivos centrais;
- testes;
- plataformas;
- riscos;
- compatibilidade;
- screenshots ou logs quando aplicável;
- checklist de segurança.

PR de adaptador não deve redefinir contrato comum sem PR correspondente no núcleo.

---

## 14. Critério para alterar a API pública

É obrigatório criar uma RFC em:

```text
rfcs/NNNN-titulo.md
```

A RFC deve conter:

- motivação;
- contrato Lua;
- capacidades;
- comportamento em OoT;
- comportamento em MM;
- erros;
- compatibilidade;
- impacto de segurança;
- plano de testes;
- plano de depreciação.

Não implemente uma API pública nova antes de registrar sua semântica.

---

## 15. Proibições para agentes

Não faça:

- acesso direto a ponteiro a partir de Lua;
- LuaJIT FFI;
- API gerada automaticamente a partir de todos os headers do jogo;
- exposição irrestrita de `gSaveContext`;
- execução assíncrona de lógica do jogo fora da thread principal;
- alteração silenciosa de manifesto ou versão;
- download automático de mods sem verificação;
- atualização automática executável sem consentimento;
- telemetria;
- uso de ROM em testes versionados;
- alteração conjunta dos dois adaptadores sem testes em ambos;
- “correção rápida” que duplica o núcleo nos dois forks.

---

## 16. Definição global de pronto

Uma tarefa só está pronta quando:

- código compilado;
- testes relevantes executados;
- documentação atualizada;
- sem arquivos protegidos;
- sem caminho duplicado no outro fork;
- claim atualizado;
- handoff criado;
- PR aberto;
- riscos conhecidos declarados;
- revisão do agente integrador concluída.

A conclusão do projeto exige que o mesmo pacote de mod comum seja carregado pelos dois hosts sem alteração de código.
