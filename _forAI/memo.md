# Memo

## 목차

- [제품 기준선](#제품-기준선)
- [기본 설정값](#기본-설정값)
- [런타임 구조 메모](#런타임-구조-메모)
- [동작 규칙](#동작-규칙)
- [반복 금지](#반복-금지)

## 제품 기준선

- 엔진: Unreal Engine **5.8** (Windows 11 / PowerShell)
- 프로젝트: `baro_unreal` — C++ 게임 모듈(에디터 타깃 `baro_unrealEditor`)
- 통합 출처: 레벨=`parking_area`(Parking_Project), CCTV 시뮬=`baro_world 5.8`
- TODO: 최종 타깃 런타임(standalone/PIE)·배포 형태 확정

## 기본 설정값

- MCP 서버(에디터 내장): `http://127.0.0.1:8000/mcp`, `bAutoStartServer=True`
- **포트(카메라 4대, 인덱스 자동 부여)**: HTTP CGI **8081~8084**(BaseHttpPort+i), 연속 MJPEG TCP **8091~8094**(BaseMjpegPort+i). baro_calory `config.json devices[].port/mjpegPort`와 1:1.
- **스트림**: `StreamFps=30`(DefaultGame.ini 오버라이드 — 코드 기본 15), 1280×720 q80. **스냅샷**: QHD 2560×1440 q92, 워밍업 0.
- **톤**: 노출 -0.7 + **대비 1.2**(DefaultGame.ini `CaptureContrast` — 코드 기본 1.6은 흰 차+직사광에서 "탄" 세피아, 2026-07-02 실측 개정). 라이브 스윕은 `/api/capture-tuning`.
- **표준 실행**: standalone `UnrealEditor.exe <uproject> -game -windowed -ResX=960 -ResY=540` — `GameDefaultMap=/Game/simulator/LV_Park_sim_01`, `GlobalDefaultGameMode=BaroSimGameMode`(구체 폰 없음·월드 렌더 OFF·커서 표시·**ESC 종료**·HUD에 채널별 실측 fps). 완전 무창은 `-RenderOffscreen -log`(-nullrhi 금지 — SceneCapture가 못 돎).

## 런타임 구조 메모

- `UHucomsServerSubsystem`(UTickableWorldSubsystem, Game/PIE): BeginPlay에 채널(카메라당 HTTP 라우터+MJPEG TCP 서버) 기동. Tick(게임스레드)에서 모터 슬루 → 카메라 미러 → **CCTV 시점 텍스처 스트리머 등록**(AddViewInformation, 줌 FOV 반영) → 클라이언트 있는 채널만 StreamFps로 CaptureJpeg→`FMjpegStreamServer::UpdateFrame`.
- `FMjpegStreamServer`(FRunnable): 프레임 시퀀스 게이트 + auto-reset FEvent — 새 프레임 도착 시에만 클라이언트별 송신(SentSeq), 송신 중 ClientsLock 잡지 않음(게임스레드 HasClients 프리즈 방지). 페이싱은 producer(Tick accumulator, 잔여 보존+클램프)가 결정.
- 캡처 체인은 아직 게임스레드 동기(CaptureScene→ReadPixels 플러시→JPEG 인코드, 카메라당 ~30-70ms) — 4캠 동시 24fps가 필요해지면 오프스레드 인코드+비동기 리드백(baro_calory plan 로드맵).

## 동작 규칙

- **setcenter = 줌 인식 LINEAR 모델**: 델타 = (픽셀오프셋/프레임) × **현재 zoompos의 실효 FOV** × 100. zoompos→HFOV는 cam-001 실측 표(`HucomsProtocol::ZoomPosToHFov` = baro_calory `fov-convert.zoomPosToHFov`와 동일 표 — **한쪽 수정 시 반드시 동기화**). VFOV는 tan 비례 축소. 광각 상수 고정은 줌 배율만큼 과이동(구버그).
- PTZ 부호·틸트 규약은 dev_log의 "PTZ 좌표·부호 규약(Canonical)" 표를 따른다.

## 반복 금지

- **UE 버전업/신규 프로젝트 "컴파일 에러"는 대개 코드가 아니라 빌드환경(Target.cs V6→V7) 불일치** — Shared 환경 확인.
- MCP는 프로젝트별 옵트인(플러그인+AutoStart), 클라이언트 등록은 전역 1회. (상세: `ready_unreal` readme 부록 A/B)
- **에디터로 성능 테스트 금지**: 에디터는 포커스 잃으면 "Use Less CPU when in Background" 스로틀로 게임 틱 ~3.3fps(스트림도 같이 붕괴). 브라우저를 보는 순간 에디터는 항상 백그라운드다. **성능은 standalone `-game`으로 실측**(스탠드얼론은 스로틀 없음).
- **SceneCapture는 텍스처 스트리머에 시점을 등록하지 않는다**(UE5.8 엔진 소스 확인 — 뷰포트 뷰만 등록). 캡처 전용 카메라는 `IStreamingManager::AddViewInformation`(위치+줌 FOV)을 직접 등록해야 원거리 mip이 올라온다. 거리 기반 폴리지 컬링은 FOV 무시 → `LODDistanceFactor`로 줌 보정.
- Live Coding 활성(에디터/게임 실행 중) 상태에선 CLI 빌드 거부됨 — 프로세스 닫고 빌드. 새 UCLASS 추가는 어차피 풀 리빌드 필요.
