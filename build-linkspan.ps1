<#
.SYNOPSIS
    Compila o launcher Link-Span e os dois hosts em um pacote Windows único.

.DESCRIPTION
    Inicializa os submódulos, compila `link-span.exe`, chama o helper validado de
    cada host e normaliza os binários em `x64/<Config>`. ROMs e assets base não
    são copiados nem baixados: coloque `oot.o2r`/`mm.o2r` ou suas ROMs legítimas
    ao lado de `link-span.exe` depois do build.

.PARAMETER Config
    Debug | Release | RelWithDebInfo | MinSizeRel. Padrão: Release.

.PARAMETER LauncherOnly
    Compila e testa apenas o supervisor, sem compilar Shipwright/2Ship.

.PARAMETER SkipSubmodules
    Não executa `git submodule update --init --recursive`.

.PARAMETER SkipAssets
    Repassa `-SkipAssets` aos builds dos hosts.

.PARAMETER ValidateOnly
    Valida layout e ferramentas sem configurar ou compilar.

.PARAMETER SkipOot / SkipMm
    Omite explicitamente o host correspondente. O pacote pode conter apenas
    um dos jogos; o launcher seleciona automaticamente o jogo disponível.
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
    [string]$Config = 'Release',
    [switch]$LauncherOnly,
    [switch]$SkipSubmodules,
    [switch]$SkipAssets,
    [switch]$SkipOot,
    [switch]$SkipMm,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'build/link-span'
$outputDir = Join-Path $root "x64/$Config"
$ootRoot = Join-Path $root 'hosts/shipwright'
$mmRoot = Join-Path $root 'hosts/2ship2harkinian'
$hostBuilder = Join-Path $root 'build-game.ps1'
$packagingModule = Join-Path $root 'tools/LinkSpanPackaging.psm1'
Import-Module -Name $packagingModule -Force

function Step([string]$message) {
    Write-Host ""
    Write-Host "==> $message" -ForegroundColor Cyan
}

function Fail([string]$message) {
    throw "Link-Span: $message"
}

function Require-Command([string]$name) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $command) { Fail "comando '$name' não encontrado no PATH" }
    return $command.Source
}

function Test-HostLayout([string]$path, [string]$gameDirectory) {
    return (Test-Path -LiteralPath (Join-Path $path 'CMakeLists.txt') -PathType Leaf) -and
           (Test-Path -LiteralPath (Join-Path $path "$gameDirectory/CMakeLists.txt") -PathType Leaf)
}

function Copy-HostPackage([string]$hostRoot, [string]$exeName, [string]$gameId) {
    $destination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $outputDir `
        -Config $Config -ExeName $exeName -GameId $gameId
    Write-Host "    [OK] $gameId -> $destination" -ForegroundColor Green
}

Step 'Validando ferramentas e layout'
Require-Command git | Out-Null
Require-Command cmake | Out-Null
if (-not (Test-Path -LiteralPath $hostBuilder -PathType Leaf)) {
    Fail 'build-game.ps1 não encontrado na raiz'
}
foreach ($processName in @('link-span', 'soh', '2ship')) {
    $running = Get-Process -Name $processName -ErrorAction SilentlyContinue
    if ($running) { Fail "$processName.exe está aberto (PID $($running.Id -join ', ')); feche-o antes do build" }
}

if (-not $LauncherOnly) {
    if (-not $SkipSubmodules) {
        Step 'Inicializando submódulos dos hosts'
        & git -C $root submodule update --init --recursive
        if ($LASTEXITCODE -ne 0) { Fail 'git submodule update falhou' }
    }
    $hasOot = -not $SkipOot -and (Test-HostLayout $ootRoot 'soh')
    $hasMm = -not $SkipMm -and (Test-HostLayout $mmRoot 'mm')
    if (-not $hasOot) { Write-Host '    [INFO] OoT ausente/omitido; continuando com MM.' -ForegroundColor Yellow }
    if (-not $hasMm) { Write-Host '    [INFO] MM ausente/omitido; continuando com OoT.' -ForegroundColor Yellow }
    if (-not $hasOot -and -not $hasMm) { Fail 'nenhum host disponível; informe submódulos válidos ou use -LauncherOnly' }
}

if ($ValidateOnly) {
    Write-Host "    [OK] validação concluída; nenhuma configuração ou compilação executada" -ForegroundColor Green
    return
}

Step "Configurando supervisor ($Config / x64)"
& cmake -S $root -B $buildDir -G 'Visual Studio 17 2022' -A x64 -T v143
if ($LASTEXITCODE -ne 0) { Fail 'configuração CMake do supervisor falhou' }

Step 'Compilando e testando link-span.exe'
& cmake --build $buildDir --target linkspan_launcher linkspan_asset_discovery_tests package_linkspan_demo --config $Config -- /m:1
if ($LASTEXITCODE -ne 0) { Fail 'build do supervisor falhou' }
& ctest --test-dir $buildDir -C $Config -R '^linkspan_asset_discovery_tests$' --output-on-failure
if ($LASTEXITCODE -ne 0) { Fail 'testes do supervisor falharam' }

if (-not $LauncherOnly) {
    $common = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $hostBuilder, '-Config', $Config, '-SkipSubmodules')
    if ($SkipAssets) { $common += '-SkipAssets' }
    if ($hasOot) {
        Step 'Compilando Shipwright (OoT)'
        & powershell @common -Game oot -HostPath $ootRoot
        if ($LASTEXITCODE -ne 0) { Fail 'build de Shipwright falhou' }
    }
    if ($hasMm) {
        Step 'Compilando 2Ship2Harkinian (MM)'
        & powershell @common -Game mm -HostPath $mmRoot
        if ($LASTEXITCODE -ne 0) { Fail 'build de 2Ship2Harkinian falhou' }
    }

    Step 'Montando pacote local'
    if ($hasOot) { Copy-HostPackage $ootRoot 'soh.exe' 'oot' }
    if ($hasMm) { Copy-HostPackage $mmRoot '2ship.exe' 'mm' }
}

New-Item -ItemType Directory -Path (Join-Path $outputDir 'mods') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $outputDir 'state') -Force | Out-Null
$demoPackage = Join-Path $buildDir 'examples/link-home-to-clock-tower.shipmod'
if (Test-Path -LiteralPath $demoPackage -PathType Leaf) {
    Copy-Item -LiteralPath $demoPackage -Destination (Join-Path $outputDir 'mods/link-home-to-clock-tower.shipmod') -Force
}
Write-Host ""
Write-Host "Build concluído: $(Join-Path $outputDir 'link-span.exe')" -ForegroundColor Green
Write-Host 'Coloque os assets legítimos ao lado do executável e abra link-span.exe.'
