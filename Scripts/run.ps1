<#
.SYNOPSIS
  Run baro_unreal by using the local .env settings.

.EXAMPLE
  ./Scripts/run.ps1

.EXAMPLE
  ./Scripts/run.ps1 -Mode Editor

.EXAMPLE
  ./Scripts/run.ps1 -Mode Packaged

.EXAMPLE
  ./Scripts/run.ps1 -WaitForExit
#>
param(
    [ValidateSet("Game", "Editor", "Packaged")] [string]$Mode = "Game",
    [string]$Map = "",
    [int]$ResX = 0,
    [int]$ResY = 0,
    [int]$ScenePort = 0,
    [switch]$Fullscreen,
    [switch]$Log,
    [switch]$WaitForExit,
    [string[]]$ExtraArgs = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

if ($ResX -le 0) { $ResX = [int](Get-BaroSetting -Name "RUN_RESX" -Default "960") }
if ($ResY -le 0) { $ResY = [int](Get-BaroSetting -Name "RUN_RESY" -Default "540") }
if ([string]::IsNullOrWhiteSpace($Map)) { $Map = $BaroDefaultMap }

$editorExe = Join-Path $BaroEngine "Engine\Binaries\Win64\UnrealEditor.exe"
$packagedExe = Get-BaroSetting -Name "PACKAGED_EXE" -Default (Join-Path $BaroRoot "Packaged\Win64\baro_unreal.exe")

if ($Mode -eq "Packaged") {
    Assert-BaroFile -Path $packagedExe -Message "패키지 실행 파일 없음. 먼저 ./Scripts/package.ps1 을 실행하세요"
    $exe = $packagedExe
    $args = @()
}
else {
    Assert-BaroFile -Path $editorExe -Message "UnrealEditor.exe 없음. .env의 UE_PATH를 확인하세요"
    Assert-BaroFile -Path $BaroProject -Message "uproject 없음. PROJECT_FILE 또는 프로젝트 경로를 확인하세요"
    $exe = $editorExe
    $args = @($BaroProject)
}

if ($Mode -eq "Game") {
    if (-not [string]::IsNullOrWhiteSpace($Map)) {
        $args += $Map
    }
    $args += "-game"
}
elseif ($Mode -eq "Editor") {
    if (-not [string]::IsNullOrWhiteSpace($Map)) {
        $args += $Map
    }
}

$windowed = (-not $Fullscreen) -and (Get-BaroBoolSetting -Name "RUN_WINDOWED" -Default $true)
if ($windowed) {
    $args += @("-windowed", "-ResX=$ResX", "-ResY=$ResY")
}

if ($Log) {
    $args += "-log"
}

# 씬 제어 포트 지정(플러그인 v0.1.16~ 커맨드라인 스위치 — 다중 인스턴스 분리용).
# Base 포트(-BaseHttpPort/-BaseMjpegPort)가 필요하면 -ExtraArgs 로 넘긴다.
if ($ScenePort -gt 0) {
    $args += "-ScenePort=$ScenePort"
}

if ($ExtraArgs.Count -gt 0) {
    $args += $ExtraArgs
}

Write-Host "==================== baro_unreal 실행 ====================" -ForegroundColor Cyan
Write-Host (" Mode   : {0}" -f $Mode)
Write-Host (" Exe    : {0}" -f $exe)
if ($Mode -ne "Packaged") { Write-Host (" Project: {0}" -f $BaroProject) }
Write-Host (" Map    : {0}" -f $Map)
Write-Host (" Window : {0}x{1} {2}" -f $ResX, $ResY, $(if ($windowed) { "windowed" } else { "fullscreen/default" }))
Write-Host (" Wait   : {0}" -f ($(if ($WaitForExit) { "yes" } else { "no" })))
Write-Host "==========================================================" -ForegroundColor Cyan

$processArgs = @{
    FilePath = $exe
    ArgumentList = $args
    WorkingDirectory = $BaroRoot
    PassThru = $true
}

if ($WaitForExit) {
    $processArgs.Wait = $true
}

$process = Start-Process @processArgs

if ($WaitForExit) {
    $code = $process.ExitCode
    if ($null -ne $code -and $code -ne 0) {
        throw "실행 실패 (exit $code) — 위 로그를 확인하세요."
    }
}
else {
    Write-Host ("실행 시작됨 (PID {0})" -f $process.Id) -ForegroundColor Green
}
