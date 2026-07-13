# RFC 0005 — Catálogo portátil de itens

## Motivação

OoT e MM usam enums, slots e layouts de save diferentes para itens equivalentes.
O núcleo compartilhado não pode copiar esses valores nem convertê-los por cast.
Também precisa preservar equipamento exclusivo quando o mundo de destino não
consegue representá-lo.

## Contrato

O estado portátil usa somente IDs canônicos estáveis. O catálogo registra, para
cada ID:

- categoria;
- mundos em que o ID é nativo;
- quantidade máxima portátil;
- possibilidade de equipamento;
- necessidade de asset lógico enquanto equipado;
- tradução explícita por mundo, quando houver uma equivalência aprovada.

Exemplos iniciais:

```text
shared.bombs
oot.sword.master -> mm.sword.razor
mm.sword.gilded -> oot.sword.biggoron
mm.mask.deku    -> sem tradução em OoT
```

Uma resolução retorna o ID canônico original e o ID que o adaptador deve
materializar no destino. O ID original permanece no estado canônico para que o
round-trip não troque definitivamente o equipamento do jogador.

Mods podem registrar definições namespaced, como `community.example.tool`, sem
alterar o catálogo embutido. IDs desconhecidos não são aceitos silenciosamente.

## Capabilities e Lua

Esta RFC não adiciona função Lua nem capability pública. A futura superfície de
viagem só poderá anunciar itens que existam no catálogo efetivo da sessão.

## OoT

O adaptador OoT conterá uma tabela local entre IDs canônicos e `ItemId`,
`InventorySlot` e valores de equipamento do Shipwright. Nenhum desses valores
entra no núcleo.

## MM

O adaptador MM conterá uma tabela local equivalente para 2Ship2Harkinian. Máscaras
e outros recursos exclusivos sem tradução permanecem adiados no estado canônico.

## Erros

- ID inválido ou duplicado: `InvalidArgument`;
- quantidade ou equipamento incompatível: `InvalidArgument`;
- item sem representação no destino: `Unsupported`;
- tradução apontando para definição ausente ou incompatível: `InvalidState`.

## Compatibilidade

Adicionar definição ou tradução é compatível em `MINOR`. Alterar o significado
de um ID existente ou remover uma tradução exige o processo de depreciação e uma
versão `MAJOR` da API afetada.

## Segurança

IDs não carregam endereços, paths ou valores da ABI do host. Assets são referências
lógicas com owner explícito. O catálogo limita quantidades antes que um adaptador
toque o save do jogo.

## Testes

- itens comuns resolvem sem tradução nos dois mundos;
- espadas usam traduções explícitas nos dois sentidos;
- item exclusivo sem tradução é adiado;
- quantidades, equipamento e assets inválidos são rejeitados;
- registro namespaced por mod é permitido e duplicatas são rejeitadas.

## Depreciação

Uma tradução antiga permanece disponível por ao menos um ciclo `MINOR`. A
substituição deve ser documentada antes da remoção em versão `MAJOR`.
