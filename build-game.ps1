<#
.SYNOPSIS
    One-shot Windows build for the ShipLua-enabled forks of Shipwright (OoT) and
    2Ship2Harkinian (MM).

.DESCRIPTION
    Designed for a new user who has just cloned the BaiterYamato fork of either
    host and wants to build a runnable .exe without reading the upstream build
    docs. Runs from inside the host clone (or with -HostPath pointing at it) and:

      1. verifies prerequisites (Git, CMake >= 3.26, Python 3, VS 2022 + v143);
      2. inits all git submodules (libultraship, ZAPDTR, OTRExporter, extern/ship-lua);
      3. configures CMake with the "Visual Studio 17 2022" generator, v143 toolset, x64;
      4. generates the custom .o2r asset pack WITHOUT a ROM (--norom), so the
         executable builds and the custom-asset pipeline is exercised;
      5. builds the game target;
      6. prints where the .exe is and what to do next.

    The game is NOT playable until a real ROM is supplied via ExtractAssets
    (see "Next steps" in the final summary). The no-rom target only produces
    the ship-defined .o2r (soh.o2r / 2ship.o2r), not the base game assets.

.PARAMETER Game
    oot | mm | auto (default). auto detects from the host folder structure:
    presence of soh/CMakeLists.txt -> oot; mm/CMakeLists.txt -> mm.

.PARAMETER HostPath
    Path to the cloned host repository (the one containing CMakeLists.txt with
    project(Ship ...) or project(2Ship ...)). Default: current directory. If the
    script is invoked from inside extern/ship-lua/ of a host clone, the parent's
    parent is used automatically.

.PARAMETER Config
    Debug | Release | RelWithDebInfo | MinSizeRel. Default: Release.

.PARAMETER BuildDir
    CMake build directory (relative to HostPath). Default: build/x64.

.PARAMETER SkipSubmodules
    Skip `git submodule update --init --recursive` (use if submodules are
    already initialised and you want to avoid the network).

.PARAMETER SkipAssets
    Skip the .o2r asset generation step.

.PARAMETER SkipBuild
    Configure only; do not build.

.EXAMPLE
    # From the root of a freshly cloned BaiterYamato/Shipwright-HyliaFoundry fork:
    .\extern\ship-lua\build-game.ps1

.EXAMPLE
    # Explicit game + custom host path:
    .\extern\ship-lua\build-game.ps1 -Game oot -HostPath C:\code\Shipwright-HyliaFoundry

.EXAMPLE
    # Debug build, skip submodule init (already done):
    .\extern\ship-lua\build-game.ps1 -Config Debug -SkipSubmodules
#>
[CmdletBinding()]
param(
    [ValidateSet('oot','mm','auto')]
    [string]$Game = 'auto',

    [string]$HostPath,

    [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
    [string]$Config = 'Release',

    [string]$BuildDir = 'build/x64',

    [switch]$SkipSubmodules,

    [switch]$SkipAssets,

    [switch]$SkipBuild,

    [switch]$Help
)

$ErrorActionPreference = 'Stop'

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

function Write-Step([string]$msg) {
    Write-Host ""
    Write-Host "==> $msg" -ForegroundColor Cyan
}

function Write-Ok([string]$msg) {
    Write-Host "    [OK] $msg" -ForegroundColor Green
}

function Write-Warn([string]$msg) {
    Write-Host "    [!]  $msg" -ForegroundColor Yellow
}

function Die([string]$msg) {
    Write-Host ""
    Write-Host "ERROR: $msg" -ForegroundColor Red
    Write-Host ""
    exit 1
}

if ($Help) {
    Get-Help $MyInvocation.MyCommand.Path -Detailed
    exit 0
}

# -----------------------------------------------------------------------------
# Banner
# -----------------------------------------------------------------------------

Write-Host ""
Write-Host "  ____ _  _ ____ ____    ____                  " -ForegroundColor Magenta
Write-Host "  / ___|| || | ___|  _ \ |  _ \ ___  __ _  ___ " -ForegroundColor Magenta
Write-Host "  \___ \| __ || __ \| |_) )| |_) / _ \/ _` |/ _ \" -ForegroundColor Magenta
Write-Host "   ___) | ||_| ___/|  _ < |  _ <  __/ (_| | (_) |" -ForegroundColor Magenta
Write-Host "  |____/ \___|____||_| \_\|_| \_\___|\__,_|\___/ " -ForegroundColor Magenta
Write-Host "  game build helper (Windows / MSVC v143)" -ForegroundColor DarkGray
Write-Host ""

# -----------------------------------------------------------------------------
# Resolve HostPath
# -----------------------------------------------------------------------------

Write-Step "Resolving host repository path"

if (-not $HostPath) {
    # If invoked from inside extern/ship-lua/, walk up two levels to the host root.
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $candidate = Split-Path -Parent (Split-Path -Parent $scriptDir)
    if (Test-Path (Join-Path $candidate 'CMakeLists.txt')) {
        $HostPath = $candidate
    } else {
        $HostPath = (Get-Location).Path
    }
}

$HostPath = (Resolve-Path -LiteralPath $HostPath -ErrorAction SilentlyContinue).Path
if (-not $HostPath -or -not (Test-Path (Join-Path $HostPath 'CMakeLists.txt'))) {
    Die "No CMakeLists.txt found under '$HostPath'. Run this script from inside a Shipwright-HyliaFoundry or 2ship2harkinian clone, or pass -HostPath <path>."
}

Write-Ok "Host repository: $HostPath"

# -----------------------------------------------------------------------------
# Detect game
# -----------------------------------------------------------------------------

Write-Step "Detecting game"

$hostCmake = Get-Content (Join-Path $HostPath 'CMakeLists.txt') -Raw

if ($Game -eq 'auto') {
    if (Test-Path (Join-Path $HostPath 'soh/CMakeLists.txt')) {
        $Game = 'oot'
    } elseif (Test-Path (Join-Path $HostPath 'mm/CMakeLists.txt')) {
        $Game = 'mm'
    } else {
        Die "Could not auto-detect game (no soh/ or mm/ directory under host root). Pass -Game oot or -Game mm."
    }
}

switch ($Game) {
    'oot' {
        $assetTarget   = 'GenerateSohOtr'
        $exeName       = 'soh.exe'
        $extractTarget = 'ExtractAssets'
        $projectId     = 'Shipwright'
        Write-Ok "Game: Ocarina of Time (Shipwright)"
    }
    'mm' {
        $assetTarget   = 'Generate2ShipOtr'
        $exeName       = '2ship.exe'
        $extractTarget = 'ExtractAssets'
        $projectId     = '2Ship2Harkinian'
        Write-Ok "Game: Majora's Mask (2Ship2Harkinian)"
    }
}

# -----------------------------------------------------------------------------
# Prerequisites
# -----------------------------------------------------------------------------

Write-Step "Checking prerequisites"

# Git
$git = (Get-Command git.exe -ErrorAction SilentlyContinue)
if (-not $git) {
    Die "Git not found on PATH. Install from https://git-scm.com/download/win and reopen the terminal."
}
$gitVersion = (& git --version) 2>$null
Write-Ok "$gitVersion"

# CMake >= 3.26
$cmake = (Get-Command cmake.exe -ErrorAction SilentlyContinue)
if (-not $cmake) {
    Die "CMake not found on PATH. Install from https://cmake.org/download/ (the Windows x64 Installer) and tick 'Add CMake to system PATH'. Reopen the terminal after install."
}
$cmakeVersionOut = (& cmake --version) 2>$null | Select-Object -First 1
if ($cmakeVersionOut -match 'cmake version (\d+)\.(\d+)\.(\d+)') {
    $cmMajor = [int]$Matches[1]; $cmMinor = [int]$Matches[2]
    if ($cmMajor -lt 3 -or ($cmMajor -eq 3 -and $cmMinor -lt 26)) {
        Die "CMake $cmMajor.$cmMinor found, but the host requires >= 3.26. Upgrade from https://cmake.org/download/."
    }
    Write-Ok "$cmakeVersionOut (>= 3.26)"
} else {
    Die "Could not parse CMake version from: $cmakeVersionOut"
}

# Python 3
$py = (Get-Command python.exe -ErrorAction SilentlyContinue) -or (Get-Command py.exe -ErrorAction SilentlyContinue)
$pythonExe = if (Get-Command python.exe -ErrorAction SilentlyContinue) { 'python.exe' } else { 'py.exe' }
if (-not $py) {
    Die "Python 3 not found on PATH. Install from https://www.python.org/downloads/windows/ and tick 'Add Python to PATH' in the installer. Or install via the Visual Studio Installer (Python development workload)."
}
$pyVersion = (& $pythonExe --version) 2>$null
Write-Ok "$pyVersion"

# Visual Studio 2022 + v143 toolset via vswhere. The catalog component IDs are
# not always registered locally, so we (a) require the generic VC.Tools component
# to confirm the C++ workload, then (b) verify the v143 toolset is physically
# present by listing VC/Tools/MSVC/* and checking the major.minor >= 14.30.
$vswhereExe = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhereExe)) {
    Die "vswhere.exe not found at '$vswhereExe'. Install Visual Studio 2022 (Community is free) from https://visualstudio.microsoft.com/vs/ with the 'Desktop development with C++' workload."
}

# (a) C++ workload present?
$vsInstall = & $vswhereExe -latest `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath 2>$null
if (-not $vsInstall) {
    # Fall back to "any VS install" so we can give a more specific message.
    $vsInstallAny = & $vswhereExe -latest -property installationPath 2>$null
    if ($vsInstallAny) {
        Die "Visual Studio is installed at $vsInstallAny but the C++ workload is missing. Open the Visual Studio Installer -> Modify -> select 'Desktop development with C++'."
    }
    Die "Visual Studio 2022 was not found. Install VS 2022 (Community is free) from https://visualstudio.microsoft.com/vs/ and select the 'Desktop development with C++' workload."
}

# (b) v143 toolset physically present? (MSVC 14.30+ == v143)
# vswhere's -find glob is finicky (single '*' matches nothing under a versioned
# dir), so list VC/Tools/MSVC directly from installationPath and look for a
# version directory >= 14.30.
$msvcRoot = Join-Path $vsInstall 'VC\Tools\MSVC'
$v143Found = $false
$v143Version = ''
if (Test-Path $msvcRoot) {
    $msvcDirs = Get-ChildItem -Path $msvcRoot -Directory -ErrorAction SilentlyContinue
    foreach ($dir in $msvcDirs) {
        if ($dir.Name -match '^14\.(\d+)\.') {
            $minor = [int]$Matches[1]
            if ($minor -ge 30) {
                $v143Found = $true
                $v143Version = $dir.Name
                break
            }
        }
    }
}
if (-not $v143Found) {
    Die "VS 2022 is installed at $vsInstall but the MSVC v143 toolset (14.30+) was not found under VC/Tools/MSVC. Open the Visual Studio Installer -> Modify -> 'Individual components' -> tick 'MSVC v143 - VS 2022 C++ x64/x86 build tools'."
}
$vsVersion = & $vswhereExe -latest -property installationVersion 2>$null
Write-Ok "Visual Studio 2022 (v$vsVersion) at $vsInstall"
Write-Ok "MSVC v143 toolset ($v143Version)"

Write-Host ""
Write-Host "    All prerequisites satisfied." -ForegroundColor Green

# -----------------------------------------------------------------------------
# Submodules
# -----------------------------------------------------------------------------

if (-not $SkipSubmodules) {
    Write-Step "Initialising git submodules (libultraship, ZAPDTR, OTRExporter, extern/ship-lua)"
    Push-Location $HostPath
    try {
        & git submodule update --init --recursive
        if ($LASTEXITCODE -ne 0) {
            Die "git submodule update failed (exit $LASTEXITCODE). Check your network and try with -SkipSubmodules if submodules are already initialised."
        }
    } finally {
        Pop-Location
    }
    Write-Ok "Submodules up to date"
} else {
    Write-Warn "Skipping submodule init (-SkipSubmodules)"
}

# -----------------------------------------------------------------------------
# Configure
# -----------------------------------------------------------------------------

$buildPath = Join-Path $HostPath $BuildDir
Write-Step "Configuring CMake (Visual Studio 17 2022, v143, x64)"

if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
}

& cmake -S $HostPath -B $buildPath -G 'Visual Studio 17 2022' -T v143 -A x64
if ($LASTEXITCODE -ne 0) {
    Die "CMake configure failed (exit $LASTEXITCODE). See the messages above; common causes are missing VS components or a stale build directory (delete '$buildPath' and retry)."
}
Write-Ok "CMake configure complete -> $buildPath"

if ($SkipBuild) {
    Write-Warn "Stopping after configure (-SkipBuild)"
    exit 0
}

# -----------------------------------------------------------------------------
# Build ZAPD (asset extractor) first
# -----------------------------------------------------------------------------

Write-Step "Building ZAPD (asset extraction tool)"
& cmake --build $buildPath --target ZAPD --config $Config
if ($LASTEXITCODE -ne 0) {
    Die "Building ZAPD failed (exit $LASTEXITCODE)."
}
Write-Ok "ZAPD built"

# -----------------------------------------------------------------------------
# Generate assets (no ROM)
# -----------------------------------------------------------------------------

if (-not $SkipAssets) {
    Write-Step "Generating custom .o2r (--norom; full game assets still require a ROM)"
    & cmake --build $buildPath --target $assetTarget --config $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Warn "Asset generation target '$assetTarget' failed (exit $LASTEXITCODE). The game .exe can still build; rerun with -SkipAssets to skip this step."
    } else {
        Write-Ok "Custom .o2r generated"
    }
} else {
    Write-Warn "Skipping asset generation (-SkipAssets)"
}

# -----------------------------------------------------------------------------
# Build the game
# -----------------------------------------------------------------------------

Write-Step "Building $projectId (--config $Config)"
& cmake --build $buildPath --config $Config
if ($LASTEXITCODE -ne 0) {
    Die "Build failed (exit $LASTEXITCODE). See the compiler output above."
}
Write-Ok "Build complete"

# -----------------------------------------------------------------------------
# Locate the executable
# -----------------------------------------------------------------------------

$exeCandidates = @(
    (Join-Path $buildPath "x64/$Config/$exeName"),
    (Join-Path $buildPath "$Config/$exeName"),
    (Join-Path $buildPath $exeName)
)
$exeFound = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

# -----------------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------------

Write-Host ""
Write-Host "==================================================================" -ForegroundColor Green
Write-Host "  BUILD SUCCEEDED" -ForegroundColor Green
Write-Host "==================================================================" -ForegroundColor Green
Write-Host "  Game           : $projectId"
Write-Host "  Configuration  : $Config"
Write-Host "  Build dir      : $buildPath"
if ($exeFound) {
    Write-Host "  Executable     : $exeFound" -ForegroundColor White
} else {
    Write-Warn "Executable $exeName not found at the usual locations; check $buildPath manually."
}
Write-Host ""
Write-Host "  Next steps:" -ForegroundColor Cyan
if (-not $SkipAssets) {
    Write-Host "    - Custom .o2r was generated WITHOUT a ROM."
    Write-Host "      To make the game playable, supply a legitimate Zelda ROM and run:"
    Write-Host "        cmake --build `"$buildPath`" --target $extractTarget --config $Config"
} else {
    Write-Host "    - Asset generation was skipped; run when ready:"
    Write-Host "        cmake --build `"$buildPath`" --target $assetTarget --config $Config"
}
if ($exeFound) {
    Write-Host "    - Launch:"
    Write-Host "        & `"$exeFound`""
}
Write-Host ""
