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
