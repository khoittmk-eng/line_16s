param([string]$KeilPath = 'C:\Keil_v5\UV4\UV4.exe')
$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
if (!(Test-Path -LiteralPath $KeilPath)) { throw "Keil not found: $KeilPath. Set -KeilPath." }
$buildPath = Join-Path $repoPath 'build'
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
$logPath = Join-Path $buildPath 'keil-build.log'
# Avoid reading a stale successful log if the current invocation fails.
Set-Content -LiteralPath $logPath -Value ''
$projectPath = Join-Path $repoPath 'linefollower.uvprojx'
$buildArgs = '-r "{0}" -t "LineFollower" -o "{1}" -j0' -f $projectPath, $logPath
$process = Start-Process -FilePath $KeilPath -ArgumentList $buildArgs -WorkingDirectory $repoPath -WindowStyle Hidden -PassThru -Wait
$logText = Get-Content -LiteralPath $logPath -Raw
Write-Output $logText
if ($process.ExitCode -gt 1 -or $logText -notmatch '0 Error\(s\)' -or !(Test-Path (Join-Path $buildPath 'linefollower.hex'))) {
    throw "Build failed (Keil exit $($process.ExitCode)). See $logPath"
}

$configText = Get-Content -LiteralPath (Join-Path $repoPath 'config/board.h') -Raw
python (Join-Path $repoPath 'scripts/check-firmware-layout.py') (Join-Path $buildPath 'linefollower.hex')
if ($LASTEXITCODE -ne 0) { throw 'Firmware overlaps reserved black-box Flash or has an invalid HEX layout.' }
$firmwareTag = [regex]::Match($configText, '#define BOARD_FIRMWARE_TAG "([A-Za-z0-9_-]+)"').Groups[1].Value
if ($firmwareTag) {
    $versionedHex = Join-Path $buildPath ("linefollower-{0}.hex" -f $firmwareTag)
    Copy-Item -LiteralPath (Join-Path $buildPath 'linefollower.hex') -Destination $versionedHex -Force
    Write-Output ("Firmware {0}: {1}" -f $firmwareTag, $versionedHex)
    Get-FileHash -LiteralPath $versionedHex -Algorithm SHA256 | Format-List
}
