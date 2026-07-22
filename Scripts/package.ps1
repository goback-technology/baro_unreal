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

.PARAMETER Zip
  패키징 후 배포용 zip + .sha256 + .info.txt 를 Packaged\ 에 생성한다.
  이름은 [앱 버전](Config\DefaultGame.ini ProjectVersion)에서 자동으로 만든다:
  baro_unreal_sim_v<앱버전>_<yyyyMMdd>.zip
  ※ 2026-07-20 이전 수동 zip 은 [플러그인] 버전으로 이름을 붙여(v0.1.4 = plugin, app 은 0.1.0)
    무엇의 버전인지 알 수 없었다. 배포본을 대표하는 건 앱 버전이므로 여기서 통일한다.

.PARAMETER Force
  -Zip 대상 파일이 이미 있으면 덮어쓴다. 기본은 실패 — 이미 배포한 산출물을 실수로
  덮어쓰지 않게 한다.

.EXAMPLE
  ./Scripts/package.ps1                          # Win64 Development, sim_01
.EXAMPLE
  ./Scripts/package.ps1 -Config Shipping         # Win64 Shipping (배포용)
.EXAMPLE
  ./Scripts/package.ps1 -Config Shipping -Zip    # 패키징 + 배포 zip 생성
#>
param(
    [ValidateSet("Development", "Shipping")] [string]$Config = "Development",
    [string]$Map = "",
    [switch]$Clean,
    [switch]$Zip,
    [switch]$Force
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
    # CrashReportClient 를 함께 스테이징 — BuildCookRun 은 ini(IncludeCrashReporter)를 읽지 않아
    # 이 플래그가 없으면 크래시 시 "Could not start crash report client"로 덤프가 유실된다(2026-07-17 사고).
    "-CrashReporter"
)
if ($noDebug) { $uatArgs += "-nodebuginfo" }
if ($Clean)   { $uatArgs += "-clean" }

# 재사용 아카이브에는 실행 후 생성된 Saved/GameUserSettings와 구 바이너리가 남는다.
# 검증된 Win64 출력 경로만 비워, 배포본이 항상 깨끗한 상태에서 생성되게 한다.
if (Test-Path -LiteralPath $Archive) {
    Write-Host (" 기존 아카이브 정리: {0}" -f $Archive) -ForegroundColor Yellow
    Remove-Item -LiteralPath $Archive -Recurse -Force
}

# Zen 스토어 경쟁 조건 대비 재시도.
#   zenserver 는 상주 데몬이 아니라 sponsor 프로세스(에디터/쿡)가 전부 사라지면 스스로 종료한다.
#   BuildCookRun 은 쿡을 별도 프로세스로 돌린 뒤 UAT 본체가 oplog 를 HTTP 로 되읽어 스테이징하는데,
#   UAT 는 sponsor 로 등록되지 않는다(sponsor 슬롯은 UE 프로세스만 쓰는 공유메모리). 쿡이 끝나 sponsor 가
#   0 이 되는 순간 zenserver 가 내려가면 UAT 의 읽기가 스트림 중간에 끊긴다:
#     "Failed reading oplog from Zen ... Error while copying content to a stream"
#   쿡 산출물 자체는 Zen 에 온전하므로 재시도하면 캐시 히트로 통과한다(실측 2026-07-10: 재시도 51초 성공).
#   외부에서 sponsor 를 심는 지원 경로가 없어 재시도가 유일한 실용 해법이다.
$uatLog = Join-Path $BaroRoot "Saved\Logs\package-uat.log"
New-Item -ItemType Directory -Force -Path (Split-Path $uatLog) | Out-Null

$maxAttempts = 2
for ($attempt = 1; $attempt -le $maxAttempts; $attempt++) {
    if ($attempt -gt 1) {
        Write-Host (" Zen 경쟁 조건 감지 — 재시도 {0}/{1}" -f $attempt, $maxAttempts) -ForegroundColor Yellow
    }

    & $UAT @uatArgs 2>&1 | Tee-Object -FilePath $uatLog
    $code = $LASTEXITCODE
    if ($code -eq 0) { break }

    # Zen 실패일 때만 재시도한다. 컴파일/쿡 에러를 재시도하면 시간만 버린다.
    $zenRace = Select-String -Path $uatLog -Quiet `
        -Pattern 'Failed reading oplog from Zen', 'Failed to send oplog request to Zen'
    if (-not $zenRace -or $attempt -eq $maxAttempts) {
        throw "패키징 실패 (exit $code) — 위 로그를 확인하세요. (UAT 로그: $uatLog)"
    }
}

Write-Host "==================== 완료 ====================" -ForegroundColor Green
Write-Host (" 산출물: {0}" -f $Archive) -ForegroundColor Green

# ---- 배포 zip (선택) ----
if ($Zip) {
    # 버전은 항상 소스에서 읽는다 — 손으로 이름을 짓지 않는 것이 이 단계의 목적이다.
    $gameIni = Join-Path $Root "Config\DefaultGame.ini"
    Assert-BaroFile -Path $gameIni -Message "DefaultGame.ini 없음 — 앱 버전을 읽을 수 없습니다"
    $appVersion = (Select-String -LiteralPath $gameIni -Pattern '^\s*ProjectVersion\s*=\s*(.+?)\s*$' |
        Select-Object -First 1).Matches.Groups[1].Value
    if ([string]::IsNullOrWhiteSpace($appVersion)) {
        throw "DefaultGame.ini 에서 ProjectVersion 을 찾지 못했습니다 (zip 이름을 만들 수 없음)"
    }

    # 플러그인 버전은 이름에 넣지 않고 .info.txt 에 기록한다(이름은 앱 버전 하나로 단순하게).
    $upluginPath = Join-Path $Root "Plugins\baroCCTVSimulator\baroCCTVSimulator.uplugin"
    $pluginVersion = if (Test-Path -LiteralPath $upluginPath) {
        (Get-Content -LiteralPath $upluginPath -Raw | ConvertFrom-Json).VersionName
    } else { "(unknown)" }

    $stamp   = Get-Date -Format "yyyyMMdd"
    $zipName = "baro_unreal_sim_v{0}_{1}.zip" -f $appVersion, $stamp
    $zipPath = Join-Path $PackagedRoot $zipName

    foreach ($p in @($zipPath, "$zipPath.sha256", "$zipPath.info.txt")) {
        if ((Test-Path -LiteralPath $p) -and -not $Force) {
            throw "이미 존재합니다: $p`n이미 배포한 산출물일 수 있습니다. 덮어쓰려면 -Force 를 주세요."
        }
    }

    # 실행 흔적(로그·크래시·GameUserSettings)이 배포본에 섞이면 안 된다.
    # 패키징 직후엔 없지만, 검증 실행을 한 뒤 zip 을 만드는 흐름이 흔해 방어적으로 지운다.
    $savedInArchive = Join-Path $Archive "baro_unreal\Saved"
    if (Test-Path -LiteralPath $savedInArchive) {
        Write-Host (" 실행 흔적 제거: {0}" -f $savedInArchive) -ForegroundColor Yellow
        Remove-Item -LiteralPath $savedInArchive -Recurse -Force
    }

    Write-Host (" 압축 중: {0}" -f $zipName) -ForegroundColor Cyan
    Write-Host " (3GB+ 라 수 분 걸립니다)"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    foreach ($p in @($zipPath, "$zipPath.sha256", "$zipPath.info.txt")) {
        if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Force }
    }
    # includeBaseDirectory=$false → Win64 폴더 자체가 아니라 그 '내용'이 zip 루트에 온다
    # (baro_unreal/, Engine/, baro_unreal.exe … — 기존 배포 zip 과 동일 구조).
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $Archive, $zipPath, [System.IO.Compression.CompressionLevel]::Optimal, $false)

    $hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLower()
    # sha256sum 바이너리 모드 형식 — 기존 사이드카와 동일하게 유지(검증 도구 호환).
    "$hash *$zipName" | Set-Content -LiteralPath "$zipPath.sha256" -Encoding ascii

    # 무엇이 들어있는 zip 인지 남긴다. 이름만으론 플러그인/커밋을 알 수 없어 과거에 혼동이 있었다.
    # 커밋되지 않은 변경이 섞인 빌드는 SHA 만으론 재현이 안 되므로 -dirty 로 표시한다.
    function Get-BaroCommitLabel([string]$RepoPath) {
        $sha = (& git -C $RepoPath rev-parse --short HEAD 2>$null)
        if ([string]::IsNullOrWhiteSpace($sha)) { return "(git 없음)" }
        # 서브모듈 포인터 변경(' m ')은 부모의 dirty 로 치지 않는다 — 별도 줄로 이미 기록된다.
        $dirty = (& git -C $RepoPath status --porcelain --untracked-files=no 2>$null) |
            Where-Object { $_ -and $_ -notmatch '^ m ' }
        if ($dirty) { return "$sha-dirty" }
        return $sha
    }
    $appCommit    = Get-BaroCommitLabel $Root
    $pluginCommit = Get-BaroCommitLabel (Join-Path $Root "Plugins\baroCCTVSimulator")
    @(
        "baro_unreal 시뮬레이터 배포본"
        "  zip            : $zipName"
        "  앱 버전         : $appVersion   (Config/DefaultGame.ini ProjectVersion)"
        "  플러그인 버전    : $pluginVersion   (baroCCTVSimulator.uplugin VersionName)"
        "  Config         : $Config"
        "  Map            : $Map"
        "  빌드 일시       : {0}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
        "  app commit     : $appCommit"
        "  plugin commit  : $pluginCommit"
        "  sha256         : $hash"
    ) | Set-Content -LiteralPath "$zipPath.info.txt" -Encoding utf8

    $zipGB = [math]::Round((Get-Item -LiteralPath $zipPath).Length / 1GB, 2)
    Write-Host (" 배포 zip : {0}  ({1} GB)" -f $zipPath, $zipGB) -ForegroundColor Green
    Write-Host ("            앱 v{0} / 플러그인 v{1}" -f $appVersion, $pluginVersion) -ForegroundColor Green
}
