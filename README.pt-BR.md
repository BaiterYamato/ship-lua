<p align="center">
  <img src="docs/assets/banner.png" alt="Link-Span">
</p>

# Link-Span

[English](README.md) · **Português**

**Um modloader Lua compartilhado para [Shipwright](https://github.com/HarbourMasters/Shipwright) (Ocarina of Time) e [2Ship2Harkinian](https://github.com/HarbourMasters/2ship2harkinian) (Majora's Mask).**

Escreva um mod em Lua **uma vez** e rode nos dois jogos. Recursos exclusivos de cada jogo ficam atrás de capacidades e namespaces (`ship.mm.*`, `ship.oot.*`), então nada é simulado artificialmente.

```lua
local ship = require("ship")

ship.events.on("game.ready", function()
    ship.log.info("Olá de " .. ship.game.id() .. " host=" .. ship.game.host_version())
end)
```

Esse mesmo arquivo imprime `Olá de mm ...` no 2Ship e `Olá de oot ...` no Shipwright.

---

## Estado

| Parte | Estado |
|---|---|
| Runtime Lua 5.4 isolado por mod + sandbox | ✅ |
| Manifesto, descoberta, SemVer, grafo de dependências | ✅ |
| Event dispatcher (observe/filter/transform/consume) + timers | ✅ |
| API `ship.*` + codegen (bindings C++ + LuaDoc) | ✅ |
| Root loader (`.shipmod`, compatibilidade, ordem de deps, isolamento de falhas) | ✅ |
| CI Linux + Windows verde | ✅ |
| Adaptador **2Ship (MM)** carregando mods in-game | ✅ (`hello-world`, `dog-spawner`) |
| Adaptador **Shipwright (OoT)** | em progresso |

Detalhes de integração em [`coordination/INTEGRATION.md`](coordination/INTEGRATION.md).

---

## Comece aqui

- **Quer escrever um mod?** → **[Guia: Escrevendo mods](docs/writing-mods.md)**
- **Quer adicionar o Link-Span ao source do seu jogo e compilar?** → **[Guia: Integração no host](docs/host-integration.md)**
- **Referência da API** (gerada dos schemas) → [`generated/docs/api-reference.md`](generated/docs/api-reference.md)
- **Exemplos** → [`examples/`](examples/)
  - [`hello-world`](examples/hello-world/) — o mod mínimo (loga a identidade do host).
  - [`actor-spawn`](examples/actor-spawn/) — atores genéricos com ownership e handles seguros, testado sem ROM.
  - [`dog-spawner`](examples/dog-spawner/) — hotkey **F** que spawna um cachorro no MM (usa `ship.mm.*`).

## Início rápido (Windows)

Se você quer rodar o Link-Span dentro do jogo (não apenas desenvolver o núcleo),
clone um dos forks de host habilitados para Link-Span com os submódulos, e rode o
helper de build embutido:

```powershell
# 1. Clone o fork do host (OoT ou MM) com todos os submódulos
git clone --recurse-submodules https://github.com/BaiterYamato/Shipwright-HyliaFoundry.git
#   ou: https://github.com/BaiterYamato/2ship2harkinian.git

# 2. De dentro do clone, rode o script de build one-shot
cd Shipwright-HyliaFoundry
.\extern\link-span\build-game.ps1
```

O `build-game.ps1` vive neste repositório (no submódulo `extern/link-span` dentro
do clone do host). Ele verifica os pré-requisitos (Git, CMake ≥ 3.26, Python 3,
Visual Studio 2022 com o toolset MSVC v143), inicializa os submódulos, configura
o CMake, gera o pacote de assets `.o2r` customizado **sem ROM**, e compila o
executável do jogo. Ele detecta automaticamente se o host é OoT ou MM.

Você também pode copiar `build-game.ps1` diretamente para a raiz do clone do
Shipwright-HyliaFoundry ou do 2ship2harkinian e executá-lo de qualquer diretório.
O helper procura primeiro a própria localização e detecta `soh/` ou `mm/`. Para
conferir a detecção sem configurar nem compilar, use `-ValidateOnly`.

O primeiro build produz um `.exe` executável, mas o jogo **não fica jogável até
você fornecer uma ROM legítima de Zelda** — o passo sem ROM apenas constrói os
assets definidos pelo ship. O script imprime o comando `ExtractAssets` exato para
rodar em seguida quando você tiver uma ROM. Veja `build-game.ps1 -Help` para
todas as flags (seleção de jogo, configuração, pular etapas).

## Instalando um mod no jogo

Os mods ficam numa pasta `mods/` ao lado do executável do jogo. Cada mod é:

- uma **pasta** com `manifest.toml` + `main.lua` (mais fácil para desenvolver), ou
- um arquivo **`.shipmod`** (um ZIP com os mesmos arquivos).

```text
<pasta-do-jogo>/
└── mods/
    ├── hello-world.shipmod
    └── meu-mod/
        ├── manifest.toml
        └── main.lua
```

Abra o jogo: os mods são descobertos, validados (jogo/versão/API), carregados em ordem de dependência, e o evento `game.ready` é disparado. Falhas de um mod não derrubam os outros. Os logs saem no console e no arquivo de log do jogo (ex.: `logs/2 Ship 2 Harkinian.log`).

## Buildando o núcleo (desenvolvimento)

Requer CMake ≥ 3.20, Ninja e um compilador C++20. Lua 5.4, toml++ e miniz são baixados via FetchContent.

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

No Windows com MinGW, garanta o runtime no `PATH` (senão os testes travam num diálogo de DLL):

```powershell
$env:Path = "C:\ProgramData\mingw64\mingw64\bin;$env:Path"
```

## Arquitetura (resumo)

```text
Mod Lua
   ↓
API pública versionada (ship.*)   ← gerada de schema/*.yml
   ↓
runtime + serviços compartilhados (este repositório)
   ↓
IGameAdapter
   ├── ShipwrightAdapter (OoT)
   └── TwoShipAdapter (MM)   ← carrega mods, dispara eventos, expõe ship.mm.*
```

O núcleo **nunca** inclui headers dos jogos. Cada adaptador (dentro do fork de cada jogo) traduz estruturas internas em snapshots/handles/eventos e pode instalar bindings específicos do host (`ship.mm.*`, `ship.oot.*`).

## Contribuindo

Leia [`AGENTS.md`](AGENTS.md) (contrato de trabalho) e [`PLAN.md`](PLAN.md) (roteiro). Mudanças na API pública exigem uma RFC em [`rfcs/`](rfcs/). O estado de coordenação vive em [`coordination/`](coordination/).

## Licença

Núcleo do Link-Span sob a licença do repositório. **Nunca** versione ROMs, `.z64`/`.n64`/`.o2r` ou qualquer asset protegido.
