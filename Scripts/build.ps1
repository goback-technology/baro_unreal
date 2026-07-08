<#
.SYNOPSIS
  Build baro_unreal by using the local .env settings.

.EXAMPLE
  ./Scripts/build.ps1

.EXAMPLE
  ./Scripts/build.ps1 -Target Game -Config Development
#>
param(
    [ValidateSet("Editor", "Game")] [string]$Target = "Editor",
    [ValidateSet("Win64", "Linux")] [string]$Platform = "Win64",
    [ValidateSet("Debug", "DebugGame", "Development", "Shipping")] [string]$Config = "Development",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

if ($Target -eq "Editor" -and $Config -eq "Shipping") {
    throw "Editor target은 Shipping으로 빌드하지 않습니다. Development 또는 DebugGame을 사용하세요."
}

$buildBat = Join-Path $BaroEngine "Engine\Build\BatchFiles\Build.bat"
$targetName = if ($Target -eq "Editor") { "baro_unrealEditor" } else { "baro_unreal" }

Assert-BaroFile -Path $buildBat -Message "Build.bat 없음. .env의 UE_PATH를 확인하세요"
Assert-BaroFile -Path $BaroProject -Message "uproject 없음. PROJECT_FILE 또는 프로젝트 경로를 확인하세요"

Write-Host "==================== baro_unreal 빌드 ====================" -ForegroundColor Cyan
Write-Host (" UE_PATH : {0}" -f $BaroEngine)
Write-Host (" Project : {0}" -f $BaroProject)
Write-Host (" Target  : {0}" -f $targetName)
Write-Host (" Platform: {0}" -f $Platform)
Write-Host (" Config  : {0}" -f $Config)
Write-Host "==========================================================" -ForegroundColor Cyan

$buildArgs = @(
    $targetName,
    $Platform,
    $Config,
    "-Project=$BaroProject",
    "-WaitMutex"
)

if ($Clean) {
    $buildArgs += "-Clean"
}

& $buildBat @buildArgs
$code = $LASTEXITCODE
if ($code -ne 0) {
    throw "빌드 실패 (exit $code) — 위 로그를 확인하세요."
}

Write-Host "==================== 빌드 완료 ====================" -ForegroundColor Green
