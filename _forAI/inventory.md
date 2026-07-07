# Inventory

## 목차

- [Repository](#repository)
- [Top-level structure](#top-level-structure)
- [Entrypoints and key modules](#entrypoints-and-key-modules)
- [Build and validation commands](#build-and-validation-commands)
- [Tests](#tests)
- [Notes](#notes)

## Repository

- Name: `baro_unreal`
- Path: `C:\works\ue_prjs\baro_unreal`
- Summary: UE 5.8 C++ 프로젝트. 주차장 CCTV 시뮬레이터. parking_area 주차장 환경 + baro_world 5.8 CCTV 시뮬레이터 C++를 통합 완료(2026-07-01).

## Top-level structure

- `baro_unreal.uproject` — EngineAssociation 5.8. Plugins: ModelingToolsEditorMode, **ModelContextProtocol**·**AllToolsets**(둘 다 `TargetAllowList=Editor` — 게임 패키지에서 제외, 2026-07-06), PCG, Niagara, DatasmithContent, VariantManager, CineCameraSceneCapture, GeometryScripting, **baroCCTVSimulator**(Runtime, CCTV C++ 플러그인). ※ **RYUKoreaBuilidngCreator는 현재 uproject에서 비활성**(sim 레벨 베이크 완료분 기준) — `Plugins/` 소스는 잔존해 컴파일만 됨. 미베이크 원본 레벨(LV_Park_01/04/07_A 등)을 에디터에서 열려면 재활성 필요.
- `Source/baro_unreal/` — 게임 모듈. CCTV 시뮬 13파일 이식됨: `HucomsServerSubsystem`, `HucomsProtocol`, `PTZCamera`, `PTZCaptureComponent`, `PTZPlayerController`, `MjpegStreamServer`, `CenteringClientComponent` (+ 기본 `baro_unreal.*`).
- `Source/*.Target.cs` — V7 / Unreal5_8.
- `Content/` — parking_area에서 이관한 30GB 환경(주차장 레벨 `Levels/LV_Park_01~08`, 대형 에셋팩 BlackAlder/Fab/Cars/Road_Creator_Pro/UltraDynamicSky 등) + 기본 `test01`.
- `Plugins/RYUKoreaBuilidngCreator/` — 한국 건물팩(콘텐츠 2.6GB + Runtime C++ 모듈, 5.8 재빌드됨).
- `Config/` + `Saved/Config/.../EditorPerProjectUserSettings.ini`(MCP 자동시작).

## Entrypoints and key modules

- 서버 자동기동: `UHucomsServerSubsystem`(UTickableWorldSubsystem) — `OnWorldBeginPlay`에서 Hucoms HTTP CGI 라우터 기동(Game/PIE), 기본 포트 8081. Tick에서 PTZ 모터 슬루 + 카메라 미러 + MJPEG 캡처.
- `APTZCamera` — Pan/Tilt/Zoom 짐벌 액터(LV_Park_01에 `PTZ_Cam_01~04` 배치됨). `UPTZCaptureComponent`가 SceneCapture→JPEG.
- `UCenteringClientComponent` — baro_vla 추론 서버 관측 클라이언트(번호판 검출).
- 상세 프로토콜/단위/기하는 baro_world 5.8 `_forAI/memo.md` 참고(동일 계약).

## Build and validation commands

- C++ 빌드(에디터 타깃):
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" `
    baro_unrealEditor Win64 Development -Project="C:\works\ue_prjs\baro_unreal\baro_unreal.uproject" -WaitMutex
  ```
- 에디터 실행: `.uproject`를 UnrealEditor 5.8로 열기(MCP :8000 자동 기동).
- **패키징(sim_01 단독, 한방 배포)** — 에디터를 반드시 닫고 실행:
  ```powershell
  ./Scripts/package.ps1                  # Win64 Development, sim_01만 쿡 → Packaged/Win64 (~3.4GB)
  ./Scripts/package.ps1 -Config Shipping # 배포용 최적화(심볼 제외)
  ./Scripts/package.ps1 -Platform Linux  # 우분투: 툴체인 v26_clang-20.1.8-rockylinux8 설치 후
  ```
  (RunUAT BuildCookRun 래퍼. `-map`으로 맵 격리, MCP 플러그인은 게임 타깃 제외. 상세는 dev_log 2026-07-06 / 전역 메모리 `baro-unreal-packaging-cli`.)
- **패키지 실행 검증**: `Packaged/Win64/baro_unreal.exe` 실행 → 로그 `[Hucoms] 시뮬레이터 서버 시작 — 채널 N/N` 확인. 실 게임/소켓은 **자식 프로세스**(`baro_unreal/Binaries/Win64/baro_unreal.exe`)가 소유하므로 **포트 기준**으로 확인:
  ```powershell
  Get-NetTCPConnection -State Listen -LocalPort 8081,8082,8091,8092
  ```
  제어 = `BaseHttpPort 8081`+카메라인덱스(`127.0.0.1` 바인딩), MJPEG = `BaseMjpegPort 8091`+인덱스(`0.0.0.0`). sim_01 = 카메라 2대 → 8081/8082, 8091/8092.
- C++ 에디터 빌드(코드 변경 시): 위 Build.bat `baro_unrealEditor Win64 Development`. 새 UCLASS/UPROPERTY는 Live Coding 불가 → 에디터 닫고 CLI 풀 리빌드.

## Tests

- TODO: 현재 없음. 이식 후 검증 방식 정의.

## Notes

- Target.cs는 신규 5.8 프로젝트라 V7/Unreal5_8 기본값일 것(확인 필요). baro_world 5.8 이식 시 Build.cs 의존성
  (HTTP/Json/HTTPServer/Sockets/Networking/EnhancedInput) 반영 필요 — `plan.md` 참조.
- 상세 항목(엔트리포인트/테스트)은 이식 진행에 따라 채운다.
