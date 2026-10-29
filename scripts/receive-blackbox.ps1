param(
    [string]$Port = '',
    [string]$OutputDir = '',
    [switch]$ListPorts
)
$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
$serialDeps = Join-Path $repoPath 'build/serial-deps'
$pythonCommand = Get-Command python -ErrorAction SilentlyContinue
if (!$pythonCommand) { throw 'Python 3 is required. Install Python, then reopen the terminal.' }
$pythonPath = $pythonCommand.Source
# Local dependencies only. No global Python package/config changes.
$envCheck = 'import sys, importlib.util; sys.path.insert(0, sys.argv[1]); m = __import__(''serial'') if importlib.util.find_spec(''serial'') else None; sys.exit(0 if getattr(m, ''VERSION'', None) == ''3.5'' else 1)'
& $pythonPath -c $envCheck $serialDeps 2>$null
if ($LASTEXITCODE -ne 0) {
    New-Item -ItemType Directory -Force -Path $serialDeps | Out-Null
    Write-Host 'Installing pySerial 3.5 into build/serial-deps (first run)...'
    & $pythonPath -m pip install --disable-pip-version-check --upgrade --target $serialDeps -r (Join-Path $PSScriptRoot 'requirements-serial.txt')
    if ($LASTEXITCODE -ne 0) { throw 'pySerial installation failed. Check internet/pip and retry.' }
}
$receiverArgs = @((Join-Path $PSScriptRoot 'receive-blackbox.py'))
if ($Port) { $receiverArgs += @('--port', $Port) }
if ($OutputDir) { $receiverArgs += @('--out-dir', $OutputDir) }
if ($ListPorts) { $receiverArgs += '--list' }
& $pythonPath @receiverArgs
exit $LASTEXITCODE
