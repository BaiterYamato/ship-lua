param(
    [Parameter(Mandatory)]
    [string]$ModulePath
)

$ErrorActionPreference = 'Stop'
$root = Join-Path ([System.IO.Path]::GetTempPath()) ("linkspan-package-test-" + [guid]::NewGuid())

try {
    $hostRoot = Join-Path $root 'host'
    $release = Join-Path $hostRoot 'x64/Release'
    $output = Join-Path $root 'package'
    New-Item -ItemType Directory -Path (Join-Path $release 'assets/nested') -Force | Out-Null
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $release '2ship.exe'), 'host')
    [System.IO.File]::WriteAllText((Join-Path $release 'ZAPD.exe'), 'extractor')
    [System.IO.File]::WriteAllText((Join-Path $release 'runtime.dll'), 'dependency')
    [System.IO.File]::WriteAllText((Join-Path $release 'mm.o2r'), 'archive')
    [System.IO.File]::WriteAllText((Join-Path $release 'assets/nested/fixture.xml'), 'asset')

    Import-Module -Name $ModulePath -Force
    $destination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $output `
        -Config Release -ExeName '2ship.exe' -GameId mm

    $expected = @(
        (Join-Path $destination '2ship.exe'),
        (Join-Path $destination 'ZAPD.exe'),
        (Join-Path $destination 'runtime.dll'),
        (Join-Path $destination 'assets/nested/fixture.xml'),
        (Join-Path $output 'mm.o2r')
    )
    foreach ($path in $expected) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "V-LINK-1 falhou: artefato runtime ausente em $path"
        }
    }

    Write-Host 'V-LINK-1: pacote inclui host, extractor, dependências, assets e archives.' -ForegroundColor Green
    $ootRelease = Join-Path $hostRoot 'x64/Release-oot'
    New-Item -ItemType Directory -Path $ootRelease -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $ootRelease 'soh.exe'), 'host')
    [System.IO.File]::WriteAllText((Join-Path $ootRelease 'oot.o2r'), 'archive')
    $ootDestination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $output `
        -Config 'Release-oot' -ExeName 'soh.exe' -GameId oot
    foreach ($path in @((Join-Path $ootDestination 'soh.exe'), (Join-Path $output 'oot.o2r'))) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "V-LINK-2 falhou: pacote OoT-only incompleto em $path"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $ootDestination 'assets')) {
        throw 'V-LINK-2 falhou: assets deveria ser opcional'
    }
    Write-Host 'V-LINK-2: pacote OoT-only sem assets foi aceito.' -ForegroundColor Green
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
