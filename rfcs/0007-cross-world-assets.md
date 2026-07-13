# RFC 0007 — Assets lógicos entre OoT e MM

- Status: primeira fatia implementável
- API alvo: interna no ciclo 0.2; sem superfície Lua
- Tarefa: WORLD-004

## Motivação

O estado portátil pode referenciar um equipamento visual de outro jogo. Encontrar
um nome dentro de `oot.o2r` ou `mm.o2r` não prova que o host atual consegue
carregá-lo: versões de archive, factories e estruturas de cena podem divergir.
O núcleo precisa aceitar somente provas produzidas pelo host depois de validar e
carregar o bundle, sem receber paths locais ou tipos internos.

## Contrato

`WorldAssetCatalog` registra descritores lógicos e provas de runtime. Um descritor
contém:

- `AssetReference` com owner e ID namespaced;
- classe semântica do bundle;
- nome e versão do contrato de factory;
- hosts em que o bundle foi projetado para funcionar;
- exigência de namespace isolado.

Uma prova contém o mesmo asset e contrato, o host que realizou a inspeção e três
resultados explícitos:

1. archive validado;
2. namespace isolado;
3. bundle carregado pela factory esperada.

Somente uma prova exata e completamente positiva torna `CanResolve()` verdadeiro.
A presença do arquivo, sozinha, nunca é um campo do contrato e não resolve o
asset.

## Namespaces

- owner `oot` exige ID iniciado por `oot.`;
- owner `mm` exige ID iniciado por `mm.`;
- IDs têm no máximo 128 bytes e usam apenas minúsculas ASCII, dígitos, `.`, `_`
  e `-`;
- descritores duplicados são fatais;
- contratos de factory usam a mesma gramática de ID canônico.

O path físico permanece dentro do adaptador. Um backend pode mapear
`oot.player.sword.master` para vários display lists/textures, mas deve considerar
o bundle carregado apenas quando todas as dependências exigidas estiverem prontas.

## Contratos iniciais

| Contrato | Uso | Hosts potenciais |
|---|---|---|
| `fast.display_list.bundle` v1 | modelos/equipamento com display lists, vértices e texturas | OoT, MM |
| `soh.scene.oot` v1 | scene/room de OoT | OoT |
| `soh.scene.mm` v1 | scene/room de MM | MM |

Os dois ports registram as mesmas factories Fast para display list, vertex e
texture. Isso permite um futuro asset pack estrangeiro já namespaced; não autoriza
montar o archive completo do outro jogo, que poderia colidir paths ou conter
factories de scene incompatíveis.

## Ciclo de vida

Provas pertencem ao host e à geração atual dos archives. Reload/unmount chama
`ClearHost(host)`, invalidando todas as resoluções daquele host antes de remontar.
Uma nova prova inválida remove a resolução obsoleta do mesmo asset/host, sem
remover uma prova válida produzida pelo outro host.

## Integração com a viagem

`IWorldAdapter::CanResolveAsset()` consulta o catálogo da geração ativa. Itens
traduzidos pelo catálogo canônico usam assets nativos e não exigem o asset da
origem. Itens aceitos sem tradução e equipados continuam bloqueados pelo
`WorldSession` quando não houver prova válida.

## Erros

- `invalid_argument`: ID, owner, host, contrato ou versão inválidos;
- `invalid_state`: descritor duplicado;
- `unsupported`: host não compatível, archive/factory/bundle não validado ou
  assinatura divergente.

Não há fallback implícito, montagem automática de archive nem soft-ignore.

## Segurança e ativos protegidos

O catálogo não contém ROM, bytes de O2R, paths ou hashes de conteúdo protegido.
O usuário fornece os archives legítimos localmente. Um futuro montador estrangeiro
deve trabalhar com allowlist de bundles e namespace isolado; adicionar o game
version do outro port à allowlist global não é permitido por esta RFC.

## Compatibilidade

`contractVersion` evolui por bundle/factory e não altera `api_version`. Mudança de
layout ou dependências exige nova versão. Descritores antigos permanecem válidos
enquanto o host continuar produzindo prova para aquela versão.

## Plano de testes

- descritor/prova nativos em OoT e MM;
- bundle Fast estrangeiro aceito somente em host explicitamente compatível;
- rejeição por namespace, host, contrato, versão, archive, isolamento e carga;
- simples presença de arquivo não representável como prova válida;
- invalidação por `ClearHost` sem afetar o outro host;
- `WorldSession` continua rejeitando equipamento sem resolução.
