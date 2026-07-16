[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ScriptPath
)

$ErrorActionPreference = 'Stop'
$ScriptPath = (Resolve-Path -LiteralPath $ScriptPath).Path
$shell = (Get-Process -Id $PID).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("link-span-build-game-tests-" + [guid]::NewGuid())

function New-FakeHost([string]$Name, [string]$GameDirectory) {
    $hostRoot = Join-Path $tempRoot $Name
    New-Item -ItemType Directory -Path (Join-Path $hostRoot $GameDirectory) -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $hostRoot 'CMakeLists.txt') -Value 'cmake_minimum_required(VERSION 3.26)'
    Set-Content -LiteralPath (Join-Path $hostRoot "$GameDirectory/CMakeLists.txt") -Value "project($GameDirectory)"
    return $hostRoot
}

function Invoke-Validation([string]$TestScript, [string]$WorkingDirectory, [string[]]$Arguments) {
    Push-Location $WorkingDirectory
    try {
        $output = & $shell -NoProfile -ExecutionPolicy Bypass -File $TestScript @Arguments 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "build-game.ps1 failed with exit ${LASTEXITCODE}:`n$($output -join [Environment]::NewLine)"
        }
        return ($output -join [Environment]::NewLine)
    } finally {
        Pop-Location
    }
}

try {
    New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
    $outside = Join-Path $tempRoot 'outside'
    New-Item -ItemType Directory -Path $outside -Force | Out-Null

    $ootRoot = New-FakeHost 'Shipwright-HyliaFoundry' 'soh'
    $ootScript = Join-Path $ootRoot 'build-game.ps1'
    Copy-Item -LiteralPath $ScriptPath -Destination $ootScript
    $ootOutput = Invoke-Validation $ootScript $outside @('-ValidateOnly')
    if ($ootOutput -notmatch 'Game: Ocarina of Time' -or $ootOutput -notmatch [regex]::Escape($ootRoot)) {
        throw "Direct OoT host detection failed:`n$ootOutput"
    }

    $mmRoot = New-FakeHost '2ship2harkinian' 'mm'
    $submoduleDir = Join-Path $mmRoot 'extern/link-span'
    New-Item -ItemType Directory -Path $submoduleDir -Force | Out-Null
    $mmScript = Join-Path $submoduleDir 'build-game.ps1'
    Copy-Item -LiteralPath $ScriptPath -Destination $mmScript
    $mmOutput = Invoke-Validation $mmScript $outside @('-ValidateOnly')
    if ($mmOutput -notmatch "Game: Majora's Mask" -or $mmOutput -notmatch [regex]::Escape($mmRoot)) {
        throw "Submodule MM host detection failed:`n$mmOutput"
    }

    $explicitOutput = Invoke-Validation $ScriptPath $outside @('-HostPath', $ootRoot, '-Game', 'oot', '-ValidateOnly')
    if ($explicitOutput -notmatch 'no build actions were executed') {
        throw "Explicit -HostPath validation failed:`n$explicitOutput"
    }

    Write-Host 'build-game.ps1 tests: 3/3 passed'
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
