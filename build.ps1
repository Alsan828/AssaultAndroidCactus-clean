param([Parameter(Mandatory = $true)][string]$EngineDir)
$ErrorActionPreference = 'Stop'
$projectFile = Join-Path $PSScriptRoot 'AssaultAndroidCactus.uproject'
$buildTool = Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $buildTool -PathType Leaf)) { throw 'UE Build.bat was not found. Check -EngineDir.' }
& $buildTool -projectfiles "-Project=$projectFile" -game -engine -progress
if ($LASTEXITCODE -ne 0) { throw 'Project file generation failed.' }
& $buildTool AssaultAndroidCactusEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Editor build failed.' }
