# Binding Lua mínimo

`LuaApiBinding` instala o módulo interno `ship` em cada `LuaRuntime`. O binding
é criado antes do entrypoint e destruído antes do `lua_State`.

## Importação segura

O sandbox continua sem a biblioteca `package`. Um `require` restrito aceita
somente `require("ship")` e devolve a mesma tabela por identidade de módulo.
Não há busca em disco, `package.loadlib`, DLL/SO ou módulos externos.

## Superfície disponível

- `ship.game.id()` e `ship.game.host_version()` usam contexto injetado pelo host;
- `ship.runtime.version()` e `ship.api.version()` mantêm versões separadas;
- `ship.capabilities.has/list` usam uma lista ordenada e sem duplicatas;
- `ship.events.on/off` conectam callbacks ao `EventDispatcher`;
- `ship.log.debug/info/warn/error` preservam o ID proprietário do mod.

Se o bootstrap ainda não forneceu jogo ou versão do host, apenas essas funções
falham de forma protegida; o restante do módulo continua disponível.
Capabilities desconhecidas, ainda planejadas ou incompatíveis com o jogo são
recusadas durante a instalação do binding.

## Eventos e lifecycle

Payloads são convertidos recursivamente em valores Lua: nil, booleano, inteiro,
número, texto, array e objeto. Nenhum ponteiro ou layout interno é exposto.

Callbacks usam `lua_pcall` com traceback. Uma falha é devolvida ao dispatcher e
não interrompe outros mods. Após três falhas consecutivas, a inscrição é
removida. `ship.events.off` e unload liberam tanto a inscrição quanto a
referência no registry Lua.

O `ModHost` define os eventos do schema gerado, instala o binding e oferece
`DispatchEvent`. O mesmo script `hello-world` é exercitado em memória com
contextos OoT e MM, sem ROM.
