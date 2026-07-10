# Memo

## 목차

- [제품 기준선](#제품-기준선)
- [기본 설정값](#기본-설정값)
- [런타임 구조 메모](#런타임-구조-메모)
- [동작 규칙](#동작-규칙)
- [반복 금지](#반복-금지)
- [RYU 플러그인-프리 베이크 (이어작업 레시피)](#ryu-플러그인-프리-베이크-이어작업-레시피)

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
- `/scene/slots` 표시명은 에디터 Actor Label이 기준이다. 응답은 `id=GetName()`(RPC 안정 식별자)과 `label=GetActorLabel()`(웹 표시명)을 모두 제공한다. 프론트에서 `BP_ParkingSlot_C_*` 이름을 임의 변환하지 않는다.
- 플러그인 버전은 `baroCCTVSimulator.uplugin` `VersionName`이 단일 출처다. 현재 **0.1.3**(0.1.2=차종 카탈로그 리플렉션, 0.1.3=캡처 VT 스로틀 해제)이며 `/scene/catalog.pluginVersion`, 웹 `/simulator` 씬 카드, `BaroSimHUD`에 표시된다.
- **SceneCapture 전용 렌더 3중 함정**(캡처가 안 보이거나 뭉개지면 이 순서로 의심): ① 텍스처 스트리머 뷰 미등록(AddViewInformation) ② 폴리지 LOD 줌 보정(LODDistanceFactor) ③ **VT 페이지 스로틀**(`bOverrideVirtualTextureThrottle=true` — 주차라인 데칼 사건 2026-07-10, dev_log 참조).

## 반복 금지

- **UE 버전업/신규 프로젝트 "컴파일 에러"는 대개 코드가 아니라 빌드환경(Target.cs V6→V7) 불일치** — Shared 환경 확인.
- MCP는 프로젝트별 옵트인(플러그인+AutoStart), 클라이언트 등록은 전역 1회. (상세: `ready_unreal` readme 부록 A/B)
- **에디터로 성능 테스트 금지**: 에디터는 포커스 잃으면 "Use Less CPU when in Background" 스로틀로 게임 틱 ~3.3fps(스트림도 같이 붕괴). 브라우저를 보는 순간 에디터는 항상 백그라운드다. **성능은 standalone `-game`으로 실측**(스탠드얼론은 스로틀 없음).
- **SceneCapture는 텍스처 스트리머에 시점을 등록하지 않는다**(UE5.8 엔진 소스 확인 — 뷰포트 뷰만 등록). 캡처 전용 카메라는 `IStreamingManager::AddViewInformation`(위치+줌 FOV)을 직접 등록해야 원거리 mip이 올라온다. 거리 기반 폴리지 컬링은 FOV 무시 → `LODDistanceFactor`로 줌 보정.
- Live Coding 활성(에디터/게임 실행 중) 상태에선 CLI 빌드 거부됨 — 프로세스 닫고 빌드. 새 UCLASS 추가는 어차피 풀 리빌드 필요.

## RYU 플러그인-프리 베이크 (이어작업 레시피)

> 목표: `RYUKoreaBuilidngCreator`(한국 건물팩) 플러그인 없이 열리는 이식 가능한 맵. **sim_01 완료(2026-07-04). 남은 레벨: `LV_Park_01 / 04 / 07_A`, `Unity/LV_Park_01_U / 04_U`.** (sim_02는 RYU無.)

**핵심 사실**: RYU 빌딩(`BP_RYUBuilding_*`) 상세 지오메트리는 **PCG가 `/Script/RYUKoreaBuilidngCreator` C++로 매 로드 생성하는 transient**(생성기 `BP_GenerateUpperDistributionByGrammar` 등 + 구조체 `SymbolMeshVariation`). 콘텐츠만 /Game로 옮기고 플러그인을 끄면 **상세가 소실**(컴포넌트 ~68→8=기초박스+옥상만). ⇒ **반드시 스태틱 베이크 후 플러그인 제거**.

**전제(이미 완료)**: RYU 콘텐츠는 `/Game/_RYU_Portable/`에 있고 7레벨 참조 재연결됨(RYU참조 0). 플러그인은 현재 **ENABLED**.

**레벨별 반복 절차**:
1. 대상 레벨 로드 → RYU빌딩 액터 확인.
2. 상세 재생성 보장: 각 빌딩 PCG 컴포넌트 `generationTrigger=GenerateOnLoad` → `save_assets([])` → `load_level` 리로드 → 컴포넌트 8→수십으로 복원 확인(`get_components`).
3. 빌딩 전체 선택 → **Merge Actors**(Method=**Merge**, **Merge Materials=off**(룩 보존), Replace Source=off) → 스태틱메시 저장. (MCP 툴 없음 — 에디터 수동)
4. 배치 정렬: 결과 메시 피벗=첫 선택 액터 위치. `add_to_scene_from_asset` 후 그 액터의 world location으로 이동. 검증=원본 대비 **X-max·Z-max 일치**(min차는 스플라인/디버그).
5. 원본 BP 삭제 → 저장 → 전이검증: 레벨 deps에 `/RYUKoreaBuilidngCreator` **및** `/Script/RYU` 둘 다 **0**.
6. **전 레벨 완료 후에만** uproject서 RYU disable(그전 금지 — 미베이크 레벨 깨짐).

**도구/함정(unreal-mcp)**: `AssetTools.move`는 리다이렉터 남김 → 에디터 **"Update Redirector References"**로 참조 재작성(MCP 툴 없음). `ProgrammaticToolset`=등록툴 배치 실행이나 `get_properties`(누락 프로퍼티)는 스크립트 abort → `get_components(actor, StaticMeshComponent)`로 안전 iteration. 대형 결과는 tool-results 파일로 저장됨. Merge Actors·Fix Up Redirectors·PCG bake 전부 MCP 툴 없음(에디터 수동).
