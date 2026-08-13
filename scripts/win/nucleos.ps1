[CmdletBinding()]
param(
    [ValidateSet('check', 'build', 'iso', 'qemu', 'virtualbox', 'clean')]
    [string]$Command = 'check',
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Invoke-Python([string[]]$Arguments) {
    $python = Get-Command py.exe -ErrorAction SilentlyContinue
    if (-not $python) { $python = Get-Command python.exe -ErrorAction SilentlyContinue }
    if (-not $python) { throw 'No se encontró Python. Ejecuta tools\setup\install-deps.bat.' }
    & $python.Source @Arguments
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-Make([string[]]$Arguments) {
    $make = Get-Command make.exe -ErrorAction SilentlyContinue
    if ($make) {
        Push-Location $Root
        try { & $make.Source @Arguments } finally { Pop-Location }
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        return
    }
    $wsl = Get-Command wsl.exe -ErrorAction SilentlyContinue
    if (-not $wsl) {
        throw 'No se encontró make.exe ni wsl.exe. Ejecuta tools\setup\install-deps.bat.'
    }
    $linuxRoot = (& $wsl.Source wslpath -a $Root).Trim()
    $commandLine = 'make ' + (($Arguments | ForEach-Object { "'" + ($_ -replace "'", "'\\''") + "'" }) -join ' ')
    & $wsl.Source --cd $linuxRoot -- bash -lc $commandLine
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Command) {
    'check' {
        & (Join-Path $Root 'tools\setup\install-deps.bat') --check
        Invoke-Python @((Join-Path $Root 'scripts\linux\check-layout.py'))
    }
    'build' { Invoke-Make @('all') }
    'iso' { Invoke-Make @('PYTHON=python3', 'echo-iso') }
    'qemu' { Invoke-Make @('PYTHON=python3', 'run') }
    'virtualbox' {
        & (Join-Path $PSScriptRoot 'run-virtualbox.ps1') -Build:$Build
    }
    'clean' { Invoke-Make @('clean') }
}
