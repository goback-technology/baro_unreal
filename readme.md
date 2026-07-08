# baro_unreal

주차장 CCTV 시뮬레이터 — Unreal Engine 5.8 / C++. baro_calory(Node.js 관제 시스템)의
Hucoms PTZ 카메라를 가상으로 재현해, 실기 없이 온보딩·주차면 발견·번호판 호밍을 개발/검증한다.

## 목차

- [개요](#개요)
- [기술 스택](#기술-스택)
- [저장소 전략 (code-only)](#저장소-전략-code-only)
- [클론 후 셋업 — 에셋·플러그인 재취득](#클론-후-셋업--에셋플러그인-재취득)
- [빌드 / 실행](#빌드--실행)
- [관련 문서](#관련-문서)

## 개요

- 실제 Hucoms 카메라 프로토콜(CGI setcenter / MJPEG / PTZ status)을 UE 내에서 서버로 재현.
- `APTZCamera`(팬/틸트/줌 짐벌) + `PTZCaptureComponent`(SceneCapture→JPEG) + `HucomsServerSubsystem`(HTTP/MJPEG 서버).
- baro_calory 웹 UI가 이 시뮬레이터를 실기와 동일한 API로 제어(포트 8081~8084).

## 기술 스택

- Unreal Engine **5.8**, C++ (`Source/baro_unreal/`)
- 모듈 의존: HTTP / Json / HTTPServer / Sockets / Networking (`baro_unreal.Build.cs`)
- 상용 에셋: BlackAlder, Fab, CityKitBR, Road Creator Pro, UltraDynamicSky, RYU Korean Building Creator 등

## 저장소 전략 (code-only)

이 저장소는 **소스코드 · Config · 문서만** 버전관리한다. 이유:

- `Content/` 는 **약 30GB** 로 대부분 **상용 마켓플레이스/외부 임포트 에셋**이다 → 재배포 불가(라이선스) + git·GitHub 부적합(파일 100MB 하드리밋, 무료 LFS 1GB).
- `Plugins/RYUKoreaBuilidngCreator` 는 상용 플러그인(2.6GB) 이다.

따라서 `.gitignore` 로 `Content/`, `Plugins/`, 그리고 모든 빌드 산출물/캐시(`Binaries` `Intermediate` `Saved` `DerivedDataCache` `.vs` 등)를 제외한다.
에셋까지 버전관리가 필요해지면 **Perforce** 또는 **자체호스팅 Git LFS** 가 UE 표준 경로다.

## 클론 후 셋업 — 에셋·플러그인 재취득

> ⚠️ 신규 클론에는 `Content/`·`Plugins/` 가 **없다**. 복구 전에는 에디터가 정상적으로 열리지 않는다.

1. **에셋 복구**: `Content/` 를 백업(또는 원본 마켓플레이스/외부 프로젝트)에서 복사해 넣는다.
   상용 에셋(BlackAlder, CityKit, Road Creator Pro, UltraDynamicSky 등)은 Fab/Epic 계정으로 재취득.
2. **플러그인 복구**: `Plugins/RYUKoreaBuilidngCreator/` 를 배치하고, 5.8 미대응 시 소스에서 재컴파일.
3. C++ 빌드(아래) → `.uproject` 실행.

## 빌드 / 실행

Windows 기준 상세 절차는 [`docs/windows_build_run.md`](docs/windows_build_run.md)를 먼저 보면 된다.
아래는 자주 쓰는 빠른 명령만 모은 것이다.

### 처음 한 번만

1. Unreal Engine **5.8** 설치.
2. Visual Studio 2022에서 **Game development with C++** 워크로드 설치.
3. 신규 클론이면 `Content/`와 필요한 상용 에셋을 백업/원본에서 복구.
4. 플러그인 서브모듈 받기:

```powershell
git submodule update --init --recursive
```

5. 로컬 환경 파일 만들기:

```powershell
Copy-Item .env.example .env
notepad .env
```

`UE_PATH`를 자기 PC의 Unreal Engine 5.8 설치 경로로 맞춘다.
`.env`는 PC마다 달라지는 파일이라 Git에 올리지 않는다.

### 프로젝트 파일 생성

탐색기에서 `baro_unreal.uproject` 우클릭 → **Generate Visual Studio project files**.

우클릭 메뉴가 없으면 PowerShell에서:

```powershell
& "C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe" /projectfiles "C:\works\ue_prjs\baro_unreal\baro_unreal.uproject"
```

### 에디터 빌드

```powershell
.\Scripts\build.ps1
```

빌드가 끝나면 `baro_unreal.uproject`를 열거나, 생성된 `baro_unreal.sln`에서
`baro_unrealEditor` / `Development Editor` / `Win64`로 빌드해도 된다.

### 실행

- 에디터에서 확인: `baro_unreal.uproject` 열기 → 상단 **Play** 버튼.
- CCTV 서버처럼 실행:

```powershell
.\Scripts\run.ps1
```

기본 실행 맵은 `/Game/simulator/LV_Park_sim_01`이다. standalone 실행 창이 검게 보여도 정상이다.
메인 화면 렌더를 줄이고 CCTV 캡처/HTTP 서버를 돌리는 용도라서, 카메라 영상은 별도 SceneCapture로 나온다.

### 패키지 만들기

```powershell
.\Scripts\package.ps1
```

결과물은 `Packaged/Win64/baro_unreal.exe`에 생성된다.

### 실행 확인

```powershell
Get-NetTCPConnection -State Listen -LocalPort 8081,8082,8083,8084,8091,8092,8093,8094

Invoke-WebRequest `
  "http://127.0.0.1:8081/cgi-bin/control/ptzf_status.cgi?action=getptzfpos" `
  -UseBasicParsing |
  Select-Object -ExpandProperty Content
```

HTTP 포트는 보통 `8081~8084`, MJPEG 포트는 보통 `8091~8094`를 쓴다.
실제로 열리는 포트 수는 실행한 레벨의 CCTV 카메라 수에 따라 달라진다.

## 관련 문서

- 개발 이력·설계 결정·교훈: [`_forAI/`](_forAI/) (`dev_log.md`, `plan.md`, `inventory.md`, `memo.md`).
- PTZ 좌표·부호 규약(진실의 출처)·SceneCapture 화질 교훈은 `_forAI/dev_log.md` 참조.
- Windows 빌드/실행 상세 가이드: [`docs/windows_build_run.md`](docs/windows_build_run.md).
- 씬 제어 API(`/scene/*`) 레퍼런스 — 주차면·차량·카메라 파라미터 런타임 제어 + 3D→2D 오버레이 투영:
  [`docs/scene-control-api.md`](docs/scene-control-api.md).
