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
    [System.IO.File]::WriteAllText((Join-Path $hostRoot 'mm.o2r'), 'base archive')
    [System.IO.File]::WriteAllText((Join-Path $hostRoot '2ship.o2r'), 'custom archive')
    [System.IO.File]::WriteAllText((Join-Path $release 'assets/nested/fixture.xml'), 'asset')

    Import-Module -Name $ModulePath -Force
    $destination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $output `
        -Config Release -ExeName '2ship.exe' -GameId mm

    $expected = @(
        (Join-Path $destination '2ship.exe'),
        (Join-Path $destination 'ZAPD.exe'),
        (Join-Path $destination 'runtime.dll'),
        (Join-Path $destination 'assets/nested/fixture.xml'),
        (Join-Path $output 'mm.o2r'),
        (Join-Path $output '2ship.o2r')
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
    [System.IO.File]::WriteAllText((Join-Path $hostRoot 'oot.o2r'), 'base archive')
    New-Item -ItemType Directory -Path (Join-Path $hostRoot 'build/x64/soh') -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $hostRoot 'build/x64/soh/soh.o2r'), 'custom archive')
    $ootOnlyOutput = Join-Path $root 'package-oot'
    $ootDestination = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $ootOnlyOutput `
        -Config 'Release-oot' -ExeName 'soh.exe' -GameId oot
    foreach ($path in @(
        (Join-Path $ootDestination 'soh.exe'),
        (Join-Path $ootOnlyOutput 'oot.o2r'),
        (Join-Path $ootOnlyOutput 'soh.o2r')
    )) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "V-LINK-2 falhou: pacote OoT-only incompleto em $path"
        }
    }
    if (Test-Path -LiteralPath (Join-Path $ootDestination 'assets')) {
        throw 'V-LINK-2 falhou: assets deveria ser opcional'
    }
    foreach ($opposite in @('mm.o2r', '2ship.o2r', 'hosts/mm')) {
        if (Test-Path -LiteralPath (Join-Path $ootOnlyOutput $opposite)) {
            throw "V-LINK-2 falhou: pacote OoT contém resíduo de MM: $opposite"
        }
    }
    Write-Host 'V-LINK-2: pacote OoT-only sem assets foi aceito.' -ForegroundColor Green

    $romFreeOutput = Join-Path $root 'package-rom-free'
    $romFreeHost = Copy-LinkSpanHostPackage -HostRoot $hostRoot -OutputDir $romFreeOutput `
        -Config Release -ExeName '2ship.exe' -GameId mm -ExcludeGameArchives
    foreach ($path in @(
        (Join-Path $romFreeHost '2ship.exe'),
        (Join-Path $romFreeHost 'ZAPD.exe'),
        (Join-Path $romFreeHost 'assets/nested/fixture.xml')
    )) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "V-LINK-7 falhou: runtime ROM-free incompleto em $path"
        }
    }
    foreach ($protected in @('mm.o2r', '2ship.o2r')) {
        if (Test-Path -LiteralPath (Join-Path $romFreeOutput $protected)) {
            throw "V-LINK-7 falhou: archive protegido copiado para pacote ROM-free: $protected"
        }
    }
    Assert-LinkSpanPackageIsRomFree -Path $romFreeOutput | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $romFreeOutput 'private.z64'), 'fixture')
    $blocked = $false
    try {
        Assert-LinkSpanPackageIsRomFree -Path $romFreeOutput | Out-Null
    } catch {
        $blocked = $true
    }
    if (-not $blocked) {
        throw 'V-LINK-7 falhou: scanner aceitou uma ROM no pacote'
    }
    Remove-Item -LiteralPath (Join-Path $romFreeOutput 'private.z64') -Force
    Write-Host 'V-LINK-7: pacote ROM-free preserva runtime/extractor e rejeita arquivos protegidos.' -ForegroundColor Green

    $stage = Join-Path $root 'stage'
    $publish = Join-Path $root 'publish-existing'
    New-Item -ItemType Directory -Path (Join-Path $stage 'hosts/oot') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $publish 'hosts/oot') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $publish 'Save') -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $stage 'link-span.exe'), 'new launcher')
    [System.IO.File]::WriteAllText((Join-Path $stage 'hosts/oot/soh.exe'), 'new host')
    [System.IO.File]::WriteAllText((Join-Path $publish 'hosts/oot/soh.exe'), 'old host')
    foreach ($asset in @('oot.o2r', 'oot.otr', 'mm.o2r')) {
        [System.IO.File]::WriteAllText((Join-Path $publish $asset), "user asset $asset")
    }
    [System.IO.File]::WriteAllText((Join-Path $publish 'Save/slot.sav'), 'user save')
    New-Item -ItemType Directory -Path (Join-Path $publish 'mods/custom-user-mod') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $publish 'mods/.shiplua-cache') -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $publish 'mods/custom-user-mod/main.lua'), 'user mod')
    [System.IO.File]::WriteAllText((Join-Path $publish 'mods/jump.shipmod'), 'stale generated mod')
    Publish-LinkSpanPackage -StageDir $stage -Destination $publish
    foreach ($asset in @('oot.o2r', 'oot.otr', 'mm.o2r')) {
        $path = Join-Path $publish $asset
        if ((Get-Content -LiteralPath $path -Raw) -ne "user asset $asset") {
            throw "V-LINK-6 falhou: asset do usuário foi removido ou alterado: $asset"
        }
    }
    if ((Get-Content -LiteralPath (Join-Path $publish 'Save/slot.sav') -Raw) -ne 'user save') {
        throw 'V-LINK-6 falhou: save do usuário foi alterado'
    }
    if ((Get-Content -LiteralPath (Join-Path $publish 'hosts/oot/soh.exe') -Raw) -ne 'new host') {
        throw 'V-LINK-6 falhou: host gerado não foi atualizado'
    }
    if (-not (Test-Path -LiteralPath (Join-Path $publish 'mods/custom-user-mod/main.lua') -PathType Leaf)) {
        throw 'V-LINK-6 falhou: mod não gerado do usuário foi removido'
    }
    if (Test-Path -LiteralPath (Join-Path $publish 'mods/jump.shipmod')) {
        throw 'V-LINK-6 falhou: pacote de demo obsoleto não foi removido'
    }
    if (Test-Path -LiteralPath (Join-Path $publish 'mods/.shiplua-cache')) {
        throw 'V-LINK-6 falhou: cache derivado do modloader não foi limpo'
    }
    Write-Host 'V-LINK-6: republicação preserva assets base e saves do usuário.' -ForegroundColor Green
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
