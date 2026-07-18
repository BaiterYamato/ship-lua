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
        [Parameter(Mandatory)] [ValidateSet('oot', 'mm')] [string]$GameId,
        [switch]$ExcludeGameArchives
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
    $portArchiveName = if ($GameId -eq 'oot') { 'soh.o2r' } else { '2ship.o2r' }
    $archiveNames = if ($ExcludeGameArchives) {
        @($portArchiveName)
    } elseif ($GameId -eq 'oot') {
        @('oot.o2r', 'oot.otr', $portArchiveName)
    } else {
        @('mm.o2r', $portArchiveName)
    }
    $archiveDirectories = @(
        $sourceDir,
        $HostRoot,
        (Join-Path $HostRoot "x64/$Config"),
        (Join-Path $HostRoot "build/x64/$Config"),
        (Join-Path $HostRoot "build/x64/x64/$Config"),
        (Join-Path $HostRoot 'build/x64/soh'),
        (Join-Path $HostRoot 'build/x64/mm'),
        (Join-Path $HostRoot 'soh'),
        (Join-Path $HostRoot 'mm')
    ) | Select-Object -Unique
    foreach ($archiveName in $archiveNames) {
        $archive = $archiveDirectories |
            ForEach-Object { Join-Path $_ $archiveName } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($archive) {
            Copy-Item -LiteralPath $archive -Destination (Join-Path $OutputDir $archiveName) -Force
        } elseif ($archiveName -eq $portArchiveName) {
            throw "Link-Span: archive runtime obrigatório $portArchiveName não foi localizado para $GameId"
        }
    }

    if (-not (Test-Path -LiteralPath (Join-Path $destination $ExeName) -PathType Leaf) -or
        ($hasAssets -and -not (Test-Path -LiteralPath $assetsDestination -PathType Container))) {
        throw "Link-Span: pacote runtime incompleto para $GameId"
    }

    return $destination
}

function Assert-LinkSpanPackageIsRomFree {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$Path
    )

    $root = [System.IO.Path]::GetFullPath($Path)
    $rootPrefix = $root.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $allowedPortArchives = @('soh.o2r', '2ship.o2r')
    $protectedExtensions = @('.z64', '.n64', '.v64', '.o2r', '.otr')
    $protectedFiles = @(
        Get-ChildItem -LiteralPath $Path -Recurse -File -Force -ErrorAction Stop |
            Where-Object {
                $fullName = [System.IO.Path]::GetFullPath($_.FullName)
                $relative = if ($fullName.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                    $fullName.Substring($rootPrefix.Length)
                } else {
                    $fullName
                }
                $isAllowedPortArchive = $relative -in $allowedPortArchives
                $_.Extension.ToLowerInvariant() -in $protectedExtensions -and -not $isAllowedPortArchive
            }
    )
    if ($protectedFiles.Count -gt 0) {
        $relative = $protectedFiles | ForEach-Object {
            $fullName = [System.IO.Path]::GetFullPath($_.FullName)
            if ($fullName.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                $fullName.Substring($rootPrefix.Length)
            } else {
                $fullName
            }
        }
        throw "Link-Span: ROM-free package contains protected game files: $($relative -join ', ')"
    }
    return $true
}

function Publish-LinkSpanPackage {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [string]$StageDir,
        [Parameter(Mandatory)] [string]$Destination
    )

    $destinationFull = [System.IO.Path]::GetFullPath($Destination)
    New-Item -ItemType Directory -Path $destinationFull -Force | Out-Null
    $generated = @(
        'link-span.exe', 'README.md', 'hosts/oot', 'hosts/mm', 'soh.o2r', '2ship.o2r',
        'mods/.shiplua-cache',
        'mods/hello-world', 'mods/jump', 'mods/dog-spawner',
        'mods/hello-world.shipmod', 'mods/jump.shipmod', 'mods/dog-spawner.shipmod',
        'mods/link-home-to-clock-tower.shipmod'
    )
    foreach ($relative in $generated) {
        $target = [System.IO.Path]::GetFullPath((Join-Path $destinationFull $relative))
        $prefix = $destinationFull.TrimEnd('\', '/') +
            [System.IO.Path]::DirectorySeparatorChar
        if (-not $target.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Link-Span: destino gerado escapou do pacote: $target"
        }
        if (Test-Path -LiteralPath $target) {
            Remove-Item -LiteralPath $target -Recurse -Force
        }
    }
    Get-ChildItem -LiteralPath $StageDir -Force |
        Copy-Item -Destination $destinationFull -Recurse -Force
}

Export-ModuleMember -Function Find-LinkSpanHostExecutable, Copy-LinkSpanHostPackage, Publish-LinkSpanPackage, Assert-LinkSpanPackageIsRomFree
