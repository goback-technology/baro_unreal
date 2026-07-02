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
- [x] 서버 기동/포트 검증 + PTZ↔서버 미러링 + 클린 시뮬 레벨(`LV_Park_sim_01`, 4모서리 CCTV 8081~8084).
- [x] 스트림 파이프라인 30fps(StreamFps=30 + 페이싱/락 수정) + 미니멀 standalone 모드(BaroSimGameMode, 월드 렌더 OFF, ESC 종료, fps HUD). (2026-07-02)
- [x] 화질: SceneCapture 선명도/톤(TAA+Lumen, QHD, 노출-0.7/대비1.2) + 원거리 mip/LOD(스트리머 뷰 등록+LODDistanceFactor) + setcenter 줌 인식. (2026-07-02)
- [ ] **다음(성능, 4캠 동시 24fps 필요 시)**: JPEG 인코딩 오프스레드(UE::Tasks) → 블로킹 ReadPixels→비동기 FRHIGPUTextureReadback → jpeg.cgi 스냅샷 캐시. (근거: baro_calory dev_log 2026-07-02 저녁 진단)
- [ ] PIE 일시정지/캡처 실패 시 스트림 keepalive 없음(클라이언트 무프레임 대기) — 소비자 read-timeout 이슈가 실제로 생기면 idle 재전송 추가.
- [ ] 나머지 LV_Park 레벨(02~08)에도 PTZ 배치 적용(필요 시).

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
