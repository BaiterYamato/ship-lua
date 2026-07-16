# RFC 0008 — Supervisor Link-Span e troca de host

- Status: accepted for implementation
- API alvo: processo local e protocolo interno; API Lua permanece gated
- Tarefa: LINK-001

## Motivação

Shipwright e 2Ship2Harkinian possuem runtimes, símbolos e loops de jogo próprios.
Ligá-los diretamente em um único processo produziria colisões de ABI e duplicaria
responsabilidades dos hosts. O processo visível ao jogador será único —
`link-span.exe` — mas ele atuará como supervisor de um host OoT ou MM por vez.

## Layout canônico

```text
link-span/
  hosts/shipwright/          # submódulo do fork baseado no upstream HarbourMasters
  hosts/2ship2harkinian/     # submódulo do fork baseado no upstream HarbourMasters
  build-linkspan.ps1
  x64/Release/
    link-span.exe
    hosts/oot/soh.exe
    hosts/oot/assets/
    hosts/mm/2ship.exe
    hosts/mm/assets/
    mods/
    state/
```

Os submódulos apontam para os forks BaiterYamato porque os upstreams puros não
contêm o bridge Link-Span. Cada fork mantém o respectivo upstream HarbourMasters
como origem arquitetural e de sincronização.

## Seleção do jogo

O supervisor procura assets somente no diretório de `link-span.exe`:

- OoT: `oot.o2r`, `oot.otr` ou ROM identificada como Ocarina of Time;
- MM: `mm.o2r` ou ROM identificada como Majora's Mask;
- um jogo disponível: inicia esse host;
- ambos disponíveis: mostra um seletor OoT/MM;
- nenhum disponível: mostra uma mensagem única com os nomes aceitos e não inicia host.

ROMs são apenas detectadas localmente pelo cabeçalho N64. Nunca são copiadas,
empacotadas, lidas além do cabeçalho necessário ou adicionadas ao Git.

## Execução do host

O filho recebe por ambiente:

```text
LINKSPAN_ROOT
LINKSPAN_WORLD
LINKSPAN_SESSION_DIR
LINKSPAN_HANDOFF_PATH
```

O diretório de trabalho do filho é o diretório do launcher. Assim, ambos usam a
mesma pasta `mods/`, os mesmos assets fornecidos pelo usuário e a mesma sessão,
sem compartilhar ponteiros ou structs nativas.

### Invariante de pacote V-LINK-1

Cada diretório de host deve conter o executável, a pasta runtime `assets/`, as
dependências dinâmicas e o extractor do respectivo build. Um pacote incompleto
deve falhar durante o build, antes que `link-span.exe` seja oferecido ao usuário.

## Troca entre mundos

O host de origem deve:

1. capturar `PortablePlayerState` pelo adaptador;
2. traduzir o destino lógico (`mm.clock_tower.entrance`, por exemplo);
3. publicar `WorldHandoff` v1 autenticado e atômico;
4. gravar `state/next-world` com `oot` ou `mm`;
5. encerrar com o código reservado `73`.

O supervisor valida o destino disponível, remove a requisição consumida e inicia
o outro host. O host de destino autentica o envelope, rejeita replay, prepara o
save nativo de forma transacional e só então confirma a entrada.

O launcher não interpreta inventário e não converte saves. Essas operações
continuam pertencendo a `WorldSession`, `WorldHandoffCodec`, `PortableItemCatalog`
e aos adaptadores dos hosts.

## Contrato do mod demonstrativo

O mod registra uma ação apenas quando `world.travel` estiver anunciada. Em OoT,
ela solicita `mm.clock_tower.entrance`. A capability só pode ser anunciada por um
host que implemente todos os cinco passos do protocolo acima. Até esse bridge ser
integrado nos dois forks, o mod deve se desativar com warning determinístico.

## Segurança e limites

- no máximo 16 trocas consecutivas por execução do supervisor;
- destino limitado a `oot`/`mm` e IDs lógicos validados;
- executáveis resolvidos apenas em caminhos filhos conhecidos;
- nenhuma linha de comando arbitrária vinda de mod ou arquivo de sessão;
- handoff autenticado, anti-replay e publicado atomicamente;
- launcher não baixa mods, hosts, ROMs ou assets.

## Compatibilidade

O protocolo de processo evolui separadamente de `api_version` e do formato do
handoff. Novos hosts podem mudar seus paths de build sem alterar o contrato do
launcher; `build-linkspan.ps1` é responsável por normalizar o pacote final.

## Plano de testes

- descoberta isolada de `oot.o2r` e `mm.o2r`;
- ambos os assets produzem estado de seleção;
- ausência de asset produz estado vazio;
- cabeçalhos `.z64`, `.v64` e `.n64` reconhecidos sem ler a ROM inteira;
- arquivo com extensão de ROM e cabeçalho desconhecido é ignorado;
- host ausente falha antes de criar processo;
- `next-world` inválido ou host indisponível não é executado;
- build MSVC Release produz `x64/Release/link-span.exe`.
- `V-LINK-3`: builds Windows MSVC e MinGW ligam o launcher GUI com o entry point Unicode `wWinMain`.
- `V-LINK-4`: o host rejeita mods descompactados e `.shipmod` com `requires_both_games = true` quando OoT ou MM não está disponível.
- `V-LINK-5`: `world.travel` só é anunciado sob o supervisor dual; o destino aceita apenas handoff autenticado da sessão corrente e da sequência imediatamente anterior.
- `V-LINK-1`: fixture de host preserva `assets/`, `ZAPD.exe`, DLLs e archives.

## Registro de falhas

- `B-LINK-1 | 2026-07-15 | o empacotador copiou apenas o executável e descartou assets/ZAPD | V-LINK-1`
- `B-LINK-2 | 2026-07-16 | o target GUI usava wWinMain sem selecionar o startup Unicode no MinGW | V-LINK-3`
- `B-LINK-3 | 2026-07-16 | requires_both_games era parseado, mas não participava da compatibilidade do host | V-LINK-4`

## Gate de conclusão do teleporte

`LINK-001` conclui o supervisor e o pacote. A prova “casa do Link -> entrada da
torre” só pode ser marcada como concluída depois de:

1. integrar as pilhas OoT e MM em `lua/main`;
2. adicionar a capability/API Lua por RFC SemVer MINOR;
3. ligar capture + handoff + exit 73 nos dois hosts;
4. executar conformance real OoT -> MM -> OoT com assets legítimos.
