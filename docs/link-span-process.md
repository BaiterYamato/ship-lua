# Processo local do Link-Span

## Resultado esperado

Um clone do Link-Span contém o núcleo compartilhado e dois hosts pinados. O
usuário executa um único build e abre um único launcher:

```powershell
git clone --recurse-submodules https://github.com/BaiterYamato/link-span.git
cd link-span
.\build-linkspan.ps1
.\x64\Release\link-span.exe
```

O launcher não contém ROMs nem baixa conteúdo protegido.

## Seleção por arquivos

Coloque no mesmo diretório de `link-span.exe`:

| Arquivo local | Resultado |
|---|---|
| `oot.o2r` ou `oot.otr` | abre Shipwright |
| `mm.o2r` | abre 2Ship2Harkinian |
| ROM N64 legítima reconhecida pelo cabeçalho | abre o jogo correspondente |
| assets dos dois jogos | mostra seletor OoT/MM |
| nenhum asset reconhecido | mostra a orientação de assets ausentes |

Arquivos `.z64`, `.v64` e `.n64` são classificados pelo magic e título interno,
não apenas pelo nome do arquivo.

## Layout do pacote

```text
x64/Release/
  link-span.exe
  oot.o2r                 # fornecido pelo usuário
  mm.o2r                  # fornecido pelo usuário
  hosts/oot/
    soh.exe
    assets/               # recursos runtime/extractor do Shipwright
  hosts/mm/
    2ship.exe
    assets/               # recursos runtime/extractor do 2Ship
  mods/                   # compartilhada pelos dois hosts
  state/                  # handoff e requisição de troca
```

## Build rápido do supervisor

Para validar somente o launcher, sem compilar os jogos:

```powershell
.\build-linkspan.ps1 -LauncherOnly
```

Para conferir ferramentas e submódulos sem alterar builds:

```powershell
.\build-linkspan.ps1 -ValidateOnly
```

## Teleporte entre jogos

O núcleo já possui estado portátil, catálogo de itens, tradução e envelope
autenticado. O supervisor reserva o exit code `73` e consome
`state/next-world`. O bridge final dos hosts ainda precisa ser integrado para:

1. capturar o save OoT;
2. publicar o handoff para `mm.clock_tower.entrance`;
3. encerrar OoT com código 73;
4. iniciar MM;
5. autenticar e materializar o mesmo estado portátil.

Enquanto a capability `world.travel` não estiver integrada nos dois hosts, o mod
demonstrativo permanece desativado de forma segura. Isso evita perder itens ou
corromper um save nativo tratando presença de assets como compatibilidade.

O build empacota o exemplo em
`x64/Release/mods/link-home-to-clock-tower.shipmod`. Hosts antigos o rejeitam de
forma explícita por exigir API `0.3` e `world.travel`; ele não executa um
teleporte parcial.

## Mods que exigem os dois jogos

Um mod declara exigir ambos os hosts com `requires_both_games = true` no
manifesto. Esse campo é deliberadamente distinto de `games = ["oot", "mm"]`:
este último significa "roda em qualquer um dos hosts", enquanto
`requires_both_games = true` significa "só faz sentido com os dois presentes"
(por exemplo, um mod de teleporte entre mundos).

### Decisão de escaneamento (LINK-002)

O launcher verifica `requires_both_games` **apenas em mods descompactados**
(`mods/**/manifest.toml`), lendo o campo via regex. Essa varredura é
intencionalmente barata e independente do parser TOML/Lua — o launcher é um
supervisor mínimo e não deve depender do runtime do modloader.

**Pacotes `.shipmod` (ZIP) não são inspecionados pelo launcher.** A autoridade
para validar a compatibilidade de um pacote continua sendo o host: quando o
modloader do host carrega um `.shipmod`, ele parseia o manifesto pelo
`ManifestParser` (campo `requiresBothGames` em `Manifest`) e reporta o próprio
erro de compatibilidade se as dependências não forem satisfeitas. Isso mantém o
launcher livre de dependências de extração/ZIP e centraliza a validação semântica
no núcleo, onde os testes do parser já cobrem os três estados do campo (`true`,
`false` e ausente).

Quando um mod dual é detectado e apenas um asset está disponível, o launcher
exibe a mensagem e retorna o **código de saída 8** antes de iniciar qualquer host.
