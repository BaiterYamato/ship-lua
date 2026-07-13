# RFC 0005 — Handoff autenticado entre processos

- Status: primeira fatia implementável
- API alvo: interna no ciclo 0.2; sem superfície Lua
- Tarefa: WORLD-002

## Motivação

OoT e MM podem executar em processos separados. O supervisor precisa transportar
o mesmo estado portátil definido por `WORLD-001` sem aceitar arquivos parciais,
adulterados, incompatíveis ou repetidos. O arquivo não é um save completo e não
contém structs, IDs crus ou paths dos hosts.

## Modelo de ameaça

O envelope pode ser truncado, alterado, substituído por uma versão anterior ou
lido enquanto outro processo publica a próxima viagem. O diretório compartilhado
não é considerado íntegro. O segredo de autenticação é provisionado pelo
supervisor por um canal externo e nunca é persistido no envelope.

Esta fatia não protege confidencialidade: inventário e destino permanecem
legíveis. Também não protege um host ou supervisor já comprometido que possua o
segredo.

## Contrato C++ interno

`WorldHandoffCodec` recebe:

- identificador aleatório de sessão com 128 bits;
- sequência monotônica maior que zero;
- mundo de origem e destino lógico;
- `PortablePlayerState` validado;
- chave com pelo menos 256 bits.

O codec oferece serialização/leitura e publicação atômica. A leitura pode receber
a última sequência aceita; valores menores ou iguais são rejeitados como replay.
Nenhuma função é ligada ao namespace Lua `ship` neste ciclo.

## Formato v1

Inteiros usam little-endian. O envelope contém:

```text
magic[8] = "SHLWRLD\0"
format_version: u16 = 1
flags: u16 = 0
payload_size: u32
payload:
  session_id[16]
  sequence: u64
  source_world: u8
  destination_world: u8
  destination_id: string(u32 + UTF-8)
  health: u16
  health_capacity: u16
  rupees: u32
  item_count: u32
  items[]:
    id: string
    origin_world: u8
    quantity: u32
    equipped: u8
    has_visual_asset: u8
    visual_asset?: { owner_world: u8, id: string }
authentication_tag[32]
```

O tag é `HMAC-SHA-256(chave, cabeçalho || payload)`. A comparação é feita em
tempo constante. Cabeçalho e payload só são semanticamente aceitos depois da
verificação do tag.

## Limites e validação

- chave mínima: 32 bytes;
- envelope máximo: 4 MiB;
- no máximo 256 itens, conforme o contrato portátil;
- IDs de destino e item seguem os validadores do contrato de mundo;
- strings individuais são limitadas a 4096 bytes;
- bytes reservados, flags e trailing bytes diferentes do formato são rejeitados;
- versão desconhecida é rejeitada depois da autenticação;
- sequência zero e replay são rejeitados;
- estado portátil e destino são revalidados depois do parse.

## Publicação transacional

`WriteFile` serializa tudo em memória e chama `AtomicFile::Write`. Assim, leitores
observam o envelope anterior completo ou o novo envelope completo. Um tag válido
não substitui o protocolo de consumo: o supervisor deve persistir a maior sequência
aceita antes de iniciar o host de destino.

## Comportamento em OoT e MM

O formato é idêntico nos dois hosts. Cada adaptador continua responsável por
capturar e materializar itens nativos; o envelope carrega apenas os IDs canônicos,
assets lógicos e destino definidos pelo núcleo.

## Erros

- argumento inválido: chave curta, sequência zero, estado ou destino inválido;
- permissão negada: tag incorreto;
- estado inválido: replay;
- unsupported: versão ou flags desconhecidos;
- resource limit: envelope, coleção ou string acima do limite;
- host failure: leitura ou publicação no filesystem.

Mensagens de erro não incluem a chave nem o tag recebido.

## Compatibilidade e depreciação

O `format_version` evolui separadamente de `api_version`. Extensões compatíveis
podem usar uma futura versão do formato; qualquer mudança na interpretação dos
bytes exige novo número de versão. A versão 1 permanece legível por pelo menos um
ciclo minor depois da introdução de sua substituta.

## Plano de testes

- round-trip completo com itens comuns, diferidos e asset lógico;
- vetor determinístico de HMAC-SHA-256;
- chave incorreta, adulteração e truncamento;
- versão, flags, trailing bytes e limites inválidos;
- replay por sequência;
- publicação e substituição atômica em arquivo;
- matriz MinGW e MSVC, seguida de Linux/macOS na CI.
