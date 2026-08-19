[CmdletBinding()]
param(
    [ValidateSet('check', 'build', 'iso', 'qemu', 'clean')]
    [string]$Command = 'check',
    [string]$InstallDir = (Join-Path $HOME 'CortexOS'),
    [string[]]$Arguments = @()
)

$ErrorActionPreference = 'Stop'
$ArchiveUrl = if ($env:CORTEXOS_ARCHIVE_URL) {
    $env:CORTEXOS_ARCHIVE_URL
} else {
    'https://codeload.github.com/CRISTOP-bot/nucleos/legacy.tar.gz/refs/heads/main'
}

if (Test-Path $InstallDir) {
    throw "El directorio ya existe: $InstallDir. Usa -InstallDir para elegir otra ubicación."
}

$tar = Get-Command tar.exe -ErrorAction SilentlyContinue
if (-not $tar) {
    throw 'Se requiere tar.exe. Está incluido en las versiones recientes de Windows; también puedes usar WSL.'
}

$tempDir = Join-Path ([IO.Path]::GetTempPath()) ("cortexos-" + [Guid]::NewGuid().ToString('N'))
$archive = Join-Path $tempDir 'cortexos.tar.gz'
New-Item -ItemType Directory -Path $tempDir | Out-Null
try {
    Invoke-WebRequest -UseBasicParsing -Uri $ArchiveUrl -OutFile $archive
    New-Item -ItemType Directory -Path $InstallDir | Out-Null
    & $tar.Source -xzf $archive --strip-components=1 -C $InstallDir
    if ($LASTEXITCODE -ne 0) { throw "tar.exe terminó con código $LASTEXITCODE." }
} finally {
    Remove-Item -LiteralPath $tempDir -Recurse -Force -ErrorAction SilentlyContinue
}

$runner = Join-Path $InstallDir 'project\scripts\win\nucleos.ps1'
& powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File $runner -Command $Command @Arguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
