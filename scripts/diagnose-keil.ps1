param([string]$KeilRoot = 'C:\Keil_v5')
$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
[xml]$project = Get-Content -LiteralPath (Join-Path $repoPath 'linefollower.uvprojx') -Raw
[xml]$options = Get-Content -LiteralPath (Join-Path $repoPath 'linefollower.uvoptx') -Raw
$target = $project.Project.Targets.Target | Where-Object { $_.TargetName -eq 'LineFollower' }
$debug = $options.ProjectOpt.Target | Where-Object { $_.TargetName -eq 'LineFollower' }
if (!$target -or !$debug) { throw 'LineFollower target/options not found.' }
$driver = [string]$debug.TargetOption.DebugOpt.pMon
$shared = [string]$target.TargetOption.Utilities.bUseTDR
Write-Output ('Device: {0}; debug driver: {1}' -f $target.TargetOption.TargetCommonOption.Device,$driver)
Write-Output ('Use Debug Driver for Flash: {0}' -f ($shared -eq '1'))
if ($driver -notmatch '^STLink\\ST-LINKIII-KEIL_SWO\.dll$') {
    Write-Output 'WARN: select ST-Link Debugger in Options for Target > Debug.'
}
if ($shared -ne '1') {
    Write-Output 'WARN: enable Use Debug Driver in Options for Target > Utilities.'
}
$driverPath = Join-Path (Join-Path $KeilRoot 'ARM') $driver
if (Test-Path -LiteralPath $driverPath) {
    Write-Output ('Driver DLL exists: {0}' -f $driverPath)
} else { Write-Output ('WARN: driver DLL missing: {0}' -f $driverPath) }
$stEntry = @($debug.TargetOption.TargetDriverDllRegistry.SetRegEntry |
    Where-Object { $_.Key -eq 'ST-LINKIII-KEIL_SWO' })
if (!$stEntry.Count) {
    Write-Output 'INFO: no saved ST-Link-specific Settings entry. Open Debug > Settings and confirm SWD/clock/reset/Flash Download on this PC.'
} else { Write-Output 'ST-Link-specific Settings entry exists; verify its reset mode against actual NRST wiring.' }
$packID = [string]$target.TargetOption.TargetCommonOption.PackID
$parts = $packID.Split('.')
if ($parts.Length -ge 3) {
    $packRelative = Join-Path (Join-Path $parts[0] $parts[1]) ($parts[2..($parts.Length-1)] -join '.')
    $packRoots = @((Join-Path $env:LOCALAPPDATA 'Arm\Packs'), (Join-Path $KeilRoot 'ARM\PACK'))
    $found = @($packRoots | ForEach-Object { Join-Path (Join-Path $_ $packRelative) 'Flash\STM32F10x_128.FLM' } |
        Where-Object { Test-Path -LiteralPath $_ })
    if ($found.Count) { Write-Output ('F1 Flash algorithm exists: {0}' -f $found[0]) }
    else { Write-Output ('WARN: STM32F10x_128.FLM not found for {0}; check Pack Installer.' -f $packID) }
}
$firmware = Join-Path $repoPath 'build\linefollower.hex'
if (Test-Path -LiteralPath $firmware) {
    python (Join-Path $repoPath 'scripts\check-firmware-layout.py') $firmware
    if ($LASTEXITCODE -ne 0) { throw 'Firmware layout check failed.' }
}
Write-Output 'Read-only check finished. It does not connect, reset, erase, flash, or change option bytes.'
Write-Output 'Wiring: DIO->PA13, CLK->PA14, common GND; NRST is additionally required for Connect under Reset.'
Write-Output 'See docs/keil-download.md for the first-download recovery and Settings checklist.'
