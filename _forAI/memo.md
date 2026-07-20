# Memo

## 목차

- [제품 기준선](#제품-기준선)
- [기본 설정값](#기본-설정값)
- [런타임 구조 메모](#런타임-구조-메모)
- [동작 규칙](#동작-규칙)
- [반복 금지](#반복-금지)
- [메모리 누수 원인과 대응 (2026-07-20)](#메모리-누수-원인과-대응-2026-07-20)
- [RYU 플러그인-프리 베이크 (이어작업 레시피)](#ryu-플러그인-프리-베이크-이어작업-레시피)

## 제품 기준선

- 엔진: Unreal Engine **5.8** (Windows 11 / PowerShell), **Win64 전용**. Linux/Vulkan 버전은 2026-07-10부로 보류.
- 프로젝트: `baro_unreal` — C++ 게임 모듈(에디터 타깃 `baro_unrealEditor`)

## 메모리 누수 원인과 대응 (2026-07-20)

- **원인 확정**: UE 5.8에서 Lumen이 활성화된 `SceneCaptureComponent2D`의 `bAlwaysPersistRenderingState=true`가 persistent rendering ViewState를 계속 유지하면서 process RAM을 증가시켰다. JPEG 인코딩, Readback, 소켓 전송, TemporalAA 단독이 주원인은 아니었다.
- **A/B 검증**: SceneCapture-only 약 `+1.835 MB/s`, `bAlwaysPersistRenderingState=false`는 warm-up 후 약 `-0.253 MB/s`, Lumen off는 약 `+0.086 MB/s`였다. 원래 전체 경로는 약 `+1.2~1.9 MB/s`였다.
- **수정**: `PTZCaptureComponent.cpp`에서 `bAlwaysPersistRenderingState=false`로 변경해 캡처별 persistent ViewState 보존을 차단했다.
- **재발 감시**: `BaroSystemMonitorSubsystem`이 1초마다 CPU/RAM/GPU/VRAM/FPS를 수집하고 30초마다 UE 로그와 `Saved/Logs/BaroHealth-*.csv`에 기록한다. 최근 120초 RAM slope가 `20 MB/min`을 넘으면 `LEAK_SUSPECTED`로 표시한다. 큰 일회성 증가(`baro.Health.ResetJumpMB`, 기본 256MB)는 workload transition으로 분리한다.
- **UI**: `BaroSystemMonitorWidget`이 우상단에 상태, CPU, GPU frame time, RAM/slope, VRAM, FPS를 표시한다. MCP callable tool이 현재 노출되지 않아 `.uasset` WBP 대신 native UMG로 구현했으며 Blueprint/WBP 확장이 가능하다.
- **최종 검증**: Development 패키지 166초 실행에서 `LEAK_SUSPECTED` 없이 `HEALTHY`를 유지했다. GPU process utilization은 현재 RHI에서 제공되지 않아 `N/A`, GPU frame ms와 VRAM은 표시된다.
- **별도 관찰**: `TEXTURE STREAMING POOL OVER 59.859 MiB BUDGET`은 이번 CPU 메모리 누수와 별개인 texture streaming budget 경고다. Smart App Control 차단도 로컬 unsigned plugin DLL에 대한 Windows 보안 정책이며, 이교수님이 처리 완료했다.
- 통합 출처: 레벨=`parking_area`(Parking_Project), CCTV 시뮬=`baro_world 5.8`
- 배포 기준: Win64 Shipping, 일반 창모드 960×540. Linux 재개는 별도 요청 전까지 범위 밖이다.

## 기본 설정값

- MCP 서버(에디터 내장): `http://127.0.0.1:8000/mcp`, `bAutoStartServer=True`
- **포트(카메라 인덱스만큼 자동 부여)**: HTTP CGI = `BaseHttpPort` **8081**+i, 연속 MJPEG TCP = `BaseMjpegPort` **8091**+i. 씬 제어는 **8095** 고정. baro_calory `config.json devices[].port/mjpegPort`와 1:1.
  - **카메라 수는 맵마다 다르다** — 기본 맵 `LV_Park_sim_01` = 2대(8081·8082 / 8091·8092), `sim_02` = 2대, `sim_03` = 4대(8081~8084 / 8091~8094). `DefaultEngine.ini [HTTPServer.Listeners]`가 8081~8084를 미리 예약(상한 슈퍼셋)해 둔 것이지 항상 4포트가 열린다는 뜻이 아니다.
- **스트림**: `StreamFps=30`(DefaultGame.ini 오버라이드 — 코드 기본 15), 1280×720 q80. **스냅샷**: QHD 2560×1440 q92, 워밍업 0.
- **톤**: 노출 -0.7 + **대비 1.2**(DefaultGame.ini `CaptureContrast` — 코드 기본 1.6은 흰 차+직사광에서 "탄" 세피아, 2026-07-02 실측 개정). 라이브 스윕은 `/api/capture-tuning`.
- **표준 실행**: `./Scripts/run.ps1`(기본 `-Mode Game`) — `.env`의 `UE_PATH`/`DEFAULT_MAP`/`RUN_RESX`/`RUN_RESY`를 읽어 standalone `UnrealEditor.exe <uproject> -game -windowed -ResX=960 -ResY=540`을 띄운다. `GameDefaultMap=/Game/simulator/LV_Park_sim_01`, `GlobalDefaultGameMode=/Script/baro_unreal.BaroUnrealGameMode`(앱 게임모드 — 플러그인 `ABaroSimGameMode` 상속, HUD만 앱 것으로 교체. 구체 폰 없음·월드 렌더 OFF·커서 표시·**ESC 종료**). 완전 무창은 `-RenderOffscreen -log`(-nullrhi 금지 — SceneCapture가 못 돎).
- **앱 버전**: `DefaultGame.ini [/Script/EngineSettings.GeneralProjectSettings] ProjectVersion`(현 **0.1.0**). 플러그인 `.uplugin` VersionName(현 0.1.3)과 **별개**다. HUD 제목줄=`baro_unreal v0.1.0`, 그 아래 작은 줄=`plugin baroCCTVSimulator v0.1.3`. 앱 코드 수정 시 ProjectVersion 끝자리 +1.
- **HUD 서빙 주소**: `ABaroUnrealHUD`가 `ISocketSubsystem::GetLocalAdapterAddresses()`로 IPv4를 열거해 첫 줄에 표시한다. `127.`과 **`169.254.`(APIPA 링크로컬)** 를 둘 다 걸러야 한다 — 이 개발 PC엔 169.254가 4개(끊긴 Wi-Fi·블루투스 PAN 등), 실제 LAN은 이더넷 하나(`192.168.0.211`)뿐이다.
- **배포 창/품질 기본값**: `DefaultGameUserSettings.ini`의 `FullscreenMode=2`(일반 창), 960×540, Epic(3), `sg.ResolutionQuality=100`. 메인 창 크기와 CCTV 캡처(QHD 2560×1440 q92)는 서로 독립이다. Cinematic(4)은 다중 SceneCapture의 VRAM 압박으로 mip/Lumen 품질이 흔들릴 수 있어 강제하지 않는다.

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
- **플러그인은 "최소한의 카메라" — 앱 고유 기능을 넣지 않는다.** HUD·버전 표기·서빙 주소처럼 이 앱에만 필요한 것은 호스트 게임 모듈(`Source/baro_unreal/`)에서 상속으로 해결한다. 플러그인 클래스는 전부 `BAROCCTVSIMULATOR_API` export이고 `DrawHUD()`가 virtual, `ABaroSimGameMode` 생성자가 public이라 `HUDClass` 교체만으로 충분하다. 플러그인 `Build.cs`의 `Sockets`/`Networking`/`Projects`는 Private 의존이라 전이되지 않으니 게임 `Build.cs`에 직접 추가한다. (근거: 3프로젝트 공용 서브모듈 → 한 줄 수정도 버전 범프·풀 리빌드·push·서브모듈 갱신 사슬.)
- **제어 API 무인증은 의도된 결정**: 씬 제어(8095)와 Hucoms CGI(8081~8084)는 토큰·API키·origin 검사가 없고 `DefaultEngine.ini [HTTPServer.Listeners]`에서 포트별 `BindAddress=any`로 LAN에 열어 둔다. 이 시뮬레이터는 **개발 보조용이며 내부망 전용**이므로 인증을 두지 않는다(2026-07-10 확정). 입력 하드닝은 값 클램핑(차종·색·번호판 정규화)까지가 범위다. MCP(8000)는 override를 주지 않아 localhost로 남는다 — 여기에 `BindAddress=any`를 추가하지 말 것.

## 반복 금지

- **UE 버전업/신규 프로젝트 "컴파일 에러"는 대개 코드가 아니라 빌드환경(Target.cs V6→V7) 불일치** — Shared 환경 확인.
- MCP는 프로젝트별 옵트인(플러그인+AutoStart), 클라이언트 등록은 전역 1회. (상세: `ready_unreal` readme 부록 A/B)
- **에디터로 성능 테스트 금지**: 에디터는 포커스 잃으면 "Use Less CPU when in Background" 스로틀로 게임 틱 ~3.3fps(스트림도 같이 붕괴). 브라우저를 보는 순간 에디터는 항상 백그라운드다. **성능은 standalone `-game`으로 실측**(스탠드얼론은 스로틀 없음).
- **SceneCapture는 텍스처 스트리머에 시점을 등록하지 않는다**(UE5.8 엔진 소스 확인 — 뷰포트 뷰만 등록). 캡처 전용 카메라는 `IStreamingManager::AddViewInformation`(위치+줌 FOV)을 직접 등록해야 원거리 mip이 올라온다. 거리 기반 폴리지 컬링은 FOV 무시 → `LODDistanceFactor`로 줌 보정.
- Live Coding 활성(에디터/게임 실행 중) 상태에선 CLI 빌드 거부됨 — 프로세스 닫고 빌드. 새 UCLASS 추가는 어차피 풀 리빌드 필요.
- **패키징이 `Failed reading oplog from Zen ... Error while copying content to a stream`으로 죽는 건 코드 문제가 아니다.** zenserver는 상주 데몬이 아니라 **sponsor 프로세스(에디터/쿡)가 0이 되면 자결**한다. `BuildCookRun`은 쿡을 별도 프로세스로 돌린 뒤 UAT 본체가 oplog를 되읽어 스테이징하는데, UAT는 sponsor가 아니다(sponsor 슬롯 = UE 프로세스만 쓰는 공유메모리 `SponsorPids[8]`). 쿡이 끝나는 순간 서버가 내려가면 읽기가 스트림 중간에 끊긴다. `--owner-pid`는 종료 신호용이지 sponsor가 아니라 외부에서 심을 방법이 없다. **해법은 재시도**(쿡 결과는 Zen에 온전 → 캐시 히트로 통과). `Scripts/package.ps1`이 Zen 오류일 때만 1회 자동 재시도한다. 진단은 `%LOCALAPPDATA%\UnrealEngine\Common\Zen\Data\logs\zenserver*.log`의 `exiting since sponsor processes are all gone`으로.
- **클론 직후 `git submodule update --init --recursive` 필수** — `Plugins/baroCCTVSimulator`가 서브모듈이라 빠뜨리면 CCTV 클래스가 통째로 사라진 채 레벨이 열린다(참조 깨짐이 액터 소실로 보임).
- **uproject Plugins 배열에 없다 = 비활성이 아니다.** 프로젝트 로컬 플러그인은 `EnabledByDefault` 미지정 시 **기본 활성**(`FPlugin::IsEnabledByDefault`: Unspecified → `LoadedFrom == Project`). RYU가 여기 해당한다 — 끄려면 `"Enabled": false`를 명시해야 한다.

## RYU 플러그인-프리 베이크 (이어작업 레시피)

> 목표: `RYUKoreaBuilidngCreator`(한국 건물팩) 플러그인 없이 열리는 이식 가능한 맵. **sim_01 완료(2026-07-04). 남은 레벨: `LV_Park_01 / 04 / 07_A`, `Unity/LV_Park_01_U / 04_U`.** (sim_02는 RYU無.)

**핵심 사실**: RYU 빌딩(`BP_RYUBuilding_*`) 상세 지오메트리는 **PCG가 `/Script/RYUKoreaBuilidngCreator` C++로 매 로드 생성하는 transient**(생성기 `BP_GenerateUpperDistributionByGrammar` 등 + 구조체 `SymbolMeshVariation`). 콘텐츠만 /Game로 옮기고 플러그인을 끄면 **상세가 소실**(컴포넌트 ~68→8=기초박스+옥상만). ⇒ **반드시 스태틱 베이크 후 플러그인 제거**.

**전제(이미 완료)**: RYU 콘텐츠는 `/Game/_RYU_Portable/`에 있고 7레벨 참조 재연결됨(RYU참조 0). 플러그인은 현재 **ENABLED** — uproject Plugins 배열에 항목이 없지만 프로젝트 로컬 플러그인이라 기본 활성이다(위 「반복 금지」 참조). 즉 "이미 껐다"고 착각하지 말 것.

**레벨별 반복 절차**:
1. 대상 레벨 로드 → RYU빌딩 액터 확인.
2. 상세 재생성 보장: 각 빌딩 PCG 컴포넌트 `generationTrigger=GenerateOnLoad` → `save_assets([])` → `load_level` 리로드 → 컴포넌트 8→수십으로 복원 확인(`get_components`).
3. 빌딩 전체 선택 → **Merge Actors**(Method=**Merge**, **Merge Materials=off**(룩 보존), Replace Source=off) → 스태틱메시 저장. (MCP 툴 없음 — 에디터 수동)
4. 배치 정렬: 결과 메시 피벗=첫 선택 액터 위치. `add_to_scene_from_asset` 후 그 액터의 world location으로 이동. 검증=원본 대비 **X-max·Z-max 일치**(min차는 스플라인/디버그).
5. 원본 BP 삭제 → 저장 → 전이검증: 레벨 deps에 `/RYUKoreaBuilidngCreator` **및** `/Script/RYU` 둘 다 **0**.
6. **전 레벨 완료 후에만** uproject서 RYU disable(그전 금지 — 미베이크 레벨 깨짐).

**도구/함정(unreal-mcp)**: `AssetTools.move`는 리다이렉터 남김 → 에디터 **"Update Redirector References"**로 참조 재작성(MCP 툴 없음). `ProgrammaticToolset`=등록툴 배치 실행이나 `get_properties`(누락 프로퍼티)는 스크립트 abort → `get_components(actor, StaticMeshComponent)`로 안전 iteration. 대형 결과는 tool-results 파일로 저장됨. Merge Actors·Fix Up Redirectors·PCG bake 전부 MCP 툴 없음(에디터 수동).
