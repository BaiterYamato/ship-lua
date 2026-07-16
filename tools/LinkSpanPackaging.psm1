Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-LinkSpanHostExecutable {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$HostRoot,
        [Parameter(Mandatory)] [string]$Config,
        [Parameter(Mandatory)] [string]$Name
    )

    $candidates = @(
        (Join-Path $HostRoot "x64/$Config/$Name"),
        (Join-Path $HostRoot "build/x64/x64/$Config/$Name"),
        (Join-Path $HostRoot "build/x64/$Config/$Name"),
        (Join-Path $HostRoot "build/x64/$Name")
    )
    return $candidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
}

function Copy-LinkSpanHostPackage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$HostRoot,
        [Parameter(Mandatory)] [string]$OutputDir,
        [Parameter(Mandatory)] [string]$Config,
        [Parameter(Mandatory)] [string]$ExeName,
        [Parameter(Mandatory)] [ValidateSet('oot', 'mm')] [string]$GameId
    )

    $executable = Find-LinkSpanHostExecutable -HostRoot $HostRoot -Config $Config -Name $ExeName
    if (-not $executable) {
        throw "Link-Span: $ExeName não foi localizado após o build de $GameId"
    }

    $sourceDir = Split-Path -Parent $executable
    # `-SkipAssets` builds (and hosts that already contain an O2R archive) do
    # not have an assets/ directory.  The launcher can still run that world;
    # leave extraction to the host instead of rejecting an otherwise valid
    # single-world package.
    $assetsSource = Join-Path $sourceDir 'assets'
    $hasAssets = Test-Path -LiteralPath $assetsSource -PathType Container

    $destination = Join-Path $OutputDir "hosts/$GameId"
    $assetsDestination = Join-Path $destination 'assets'
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    if ($hasAssets) {
        New-Item -ItemType Directory -Path $assetsDestination -Force | Out-Null
    }

    Copy-Item -LiteralPath $executable -Destination (Join-Path $destination $ExeName) -Force
    Get-ChildItem -LiteralPath $sourceDir -File -Force -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Extension -in @('.dll', '.json', '.ini') -or $_.Name -ieq 'ZAPD.exe'
        } |
        Copy-Item -Destination $destination -Force
    if ($hasAssets) {
        Get-ChildItem -LiteralPath $assetsSource -Force -ErrorAction Stop |
            Copy-Item -Destination $assetsDestination -Recurse -Force
    }
    $archiveNames = if ($GameId -eq 'oot') {
        @('oot.o2r', 'oot.otr', 'soh.o2r')
    } else {
        @('mm.o2r', '2ship.o2r')
    }
    $archiveDirectories = @(
        $sourceDir,
        $HostRoot,
        (Join-Path $HostRoot "x64/$Config"),
        (Join-Path $HostRoot "build/x64/$Config"),
        (Join-Path $HostRoot "build/x64/x64/$Config"),
        (Join-Path $HostRoot 'build/x64/soh'),
        (Join-Path $HostRoot 'build/x64/mm')
    ) | Select-Object -Unique
    foreach ($archiveName in $archiveNames) {
        $archive = $archiveDirectories |
            ForEach-Object { Join-Path $_ $archiveName } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($archive) {
            Copy-Item -LiteralPath $archive -Destination (Join-Path $OutputDir $archiveName) -Force
        }
    }

    if (-not (Test-Path -LiteralPath (Join-Path $destination $ExeName) -PathType Leaf) -or
        ($hasAssets -and -not (Test-Path -LiteralPath $assetsDestination -PathType Container))) {
        throw "Link-Span: pacote runtime incompleto para $GameId"
    }

    return $destination
}

Export-ModuleMember -Function Find-LinkSpanHostExecutable, Copy-LinkSpanHostPackage
