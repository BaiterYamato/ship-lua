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
    [ValidateSet('auto','oot','mm','dual')]
    [string]$Games = 'auto',
    [string]$OotHostPath,
    [string]$MmHostPath,
    [string]$OutputDir,
    [switch]$SkipOot,
    [switch]$SkipMm,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root 'build/link-span'
$launcherOutputDir = Join-Path $root "x64/$Config"
$ootRoot = if ($OotHostPath) { $OotHostPath } else { Join-Path $root 'hosts/shipwright' }
$mmRoot = if ($MmHostPath) { $MmHostPath } else { Join-Path $root 'hosts/2ship2harkinian' }
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

function Copy-HostPackage([string]$hostRoot, [string]$exeName, [string]$gameId,
                          [string]$stageDir) {
    $destination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $stageDir `
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
    if (-not $SkipSubmodules -and (-not $OotHostPath -or -not $MmHostPath)) {
        Step 'Inicializando submódulos dos hosts'
        & git -C $root submodule update --init --recursive
        if ($LASTEXITCODE -ne 0) { Fail 'git submodule update falhou' }
    }
    $availableOot = -not $SkipOot -and (Test-HostLayout $ootRoot 'soh')
    $availableMm = -not $SkipMm -and (Test-HostLayout $mmRoot 'mm')
    switch ($Games) {
        'oot' {
            if (-not $availableOot) { Fail 'perfil oot solicitado, mas o host OoT não está disponível' }
            $hasOot = $true; $hasMm = $false; $profile = 'oot'
        }
        'mm' {
            if (-not $availableMm) { Fail 'perfil mm solicitado, mas o host MM não está disponível' }
            $hasOot = $false; $hasMm = $true; $profile = 'mm'
        }
        'dual' {
            if (-not $availableOot -or -not $availableMm) { Fail 'perfil dual exige os dois hosts' }
            $hasOot = $true; $hasMm = $true; $profile = 'dual'
        }
        default {
            $hasOot = $availableOot; $hasMm = $availableMm
            if ($hasOot -and $hasMm) { $profile = 'dual' }
            elseif ($hasOot) { $profile = 'oot' }
            elseif ($hasMm) { $profile = 'mm' }
            else { Fail 'nenhum host disponível; informe paths válidos ou use -LauncherOnly' }
        }
    }
} else {
    $hasOot = $false; $hasMm = $false; $profile = 'launcher'
}

if ($OutputDir) {
    $outputDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) {
        [System.IO.Path]::GetFullPath($OutputDir)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $OutputDir))
    }
} elseif ($LauncherOnly) {
    $outputDir = $launcherOutputDir
} else {
    $outputDir = Join-Path $root "x64/packages/$profile/$Config"
}

if ($ValidateOnly) {
    Write-Host "    [OK] perfil: $profile; saída: $outputDir" -ForegroundColor Green
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

$launcherExecutable = Join-Path $launcherOutputDir 'link-span.exe'
if (-not (Test-Path -LiteralPath $launcherExecutable -PathType Leaf)) {
    Fail "launcher não foi gerado em $launcherExecutable"
}

if ($LauncherOnly) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    Copy-Item -LiteralPath $launcherExecutable -Destination (Join-Path $outputDir 'link-span.exe') -Force
    Write-Host ""
    Write-Host "Build concluído: $(Join-Path $outputDir 'link-span.exe')" -ForegroundColor Green
    return
}

$common = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $hostBuilder, '-Config', $Config,
            '-BuildDir', 'build/linkspan-x64', '-SkipSubmodules')
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

Step "Montando pacote $profile"
$stageDir = Join-Path $root ("build/link-span-package-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null
try {
    Copy-Item -LiteralPath $launcherExecutable -Destination (Join-Path $stageDir 'link-span.exe') -Force
    if ($hasOot) { Copy-HostPackage $ootRoot 'soh.exe' 'oot' $stageDir }
    if ($hasMm) { Copy-HostPackage $mmRoot '2ship.exe' 'mm' $stageDir }

    $modsDir = Join-Path $stageDir 'mods'
    New-Item -ItemType Directory -Path $modsDir -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $stageDir 'state') -Force | Out-Null
    foreach ($modName in @('hello-world', 'jump', 'dog-spawner')) {
        $source = Join-Path $root "examples/$modName"
        Copy-Item -LiteralPath $source -Destination (Join-Path $modsDir $modName) -Recurse -Force
    }
    Step 'Montando pacote local'
    $demoPackage = Join-Path $buildDir 'examples/link-home-to-clock-tower.shipmod'
    if (Test-Path -LiteralPath $demoPackage -PathType Leaf) {
        Copy-Item -LiteralPath $demoPackage -Destination (Join-Path $modsDir 'link-home-to-clock-tower.shipmod') -Force
    }
    Publish-LinkSpanPackage -StageDir $stageDir -Destination $outputDir
} finally {
    if (Test-Path -LiteralPath $stageDir) {
        Remove-Item -LiteralPath $stageDir -Recurse -Force
    }
}
Write-Host ""
Write-Host "Build concluído ($profile): $(Join-Path $outputDir 'link-span.exe')" -ForegroundColor Green
Write-Host 'Coloque os assets legítimos ao lado do executável e abra link-span.exe.'
