# Geração do contrato C++

`tools/generate_cpp_api.py` transforma os três schemas canônicos em
`generated/include/shiplua/generated/ApiBindings.h`.

O header contém:

- tipos públicos materializáveis em C++;
- versão da API e do schema;
- códigos de erro;
- IDs e descritores de funções, argumentos e retornos;
- descritores de eventos e payloads;
- capabilities e suporte por host.

Ele é metadado de binding, não uma exposição automática dos headers dos jogos.
Nenhum ponteiro, ABI C arbitrária ou layout interno pode ser gerado.

## Atualização

Após alterar um schema validado:

```powershell
rtk python tools/generate_cpp_api.py
```

O arquivo gerado é versionado para permitir revisão. O gate reproduzível é:

```powershell
rtk python tools/generate_cpp_api.py --check
```

O CTest executa esse gate e os testes do gerador. Alterar manualmente o header
sem alterar os schemas, ou esquecer de regenerá-lo após uma mudança, falha a
validação.
