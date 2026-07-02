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

- `baro_unreal.uproject` — EngineAssociation 5.8. Plugins: ModelingToolsEditorMode, **ModelContextProtocol**, **AllToolsets**, PCG, Niagara, DatasmithContent, VariantManager, CineCameraSceneCapture, GeometryScripting, **RYUKoreaBuilidngCreator**.
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
- 에디터 실행: `.uproject`를 UnrealEditor 5.8로 열기(현재 실행 중, MCP :8000 자동 기동).
- TODO: CCTV 서버 이식 후 검증용 curl/포트 확인 명령 추가(baro_world 5.8의 8081/8082 참고).

## Tests

- TODO: 현재 없음. 이식 후 검증 방식 정의.

## Notes

- Target.cs는 신규 5.8 프로젝트라 V7/Unreal5_8 기본값일 것(확인 필요). baro_world 5.8 이식 시 Build.cs 의존성
  (HTTP/Json/HTTPServer/Sockets/Networking/EnhancedInput) 반영 필요 — `plan.md` 참조.
- 상세 항목(엔트리포인트/테스트)은 이식 진행에 따라 채운다.
