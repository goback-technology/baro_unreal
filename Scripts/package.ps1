<#
.SYNOPSIS
  baro_unreal CCTV 시뮬레이터 한방 패키징 — 지정 맵만 쿡(cook)해 최소 용량으로 빌드.

.DESCRIPTION
  RunUAT BuildCookRun 으로 [지정 맵만] 쿡 + 빌드 + 스테이지 + pak + 아카이브.
  Windows 전용이며 기본은 sim_01 단독(Win64 / Development). sim_02·sim_03 은 쿡에서 제외되어 용량·쿡시간이 최소.
  MCP/모델링 계열 플러그인은 uproject TargetAllowList=Editor 로 게임 타깃에서 이미 제외됨.

  ※ 실행 전 반드시 언리얼 에디터를 닫으세요 (파일 락 / DDC 충돌 방지).

.PARAMETER Config
  Development (기본, 로그/콘솔/통계 가능) 또는 Shipping (최적화·최소용량·심볼제외).

.PARAMETER Map
  쿡할 맵(가상경로). 여러 개면 + 로 연결. 기본 = /Game/simulator/LV_Park_sim_01

.PARAMETER Clean
  이전 쿡/빌드 산출물을 지우고 처음부터 (문제 진단 시).

.EXAMPLE
  ./Scripts/package.ps1                          # Win64 Development, sim_01
.EXAMPLE
  ./Scripts/package.ps1 -Config Shipping         # Win64 Shipping (배포용)
#>
param(
    [ValidateSet("Development", "Shipping")] [string]$Config = "Development",
    [string]$Map = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# ---- 경로 ----
. (Join-Path $PSScriptRoot "common.ps1")

if ([string]::IsNullOrWhiteSpace($Map)) {
    $Map = $BaroDefaultMap
}

$Engine  = $BaroEngine
$UAT     = Join-Path $Engine "Engine\Build\BatchFiles\RunUAT.bat"
$Root    = $BaroRoot
$Project = $BaroProject
$Platform = "Win64"
$PackagedRoot = [IO.Path]::GetFullPath((Join-Path $Root "Packaged"))
$Archive = [IO.Path]::GetFullPath((Join-Path $PackagedRoot $Platform))
$ExpectedArchive = [IO.Path]::GetFullPath((Join-Path $Root "Packaged\Win64"))

if ($Archive -ne $ExpectedArchive) {
    throw "안전하지 않은 아카이브 경로입니다: $Archive"
}

Assert-BaroFile -Path $UAT -Message "RunUAT 없음. .env의 UE_PATH를 확인하세요"
Assert-BaroFile -Path $Project -Message "uproject 없음. PROJECT_FILE 또는 프로젝트 경로를 확인하세요"

# ---- 심볼: Shipping 은 pdb 제외해 슬림, Development 는 크래시 콜스택용으로 유지 ----
$noDebug = ($Config -eq "Shipping")

Write-Host "==================== baro_unreal 패키징 ====================" -ForegroundColor Cyan
Write-Host (" UE_PATH  : {0}" -f $Engine)
Write-Host (" Project  : {0}" -f $Project)
Write-Host (" Platform : {0}" -f $Platform)
Write-Host (" Config   : {0}" -f $Config)
Write-Host (" Map      : {0}" -f $Map)
Write-Host (" Archive  : {0}" -f $Archive)
Write-Host (" DebugSym : {0}" -f ($(if ($noDebug) { "제외" } else { "포함" })))
Write-Host " * 에디터가 열려 있으면 지금 닫으세요 (파일락/DDC 충돌)" -ForegroundColor Yellow
Write-Host "============================================================" -ForegroundColor Cyan

$uatArgs = @(
    "BuildCookRun"
    "-project=$Project"
    "-noP4"
    "-platform=$Platform"
    "-clientconfig=$Config"
    "-cook"
    "-map=$Map"
    "-build"
    "-stage"
    "-pak"
    "-archive"
    "-archivedirectory=$Archive"
    "-utf8output"
)
if ($noDebug) { $uatArgs += "-nodebuginfo" }
if ($Clean)   { $uatArgs += "-clean" }

# 재사용 아카이브에는 실행 후 생성된 Saved/GameUserSettings와 구 바이너리가 남는다.
# 검증된 Win64 출력 경로만 비워, 배포본이 항상 깨끗한 상태에서 생성되게 한다.
if (Test-Path -LiteralPath $Archive) {
    Write-Host (" 기존 아카이브 정리: {0}" -f $Archive) -ForegroundColor Yellow
    Remove-Item -LiteralPath $Archive -Recurse -Force
}

& $UAT @uatArgs
$code = $LASTEXITCODE
if ($code -ne 0) { throw "패키징 실패 (exit $code) — 위 로그를 확인하세요." }

Write-Host "==================== 완료 ====================" -ForegroundColor Green
Write-Host (" 산출물: {0}" -f $Archive) -ForegroundColor Green
