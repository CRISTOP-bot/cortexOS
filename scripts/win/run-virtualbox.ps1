[CmdletBinding()]
param(
    [ValidateSet('start', 'create', 'stop', 'status', 'delete')]
    [string]$Action = 'start',
    [string]$VmName = 'NucleOS',
    [string]$IsoPath = '',
    [int]$MemoryMB = 1024,
    [int]$CPUs = 2,
    [switch]$Build,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ([string]::IsNullOrWhiteSpace($IsoPath)) {
    $IsoPath = Join-Path $Root 'dist\os.iso'
}
$IsoPath = [IO.Path]::GetFullPath($IsoPath)

function Find-VBoxManage {
    $command = Get-Command VBoxManage.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $candidates = @(
        "$env:ProgramFiles\Oracle\VirtualBox\VBoxManage.exe",
        "${env:ProgramFiles(x86)}\Oracle\VirtualBox\VBoxManage.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }
    throw 'VBoxManage.exe no está disponible. Instala Oracle VirtualBox y agrega su carpeta al PATH.'
}

$VBoxManage = Find-VBoxManage
function VBox([string[]]$Arguments) {
    & $VBoxManage @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "VBoxManage falló con código $LASTEXITCODE: $($Arguments -join ' ')"
    }
}

function VmExists {
    & $VBoxManage showvminfo $VmName --machinereadable 2>$null | Out-Null
    return ($LASTEXITCODE -eq 0)
}

function Build-Iso {
    if (Test-Path $IsoPath) { return }
    if ($Build) {
        $make = Get-Command make.exe -ErrorAction SilentlyContinue
        if ($make) {
            Push-Location $Root
            try { & $make.Source PYTHON=python3 echo-iso } finally { Pop-Location }
            if ($LASTEXITCODE -eq 0 -and (Test-Path $IsoPath)) { return }
        }
        $wsl = Get-Command wsl.exe -ErrorAction SilentlyContinue
        if ($wsl) {
            $linuxRoot = (& $wsl.Source wslpath -a $Root).Trim()
            & $wsl.Source --cd $linuxRoot -- bash -lc 'make PYTHON=python3 echo-iso'
            if ($LASTEXITCODE -eq 0 -and (Test-Path $IsoPath)) { return }
        }
    }
    throw "No existe $IsoPath. Ejecuta scripts\win\nucleos.bat iso o usa -Build con MSYS2/WSL."
}

function Create-Vm {
    if (VmExists) { return }
    VBox @('createvm', '--name', $VmName, '--ostype', 'Other_64', '--register')
    VBox @('modifyvm', $VmName, '--memory', "$MemoryMB", '--cpus', "$CPUs", '--firmware', 'bios', '--boot1', 'dvd', '--boot2', 'disk', '--boot3', 'none', '--nic1', 'nat', '--audio-driver', 'none')
    VBox @('storagectl', $VmName, '--name', 'NucleOS SATA', '--add', 'sata', '--controller', 'IntelAhci')
}

function Attach-Iso {
    VBox @('storageattach', $VmName, '--storagectl', 'NucleOS SATA', '--port', '0', '--device', '0', '--type', 'dvddrive', '--medium', $IsoPath)
}

switch ($Action) {
    'status' {
        if (VmExists) { VBox @('showvminfo', $VmName, '--machinereadable') } else { Write-Host "VM '$VmName' no existe." }
    }
    'create' {
        Create-Vm
        Write-Host "VM '$VmName' creada."
    }
    'start' {
        Build-Iso
        Create-Vm
        Attach-Iso
        VBox @('startvm', $VmName, '--type', 'gui')
    }
    'stop' {
        if (VmExists) { VBox @('controlvm', $VmName, 'acpipowerbutton') }
    }
    'delete' {
        if (-not $Force) { throw 'Borrar una VM requiere -Force.' }
        if (VmExists) { VBox @('unregistervm', $VmName, '--delete') }
    }
}
