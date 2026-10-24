param([string]$CompilerBin='C:\Keil_v5\ARM\ARMCC\bin')
$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$buildPath=Join-Path $repoPath 'build'
New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
Push-Location $repoPath
try {
    [xml]$project=Get-Content -LiteralPath 'linefollower.uvprojx' -Raw
    $objects=@()
    $includes=@('app','bsp','config','vendor/Device/Include','vendor/CMSIS/Include','vendor/SPL/inc')
    $includeArgs=@()
    foreach($include in $includes) { $includeArgs+=@('-I',$include) }
    foreach($source in $project.Project.Targets.Target.Groups.Group.Files.File) {
        $inputPath=[string]$source.FilePath
        $object=Join-Path $buildPath (([IO.Path]::GetFileNameWithoutExtension($inputPath))+'.o')
        if($inputPath.EndsWith('.s')) {
            & "$CompilerBin\armasm.exe" --cpu Cortex-M3 -g --pd '__MICROLIB SETA 1' $inputPath -o $object
        } else {
            & "$CompilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=8000000 -D__MICROLIB @includeArgs -c $inputPath -o $object
        }
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $inputPath" }
        $objects+=$object
    }
    $scatter=Join-Path $buildPath 'linefollower.sct'
    $scatterText=@('LR_IROM1 0x08000000 0x00010000 {','ER_IROM1 0x08000000 0x00010000 {','*.o (RESET, +First)','*(InRoot$$Sections)','.ANY (+RO)','.ANY (+XO)','}','RW_IRAM1 0x20000000 0x00005000 {','.ANY (+RW +ZI)','}','}') -join "`n"
    [IO.File]::WriteAllText($scatter,$scatterText,[Text.UTF8Encoding]::new($false))
    $axf=Join-Path $buildPath 'linefollower.axf'
    & "$CompilerBin\armlink.exe" --cpu Cortex-M3 --library_type=microlib --strict --scatter $scatter --libpath (Join-Path $CompilerBin '..\lib') --info summarysizes --map --xref --callgraph --list (Join-Path $buildPath 'linefollower.map') --output $axf @objects
    if($LASTEXITCODE -ne 0) { throw 'Firmware link failed' }
    $hex=Join-Path $buildPath 'linefollower.hex'
    & "$CompilerBin\fromelf.exe" --i32combined --output $hex $axf
    if($LASTEXITCODE -ne 0) { throw 'HEX generation failed' }
    $tag=[regex]::Match((Get-Content config/board.h -Raw),'#define BOARD_FIRMWARE_TAG "([A-Za-z0-9_-]+)"').Groups[1].Value
    if(!$tag) { throw 'Missing firmware tag' }
    $versioned=Join-Path $buildPath ("linefollower-{0}.hex" -f $tag)
    Copy-Item -LiteralPath $hex -Destination $versioned -Force
    Write-Output "BUILD PASS: $versioned"
    Get-FileHash -LiteralPath $versioned -Algorithm SHA256 | Format-List
} finally { Pop-Location }
