# RFC 0004 — Sessão portátil entre OoT e MM

- Status: primeira fatia implementável
- API alvo: interna no ciclo 0.2; superfície Lua posterior
- Tarefa: WORLD-001

## Motivação

O objetivo do ShipLua inclui mods capazes de atravessar cidades de Ocarina of
Time e Majora's Mask mantendo vida, recursos, inventário e equipamento. Isso
não pode ser implementado copiando `SaveContext`: layouts, IDs, cenas e assets
divergem. Também não basta montar o O2R do outro jogo, pois o host precisa
entender factories, símbolos e semântica do recurso.

Esta RFC cria a fronteira que permite duas topologias sem bifurcar a API:

1. dois adaptadores vivos no mesmo processo, para um host combinado;
2. um adaptador por processo com envelope persistido/IPC em `WORLD-002`.

## Contrato do núcleo

`WorldSession` registra no máximo um `IWorldAdapter` por mundo. A viagem segue:

```text
capturar fonte
  → mesclar itens diferidos no estado canônico
  → preparar destino sem mutação
  → validar partição e assets
  → commit atômico no destino
  → publicar novo mundo ativo
```

Falha antes ou durante o commit mantém a fonte ativa e não publica snapshot
parcial. `AbortImport()` é idempotente; `CommitImport()` deve ser atômico dentro
do adaptador.

## Estado portátil

O núcleo transporta somente:

- vida atual e capacidade em unidades comuns do jogo;
- rupees;
- itens por ID canônico, quantidade, origem e estado equipado;
- referência lógica opcional ao asset visual e ao mundo que o possui.

Não transporta ponteiros, offsets, IDs de `ItemId`, structs de save, entrance
numérico ou paths físicos de O2R.

IDs são minúsculos, determinísticos e independentes de path, por exemplo:

```text
shared.sword
shared.bow
oot.hookshot
mm.mask.deku
oot.kakariko
mm.clock_town
```

## Tradução de equipamentos

O adaptador de destino classifica cada item exatamente uma vez:

- `accepted`: possui tradução de gameplay;
- `deferred`: não pode representá-lo naquele mundo.

Itens diferidos permanecem no snapshot canônico. Quando o jogador retorna a um
mundo que os aceita, reaparecem. Itens adquiridos no destino são mesclados no
próximo snapshot.

Um item equipado mantido sem tradução só pode ser aceito se seu asset lógico for
resolvido pelo destino. Um item traduzido explicitamente pelo catálogo usa o
asset nativo do equivalente no destino e deve aparecer em `translatedItemIds`.
Resolver asset estrangeiro significa que o adaptador montou e validou o
namespace do archive e possui factory/runtime compatível; encontrar um arquivo
com o mesmo nome não é suficiente.

## Assets OoT/MM

`CanResolveAsset()` é a prova mínima exigida pelo núcleo. `WORLD-004` deverá
implementar um catálogo por namespace, versão do archive, tipo do recurso e
factory. O núcleo nunca recebe path local nem abre `oot.o2r`/`mm.o2r`.

## Destinos e troca de host

Destinos usam IDs canônicos. A tradução para `entranceIndex`, `sceneId`, spawn,
room e requisitos de setup pertence ao adaptador. A troca real de executable ou
game state será coordenada depois que ambos os adaptadores implementarem
captura/restauração e o catálogo de assets.

## Erros

- `invalid_argument`: snapshot ou destino fora do contrato;
- `unsupported`: mundo, item equipado ou asset sem tradução;
- `invalid_state`: reentrada, thread errada ou preview inconsistente;
- `resource_limit`: inventário acima do limite;
- `host_failure`: captura ou commit do adaptador falhou.

## Segurança

- execução somente no thread proprietário;
- nenhum acesso arbitrário a filesystem ou archive pelo mod Lua;
- limites explícitos de vida, rupees, quantidade e número de itens;
- preview deve particionar todos os itens sem IDs extras ou duplicados;
- `translatedItemIds` deve ser subconjunto dos itens aceitos;
- commit falho não muda o mundo ativo;
- assets protegidos não entram no repositório nem em fixtures.

## Compatibilidade

A primeira fatia é C++ interna. A superfície Lua e a capability pública serão
definidas somente após OoT e MM implementarem a mesma semântica e a conformance
OoT → MM → OoT passar. Isso evita prometer teleporte antes de existir host capaz.

## Plano de testes

- dois adaptadores sintéticos registrados simultaneamente;
- viagem OoT → Clock Town → Kakariko;
- hookshot diferido em MM e restaurado em OoT;
- máscara MM preservada enquanto OoT está ativo;
- equipamento cross-world sem tradução rejeitado sem asset resolvível;
- equipamento traduzido aceita o equivalente nativo sem montar o asset de origem;
- falha de commit mantém fonte e destino intactos;
- IDs duplicados, destino inválido e thread errada rejeitados.

## Próximas fatias obrigatórias

1. `WORLD-002`: envelope versionado e persistência/IPC transacional;
2. `WORLD-003`: catálogo canônico e tabelas de tradução;
3. `WORLD-004`: montagem e validação de assets do outro port;
4. adaptadores OoT/MM de snapshot e restore;
5. host combinado ou supervisor que efetivamente troca o game state;
6. capability e API Lua somente após conformance real.
