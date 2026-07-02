# Plan

## 목차

- [Current goal](#current-goal)
- [Near-term work](#near-term-work)
- [Structure decisions](#structure-decisions)
- [Risks](#risks)

## Current goal

`baro_unreal`(신규 UE 5.8 C++ 프로젝트)에서 **주차장 CCTV 시뮬레이터**를 구성한다.
두 기존 프로젝트의 산출물을 통합한다:

- **레벨/맵**: `C:\works\ue_prjs\parking_area`(Parking_Project)의 주차장 레벨
  (`Content/Levels/LV_Park_01~07`, `Content/Level/LV_Park_08_Sign`)과 의존 에셋을 이관.
- **CCTV 시뮬레이터**: `C:\works\ue_prjs\baro_world 5.8`의 C++(Hucoms HTTP CGI 서버 ·
  PTZ 카메라 · SceneCapture JPEG · 연속 MJPEG 스트림 · centering 클라이언트)을 이식.

## Near-term work

- [x] parking_area의 주차장 레벨(LV_Park_*)과 의존 에셋을 baro_unreal로 이관 (Content 30GB 통째 robocopy).
- [x] baro_world 5.8 CCTV 소스 이식(13파일) + `BARO_WORLD_API`→`BARO_UNREAL_API` 치환. 빌드·MCP 라이브 검증 완료.
- [x] `baro_unreal.Build.cs`에 HTTP/Json/HTTPServer/Sockets/Networking 반영 (EnhancedInput은 기본 有).
- [x] 플러그인 갭 해소: RYUKoreaBuilidngCreator(2.6GB, 5.8 재컴파일) + PCG/Niagara/Datasmith/Variant/CineCameraSceneCapture/GeometryScripting 활성.
- [x] 주차장 레벨(LV_Park_01)에 PTZ CCTV 액터(`APTZCamera` 4대) 배치 + 레벨 저장.
- [ ] **다음**: PIE 실행으로 `UHucomsServerSubsystem` 서버 기동/포트(8081 CGI, 8082 MJPEG) 검증 + PTZ↔서버 미러링 확인.
- [ ] 나머지 LV_Park 레벨(02~08)에도 PTZ 배치 적용(필요 시).
- [ ] baro_world `Config/DefaultGame.ini`의 Hucoms 설정 오버라이드를 `[/Script/baro_unreal.HucomsServerSubsystem]`로 이식(선택).

## Structure decisions

- 에셋 이관: **파일 직접 복사(robocopy)** 채택 — parking_area가 5.7이고 Content 의존이 광범위해 통째 복사가 확실. `/Game` 경로 동일해 참조 자동 해소.
- CCTV 코드: **기존 `baro_unreal` 모듈에 통합** (별도 모듈 분리 안 함).
- 레벨: parking_area 레벨은 **모놀리식 `.umap`(월드파티션 아님)** — 그대로 사용.
- 플러그인: RYU는 프로젝트 로컬 `Plugins/`(콘텐츠+모듈, 5.8 재빌드), 나머지는 엔진 내장 활성.

## Risks

- parking_area의 Fab/마켓플레이스 에셋 라이선스·용량, 레벨 간 의존성 누락.
- baro_world 5.8 소스 이식 시 모듈명 하드코딩(`BARO_WORLD_API`, 로그 카테고리, config 섹션
  `[/Script/baro_world.HucomsServerSubsystem]`) 누락 → 런타임/config 미적용.
- 두 프로젝트가 같은 UE 5.8인지·에셋 버전 호환성.
