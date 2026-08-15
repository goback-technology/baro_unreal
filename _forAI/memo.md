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
- 통합 출처: 레벨=`parking_area`(Parking_Project), CCTV 시뮬=`baro_world 5.8`
- 배포 기준: Win64 Shipping, 일반 창모드 960×540. Linux 재개는 별도 요청 전까지 범위 밖이다.

## 기본 설정값

- MCP 서버(에디터 내장): `http://127.0.0.1:8000/mcp`, `bAutoStartServer=True`
- **포트(카메라 인덱스만큼 자동 부여)**: HTTP CGI = `BaseHttpPort` **8081**+i, 연속 MJPEG TCP = `BaseMjpegPort` **8091**+i. 씬 제어는 **8095** 고정. baro_calory `config.json devices[].port/mjpegPort`와 1:1.
  - **카메라 수는 맵마다 다르다** — 기본 맵 `LV_Park_sim_01` = 2대(8081·8082 / 8091·8092), `sim_02` = 2대, `sim_03` = 4대(8081~8084 / 8091~8094). `DefaultEngine.ini [HTTPServer.Listeners]`가 8081~8084를 미리 예약(상한 슈퍼셋)해 둔 것이지 항상 4포트가 열린다는 뜻이 아니다.
- **CGI·씬 포트의 LAN 노출은 리스너를 여는 코드가 선언한다**(플러그인 `HttpListenerBind.h`, v0.1.14~).
  UE `FHttpServerListenerConfig::BindAddress` 기본값이 `localhost` 라 아무 선언이 없으면 127.0.0.1 에만
  바인드된다. ini 포트 목록 방식은 **런타임 스폰 포트를 담을 수 없어** 폐기했다(2026-08-06 결함).
  `DefaultEngine.ini [HTTPServer.Listeners]` 에 포트를 명시하면 코드가 그 값을 존중한다(NIC 고정용).
  전역 `DefaultBindAddress=any` 는 여전히 금지 — 같은 모듈인 에디터 MCP(:8000)까지 LAN 에 노출된다.
  MJPEG(8091+)는 플러그인 자체 `FTcpListener` 라 항상 0.0.0.0.
  구현이 알고 있어야 하는 엔진 사실 둘: ① `[HTTPServer.Listeners]` 는 캐시되므로 GConfig 수정 뒤
  `FCoreDelegates::TSOnConfigSectionsChanged` broadcast 가 없으면 조용히 옛 값이 이긴다.
  ② 소켓은 `GetHttpRouter` 가 아니라 `StartListening()` 에서 열리고, 모듈 비활성 상태에서 만든 리스너는
  열기가 나중의 `StartAllListeners()` 로 **미뤄진다**(부팅 첫 리스너 — 실측: 8095 가 그래서 localhost 였다).
  → **"MJPEG 은 되는데 CGI 만 원격에서 연결 거부"** 면 라우터·라우트가 아니라 바인드 주소를 의심한다.
  로그는 정상으로 보이고 **localhost 테스트로는 절대 재현되지 않는다**. 판별·회귀는
  `node tools/scene-test/lan-bind-contract.mjs`(루프백 거부) 또는 `netstat -ano | findstr :<port>`.
- **다중 인스턴스(한 기기에 시뮬 N개, 2026-08-15 실측 검증)**: 시뮬레이터 본연의 포트는 `ScenePort` 하나뿐이라
  이것만 인스턴스별로 갈면 된다(카메라 0대 시작 기준 — 카메라 포트는 스폰 시 장치 속성이고, 충돌은
  `SpawnCameraRuntime`이 롤백+400 으로 처리).
  - **표준 방법(플러그인 v0.1.16~)**: `-ScenePort=8096` 커맨드라인 스위치 — **Shipping 포함 전 빌드 구성
    동작**, 우선순위 커맨드라인 > `-ini:` > ini. `run.ps1 -ScenePort 8096` 으로도 전달된다.
    Base 포트도 동일: `-BaseHttpPort=` / `-BaseMjpegPort=`. `ScenePort` 점유 시 인스턴스는 **즉시 종료**한다
    (원시 소켓 선점 프로브 — 스플릿-브레인 차단, dev_log 2026-08-15).
  - **운용 주체는 에이전트다**(2026-08-15 결정): 포트 부여·기동·헬스체크·재시작을 에이전트가 관리한다.
    그래서 자동 포트 탐색은 의도적으로 없다 — 계약은 "기동 `-ScenePort=N` → 준비 `GET :N/scene/catalog` 200
    → 실패 감지 = 프로세스 exit" 이고, 프로세스가 살아 있으면 포트는 보장된다. 이 계약은 `/scene/help`
    (「인스턴스 기동·포트 계약」)가 서빙하므로 에이전트에게 별도 문서를 줄 필요 없다.
  - **카메라 포트 블록은 인스턴스별로 분리할 것**: 카메라 DELETE 는 라우트만 제거하고 HTTP 리스너는
    프로세스 종료까지 리슨을 유지한다(엔진에 리스너 파괴 API 없음). 같은 인스턴스는 그 포트를 재사용할
    수 있지만 **다른 인스턴스는 못 쓴다** — baro_calory 인스턴스별 config 의 devices 포트를 겹치지 않게.
  - **구 배포본(≤ 앱 0.2.8, 플러그인 ≤0.1.15) Shipping 우회**: `-ini:` 가 안 먹는다(아래 「반복 금지」).
    `-UserDir=D:\sim_inst2` + `<UserDir>\baro_unreal\Saved\Config\Windows\Game.ini` 에 `ScenePort` — 사본 불필요.
    `-UserDir` 는 v0.1.16 이후에도 로그·BaroHealth CSV·GameUserSettings 인스턴스 분리용으로는 여전히 유용하다
    (기본 Shipping Saved 는 `%LOCALAPPDATA%\baro_unreal\Saved` — 전 인스턴스 공유).
  - **인스턴스당 유휴 고정비**(Shipping v0.2.8, sim_01, 카메라 0대): RAM 워킹셋 **4.0GB** / 커밋 8.8GB /
    VRAM **2.2GB** — 공유분 0, 순수 복제. RTX 5060 8GB 기준 실용 한도 **2개**(3개째는 VRAM 초과).
    단일 인스턴스 멀티월드(존 복제)안은 월드당 VRAM ~2GB 절약이지만 한 GPU 에 월드 3개+ 요구 전까지는
    프로세스 분리가 이긴다(A/B 검토·수치 상세: dev_log 2026-08-15).
- **스트림**: `StreamFps=30`(DefaultGame.ini 오버라이드 — 코드 기본 15), 1280×720 q80. **스냅샷**: QHD 2560×1440 q92, 워밍업 0.
- **톤**: 노출 -0.7 + **대비 1.2**(DefaultGame.ini `CaptureContrast` — 코드 기본 1.6은 흰 차+직사광에서 "탄" 세피아, 2026-07-02 실측 개정). 라이브 스윕은 `/api/capture-tuning`.
- **표준 실행**: `./Scripts/run.ps1`(기본 `-Mode Game`) — `.env`의 `UE_PATH`/`DEFAULT_MAP`/`RUN_RESX`/`RUN_RESY`를 읽어 standalone `UnrealEditor.exe <uproject> -game -windowed -ResX=960 -ResY=540`을 띄운다. `GameDefaultMap=/Game/simulator/LV_Park_sim_01`, `GlobalDefaultGameMode=/Script/baro_unreal.BaroUnrealGameMode`(앱 게임모드 — 플러그인 `ABaroSimGameMode` 상속, HUD만 앱 것으로 교체. 구체 폰 없음·월드 렌더 OFF·커서 표시·**ESC 종료**). 완전 무창은 `-RenderOffscreen -log`(-nullrhi 금지 — SceneCapture가 못 돎).
- **앱 버전**: `DefaultGame.ini [/Script/EngineSettings.GeneralProjectSettings] ProjectVersion`(현 **0.2.0**). 플러그인 `.uplugin` VersionName(현 0.1.6)과 **별개**다. HUD 제목줄=`baro_unreal v0.2.0`, 그 아래 작은 줄=`plugin baroCCTVSimulator v0.1.6`. **배포 zip 이름의 출처**이기도 하다(`package.ps1 -Zip` 이 이 값을 파싱). 범프 규칙: 앱 코드 수정 시 끝자리 +1, **배포 규약이 바뀌는 릴리스는 minor +1**(0.1.1→0.2.0 = 코드는 0.1.1과 동일하고 zip 이름 규약만 바뀐 릴리스).
- **배포 zip 이름 = 앱 버전**(2026-07-22 규약): `./Scripts/package.ps1 -Zip` → `Packaged/baro_unreal_sim_v<앱버전>_<yyyyMMdd>.zip` + `.sha256` + `.info.txt`. 이름은 `ProjectVersion`에서 **자동 생성**하며 손으로 짓지 않는다. 플러그인 버전은 이름에 넣지 않고 `.info.txt`에 앱/플러그인 버전과 양쪽 커밋 SHA(더러우면 `-dirty`)를 기록한다.
  ⚠️ **0.2.0 이전 zip은 플러그인 버전으로 이름이 붙어 있다** — `baro_unreal_sim_v0.1.4_20260714.zip`의 0.1.4는 플러그인이고 그때 앱은 0.1.0이었다. 숫자만 보고 신구를 비교하지 말 것(그래서 0.1.1이 아니라 0.2.0으로 범프해 역전을 없앴다).
- **HWRT Lumen**: `DefaultEngine.ini [/Script/Engine.RendererSettings] r.Lumen.HardwareRayTracing=True` — 캡처 persist 가드(플러그인 v0.1.6)와 짝. 이 줄을 빼면 SW Lumen 폴백 → 가드가 persist 를 꺼 암부 화질이 v0.1.5 수준으로 내려간다(누수는 안 남).
- **HUD 서빙 주소**: `ABaroUnrealHUD`가 `ISocketSubsystem::GetLocalAdapterAddresses()`로 IPv4를 열거해 첫 줄에 표시한다. `127.`과 **`169.254.`(APIPA 링크로컬)** 를 둘 다 걸러야 한다 — 이 개발 PC엔 169.254가 4개(끊긴 Wi-Fi·블루투스 PAN 등), 실제 LAN은 이더넷 하나(`192.168.0.211`)뿐이다.
- **배포 창/품질 기본값**: `DefaultGameUserSettings.ini`의 `FullscreenMode=2`(일반 창), 960×540, Epic(3), `sg.ResolutionQuality=100`. 메인 창 크기와 CCTV 캡처(QHD 2560×1440 q92)는 서로 독립이다. Cinematic(4)은 다중 SceneCapture의 VRAM 압박으로 mip/Lumen 품질이 흔들릴 수 있어 강제하지 않는다.

## 런타임 구조 메모

- `UHucomsServerSubsystem`(UTickableWorldSubsystem, Game/PIE): BeginPlay에 채널(카메라당 HTTP 라우터+MJPEG TCP 서버) 기동. Tick(게임스레드)에서 모터 슬루 → 카메라 미러 → **CCTV 시점 텍스처 스트리머 등록**(AddViewInformation, 줌 FOV 반영) → 클라이언트 있는 채널만 StreamFps로 CaptureJpeg→`FMjpegStreamServer::UpdateFrame`.
- `FMjpegStreamServer`(FRunnable): 프레임 시퀀스 게이트 + auto-reset FEvent — 새 프레임 도착 시에만 클라이언트별 송신(SentSeq), 송신 중 ClientsLock 잡지 않음(게임스레드 HasClients 프리즈 방지). 페이싱은 producer(Tick accumulator, 잔여 보존+클램프)가 결정.
- 캡처 체인은 아직 게임스레드 동기(CaptureScene→ReadPixels 플러시→JPEG 인코드, 카메라당 ~30-70ms) — 4캠 동시 24fps가 필요해지면 오프스레드 인코드+비동기 리드백(baro_calory plan 로드맵).

## 동작 규칙

- **setcenter = TAN 핀홀 + 구면 짐벌 모델**(플러그인 v0.1.4부터, `ApplySetCenter` — 실기 펌웨어 재현): 클릭 픽셀을 현재 zoompos 실효 FOV의 tan 광선으로 바꾸고 현재 tilt를 반영한 구면 기하로 pan/tilt 델타를 구한다. 구 "줌 인식 LINEAR(픽셀∝각도)" 서술은 2026-07-14 실측으로 폐기됨(이 줄 2026-07-23 정정). zoompos→HFOV 표의 sim 쪽 단일 출처는 `HucomsProtocol::ZoomHfovTable`(v0.1.7부터 `/scene/cameras[].intrinsics.zoomHfov`로 노출)이고, baro_calory는 API 표 우선·JS 내장표 폴백. 실카메라 표는 계속 JS(`camera-intrinsics.mjs`·`devices[].intrinsics`)에서 관리.
- PTZ 부호·틸트 규약은 dev_log의 "PTZ 좌표·부호 규약(Canonical)" 표를 따른다.
- `/scene/slots` 표시명은 에디터 Actor Label이 기준이다. 응답은 `id=GetName()`(RPC 안정 식별자)과 `label=GetActorLabel()`(웹 표시명)을 모두 제공한다. 프론트에서 `BP_ParkingSlot_C_*` 이름을 임의 변환하지 않는다.
- **차량 스폰/이동 = 슬롯당 항상 1대**(v0.1.7 수정, `EvictSlotOccupant`). 점유 슬롯에 `force:true` 로 스폰하거나 PATCH 로 이동하면 **기존 점유 차량을 파괴한 뒤** 교체한다. force 없이는 `409`. 이전 구현은 `SpawnCarActor`(AlwaysSpawn)가 옛 차 위에 겹쳐 스폰하고 `SlotOccupancy` 값만 새 carId 로 덮어써, 옛 차가 유령으로 남고 그 옛 차 DELETE 시 새 차 점유가 지워지는 교차 오염이 있었다(감사 발견). UE(`SceneControlSubsystem.cpp`)와 baro_calory `FakeSceneClient.#occupy` 를 동일 동작으로 맞췄다 — 한쪽만 고치면 "동일 표면" 계약이 깨진다.
- 플러그인 버전은 `baroCCTVSimulator.uplugin` `VersionName`이 단일 출처다. 현재 **0.1.8**(0.1.2=차종 카탈로그 리플렉션, 0.1.3=캡처 VT 스로틀 해제, 0.1.4=setcenter tan+구면 기하, 0.1.5=캡처 persist off — 0.1.6 으로 대체, 0.1.6=persist HWRT 가드, 0.1.7=/scene API 확장[boundsCm·groundReference/heightAboveReferenceGroundCm·intrinsics.zoomHfov]+force 축출, **0.1.8=BEVHeight 파인튜닝 확장[config 카메라 스포너·차종 class 라벨·가시성 GT·핀홀 명시 필드·연속 PTZ 미러]**)이며 `/scene/catalog.pluginVersion`, 웹 `/simulator` 씬 카드, `BaroSimHUD`에 표시된다. 0.1.7 은 서브모듈 `13ede2f`, 0.1.8 은 그 위에. 에디터 빌드 + 라이브 API 검증 완료(2026-07-23), **push 미수행/미배포**. 배포는 v0.2.0 soak 종료 후 Shipping 전환 시 함께.
- **config 카메라 스포너**(v0.1.8): `HucomsServerSubsystem`의 `UPROPERTY(config) TArray<FPTZCameraSpawnSpec> SpawnCameras` — `DefaultGame.ini`의 `+SpawnCameras=(...)`로 레벨 무수정 카메라 배치. `BuildChannels` 최상단에서 `GetAllActorsOfClass` 전에 스폰해 같은 패스에서 채널·포트를 받는다. 포트 자동부여 인덱스(`AutoIndex`)는 자동포트 카메라에서만 증가 → 명시 포트 스폰 카메라가 섞여도 레벨의 폴 카메라는 8081/8082 유지. 폴(메시) 없이 공중 배치 OK(캡처 소스라 렌더 대상 아님). **함정: 좁은 폐쇄형 주차장(LV_Park_sim_01, ~21×27m)은 실장비 16m/20°/44m 클리어 사이트라인 재현 불가** — 44m 남쪽은 단지 정원/건물 뒤라 주차장이 가려진다. 그래서 v0.1.8~v0.2.6 동안은 남쪽 22m 지점에 높이만 8/12/16/20m로 쌓고 각자 주차중심을 조준했다(틸트 20~42°, 슬롯 슬랜트 12~42m). **이 ini 배치는 v0.2.7 에서 걷어냈다** — 지금은 카메라 0 대로 시작하고 배치는 런타임 씬 제어 API(`POST/PATCH/DELETE /scene/cameras`)와 그것을 쓰는 웹 UI 가 맡는다. 스포너 자체는 남아 있어 부팅 고정 배치가 필요하면 쓸 수 있다. 실장비 원거리 저각이 필수면 더 개방된 레벨 필요(이교수님 판단, ini만 고치면 됨·리빌드 불요).
- **연속 PTZ velocity 미러**(v0.1.8): `pt_control.cgi?action=setptmove`(pan/tilt=right|left|up|down|stop + speed), `zf_control.cgi?action=setzfmove`(zoom=in|out|stop). 채널에 `PanVel/TiltVel/ZoomVel`(native units/sec), Tick에서 Cur 적분+Tgt 동기, 한계 클램프 시 자동정지. goptzfpos/setcenter(절대 이동)가 velocity를 0으로 리셋(goto가 jog 취소). bFixed는 무시. 성공=빈 본문. baro_calory `fake-camera-client.setPtMove/setZfMove`도 동일 부호로 미러(부호 회귀 방지 테스트 포함).
- **SceneCapture 전용 렌더 3중 함정**(캡처가 안 보이거나 뭉개지면 이 순서로 의심): ① 텍스처 스트리머 뷰 미등록(AddViewInformation) ② 폴리지 LOD 줌 보정(LODDistanceFactor) ③ **VT 페이지 스로틀**(`bOverrideVirtualTextureThrottle=true` — 주차라인 데칼 사건 2026-07-10, dev_log 참조).
- **플러그인은 "최소한의 카메라" — 앱 고유 기능을 넣지 않는다.** HUD·버전 표기·서빙 주소처럼 이 앱에만 필요한 것은 호스트 게임 모듈(`Source/baro_unreal/`)에서 상속으로 해결한다. 플러그인 클래스는 전부 `BAROCCTVSIMULATOR_API` export이고 `DrawHUD()`가 virtual, `ABaroSimGameMode` 생성자가 public이라 `HUDClass` 교체만으로 충분하다. 플러그인 `Build.cs`의 `Sockets`/`Networking`/`Projects`는 Private 의존이라 전이되지 않으니 게임 `Build.cs`에 직접 추가한다. (근거: 3프로젝트 공용 서브모듈 → 한 줄 수정도 버전 범프·풀 리빌드·push·서브모듈 갱신 사슬.)
- **제어 API 무인증은 의도된 결정**: 씬 제어(8095)와 Hucoms CGI(카메라별 포트)는 토큰·API키·origin 검사가 없고 전 인터페이스에 열려 있다(플러그인이 포트를 여는 지점에서 선언 — 위 「기본 설정값」). 이 시뮬레이터는 **개발 보조용이며 내부망 전용**이므로 인증을 두지 않는다(2026-07-10 확정). 입력 하드닝은 값 클램핑(차종·색·번호판 정규화)까지가 범위다. MCP(8000)는 플러그인이 여는 포트가 아니므로 엔진 기본값대로 localhost로 남는다 — 전역 `DefaultBindAddress=any`로 이걸 깨지 말 것.

## 반복 금지

- **UE 버전업/신규 프로젝트 "컴파일 에러"는 대개 코드가 아니라 빌드환경(Target.cs V6→V7) 불일치** — Shared 환경 확인.
- MCP는 프로젝트별 옵트인(플러그인+AutoStart), 클라이언트 등록은 전역 1회. (상세: `ready_unreal` readme 부록 A/B)
- **에디터로 성능 테스트 금지**: 에디터는 포커스 잃으면 "Use Less CPU when in Background" 스로틀로 게임 틱 ~3.3fps(스트림도 같이 붕괴). 브라우저를 보는 순간 에디터는 항상 백그라운드다. **성능은 standalone `-game`으로 실측**(스탠드얼론은 스로틀 없음).
- **SceneCapture는 텍스처 스트리머에 시점을 등록하지 않는다**(UE5.8 엔진 소스 확인 — 뷰포트 뷰만 등록). 캡처 전용 카메라는 `IStreamingManager::AddViewInformation`(위치+줌 FOV)을 직접 등록해야 원거리 mip이 올라온다. 거리 기반 폴리지 컬링은 FOV 무시 → `LODDistanceFactor`로 줌 보정.
- Live Coding 활성(에디터/게임 실행 중) 상태에선 CLI 빌드 거부됨 — 프로세스 닫고 빌드. 새 UCLASS 추가는 어차피 풀 리빌드 필요.
- **패키징이 `Failed reading oplog from Zen ... Error while copying content to a stream`으로 죽는 건 코드 문제가 아니다.** zenserver는 상주 데몬이 아니라 **sponsor 프로세스(에디터/쿡)가 0이 되면 자결**한다. `BuildCookRun`은 쿡을 별도 프로세스로 돌린 뒤 UAT 본체가 oplog를 되읽어 스테이징하는데, UAT는 sponsor가 아니다(sponsor 슬롯 = UE 프로세스만 쓰는 공유메모리 `SponsorPids[8]`). 쿡이 끝나는 순간 서버가 내려가면 읽기가 스트림 중간에 끊긴다. `--owner-pid`는 종료 신호용이지 sponsor가 아니라 외부에서 심을 방법이 없다. **해법은 재시도**(쿡 결과는 Zen에 온전 → 캐시 히트로 통과). `Scripts/package.ps1`이 Zen 오류일 때만 1회 자동 재시도한다. 진단은 `%LOCALAPPDATA%\UnrealEngine\Common\Zen\Data\logs\zenserver*.log`의 `exiting since sponsor processes are all gone`으로.
- **클론 직후 `git submodule update --init --recursive` 필수** — `Plugins/baroCCTVSimulator`가 서브모듈이라 빠뜨리면 CCTV 클래스가 통째로 사라진 채 레벨이 열린다(참조 깨짐이 액터 소실로 보임).
- **uproject Plugins 배열에 없다 = 비활성이 아니다.** 프로젝트 로컬 플러그인은 `EnabledByDefault` 미지정 시 **기본 활성**(`FPlugin::IsEnabledByDefault`: Unspecified → `LoadedFrom == Project`). RYU가 여기 해당한다 — 끄려면 `"Enabled": false`를 명시해야 한다.
- **DefaultGame.ini 의 GameFeatureData 항목은 삭제도, bIsEditorOnly=False 도 금지.** 에디터/쿡은 GameFeatures 플러그인이 로드되어 이 규칙의 존재를 요구(없으면 쿡 실패)하고, 쿡된 게임엔 모듈이 없어 False 면 부팅마다 Ensure → 오류 보고 중 전 스레드 정지로 영구 먹통 가능(2026-07-17 현장 + 07-20 로컬 2회 재현). 정답은 **bIsEditorOnly=True 유지**(쿡은 만족, 게임은 스캔 스킵).
- **쿡 중 MCP 자동시작 포트 충돌 = 쿡 실패.** 쿡 커맨드릿이 에디터 사용자 설정(`bAutoStartServer=True`)을 읽어 :8000 리슨을 시도한다. 다른 프로세스(VS Code 등)가 8000 을 쥐고 있으면 그 Error 하나로 쿡 전체가 실패 판정된다. 패키징 전 8000 점유 확인, 충돌 시 `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` 의 `bAutoStartServer` 임시 False(패키징 후 복원).
- **Shipping 에서 config 오버라이드 시도 2가지가 조용히 무시된다**(2026-08-15 실측): ① `-ini:Game:[...]:Key=` 커맨드라인
  오버라이드는 **Shipping 에서 컴파일 아웃**(엔진 `ConfigCacheIni.h:57` `ALLOW_INI_OVERRIDE_FROM_COMMANDLINE = UE_SERVER || !UE_BUILD_SHIPPING`) —
  `DefaultGame.ini` 주석의 "패키징 빌드 포함"은 Development 패키지까지만 참이다. ② install 폴더의
  `Saved\Config\Windows\Game.ini` 도 안 읽힌다 — Shipping 의 Saved 는 `%LOCALAPPDATA%\baro_unreal\Saved` 로 간다.
  둘 다 에러 없이 기본값으로 뜨므로 "오버라이드가 됐다"고 믿기 쉽다. Shipping 오버라이드는 `-UserDir`
  경유가 정답(위 「기본 설정값」 다중 인스턴스).
- **`ScenePort` 가 이미 점유된 채 뜨면 두 번째 인스턴스는 실패하지 않는다 — 127.0.0.1 로 폴백 바인드된다**(2026-08-15 실측,
  **플러그인 v0.1.16 부터는 선점 프로브 + 즉시 종료로 원천 차단** — 이 함정은 ≤0.1.15 배포본에만 남아 있다).
  엔진 `FHttpServerModule` 이 0.0.0.0 바인드 실패 후 재시도에서 기본값(localhost)으로 여는 탓 — 결과는
  **스플릿-브레인**: localhost 요청은 새 인스턴스, LAN 요청은 기존 인스턴스가 받는다. 위 「기본 설정값」의
  "부팅 첫 리스너 localhost" 함정과 같은 뿌리이며, 역시 **localhost 테스트로는 절대 안 보인다**. 구 배포본
  다중 실행은 포트 계획으로 회피할 것(`netstat -ano | findstr :<port>` 로 리슨 주소까지 확인).
  같은 뿌리의 함정 하나 더: **`GetHttpRouter(port, bFailOnBindFailure=true)` 는 부팅 첫 리스너의 점유를 못
  잡는다** — 실제 바인드가 `StartAllListeners()` 로 미뤄져 라우터가 "유효"하게 돌아오고 "서버 시작" 로그까지
  정상으로 찍힌다. 부팅 경로에서 포트 점유를 감지하려면 라우터 생성 전에 원시 소켓으로 직접 바인드 프로브를
  할 것(v0.1.16 `SceneControlSubsystem::StartServer` 참조).

## 메모리 누수 원인과 대응 (2026-07-20)

- **증상(2026-07-16 현장)**: MJPEG 클라이언트가 붙어 캡처가 도는 동안만 프로세스 CPU 메모리 증가(+1.9~2.2MB/s @30fps 720p), 연결 종료 후 미반환. 35시간 가동 시 OOM(가상 72GiB, Binned3 large pool 살아있는 할당 105GB). 진단 번들 원본: `_localfiles/system_info.zip`.
- **원인(실측 수렴)**: UE 5.8 엔진 결함 — **소프트웨어 Lumen(SDF 트레이싱) + persistent ViewState SceneCapture** 조합에서 캡처 프레임마다 CPU 할당 미회수. LLM 태그 **`DistanceFields`**(+278MB/2.5분 실측)로 귀속. 라디언스캐시·스크린프로브 템포럴·서피스캐시 피드백·GDF 재캐시 억제·브릭 아틀라스 확대 cvar 전부 무효(A/B 10회). Lumen GI off 또는 ViewState 제거 시에만 소멸. `bAlwaysPersistRenderingState`는 트리거일 뿐 결함 주체가 아니다.
- **화질 트레이드오프(v0.1.5 의 한계)**: persist 무조건 off 는 누수는 잡지만 ViewState 부재로 캡처에서 Lumen 자체가 꺼져 암부가 뭉개진다(clipLo 15.2%→18.4%, 부스 내부 디테일 소실 — 스냅샷 A/B 실측).
- **근본 대응(플러그인 v0.1.6 + 앱 설정)**: persist 를 **HWRT Lumen 가용 시에만 허용**하는 가드(`baro.Capture.PersistRenderingState` cvar, 기본 1) + `bUseRayTracingIfEnabled=true`. 앱 쪽 `DefaultEngine.ini r.Lumen.HardwareRayTracing=True`. HWRT 경로는 누수 원천(SDF/GDF 프레임 갱신)을 쓰지 않는다. RT 미지원 GPU 는 자동으로 안전 모드(persist off = v0.1.5 동작)로 강등.
- **검증 수치(2026-07-20, RTX)**: 구 v0.1.4(persist+SW Lumen) +1.86~2.07MB/s → v0.1.5(persist off) -0.57MB/s(암부 저하) → **v0.1.6(persist+HWRT) +0.053MB/s + 암부 clipLo 14.8%(구 15.2% 대비 개선)**. SW 폴백 가드 -0.343MB/s. 부팅 3/3 클린.
- **"HTTP 요청당 누수"는 오판이었다(2026-07-20 저녁 정정)**: 처음엔 `jpeg.cgi` 폴링에서 요청당 ~1.3MB, `ptzf_status.cgi`에서 ~16KB가 새는 것으로 관측했으나 **재현으로 반증**됐다. 실측: 640×360 −433KB/req, QHD q20 −1171KB/req, QHD q92 −1168KB/req, ptzf_status(25B 응답) +0.5KB/req. QHD 8분 soak(60초 버킷) 기울기는 **+40.1 → −0.0 → +3.4 → −3.3 MB/s** — 첫 버킷만 워밍업이고 이후는 진동하며 순증 0(단조 증가 없음). 원인은 CCTV 시점을 텍스처 스트리머에 등록(`AddViewInformation`)하기 때문 — QHD 캡처를 처음 돌리면 mip/스트리밍 풀이 GB 단위로 부풀고 그게 "요청당 누수"처럼 보인다. **교훈: 메모리 측정은 부팅 후 100초+ 워밍업 뒤에, 60초 버킷 여러 개로 단조성까지 확인할 것.** 진짜 누수(SW Lumen persist)는 어떤 조건에서도 일관되게 +1.9~2.1MB/s로 재현됐다 — 이게 판별 기준이다.
- **재발 감시**: `BaroSystemMonitorSubsystem`(1s 샘플, 30s CSV·UE 로그, 120s slope, 20MB/min 이상 `LEAK_SUSPECTED`) + `BaroSystemMonitorWidget`(우상단 HUD). CSV=`Saved/Logs/BaroHealth-*.csv`. 큰 일회성 할당은 `baro.Health.ResetJumpMB`(기본 256MB)로 workload transition 분리.
- **별도 관찰**: `TEXTURE STREAMING POOL OVER` 경고는 이번 CPU 누수와 무관(텍스처 풀 예산). Smart App Control 차단은 unsigned plugin DLL 에 대한 Windows 정책 — 이교수님 처리 완료.

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
