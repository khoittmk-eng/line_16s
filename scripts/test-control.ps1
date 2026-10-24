$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
$compilerBin = 'C:\Keil_v5\ARM\ARMCC\bin'
$testPath = Join-Path $repoPath 'build\control-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    # Keep the expanded soft-float suite within the emulator budget; firmware uses its own Keil options.
    foreach ($source in @('app/control.c','app/line_estimator.c','tests/test_control.c','app/robot.c','tests/test_robot.c','app/menu.c','tests/test_menu.c','tests/test_recovery.c','tests/test_speed.c')) {
        $object = Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source)) + '.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -Dsnprintf=TestSnprintf -I tests/fakes -I app -c $source -o $object
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $motorIncludes=@('-I','config','-I','vendor/Device/Include','-I','vendor/CMSIS/Include','-I','vendor/SPL/inc')
    $motorAliases=@('-DMotor_Init=DriverMotor_Init','-DMotor_SetDuty=DriverMotor_SetDuty','-DMotor_StopBrake=DriverMotor_StopBrake','-DMotor_StopCoast=DriverMotor_StopCoast','-DMotor_SafetyTick=DriverMotor_SafetyTick')
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes @motorAliases -c bsp/motor.c -o "$testPath\driver_motor.o"
    if ($LASTEXITCODE -ne 0) { throw 'Motor driver test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -I tests/fakes -c tests/test_motor.c -o "$testPath\test_motor.o"
    if ($LASTEXITCODE -ne 0) { throw 'Motor register test compile failed' }
    $axf = Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\control.o" "$testPath\line_estimator.o" "$testPath\test_control.o" "$testPath\robot.o" "$testPath\test_robot.o" "$testPath\menu.o" "$testPath\test_menu.o" "$testPath\test_recovery.o" "$testPath\test_speed.o" "$testPath\driver_motor.o" "$testPath\test_motor.o"
    if ($LASTEXITCODE -ne 0) { throw 'Test link failed' }
    python scripts/emulate-control.py $axf
    if ($LASTEXITCODE -ne 0) { throw 'Control tests failed' }
} finally { Pop-Location }
