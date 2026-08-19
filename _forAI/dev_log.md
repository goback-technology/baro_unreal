# Dev Log

## 목차

- [Entries](#entries)
- [PTZ 좌표·부호 규약 (Canonical — 진실의 출처)](#ptz-좌표부호-규약-canonical--진실의-출처)

## Entries

> **최신순(내림차순)**. 새 엔트리는 이 목록 맨 위에 추가한다.
> 각 엔트리는 그 시점의 스냅샷이다 — 나중에 사실이 바뀌어도 과거 엔트리는 고쳐 쓰지 않고,
> 최신 엔트리에서 정정한다.

- 2026-08-19: **플러그인 v0.1.14~0.1.17 을 GitHub 에 push. 이 저장소가 `baroQuantum` 의 레퍼런스 호스트가 됐다.**
  - 이교수님이 `baroQuantum` 최신화를 지시하며 **"`baro_unreal` 이 이것용 예제 파일입니다, 똑같이
    해야 합니다"** 라고 못박았다. 그래서 그쪽을 이 저장소에 맞추는 작업을 했고, 그 과정에서
    **이쪽 플러그인 커밋 6개(v0.1.14~v0.1.17 + 자동 카메라 기본값 off)가 로컬에만 있었다**는 것이
    드러났다 — `origin/main` 은 v0.1.13(`58b7835`)에 머물러 있었다. 두 호스트가 같은 커밋을
    가리키는데 원격에 없으니, **새로 클론하면 양쪽 다 서브모듈 핀이 해석되지 않는 상태**였다.
    `58b7835..b07bb8c` push 로 해소. 이 저장소의 워킹트리·핀은 바뀐 것이 없다.
  - `baroQuantum` 쪽에 옮긴 것: 플러그인 핀 `b07bb8c`, `DefaultGame.ini` 전량(포트·광학·슬루·
    캡처·생명주기), `r.Lumen.HardwareRayTracing=True`, RT 풀 1600MB, GameFeatureData
    `bIsEditorOnly=True` 가드, `DefaultGameUserSettings.ini`, `DefaultInput.ini` 창모드 4키,
    MCP·AllToolsets Editor 전용화, 콘텐츠 최소 세트 101 파일 788MB(레벨 제외).
    **주석을 걷어낸 `DefaultGame.ini` diff 가 `ProjectID`+`ProjectVersion` 한 덩어리만 남는다.**
  - 그쪽에서 확인된 것 중 이쪽에도 의미 있는 것: `C:\works\ue_prjs\baroCCTVSimulator` 는 플러그인
    문서(`INTEGRATION.md`·`README.md`)가 "단일 소스" 라 부르지만 **2026-07-03 에 버려진 스텁**
    (VersionName "1.0")이다. 진짜 소스는 두 호스트의 서브모듈이고, 둘 다 0.1.17 이다.
    또 플러그인 서브모듈의 `_forAI/memo.md` 가 버전을 0.1.10 이라 적고 있다(`.uplugin` 이 진실).
  - 상세는 `baroQuantum/_forAI/dev_log.md` 2026-08-19 엔트리 두 개.
- 2026-08-15: **플러그인 v0.1.16 — 포트 커맨드라인 스위치 3종(`-ScenePort`/`-BaseHttpPort`/`-BaseMjpegPort`). ScenePort 점유는 즉시 종료.**
  - **왜**: 아래 엔트리(같은 날)의 실측으로 Shipping 에서 포트를 지정할 방법이 없음이 확인됐다.
    A안(프로세스 분리) 운영의 전제라 이교수님 결정으로 구현(스위치 셋 다 + 충돌 시 즉시 종료).
  - **무엇**: 서버 시작 직전 `FParse::Value(FCommandLine::Get(), ...)` — `SceneControlSubsystem::StartServer`
    진입부(ScenePort, 사용처 7곳 자동 일관), `HucomsServerSubsystem::StartServers` 의 `BuildChannels` 직전
    (Base 2종, 명시 포트 카메라 우선 규칙 유지). config 로드 이후라 **커맨드라인 > `-ini:` > ini** 자연 성립.
    `run.ps1 -ScenePort N` 편의 파라미터, DefaultGame.ini 의 틀린 주석("패키징 빌드 포함") 정정.
  - **fail-fast 는 프로브로 구현했다 — `bFailOnBindFailure` 는 부팅 첫 리스너에서 헛돈다(신규 실측)**:
    소켓 바인드가 `StartAllListeners()` 로 미뤄져 포트가 점유돼 있어도 라우터가 "유효"하게 돌아오고
    "서버 시작" 로그까지 정상으로 찍힌다(1차 구현이 이걸로 헛돌아 스플릿-브레인 재현됨). 그래서
    라우터 생성 전에 원시 소켓(`FTcpSocketBuilder`) 으로 0.0.0.0 선점 프로브 → 실패 시 Error 로그 +
    `RequestEngineExit`. 재검증: 점유 포트로 띄운 3번째 인스턴스가 스스로 종료, localhost 폴백 리스너 0.
  - **검증**(에디터 `-game`, Development): 8095+8096 동시 기동(각각 0.0.0.0) · 우선순위(스위치 8096 이
    `-ini:` 8097 과 config 8095 를 이김 — 로그로 증명) · Base 2종 파싱 로그 · 충돌 즉시 종료 ·
    `camera-snapshot-contract`(8095: 32/33) · `lan-bind-contract`(LAN 핵심 전부 OK).
  - **부수 발견 3건**: ① **카메라 DELETE 는 라우트만 제거하고 HTTP 리스너는 프로세스 종료까지 리슨을
    유지한다**(엔진에 리스너 파괴 API 없음) — 같은 인스턴스는 그 포트 재사용 가능하지만 **다른 인스턴스는
    그 포트를 못 쓴다**. 다중 인스턴스의 카메라 포트 블록은 인스턴스별로 분리할 것.
    ② 그래서 고정 포트(8287/8288)를 쓰는 계약 테스트를 두 인스턴스에 연달아 돌리면 두 번째가 대량
    실패한다(간섭이지 결함 아님 — 신규 포트로 스폰 200/CGI 200/DELETE 200 확인). ③ 계약 테스트 2종에
    "부팅 카메라 존재" 시절 가정이 남아 카메라 0대 부팅에서 각 1건씩 항상 실패한다(`cameras[0]` undefined,
    "카메라 목록 비어있지 않음") — 테스트 쪽 손질 거리.
  - 커밋: 서브모듈 `fbfb629`(v0.1.16), 부모 `d77ee12`(포인터+config+run.ps1). Shipping 실증은 다음 패키징 때.
  - **후속(같은 날): 운용 주체 = 에이전트로 확정.** 시뮬레이터 인스턴스 관리(기동·포트 부여·헬스체크·재시작)를
    사람이 아니라 에이전트가 하게 한다(이교수님 방향). 그래서 자동 포트 탐색은 넣지 않는다 — 포트의 진실은
    에이전트 쪽 대장에 있고, 시뮬은 "지정 포트로 뜨거나 즉시 죽거나"만 한다(제3의 상태 없음). 운용 계약:
    기동 `-ScenePort=N` → 준비 `GET :N/scene/catalog` 200 → 실패 감지 = 프로세스 exit. 이 계약을
    `/scene/help`(scene-help.md 「인스턴스 기동·포트 계약」 신설)에 문서화 — 에이전트가 접속만 하면 배우는 자리.
    baro_kalory `_forAI/memo.md` 에도 소비자 측 계약을 기록.

- 2026-08-15: **다중 인스턴스 검토·실측 — A안(프로세스 분리) 유지 확정. Shipping 다중 실행의 정답은 `-UserDir`.**
  - **질문**: 한 기기에서 시뮬레이터 여러 개 — 포트만 바꿔 프로세스를 분리(A)할지, 한 인스턴스가
    여러 월드를 관리(B)할지. 구현 없이 검토 + 메모리 실측으로 판정(이교수님 결정: A 유지).
  - **구조 정리**: 시뮬레이터 본연의 포트는 `ScenePort` 하나다. 카메라 포트는 장치 속성(런타임 스폰은
    명시 필수)이고, 충돌은 이미 롤백+400 으로 처리된다(`SpawnCameraRuntime` — 같은 인스턴스 내 중복은
    사전 검사, 타 프로세스 점유는 OS bind 실패로 잡음). B는 UE 에서 멀티 UWorld 가 아니라 "한 월드에
    주차장 존 복제"만 현실적인데(UGameEngine 은 게임 월드 컨텍스트 1개 상정), 존 API 리팩터링·하늘/시간
    공유(UDS 월드당 1개)·단일 장애점이 대가다.
  - **실측**(Shipping v0.2.8 패키지, `LV_Park_sim_01`, 유휴·카메라 0대, 워밍업 90s+, RTX 5060 8GB / RAM 64GB):
    인스턴스당 **RAM 워킹셋 4.0GB / 커밋 8.8GB / VRAM 2.2GB**. GPU 총량이 1.9(데스크톱)→4.1(1개)→6.4GB(2개)로
    정확히 +2.2GB 계단 — 공유분 0, 순수 복제 비용. **N=2 여유(6.4/8.15GB), N=3 은 VRAM 초과(~8.6GB)** →
    이 GPU 의 실용 한도는 2. B 의 이득은 "추가 월드당 VRAM 약 2GB 절약"으로 수치화됨 — 한 GPU 에서
    월드 3개 이상을 요구하기 전까지는 A 가 이긴다.
  - **Shipping 다중 실행 레시피(검증 완료)** — 사본 불필요, 같은 `Packaged/Win64` 폴더에서:
    `baro_unreal.exe -UserDir=D:\sim_inst2` + 사전에 `<UserDir>\baro_unreal\Saved\Config\Windows\Game.ini` 에
    `[/Script/baroCCTVSimulator.SceneControlSubsystem]` / `ScenePort=8096`. 두 인스턴스가 각각 0.0.0.0:8095/8096
    으로 리슨함을 확인. `-UserDir` 는 로그·BaroHealth CSV·GameUserSettings 도 인스턴스별로 갈라 준다
    (기본 Shipping Saved 는 `%LOCALAPPDATA%\baro_unreal\Saved` — 전 인스턴스 공유라 이대로 두면 CSV 가 섞인다. 실측으로 목격).
  - **안 되는 것 2건(실측 반증)**: ① `-ini:Game:[...]:ScenePort=` 커맨드라인 오버라이드는 **Shipping 에서 컴파일 아웃**
    (엔진 `ConfigCacheIni.h:57` — `ALLOW_INI_OVERRIDE_FROM_COMMANDLINE = UE_SERVER || !UE_BUILD_SHIPPING`).
    `DefaultGame.ini` 주석의 "패키징 빌드 포함"은 Development 패키지까지만 참이다. Development·에디터 `-game` 에선
    정상 동작(스위치 반복 사용 지원 — 엔진 파서가 토큰 단위 처리, 콤마 합치기는 legacy·따옴표와 충돌).
    ② install 폴더의 `Saved\Config\Windows\Game.ini` 는 Shipping 이 읽지 않는다(Saved 가 LOCALAPPDATA 로 가므로).
  - **함정 발견**: `ScenePort` 가 충돌하면 두 번째 인스턴스가 깨끗이 실패하지 않는다 — 엔진 HttpServerModule 이
    재시도하며 **127.0.0.1 로 폴백 바인드** → localhost 요청은 새 인스턴스, LAN 요청은 기존 인스턴스가 받는
    스플릿-브레인. 로컬 테스트만으로는 안 보인다. 카메라 포트와 달리 엔진 영역이라 포트 계획으로 회피할 것.
  - `memo.md` 반영은 이 작업 마무리 시점에 하기로 함(이교수님 지시). 실험 산출물(사본·UserDir)은 삭제, 원본 무변경.

- 2026-08-12: **플러그인 v0.1.15 — 카메라 별명(Note)이 액터에 산다. 앱 0.2.8 배포·복원 검증.**
  - **왜**: 스폰 스펙의 `Note` 를 `SetActorLabel` 로만 흘렸는데 액터 라벨은 **에디터 전용**이라
    패키지에서는 남지 않았다 — 이름이 저장되지도, API 로 되읽히지도 않았다. 커미셔닝 콘솔이
    "카메라 이름"의 정본을 씬에 두기로 하면서(`camera.note` 가 이름, 웹은 사본을 안 든다) 이
    구멍이 실사용을 막았다.
  - **무엇**: `APTZCamera.Note`(UPROPERTY, `PTZ|Identity`) 신설 — 식별자가 아니라 별명(빈 값·중복
    허용, 식별은 액터 이름과 hucomsPort). 스폰(config·API) 시 기록, `GET /scene/cameras` 에
    `note` 실림, 스냅샷 저장·복원이 별명을 왕복, `PATCH /scene/cameras/:id` 가 `note` 를 받는다.
    **note 만 바꾸는 PATCH 는 레벨 저작 카메라도 허용** — 옮기지 못하는 것은 자세가 레벨의
    것이어서지, 이름까지 레벨의 것이어서가 아니다. `docs/scene-help.md` 동반 갱신.
  - **소비자**(참고): `baro_calory` 백엔드가 note 로 기기 이름을 파생하고(sim-devices), 평면도의
    설치/컨트롤 분리·rebase·hfovDeg 지시가 이 씬 계약(mount.baseYaw·PATCH yawDeg/pitchDeg·
    intrinsics.zoomHfov) 위에서 돈다 — 플러그인 쪽 추가 수정은 필요 없었다(FK 액터 구성 그대로).
  - **배포**: 앱 `ProjectVersion 0.2.8` 로 DWarf 재배포(절차 정본 docs/deploy-dwarf.md).
    **절차에 0번이 생겼다**: 배포 전 `GET /scene/snapshot` 을 파일로 받아 두고, 기동 뒤 복원 —
    2026-08-11 이 순서를 뒤집어 런타임 카메라 두 대를 잃었다(스냅샷 `_localfiles/deploy/`).
    검증: 라이브 `pluginVersion 0.1.15`, note 왕복(스폰→이름변경→스냅샷→복원) 정상.

- 2026-08-11: **`LV_Park_sim_01` — 한 열이 2.49° 돌아가 있었다. 폈다(저장 완료).**
  - **발견 경로가 웹 UI 였다.** 커미셔닝 콘솔의 평면도에서 주차면이 계단처럼 밀려 보인다는 지적 →
    `/scene/slots` 를 그룹별로 재 보니 여덟 면이 전부 `yaw −87.507607`, 그 열이 실제로 뻗은 방향도
    2.45°(= `yaw+90`). 나머지 네 그룹(0·90·−90·−180)은 열 방향과 `yaw+90` 이 소수점까지 일치했다 —
    **한 열만 손으로 놓다 틀어진 것**이지 그림의 결함이 아니었다.
  - **수정**: `BP_ParkingSlot_C_{1,4,5,6,7,8,0,9}` 8개를 `yaw −90`, `y = −1145.335239`(8개 평균 — 가장 적게
    움직이는 값, 끝단 이동 +35.9/−32.5 cm)로. x·z 는 불변. 열 방향으로 재던 간격 250 이 249.76 이
    되지만 차이가 2.4 mm 라 x 는 손대지 않았다.
  - **돌려도 안전한 근거를 먼저 확인했다**: `BP_ParkingSlot` 이 주차선을 자기 컴포넌트로 들고 있다
    (`Decal1~4` + 주차칸 스토퍼 + 장애인/임산부/전기차 표시). 도색이 액터를 따라 도므로 어긋날
    바닥 아트가 따로 없다(주변 StaticMesh 는 `Cube3`·`Baked_Buildings_sim01`). 인접 소품은 각자
    `yaw 85.92`(Sidewalk)·`100.53`(Delineator)로 제각각이고 도로는 0 — 손 배치라는 정황과 맞는다.
  - **건드리지 않은 것**: `BP_ParkingSlot_C_0`(type `1m`) 주변 간격 177.8·169.8 은 결함이 아니다 —
    좁은 자리(폭 1m)가 5m 자리 둘 사이에 낀 정상 배치다(125 + 50 = 175). `get_actor_bounds` 로
    5m 슬롯이 **폭 256 × 깊이 500** 임을 확인했다(에디터 빌보드가 bounds 를 256 정육면체로
    부풀리므로 1m 자리의 깊이는 이 방법으로는 못 잰다).
  - **검증**(standalone `-game`, 플러그인 v0.1.14): 편 열 8면 `yaw −90`·y 편차 **0.0000 cm**,
    `/scene/cameras` **0대**(레벨에 PTZ 액터 없음 + `bAutoSpawnCameraIfNone=False` + `SpawnCameras`
    전부 주석 = 기본 카메라 없이 뜨는 구성이 맞다), `/scene/help` 가 신규 라우트를 서빙
    (`POST /scene/cameras` · `PATCH·DELETE /scene/cameras/:id` · `GET·POST /scene/snapshot`),
    카메라 생명주기 왕복(스폰 :8085 → PATCH 높이 6.00→9.00 m → 스냅샷 → 삭제) 정상,
    `ptzf_status.cgi` 200 · `jpeg.cgi` 200/1.42 MB/0.23 s. 캡처 이미지로도 격자가 반듯함을 확인.
  - **레벨은 git 밖이다**(`Content/` gitignore) — 저장은 되돌릴 수단이 없어 에디터에서 사람이 눌렀다.
    액터 편집은 MCP(`ActorTools.set_actor_transform`)로, 저장 결정은 사람이 — 이 경계를 유지한다.

- 2026-08-11: **`LV_Park_sim_01` — 한 주차면 열이 2.49° 돌아가 있었다. 폈다(웹 평면도의 「비뚤어짐」 원인).**
  - **증상은 웹UI 에서 왔다**(`baro_kalory` 카메라 배치 평면도에서 한 열이 계단처럼 밀려 보임). 그림을 의심하기 전에
    `/scene/slots` 로 쟀다: 24면을 yaw 로 묶어 **각 열이 실제로 뻗은 방향**과 `yaw+90` 을 대조.
    | yaw | 개수 | 열 방향 | yaw+90 |
    |---|---|---|---|
    | 0 | 6 | −90.00° | 90.00° |
    | 90 | 4 | 0.01° | 180.00° |
    | **−87.51** | **8** | **2.45°** | **2.49°** |
    | −90 | 4 | 0.01° | 0.00° |
    | −180 | 2 | 0.00° | −90.00° |
    네 그룹은 소수점까지 일치 = 그림이 맞다. **−87.51 열만 월드 축에서 2.49° 돌아 있었다**(손 배치의 흔적 —
    옆 소품들도 `BP_Sidewalk_C_4` yaw 85.92, `BP_Barrier_Delineator_Poles_C_1` yaw 100.53 으로 제각각. 도로는 yaw 0).
  - **돌려도 안전한 근거를 먼저 확인했다**: `BP_ParkingSlot` 이 주차선을 자기 컴포넌트로 들고 있다
    (`Decal1~4` + 주차칸 스토퍼 + 장애인/임산부/전기차 표시). 도색이 액터를 따라 도므로 어긋날 바닥 아트가 없다
    (주변 StaticMesh 는 `Cube3`·`Baked_Buildings_sim01` = 건물). **이 확인 없이 돌리면 도색과 슬롯이 갈라진다.**
  - **적용**(MCP `ActorTools.set_actor_transform`, 8면): `yaw −87.507607 → −90`, `y` 는 8면 평균 **−1145.335239** 로 통일,
    `x`·`z` 불변. 평균을 쓴 이유는 최대 이동이 가장 작아서다(양끝 +35.9 / −32.5 cm). 열 간격은 열을 따라 재면
    250 → 249.76 이 되는데 차이가 2.4 mm 라 x 는 손대지 않았다.
  - **검증**(standalone `-game`): 편 열 8면 `yaw` 전부 −90, **y 편차 0.0000 cm**. 카메라 스폰(:8085) → `PATCH` 높이
    6.00→9.00 m → 스냅샷 → 삭제까지 정상. `ptzf_status.cgi` 200, `jpeg.cgi` 200(1.42 MB, 0.23 s). 캡처 화면에서도
    반듯한 격자로 나온다.
  - **`1m` 슬롯은 결함이 아니다.** `BP_ParkingSlot_C_0` 의 클래스가 `BP_ParkingSlot_1m_C` 라 `/scene/slots` 가 `"1m"` 을
    준다(`SlotTypeFromClass` = 클래스명에서 파생). 그 자리 좌우 간격이 177.8·169.8 인 것도 정상이다 — 5m 자리 반폭
    125 + 1m 자리 반폭 50 = 175. **`5m`/`6m` 의 숫자는 깊이**이고(실측: 5m 슬롯 폭 256 × 깊이 500 cm), `1m` 은 그 사이에
    낀 좁은 조각이다.
  - **`Content/` 는 git 밖이라 레벨 저장은 되돌릴 수단이 없다** — 적용만 하고 저장은 사용자가 확인 후 직접 했다.
    현장 sim 에 반영되려면 저장 → 재패키징 → 재배포까지 가야 한다(이 엔트리의 작업 뒤 Shipping 패키징 진행).
  - **쿡을 위해 에디터를 닫을 필요는 없다**: 충돌 지점은 MCP 자동시작(:8000) 하나뿐이라
    `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` 의 `bAutoStartServer` 를 쿡 동안 `False` 로 두면 된다
    (memo 「반복 금지」의 권장 방식 — 패키징 후 `True` 복원).

- 2026-08-06 (2차): **앱 v0.2.5 / 플러그인 v0.1.14 — 포트를 여는 코드가 자기 LAN 노출을 선언한다. 위 결함 수정.**
  - 수정: `FScopedHttpListenerOnAllInterfaces`(`HttpListenerBind.h/.cpp`) — 리스너를 만드는 동안만 그 포트의
    `BindAddress=any` 오버라이드를 ini 캐시에 끼우고 원복한다. 적용 지점 2곳: `StartChannelServers`
    (카메라 CGI, 부팅·런타임 공용)와 `USceneControlSubsystem::StartServer`(씬 8095).
  - **엔진 함정 두 개를 실측으로 밟았다.** ① `[HTTPServer.Listeners]` 는 캐시라 GConfig 수정 뒤
    `TSOnConfigSectionsChanged` broadcast 가 없으면 옛 값이 이긴다. ② 소켓은 `GetHttpRouter` 가 아니라
    `StartListening()` 에서 열리는데, **모듈 비활성 상태에서 만든 리스너는 열기가 나중의 `StartAllListeners()`
    로 미뤄진다** — 그래서 1차 수정 후에도 부팅 첫 리스너인 씬 8095 만 127.0.0.1 로 열렸다(카메라 포트는
    그 시점엔 이미 활성이라 스코프 안에서 0.0.0.0 으로 바인드). 소멸자가 원복 **전에** `StartAllListeners()` 를
    부르게 해서 덮었다. 진단 근거는 `Created new HttpListener on 127.0.0.1:8095` 와 `Starting all listeners...`
    의 로그 순서다.
  - ini 는 포트 목록을 지웠다 — 목록 방식이 런타임 포트를 담을 수 없다는 게 이 결함의 뿌리라 두 진실을
    남기지 않았다. 사람이 명시한 포트는 코드가 존중한다(NIC 고정용).
  - 검증(standalone `-game`, 로컬 PC 192.168.0.211): 전 리스너 `0.0.0.0`(8081~8086·8095·스폰 8287),
    **루프백이 아닌 주소로** 스폰 카메라 CGI 응답(tiltpos 3000)·스냅샷 1.06MB, 삭제 후 포트 닫힘.
    회귀: `camera-snapshot-contract.mjs` 33개 + `offset-contract.mjs` 전부 통과. 종료 후 `Saved/Config`
    오염 0(원복이 실제로 동작).
  - **신규 회귀 가드 `tools/scene-test/lan-bind-contract.mjs`** — 루프백 주소를 거부하고 비루프백으로만
    접속한다. 기존 계약 테스트 33개가 이 결함을 전부 통과시킨 이유가 localhost 였기 때문이라, 같은 함정을
    다시 안 밟으려면 이 테스트를 배포 검증에 넣어야 한다(`--host <배포기IP>`).
  - **배포까지 완료(같은 날)**: `package.ps1 -Config Development -Zip` → `baro_unreal_sim_v0.2.5_20260806.zip`
    (2.98GB) → DWarf 청크 업로드 36청크 280초(10.8MB/s) → `tar -x` 7.3초 → pm2 restart + save.
    배포기(192.168.0.22)에서 `pluginVersion 0.1.14`·카메라 6대·8083 실렌더 1.2MB 확인.
    **본 판정: `lan-bind-contract.mjs --host 192.168.0.22` 전 항목 통과** — 30분 전 같은 호출이
    타임아웃이던 스폰 카메라 CGI(:8287)가 원격에서 응답한다. 교체 전 기준선도 같은 방식으로 재현해 뒀다
    (v0.2.4: 스폰 200 + CGI 원격 도달 불가).
    ini 포트 목록을 지운 뒤였으므로 이 통과는 **패키지 빌드에서도 코드 경로가 동작함**을 함께 증명한다.
  - **부수 발견·수정: `package.ps1` 이 더티 빌드를 clean 으로 표기하고 있었다.** PowerShell `-notmatch` 는
    대소문자를 무시해, 서브모듈 포인터(`' m '`) 필터가 워킹트리 수정(`' M '`) 줄까지 전부 버렸다.
    `-cnotmatch` 로 고쳤고, 이번 zip 의 `.info.txt` 는 `4e40276-dirty` / `58b7835-dirty` 로 정정했다.
  - **미수행: 커밋·push·서브모듈 핀 갱신**(이교수님이 직접). 배포본은 커밋 전 워킹트리로 구운 것이라
    `.info.txt` 가 `-dirty` 다 — 커밋 후 재현하려면 그 커밋으로 다시 구워야 한다.
    배포기의 옛 zip `baro_unreal_sim_v0.2.4_20260806.zip`(2.98GB)은 지우지 않고 남겨 뒀다.
- 2026-08-06: **런타임 스폰 카메라의 CGI 가 원격에서 안 열린다 — 원인은 스폰 코드가 아니라 포트별 바인드 주소.**
  - 접수(소비자 세션, baro_calory 쪽): `POST /scene/cameras` 로 만든 카메라가 MJPEG(:8297)은 열리는데
    CGI(:8287)는 연결 거부. 포트쌍 8287/8297·8087/8097 **두 번 모두 동일**(포트 대역 문제 아님), 기존
    6대(8081~8086)는 정상. 보고자 가설은 "스폰 경로가 MJPEG 만 띄우고 CGI 서버를 안 띄운다".
  - **그 가설은 소스와 맞지 않는다.** `SpawnCameraRuntime` 은 부팅 경로와 같은 `StartChannelServers` 를
    호출하고, 그 함수는 라우터 획득(`GetHttpRouter(..., bFailOnBindFailure=true)`) → CGI 라우트 8개 바인드 →
    **그다음** MJPEG 순서다. 라우터가 없으면 스폰 자체가 실패(액터 파괴 + 에러)한다. 즉 **200 + MJPEG 개방은
    CGI 라우터가 이미 살아 있었다는 증거**다.
  - 진짜 원인 = **바인드 주소**. UE 5.8 `FHttpServerListenerConfig::BindAddress` 기본값이 `"localhost"` 이고,
    LAN 노출은 `DefaultEngine.ini [HTTPServer.Listeners] +ListenerOverrides=(Port=N,BindAddress=any)` 로
    **포트별로만** 열린다(우리 ini 등재: 8081~8086·8095 — 전역 `any` 를 안 쓰는 건 에디터 MCP :8000 노출 때문).
    런타임 스폰 포트는 부팅 시점 ini 에 있을 수 없으므로 127.0.0.1 에만 바인드된다 → 원격 거부. MJPEG 은
    플러그인 자체 `FTcpListener` 라 항상 0.0.0.0 → **한쪽만 열리는 비대칭이 그대로 설명된다.** 시도한 두 포트가
    둘 다 미등재라 포트쌍 교체가 판별력을 못 가졌다.
  - v0.1.13 계약 테스트 33개(`tools/scene-test/camera-snapshot-contract.mjs`)가 못 잡은 이유: **같은 호스트에서
    돌렸다.** 스폰 즉시 CGI 응답을 실제로 확인했지만 그건 localhost 응답이었다 — 원격 소비가 전제인 기능은
    회귀도 원격에서 걸어야 한다.
  - 남은 실기 확인(1분): sim 호스트에서 `netstat -ano | findstr :<스폰포트>` 가 `127.0.0.1:<port>` 인지.
    소스상으로는 확정이지만 실측으로 못 박아 두는 편이 낫다.
  - **이번 세션은 문서화만 — 코드·ini 무변경.** 수정안 2가지(플러그인에서 GConfig 런타임 주입 / ini 대역 예약)와
    검증 절차는 `plan.md` 「능동 작업」 첫 항목, 상시 규칙은 `memo.md` 「기본 설정값」에 넣었다.
    소비자 판단은 "급하지 않음"(기존 카메라 시연은 무영향).
- 2026-08-03 (3차): **플러그인 v0.1.13 — 카메라 런타임 API + 씬 스냅샷. BEV 자율 파인튜닝 루프 대비.**
  - 이교수님 방향: BEV(object3d)류를 에이전트가 자율 파인튜닝하는 기능 — 과거(카메라 고정 config
    스포너)의 데이터 다양성 한계를 풀려면 카메라 스폰/이동/삭제와 씬 저장/복원이 RPC 로 필요.
  - `POST/PATCH/DELETE /scene/cameras(/:id)` + `GET/POST /scene/snapshot`. 레벨 저작 카메라 403 보호.
    상세는 서브모듈 dev_log v0.1.13 · `docs/scene-control-api.md`. 검증: `tools/scene-test/`
    `camera-snapshot-contract.mjs` 33개 전수 + offset 회귀 통과. 앱 버전 0.2.4 유지(앱 코드 무변경).
  - **후속 확정**: BEV 훈련기는 별도 저장소 `C:\works\BEV_Trainer`(RPC·에이전틱 자동화,
    `paper_works/object3d_model_review` 는 리뷰/참고용) — 이 저장소 범위 밖.
- 2026-08-03 (2차): **플러그인 v0.1.12 — 자기서술 `GET /scene/help`. 소스 없는 세션이 포트 하나로 사용 가능.**
  - 이교수님 제기(소스·저장소 없이 쓰려면 help API 필요) + 지적(산문은 빌드에 박지 말고 텍스트로).
    산문은 플러그인 `docs/scene-help.md`(무빌드 수정·라이브 토큰), C++ 는 서빙만. 상세는 서브모듈
    dev_log v0.1.12 와 `docs/scene-control-api.md` 「GET /scene · GET /scene/help」 절.
  - 앱 코드 무변경이라 앱 버전은 0.2.4 유지. 검증: 리빌드 후 실서빙·토큰 치환·무빌드 수정 실증·
    offset-contract 재통과.
- 2026-08-03: **앱 v0.2.4 / 플러그인 v0.1.11 — 차량 배치 변형(`offset`). 주차면 소속을 유지한 채 비뚤게.**
  - 이교수님 요구: 차량을 조금씩 다르게 배치(좌우로 비껴·10도쯤 틀기·180도 반대). 먼저 "지금 RPC 로
    되는가" 를 조사해 보고했다 — **자유 `transform` 으로 렌더는 같게 만들 수 있지만 주차면 소속이
    끊긴다**(slotId=null, 점유 미등록, carId 조인 소멸). 그래서 offset 추가로 진행 승인.
  - 계약·구현·짐벌 규약 상세는 서브모듈 dev_log(v0.1.11)와 `docs/scene-control-api.md`
    「배치 기준과 변형」 절. 요지: 최종 배치 = `Offset * Base`, 변형의 축은 **주차면 로컬축**,
    offset 은 누적 델타가 아니라 값, 주차면을 옮기면 변형이 따라간다.
  - 검증: UE 자동화 5/5, standalone `-game` 실기동에서 JS 참조 구현과 10개 자세 대조(**위치 0cm,
    회전 최대 5.6e-12°**), 같은 차·같은 자리에 offset 만 다르게 준 렌더 A/B 로 육안 확인.
  - **검증은 소비 저장소가 아니라 이 저장소의 독립 하네스로 한다(이교수님 확정, 같은 날).**
    처음에 baro_calory 라우터·Fake·CLI 까지 같이 고쳤다가 "시뮬레이터 업그레이드 때 baro_calory 는
    손대지 말 것, 별도 테스트 프로그램으로" 지시로 **baro_calory 는 전량 복원**했다(복원 후 455/455,
    워킹트리 깨끗). 대신 `tools/scene-test/` 신설 — `offset-contract.mjs`(UE 회전 규약 JS 참조
    구현 + 수치·동작 계약 전수 검증, 의존성 0) + `jitter-demo.mjs`(A/B 데모 씬). 실행: sim 띄운 뒤
    `node tools/scene-test/offset-contract.mjs`. **결과: 소비자 경유(웹 /simulator·pnpm sim) 로는
    offset 이 아직 안 나간다** — baro_calory 라우터가 화이트리스트 검증이라 모르는 필드를 버린다.
    그쪽 연동은 이교수님이 별도로 결정할 때 한다.
  - 버전: 앱 `ProjectVersion` 0.2.3→**0.2.4**, 플러그인 0.1.10→**0.1.11**.
  - **미수행**: 커밋·push·서브모듈 핀 갱신·재패키징. plan 의 「캡처 페이싱 계단 양자화 1줄 수정」은
    이번 플러그인 갱신에 **넣지 않았다** — 스트림 fps 거동 변경이라 별도 실측이 따라야 해서
    배치 변경과 한 버전에 섞지 않았다. plan 항목은 그대로 남긴다.
- 2026-07-24 15:00 KST: **앱 v0.2.2 / 플러그인 v0.1.10 — 스트림 캡처 비동기 GPU 리드백. 6대 카메라당 +73%.**
  - 이교수님 요구: 6대 동시에도 카메라당 fps 유지("품질 양보 불가, 구조로 개선"). 프로파일링으로
    병목 = 동기 ReadPixels 의 GPU 완료 대기(게임 스레드 블로킹)임을 규명, 4각도 적대적 검증 후 구현.
  - 플러그인 v0.1.10: FRHIGPUTextureReadback(비동기) + 워커 인코딩. 상세는 서브모듈 dev_log.
  - 실측(standalone -game): 6대 카메라당 3.3→5.7fps(+73%), 게임 틱 2.5→24~60fps, 화질 바이트 동일,
    종료 크래시 0. baro_calory 연동(로컬 config)도 이 세션에서 별도 검증 완료.
  - config: `bAsyncStreamCapture=True`(킬스위치), ProjectVersion 0.2.1→0.2.2. 재패키징(MCP 토글).
  - 한계: 6×30fps 는 GPU/VRAM 물리 한계로 불가(6대 all-warm GPU 80%/VRAM 94%) — 이교수님 인지·수용.
- 2026-07-24 11:50 KST: **앱 v0.2.1 / 플러그인 v0.1.9 — "쓰는 카메라만 켠다" 재설계. 유휴 GPU 47.7%→2.6%.**
  - 발단: 패키지 실행 HUD 에서 **클라이언트 0·캡처 0 인데 게임 틱 2.5 fps, GPU 프레임 1150 ms**,
    RT 지오메트리 상주 1.245 GiB(예산 400 MiB)·텍스처 풀 초과 경고. 이교수님 지시 —
    "안 쓰는 것까지 켜고 있으면 안 된다. 최소 변경 말고 근본 재설계."
  - **진짜 원인은 시뮬 성능이 아니라 '누가 쓰는지 안 보인 것'이었다.** DGX(192.168.0.220)의
    uvicorn 이 카메라 6대의 `jpeg.cgi` 를 HTTP keep-alive 로 상시 순회 폴링 중이었고, HUD 의
    "클라이언트" 카운터는 **MJPEG 클라이언트만** 세므로 화면엔 "클라이언트 없음"으로 보였다.
    keep-alive 라 연결의 원격 포트가 안 바뀌어 "연결만 남았다"로 한 번 더 오판했다
    (원격 포트 불변 ≠ 요청 없음). 이교수님이 DGX 측을 중단하고서야 깨끗한 실측이 됐다.
  - 구조적 결함(위 폴링이 없어도 남는 문제): 플러그인의 캡처 자원 **해제 경로가 `EndPlay` 뿐**이라
    한 번 쓴 카메라는 종료까지 SceneCapture+ViewState+RT 를 물었고, `AddViewInformation` 은
    6대를 무조건 매 틱 등록했다. → v0.1.9 수요 기반 생명주기로 재설계(상세는 플러그인 dev_log).
  - 함정: `MaxActiveCameras=1` 만 넣으면 6대 순회 폴링과 충돌해 **1분에 173회 재생성 churn**
    (원래보다 나쁨). `MinWarmSeconds=5` 유예를 넣어 0회로 잡았다.
  - 실측(standalone -game, RTX 5060 8GB): GPU 3D **47.7% → 2.6%**, WorkingSet **21.6 → 12.9 GB**.
    유휴 30초 후 6대 자동 해제, 카메라 전환 시 이전 1대만 해제, warm 재요청 0.091s(콜드 0.191s).
  - 패키징: `Config/DefaultGame.ini` `ProjectVersion=0.2.1`. 쿡은 MCP/AllToolsets 임시 off +
    GameFeatures 에디터전용 토글 절차 그대로(2026-07-23 확립).
  - 패키지 유휴 실측: WorkingSet **15.2 → 4.17 GB**, Private 16.4 → 8.61 GB, GPU 3D 3.3%.
  - 튜닝 확정(이교수님): `IdleReleaseSeconds=10`(30초는 길다), `MaxActiveCameras=1` 유지하되
    2 이상도 즉시 가능(LRU 검증 완료 — ini 수정 없이 커맨드라인 오버라이드로 확인).
    콜드 첫 스냅샷 약 3초는 **워밍업이 아니라 자원 생성(2.2초)이 주범**이라 워밍업은 4로 유지하고
    이유를 문서화하는 쪽으로 결정(플러그인 DEVELOPER_GUIDE §9).
- 2026-07-23 17:14 KST: **v0.2.0 26시간 중간 soak — 이교수님 "합격" 판정. 누수 종결 확인.**
  - 배포 기기(다른 PC, `C:\Users\lsj-goback\Desktop\baro_unreal_sim_v0.2.0_20260722`)에서 07-22 14:55
    가동 개시, **26.3시간 시점**의 `BaroHealth-20260722-145513.csv`(3096 샘플, 391KB) 조기 회수·분석.
    48시간 완주 전이지만 정착 이후 완전 평평 구간을 하루치 확보해 결론이 났다.
  - **physical_mb 궤적이 누수가 아니라 점근선(plateau)**: 0~8h +58.6 MB/h(캐시·풀 워밍업) → 8~16h
    +2.5 MB/h → 16~26h **+0.6 MB/h** → 23~26h **0.0 MB/h(0.001 MB/min)**. 9h에 ~4660 MB 도달 후
    17시간 동안 4660→4684(+24 MB)로 사실상 정지. 구 결함 **111 MB/min**(35h 만에 OOM) 대비 정상 상태
    **0.0 MB/min** — 목표했던 결과가 실기 하루 가동으로 확인됐다. virtual/vram 도 4h 내 정착 후 밴드 진동
    (vram 6846~7034 MB, budget 11347 의 62% 로 천장 여유 있음).
  - **시험 유효성 통과(가장 중요)**: 1코어 CPU ≥40% 샘플 **2998/3096**, <5% 유휴 **0/3096** — MJPEG
    소비자가 26시간 내내 붙어 있어 **누수 경로가 실제로 계속 돌아간 상태**의 평평함이다. "소비자 미부착
    거짓 합격"(dev_log 2026-07-22 함정 #2)이 아님이 데이터로 증명됐다.
  - **`LEAK_SUSPECTED` 13건 — 미리 못박은 "0건" 기준 미달을 사후 재해석 없이 정직하게 검토**: 10건이
    첫 8h 정착 버스트(특히 3.6h 에 physical 4373→4582 급등 후 **4.8h 에 4409 로 완전 회수** — 순간부하
    후 할당자 반납, 누수의 반대 증거), 3건이 25.7h 에 physical 이 오히려 4674→4656 로 **내려간** 구간의
    2분-창 slope 노이즈. 지속 상승은 **0건**. → 헬스 모니터의 20 MB/min 임계가 순간 스파이크에 예민한
    것이지 누수가 아니다(모니터 튜닝 관찰, 별건 후보).
  - **미완(48h 완주 시 최종 확인)**: `baro_unreal.log`(가동 중 잠금 — 시뮬 생존 방증)·`Saved/Crashes/`
    는 아직 못 봤다. 종료 시 회수해 GameFeatureData Ensure 0건·Crashes 공란을 최종 확정한다.
    이 시점 결론은 **누수·OOM 위험 없음**이고, 다음 관문은 **Shipping 전환**(plan 참조).

- 2026-07-23: **scene-control-api.md 정합성 감사(215건 대조) + 정정 12곳 반영. _forAI 보강. (v0.1.7 문서 후속)**
  - 배경: 응용 SW팀 피드백(차종 치수·카메라 높이·화각 노출 요청) 접수 → README에 「프로젝트 문서(docs/)」
    「외부 소비자와 협업 컨텍스트」 신설, plan 에 확장 항목 등재. **v0.1.7 확장 구현 자체는 이교수님이
    별도 세션에서 완료**(`boundsCm`·`groundReference`·`heightAboveReferenceGroundCm`·`intrinsics.zoomHfov`
    — 계약·검증 상세는 `plan.md` 「/scene API 확장 계획」, 커밋/push/서브모듈 핀은 미수행 상태).
  - 감사 방법: 문서의 사실 주장 215건을 UE 플러그인·baro_calory 소스와 대조(9섹션 병렬 감사, 불일치는
    3인 적대 검증 2/3 다수결). v0.1.6 문서 기준으로 돌았고 도중 v0.1.7 문서 갱신이 겹쳐 **전 건을
    v0.1.7 문서·코드로 재대조**한 뒤 반영했다. (검증 에이전트 일부가 세션 한도로 중단 — 그 몫은 수동 재검증.)
  - **v0.1.7 갱신이 이미 해소한 것**: `wideHFovDeg` 69.88(선형가설 유물 — 실측 57.14), setcenter "선형 모델"
    서술(→ tan+구면), FOV 출처 표(→ intrinsics), 라우터 `CAR_TYPE_MAX=22` 하드코딩(→ catalog.carCount 동적),
    구 0.03px 검증 수치(→ 0.0098px 재검증).
  - **이번에 `docs/scene-control-api.md` 에 반영한 정정**(전부 현행 코드 실물로 확인): ① `/api/simulator/*`
    프록시 범위를 씬 부분집합으로 한정(stream/control/devices 등은 calory 자체 기능) ② FakeSceneClient
    동일 표면의 예외 — `projectPoints` 실 sim 전용, Fake 폴백 시 `/api/simulator/project` 501 ③ 씬 소스 =
    `devices[]` mode:"sim" 기기의 host+scenePort(구 `config.simulator` 는 deprecated 폴백) — 다이어그램·ini
    주석 2곳 ④ ScenePort 점유 시 바인드 실패 = `/scene/*` 전체 무응답(UE 로그만, 재시도 없음) ⑤ slots
    `type` 무접미 BP 변형은 `"slot"` 이 아니라 `"C"`(스트립 순서상 `_C` 잔존 — `"slot"` 폴백은 접두 정확일치
    비-BP 클래스만) ⑥ cameras `hucomsPort`/`mjpegPort` 는 채널 부재 시(bServeHucoms=false) 액터 설정값
    폴백(0 가능 — 조인 전 확인) ⑦ `plate.city` 는 저장·에코 전용, **렌더 미반영**(액터에는 prefix+kor+number 만
    전달) — 본문+캐논 표 ⑧ PATCH 갱신 가능 필드는 carType·color·plate·slotId 뿐, `transform` 은 조용히 무시
    ⑨ 슬롯 미배치 차량의 응답 `slotId` 는 `null`(요청 분리 표기 `""` 와 비대칭) ⑩ 라우터 경유 스폰은
    carType·color 필수(400) — sim 직결(기본값 0 채움)과의 계약 차이 명기.
  - `memo.md` 「동작 규칙」의 setcenter "줌 인식 LINEAR" 서술을 tan+구면(v0.1.4+)으로 정정하고 화각표
    단일 출처(`ZoomHfovTable` → API 노출, JS 는 폴백)를 반영했다.
  - **잔여 관찰(문서 아님, 코드 — 필요 시 별건)**: POST `force:true` 스폰은 기존 점유 차량 액터를 파괴하지
    않고 점유 매핑만 새 차량으로 교체한다 — 옛 차량이 같은 슬롯에 겹친 채 잔존하고, 이후 옛 차량을 DELETE
    하면 슬롯 점유 해제가 새 차량 점유까지 지울 수 있다(감사 반박 과정에서 코드로 확인). → **같은 날 후속
    수정(아래).**

- 2026-07-23 (BEVHeight 확장): **플러그인 v0.1.8 — 응용 SW팀 파인튜닝 요구서 대응(카메라 스포너·class·가시성 GT·핀홀·연속 PTZ).**
  - **배경**: 응용 SW팀 요구서(`_localfiles/sim_camera_request.md`) — 실장비 cam-real-002(16m/23°/36-49m)와
    현 시뮬 카메라(5.75m/57°/5-30m)의 거리 분포가 안 겹쳐 BEVHeight 파인튜닝이 실장비로 전이 안 됨. 1순위 3건
    + 인터페이스 버그를 v0.1.8 로 구현. 2순위(환경 랜덤화·씬 확장)는 레벨/에셋 트랙이라 이번 범위 제외(이교수님 결정).
  - **① config 카메라 스포너**(`HucomsServerSubsystem`): `UPROPERTY(config) TArray<FPTZCameraSpawnSpec>
    SpawnCameras` + `DefaultGame.ini +SpawnCameras=(...)`. BuildChannels 최상단에서 GetAllActorsOfClass 전에
    스폰(자동스폰 선례와 동일 경로 → 같은 패스에서 채널·포트). **포트 인덱스 버그 예방 수정**: AutoIndex 를
    자동포트 카메라에서만 증가시켜, 명시 포트 스폰 카메라가 섞여도 레벨 폴 카메라의 8081/8082 가 안 밀리게 함.
  - **② 차종 class 라벨**: `/scene/catalog.cars[].class`(car/truck/van, 이름 휴리스틱 — 봉고·탑차·포터=truck,
    스타렉스·카니발=van). **23종에 버스 없음**. UE `CarAssetToClass` = JS `carAssetToClass` 동일 소스.
  - **③ 가시성/가림 GT**: `GET /scene/cars?visibility=<cameraId|hucomsPort>` → 각 차량 `visibleRatio`(0..1) +
    `visibilityCamera` 에코. 광학중심→차량 AABB 표본점 15개 `ECC_Visibility` 라인트레이스(자기 무시). 파라미터
    없으면 기존 응답(하위호환), 없는 카메라 404. Fake 는 항상 1(가림 개념 없음).
  - **④ 연속 PTZ 미러**(404 버그 수정): `pt_control.cgi?action=setptmove`, `zf_control.cgi?action=setzfmove`
    velocity 제어(방향+속도, stop/goptzfpos 로 정지). 채널 PanVel/TiltVel/ZoomVel, Tick 적분, 한계 자동정지,
    bFixed 무시. 벤더 스펙 §8.2/8.3 준수. baro_calory 는 이 CGI 를 안 쓰지만(전부 goptz) 팀 워커가 직접 호출 —
    fake-camera-client 에도 미러(부호 회귀 방지 테스트).
  - **⑤ 핀홀 명시 필드**: `/scene/cameras[].{projection:"pinhole", distortion:null, rollDeg:0}` — 소비자가
    `focal_px = 0.5*W/tan(hfov/2)` 로 계산(소실점 fit 불필요).
  - **검증**: 에디터 증분 빌드 성공 → standalone `-game -RenderOffscreen` 라이브. 카메라 **6대**(기존 2 + 신규 4),
    포트 8081~8086/8193~8196 리슨(폴 카메라 8081/8082 유지), 높이 790/1190/1590/1990cm, 연속 PTZ pan/zoom 증감·
    stop·goto 취소, 가시성 GT 0..1, class 라벨, `/scene/project` 신규 카메라 지원, pluginVersion 0.1.8 확인.
    **신규 4대 렌더 육안 확인**(스냅샷: 주차행·차량·라인 선명). baro_calory 207/207 통과.
  - **⚠ 미해결(이교수님 판단)**: LV_Park_sim_01 은 **좁은 폐쇄형 주차장(~21×27m)** 이라 실장비 16m/20°/44m 를
    **클리어 사이트라인으로는 재현 불가** — 남쪽 44m 배치는 단지 정원(잔디·담장)·아파트동 뒤라 주차장이 통째
    가려짐(첫 배치 실측: visibleRatio 0, 렌더가 잔디/건물). **해법으로 남쪽 22m 에 높이만 8/12/16/20m 쌓고
    각자 주차중심 조준**(PitchDeg 개별 -20/-28.6/-36.1/-42.3)해 4대 전부 클리어뷰 확보(슬롯 슬랜트 12~42m,
    실장비 36~49m 와 far row 에서 겹침). 실장비 원거리 저각이 필수면 더 개방된 레벨 필요 — **ini 만 고치면 되고
    리빌드 불요**(config). 커밋·push·배포 미수행(soak 진행 중).
  - **버전**: `.uplugin` 0.1.7→0.1.8. baro_calory: root/backend-core 0.5.7, cctv-client 0.1.9, app-versions.simulator 0.4.6.

- 2026-07-23 (후속): **force 스폰 슬롯 겹침 버그 수정 — 슬롯당 항상 1대 보장(이교수님 지시 "파괴하거나 못하게").**
  - **버그**: 점유 슬롯에 `force:true` 스폰(또는 PATCH 이동) 시 `SpawnCarActor`(`AlwaysSpawn`)가 옛 차 위에
    새 차를 **물리적으로 겹쳐** 스폰하고 `SlotOccupancy` 값만 새 carId 로 덮어썼다. 결과 ① 두 액터가 같은
    슬롯에 중첩(위험) ② 옛 차량이 `Cars` 에 유령으로 잔존 ③ 옛 차량 DELETE 시 `SlotOccupancy.Remove(slot)`
    가 **새 차량 점유까지** 해제하는 교차 오염.
  - **수정 = `EvictSlotOccupant(SlotId)` 신설**(`SceneControlSubsystem.{h,cpp}`): force 덮어쓰기 직전 대상
    슬롯의 기존 점유 차량을 **액터 파괴 + `Cars`/`SlotOccupancy` 정리**로 축출한다. 스폰 경로(`HandleCars`)와
    PATCH 슬롯 이동(`HandleCarById`) 두 곳에 적용. force 없이는 종전대로 `409`. 이로써 **슬롯당 항상 1대**가
    되어 겹침·유령·교차오염이 모두 사라진다. (TMap 은 다른 키 Remove 로 타 원소 포인터가 무효화되지 않아
    PATCH 의 `S` 포인터 안전 — 코드 주석에 근거 기록.)
  - **계약 정합**: baro_calory `FakeSceneClient.#occupy` 도 동일 버그(옛 차 `this.cars` 잔존)라 **함께 수정**
    (force 시 `slot.carId` 의 옛 차를 `this.cars.delete`). 한쪽만 고치면 문서가 주장하는 "동일 표면"이 깨진다.
    `docs/scene-control-api.md` 의 force 서술을 "덮어쓰기 허용"→"기존 차량 파괴 후 교체(슬롯당 1대)"로 정정.
  - **검증**: baro_calory 전체 테스트 **203/203 통과**(신규 2건 — force 스폰 축출, PATCH force 이동 축출 +
    교차오염 회귀 가드). **UE 에디터 빌드 성공**(증분 8.2초, `UnrealEditor-baroCCTVSimulator.dll` 재링크) 후
    standalone `-game -RenderOffscreen` 로 띄워 **라이브 /scene API 12/12 통과**: force 스폰이 옛 차를
    축출(총 1대·id=새 차)·슬롯 점유 승계·옛 차 DELETE 404·교차오염 없음, PATCH force 이동도 대상 슬롯
    점유 차량 축출·원 슬롯 해제, force 없는 정상 스폰 회귀 없음. 이 수정은 **0.1.7 에 폴드**(버전 미범프)
    해 서브모듈 `13ede2f` 로 커밋됨(부모 `4f370a9` 포인터 갱신, baro_calory `a18afb2`). push 는 미수행.
    0.1.8 로 분리 원하면 재스탬프.
  - **여기까지가 확정된 상태다.** 배포 산출물 `Packaged/baro_unreal_sim_v0.2.0_20260722.zip`(2.78GB,
    sha256 `68674c4b…f618f9`, 앱 v0.2.0 / 플러그인 v0.1.6 / Development / `LV_Park_sim_01`).
    이교수님이 현장 기기에 배포해 **48시간 연속 가동**한 뒤(예상 종료 **2026-07-24(금) 14:26 KST**)
    로그 분석을 맡기신다. 아래는 다음 세션이 그대로 이어받기 위한 인계 메모다.
  - **이번 릴리스가 고친 것(48h 로 검증하려는 것)**: ① SW Lumen + persistent ViewState 캡처의 CPU 누수
    (구 v0.1.4 실측 **+1.86~2.07MB/s ≈ 6.7GB/h** → 35시간 가동 후 OOM). ② 부팅마다 터지던 GameFeatureData
    Ensure(2026-07-17 에 오류보고 처리 중 전 스레드 정지 → **15시간 먹통**). ③ CrashReportClient 미스테이징.
  - **분석에 필요한 파일**(기기에서 회수):
    `baro_unreal/Saved/Logs/BaroHealth-*.csv`(1초 샘플·30초 기록, **주 판정 자료**),
    `baro_unreal/Saved/Logs/baro_unreal.log`, `baro_unreal/Saved/Crashes/`(비어 있어야 정상),
    가능하면 작업관리자 기준 Private Bytes 도 한 번.
  - **판정 기준(합격/불합격을 미리 못박아 둔다 — 사후에 기준을 만들지 않기 위해)**:
    - **합격**: CSV `state` 가 `LEAK_SUSPECTED` 0건. `physical_mb` 가 워밍업(부팅 후 ~100초) 이후
      **단조 증가하지 않고 진동**하며, 48시간 총 증가가 수백 MB 수준. `Saved/Crashes` 비어 있음.
      로그에 `Failed to load class /Script/GameFeatures.GameFeatureData` 0건.
    - **불합격**: `memory_slope_mb_min` 이 지속적으로 양수(특히 20MB/min 이상 = `LEAK_SUSPECTED`).
      참고로 구 결함은 **111MB/min** 수준이라 재발하면 즉시 눈에 띈다. 48시간이면 320GB 규모라
      그전에 OOM 이 먼저 난다.
    - **주의(반복 금지)**: 부팅 직후 구간은 텍스처 스트리머 워밍업으로 **+40MB/s 까지 치솟는다**.
      이건 누수가 아니다(2026-07-20 오판 사례 — memo 「메모리 누수 원인과 대응」). **첫 100초는 버리고**
      60초 버킷 여러 개로 단조성을 볼 것. 임계값 조정이 필요하면 `baro.Health.*` cvar
      (`WatchMBPerMin` 5 / `LeakMBPerMin` 20 / `TrendWindow` 120 / `ResetJumpMB` 256).
  - **주의: MJPEG 스트림이 붙어 있어야 의미 있는 시험이다.** 시뮬은 클라이언트가 0 이면 캡처를 아예 안 돌려
    누수 경로가 실행되지 않는다(`HasClients()` 게이트). 48시간 동안 baro_calory 프리뷰를 열어 두거나
    소비자를 하나 붙여 둘 것. 단, baro_calory 는 같은 날 워치독이 들어가 **탭이 숨겨진 채 60초가 지나면
    스스로 끊는다** — 브라우저 탭을 방치하는 방식은 시험이 중간에 죽는다. 별도 소비자를 권장한다.
  - **불합격 시 다음 수**: `-LLM -LLMCSV` 로 기동해 LLM 태그별 성장분을 본다(구 진단에서 `DistanceFields`
    태그로 SW Lumen 을 특정했다). HWRT 가드가 실제로 걸렸는지는 `baro.Capture.PersistRenderingState`
    와 `r.Lumen.HardwareRayTracing` 값을 런타임에서 확인.

- 2026-07-22: **배포 zip 이름을 앱 버전 기준으로 통일 + 앱 v0.2.0 (코드 변경 없음, 패키징 규약 릴리스).**
  - **문제(이교수님 지적)**: 기존 배포본 `baro_unreal_sim_v0.1.4_20260714.zip` 의 `0.1.4` 는 **플러그인** 버전이었고
    그때 앱 `ProjectVersion` 은 `0.1.0` 이었다. 이름만으로 무엇의 버전인지 알 수 없어, 배포본을 대표하는
    버전이 무엇인지 매번 되짚어야 했다. zip 이 **수동 생성**이었던 것이 근본 원인이다(`package.ps1` 은
    `Packaged/Win64` 까지만 만들었다).
  - **수정 = `package.ps1 -Zip` / `-Force` 신설**: 패키징 후 `Packaged\baro_unreal_sim_v<앱버전>_<yyyyMMdd>.zip`
    + `.sha256`(`<hash> *<name>`, 기존 사이드카와 동일 형식) + `.info.txt`(앱/플러그인 버전·Config·Map·양쪽 커밋 SHA,
    워킹트리가 더러우면 `-dirty`)를 만든다. 이름은 `Config/DefaultGame.ini` 의 `ProjectVersion` 을 파싱해
    **자동 생성** — 손으로 짓는 경로를 없앤 것이 이 변경의 핵심이다. 압축 직전 아카이브의 `baro_unreal\Saved`
    를 제거하고(실행 흔적 배제 규약 자동화), 동명 파일이 있으면 실패한다(`-Force` 로만 덮어쓰기).
    구조는 `includeBaseDirectory=false` 로 Win64 폴더의 **내용**이 zip 루트에 온다(기존 zip 과 동일).
  - **왜 0.1.2 가 아니라 0.2.0 인가**: 앱 `0.1.1` 로 이름을 붙이면 옛 zip `v0.1.4`(플러그인 이름)보다 숫자가
    작아 **다운그레이드로 보인다**. 숫자 역전을 없애려고 minor 를 올렸다. **코드는 0.1.1 과 동일하고
    변경은 `Scripts/package.ps1` 과 문서뿐**이다. 이에 맞춰 범프 규칙에 "배포 규약이 바뀌는 릴리스는 minor +1"
    을 추가했다(memo 「기본 설정값」).
  - **산출물 검증**: `baro_unreal_sim_v0.2.0_20260722.zip` 2.78GB — pak 안 `ProjectVersion=0.2.0`·플러그인
    `0.1.6`, 런타임 로그 `Set ProjectVersion to 0.2.0`, `/scene/catalog.pluginVersion=0.1.6`,
    zip 파일 54/54 일치·누락 0, `baro_unreal/Saved` 0건, sha256 사이드카 실제 해시와 일치,
    부팅 Ensure 0(`Saved/Crashes` 비어 있음). Config 는 **Development**(누수 감시 중이라 로그·`BaroHealth-*.csv`
    가 필요하고 Shipping 은 `ensure` 가 컴파일 아웃돼 이상을 숨긴다 — 감시 종료 후 Shipping 전환).
  - **문서 정합(적대적 감사 반영)**: `readme.md` 「배포본(zip) 만들기」 신설(Development/Shipping 선택 명시),
    `docs/windows_build_run.md` 「배포본(zip) 만들기」 신설(팀원이 폴더를 손으로 압축하지 않도록),
    `_forAI` README/inventory/memo 의 앱 버전 스냅샷 `0.2.0` + zip 규약, `docs/scene-control-api.md` 기준
    플러그인 버전 `0.1.1→0.1.6` 및 **`cars[]` 필드 누락 보강**(v0.1.2부터 `BP_Car.Mesh_List` 리플렉션으로
    동적 — 실측 캡처로 확인, `carType` 범위를 `0..carCount-1` 로 정정).
  - **적대적 감사가 잡은 스크립트 결함 3건(전부 수정·재검증)**:
    ① **`.sha256` 이 CRLF 로 기록돼 리눅스 검증이 깨졌다(high, 실물 재현).** `Set-Content -Encoding ascii` 가
    후행 개행을 CRLF 로 쓰는데 GNU `sha256sum -c` 는 CR 을 파일명의 일부로 읽는다 →
    `'...zip'$'\r': No such file or directory / FAILED open or read`. 구 v0.1.4 사이드카는 102B(LF), 내가 만든
    v0.2.0 은 103B(CRLF)였다. **배포본을 리눅스/DGX 에서 검증하므로 무결성 확인이 통째로 무력화된다.**
    `[IO.File]::WriteAllText(..., "$hash *$zipName\n", UTF8(no BOM))` 로 교체하고 이미 만든 사이드카도 LF 로
    재작성 — 두 사이드카 모두 `sha256sum -c` **OK**, 102B 로 규격 일치 확인.
    ② **압축 중 예외 시 반쪽 zip 이 최종 이름으로 남는다(medium).** .NET 은 만들다 만 zip 을 지우지 않아,
    다음 실행의 중복 가드가 그 시체를 "이미 배포한 산출물"로 오인해 막는다(사이드카는 압축 뒤에 생기므로
    무효 표시도 없다). `.partial` 로 만든 뒤 성공 시에만 rename → "최종 이름 = 완성본" 불변식.
    ③ **중복 가드가 20분짜리 쿡 뒤에 실행됐다(medium).** 그 전에 `Packaged/Win64` 정리 단계가 직전 정상
    아카이브를 이미 파괴하므로, 실패가 확정된 실행이 빌드 시간과 아카이브를 둘 다 날렸다. 이름 확정·중복
    검사를 **쿡 전으로** 이동하고 시작 배너에 `Zip : <이름> (앱 v.. / 플러그인 v..)` 을 찍게 했다.
  - **서브모듈 문서 정정(이교수님 승인, 별건 처리 완료)**: `Plugins/baroCCTVSimulator/_forAI` 의
    memo/inventory 가 "현재 버전 **0.1.1**" 이라고 단언하고 있었다(실제 `0.1.6`). 0.1.2~0.1.5 동안 방치돼
    같은 저장소의 dev_log(0.1.2~0.1.6 전부 기록)와 내부 모순이었고 `/scene/catalog.pluginVersion` 실제값과도
    어긋났다. → **docs-only 커밋 `261edfa`**(VersionName 범프 없음 — 코드 무변경)로 정정하고 `.uplugin` 이
    단일 출처임을 각 줄에 명시해 재발을 막았다. 부모 핀 `00875b8 → 261edfa`.
  - **서브모듈 URL 을 정식 주소로 교체(이교수님 승인, 위험요소 제거)**: `baro_unreal`·`baroQuantum` 두
    소비 프로젝트가 구 `gbox3d/baroCCTVSimulator.git` 를 **GitHub 이관 리다이렉트에 의존**해 쓰고 있었다.
    리다이렉트는 만료가 없지만, GitHub 사양상 **옛 위치에 새 저장소나 fork 가 생기면 영구 삭제**된다
    ("If you create a new repository or fork at the previous repository location, the redirects … will be
    permanently deleted" — 공식 문서 확인). 개인 계정으로 fork 만 떠도 같은 이름에 안착해 끊기고, 그 뒤
    새 클론의 `submodule update` 는 **에러 없이 조용히 fork 를 추적**한다(실패보다 나쁜 무증상 분기).
    → 두 프로젝트 모두 `goback-technology/baroCCTVSimulator.git` 로 교체. `.gitmodules`·`.git/config`·
    서브모듈 `origin` **3계층 전부** 확인했고(`git submodule sync --recursive`), 새 URL 로 `ls-remote` 가
    같은 커밋(`261edfa`)을 반환하는 것까지 검증했다. 참고: 교체 전 API 조회로 `gbox3d/...` 의 정식 이름이
    `goback-technology/...` 임을 확인 — 리다이렉트는 그때까지 정상 동작 중이었다(선제 정리).
    **전 저장소 원격 전수 점검(이교수님 "위험요소는 모두")**: 같은 종류가 하나 더 있었다 —
    `baroQuantum` **자신의 origin** 도 `gbox3d/baroQuantum`(→ `goback-technology/baroQuantum` 리다이렉트)
    이었다. `git remote set-url` 로 교체(로컬 `.git/config` 이므로 커밋 대상 아님 — **새로 클론하는 사람은
    처음부터 정식 주소를 써야 한다**). 비공개 저장소는 익명 API 가 404 라 `gh` 인증 조회로 확인했다
    (`baro_calory`·`myAISkills` 는 처음부터 정식). 최종 상태: **7개 원격 + 2개 `.gitmodules` 전부 정식,
    리다이렉트 의존 0건**. 조직 이관 정리: 업무 저장소는 `goback-technology/*`, 개인 것(`ready_unreal`,
    `myAISkills`)은 `gbox3d/*` 로 정상.
  - **플랫폼: 당분간 Windows 전용 재확인(이교수님)**. 이유는 **Linux 화질 손상** — Vulkan 오프스크린에서
    VT 피드백이 동작하지 않아 주차면 라인 데칼이 렌더되지 않는다(2026-07-10 실측, `plan.md` Structure
    decisions). 서브모듈 `_forAI` 에도 "검증 플랫폼 = Windows/Win64 전용"과 그 이유를 명시했다 —
    3프로젝트 공유 플러그인이라 다른 소비 프로젝트가 Linux 를 시도할 때 같은 함정을 다시 밟지 않도록.
    재개 조건은 라인 데칼의 비-VT 머티리얼 교체가 선행되는 것.

- 2026-07-20 (저녁): **누수 근본 대응(플러그인 v0.1.6 HWRT 가드) + 07-17 먹통 원인 수정·재현 + 패키징 강화. 앱 v0.1.1.**
  - **아래 (주간) 엔트리 정정 2건**: ① `bAlwaysPersistRenderingState`는 트리거일 뿐 결함 주체가 아니다 — 진범은 **UE 5.8 소프트웨어 Lumen(SDF)의 캡처 프레임당 CPU 미회수**(LLM 태그 `DistanceFields` +278MB/2.5분 실측, 라디언스캐시·템포럴·피드백·GDF·아틀라스 cvar 전부 무효 A/B 10회). ② persist off(v0.1.5)는 **화질 회귀가 실재**한다 — ViewState 부재 = 캡처 Lumen 비활성 → 암부 뭉개짐(clipLo 15.2%→18.4%, 부스 내부 디테일 소실, 스냅샷 A/B 실측). "회귀가 되면 안 된다"(이교수님) → 가드 방식으로 대체.
  - **근본 수정 = 플러그인 v0.1.6(`00875b8`)**: `baro.Capture.PersistRenderingState` cvar(기본 1) + **HWRT 가용 시에만 persist 허용** 가드 + `bUseRayTracingIfEnabled=true` + Build.cs RHI. 앱 쪽 `DefaultEngine.ini r.Lumen.HardwareRayTracing=True` 신설. **실측: HWRT+persist +0.053MB/s(구 1.86~2.07 → 소멸) + 암부 clipLo 14.8%(구 SW persist 15.2%보다 개선, 선명도 동등)**. SW 폴백(가드 발동) -0.343MB/s. 상세는 memo 「메모리 누수 원인과 대응」.
  - **07-17 "즉시 먹통" 수정 + 로컬 2회 재현**: 원인 = `DefaultGame.ini` AssetManager 의 GameFeatureData 스캔 항목(bIsEditorOnly=False) — 쿡 게임엔 GameFeatures 모듈이 없어 부팅마다 Ensure, 그 오류 보고 중 전 스레드 정지(로그가 콜스택 직후 끊기고 포트 안 열림 — 현장과 동일 시그니처, 구 v0.1.4 패키지로 재현). **삭제는 오답**(에디터/쿡의 GameFeatures 가 규칙 존재를 요구해 쿡 실패) → **bIsEditorOnly=True 로 정정**. 신규 패키지 부팅 3/3 Ensure 없음.
  - **CrashReportClient 스테이징**: `package.ps1` UAT 인자에 `-CrashReporter` 추가(BuildCookRun 은 ini 의 IncludeCrashReporter 를 읽지 않음 — 멀티에이전트 감사 확인). 신규 패키지에 `Engine/Binaries/Win64/CrashReportClient.exe`(24.6MB) 스테이징 + "Could not start crash report client" 소멸 확인.
  - **"HTTP 요청당 누수" 관측은 같은 날 밤 반증됨** — 아래 (밤) 엔트리 참조. 첫 관측(요청당 ~16KB / QHD ~1.3MB)은 부팅 워밍업 오염이었다.
  - **중계(baro_calory) 후속 제안(미적용)**: OOM 당시 8시간 15분 잔존 MJPEG 연결의 보유자는 backend `proxyTcpMjpeg` — 스트림 개시 후 `sock.setTimeout(0)`+무기한 drain 대기+keepalive 부재로 스톨/방치 소비자를 못 끊는다(`server.mjs:715-766`). 워치독 3종(drain 데드라인·양측 keepalive·유휴 타임아웃)+탭 가시성 정책 제안(멀티에이전트 감사, 별도 작업으로).
  - **함정 기록**: 쿡 커맨드릿이 MCP 자동시작(:8000)을 시도 — VS Code 가 8000 점유 중이면 그 Error 하나로 쿡 실패. 임시로 `bAutoStartServer=False` 후 패키징, 복원함(memo 「반복 금지」).
  - **버전**: 앱 `ProjectVersion` 0.1.0→**0.1.1**, 플러그인 0.1.5→**0.1.6**(서브모듈 push 완료 — 원격이 `goback-technology/baroCCTVSimulator` 로 이관됨 안내, 구 URL 리다이렉트 동작). 진단 번들은 `_localfiles/system_info.zip`(git 제외).
- 2026-07-20: **메모리 누수 원인 확정과 모니터링/UI 구현.**
  - **이교수님 보고 요약**: Lumen + `SceneCaptureComponent2D`의 persistent rendering state가 process RAM을 계속 증가시키는 원인이었다. `bAlwaysPersistRenderingState=false`로 수정했고 A/B 및 패키지 실행 검증을 통과했다.
  - **검증 수치**: 원래 경로 `+1.2~1.9 MB/s`, SceneCapture-only `+1.835 MB/s`; NoPersist는 warm-up 후 `-0.253 MB/s`, NoLumen은 `+0.086 MB/s`. JPEG/Readback/socket은 주원인이 아니었다.
  - **재발 방지**: `BaroSystemMonitorSubsystem`이 1초 샘플링, 30초 UE log/CSV, 최근 120초 RAM slope 판정(`20 MB/min` 이상이면 leak 의심)을 수행한다. 큰 일회성 자원 할당은 `baro.Health.ResetJumpMB=256` 기준 workload transition으로 분리한다.
  - **UI**: `BaroSystemMonitorWidget` native UMG를 `BaroUnrealHUD`에 추가했다. 우상단에 상태/CPU/RAM/GPU frame time/VRAM/FPS를 표시하며 Blueprint/WBP로 확장 가능하다. 현재 MCP callable tool이 노출되지 않아 `.uasset` WBP 대신 native layout을 사용했다.
  - **최종 실행**: `Packaged/Win64/baro_unreal.exe` Development 빌드 166초 실행, `LEAK_SUSPECTED` 없이 `HEALTHY` 유지. CSV는 `Saved/Logs/BaroHealth-20260720-173441.csv`에 남겼다. GPU 사용률은 RHI 미지원으로 `N/A`다.
  - **별도 이슈**: Texture Streaming Pool `+59.859 MiB` 경고는 이번 RAM 누수와 별개다. Smart App Control 차단은 로컬 unsigned plugin DLL에 대한 Windows 보안 정책이며 이교수님이 처리 완료했다.

- 2026-07-10 (저녁): **앱 전용 HUD(앱 버전 + 외부 접속 주소) · 패키징 Zen 경쟁조건 규명/가드 · 브랜치 재편.**
  - **플러그인 무수정 원칙 확립(이교수님 지시).** `baroCCTVSimulator`는 "최소한의 카메라"다. 앱 고유 표시는
    호스트 게임 모듈에서 해결한다. 3프로젝트 공용 서브모듈이라 한 줄만 고쳐도 `.uplugin` 범프 → 풀 리빌드 →
    원격 push → 두 소비 프로젝트 포인터 갱신 사슬이 터진다. (전역 메모리 `plugin-is-minimal-camera`.)
  - **구현(플러그인 0줄 수정)**: 신규 `Source/baro_unreal/BaroUnrealHUD.{h,cpp}`(AHUD 직접 상속 — 플러그인
    `ABaroSimHUD::FpsEma`가 private이라 상속 이득 없음) + `BaroUnrealGameMode.{h,cpp}`(`ABaroSimGameMode` 상속,
    생성자에서 `HUDClass`만 교체) + `DefaultEngine.ini GlobalDefaultGameMode=/Script/baro_unreal.BaroUnrealGameMode`.
    상속 경로가 열려 있는 근거: 플러그인 클래스 전부 `BAROCCTVSIMULATOR_API` export, `DrawHUD()` virtual,
    생성자 public, `UHucomsServerSubsystem::{GetChannelCount,BaseHttpPort,BaseMjpegPort,GetChannelStatusLines}`와
    `USceneControlSubsystem::ScenePort` 공개.
    **함정**: 플러그인 `Build.cs`의 `Sockets`/`Networking`/`Projects`는 **Private 의존이라 전이되지 않는다** →
    게임 `Build.cs`에 `baroCCTVSimulator`(Public) + `Sockets`·`Projects`(Private)를 직접 추가해야 링크된다.
  - **앱 버전 신설**: `Config/DefaultGame.ini [/Script/EngineSettings.GeneralProjectSettings] ProjectVersion=0.1.0`.
    이전엔 키 자체가 없어 UE 기본값 `1.0.0.0`이었고, HUD 제목줄은 **플러그인 버전**을 앱 버전인 양 보여주고 있었다.
    이제 제목줄 `baro_unreal v0.1.0`, 아래 작은 줄 `plugin baroCCTVSimulator v0.1.3`으로 분리 표기.
  - **서빙 주소 표시**: `ISocketSubsystem::GetLocalAdapterAddresses()`로 IPv4 열거. **`127.` 뿐 아니라 `169.254.`
    (APIPA 링크로컬)도 반드시 거른다** — 이 개발 PC 실측상 169.254가 4개(끊긴 Wi-Fi·블루투스 PAN·가상 스위치)이고
    실제 LAN은 이더넷 `192.168.0.211` 하나뿐이다. `GetLocalHostAddr()`가 링크로컬을 돌려주는 경우가 있어,
    필터를 통과할 때만 맨 앞으로 올린다. 바인딩은 원래부터 전 인터페이스였다(HTTP `BindAddress=any`,
    MJPEG `FIPv4Address::Any`) — **주소를 몰라서 못 붙었던 것이지 안 열려 있던 게 아니다.**
  - ⚠️ **포트를 기본값에서 옮기면 HUD가 거짓말을 한다.** `[HTTPServer.Listeners]`는 8081~8084·8095만
    `BindAddress=any`로 지정한다. `BaseHttpPort`를 8181로 옮겨 검증했더니 8181/8182/8195는 `127.0.0.1`에만
    바인딩됐다(MJPEG 8191/8192는 코드가 Any라 `0.0.0.0`). HUD는 여전히 LAN URL을 찍으므로, 포트를 바꿀 땐
    `ListenerOverrides` 항목도 함께 추가할 것.
  - **검증**: Editor/Development 빌드 → 대체 포트(8181/8191/8195) standalone `-game`으로 **기존 인스턴스를 죽이지 않고**
    창 캡처해 HUD 확인 → Development 패키징 후 `Packaged/Win64` 실행본에서도 8081/8091/8095 · `192.168.0.211` 확인.
    (Development 빌드에서만 뜨는 엔진 온스크린 경고 `TEXTURE STREAMING POOL OVER`가 HUD 줄과 Y가 겹친다. Shipping엔 없음.)
  - **패키징 Zen 경쟁 조건 규명.** `./Scripts/package.ps1`이
    `Failed reading oplog from Zen ... Error while copying content to a stream`(UAT exit 1)으로 실패.
    **쿡은 성공했다** — `ZenLocalGetHitPct=1.0`, oplog 1716 엔트리 스냅샷 기록 완료. 메모리(여유 40.5GB)·디스크(370GB)도 무죄.
    진범은 zenserver의 수명 모델이다. zenserver는 상주 데몬이 아니라 **sponsor 프로세스가 전부 사라지면 자결**한다.
    `zenserver.1.log` 실측:
    ```
    17:33:46  added process with pid 38448 as a sponsor process   ← sponsor = 쿡 프로세스 하나뿐
    17:34:20  GC stale target process pid 38448 (exit code: 0)    ← 쿡 정상 종료
    17:34:21  exiting since sponsor processes are all gone
    ```
    그 순간 UAT는 스테이징을 위해 oplog를 HTTP로 되읽는 중이었다. **UAT는 sponsor가 아니다** — sponsor 슬롯은
    UE 프로세스만 쓰는 공유메모리 8칸(`ZenServerState.cpp` `SponsorPids[8]`)이고, `--owner-pid`는 종료 신호용일 뿐
    sponsor가 아니다(`ZenServerInterface.cpp:2223`). 즉 **외부에서 sponsor를 심는 지원 경로가 없다.**
  - **해법 = 재시도.** 쿡 산출물이 Zen에 온전하므로 재실행은 캐시 히트로 통과한다(실측 **51초, ExitCode=0**).
    `package.ps1`에 UAT 출력을 `Saved/Logs/package-uat.log`로 티잉하고, **Zen oplog 오류일 때만 1회 자동 재시도**하는
    가드를 넣었다(컴파일·쿡 에러는 즉시 실패 — 헛된 재시도 방지). `Tee-Object` 파이프라인 뒤에도 `$LASTEXITCODE`가
    보존됨을 별도 실측으로 확인.
  - **브랜치 재편**: `feat/windows-only-deploy`(aa83733)를 `main`으로 **fast-forward**(main에만 있던 커밋 0개 →
    force push 불필요). 직전 `main`(c03c6b9 = Linux/Vulkan 코드가 살아 있는 마지막 지점)을 **`dev/vulkan-port`**로
    보존해 push. `feat/windows-only-deploy`는 로컬·원격 삭제. **main = Windows 전용 기반, Linux/Mac은 실험 브랜치.**
  - **플러그인 v0.1.3 원격 반영**: `baroCCTVSimulator` origin/main `55bb988 → ea38976` push. 그전까지 v0.1.1~0.1.3이
    baro_unreal 로컬에만 있어 baroQuantum이 받을 수 없는 상태였다. baroQuantum 서브모듈도 `ea38976`으로 갱신(커밋 `4592678`).
- 2026-07-10 (문서 정리): **`_forAI` 문서 세트 감사·정리 + RYU 플러그인 활성 상태 사실 정정.**
  - 5개 문서의 사실 주장을 저장소 실물과 대조(적대적 검증 포함). 확정 19건 반영, 기각 12건 폐기.
  - **정정 1 — RYU 플러그인은 비활성이 아니다.** 2026-07-06 (저녁) 엔트리와 구 `inventory.md`는
    "RYU는 이미 uproject 비활성"이라 적었으나 사실이 아니다. `baro_unreal.uproject`의 Plugins 배열에서
    항목이 빠졌을 뿐, UE는 프로젝트 로컬 플러그인(`<Project>/Plugins/`)의 `EnabledByDefault` 미지정 시
    **기본 활성**으로 취급한다(`FPlugin::IsEnabledByDefault` — Unspecified → `LoadedFrom == Project`).
    근거: `Plugins/RYUKoreaBuilidngCreator/Binaries/Win64/UnrealEditor-RYUKoreaBuilidngCreator.dll` 실재.
    진짜로 끄려면 `{"Name":"RYUKoreaBuilidngCreator","Enabled":false}` 명시가 필요하다.
    (다행히 지금 끄면 안 되는 상태 — 미베이크 레벨이 아직 쓴다. `plan.md` 참조.)
  - **정정 2 — 포트 수**: `memo.md`가 "카메라 4대 = 8081~8084"로 고정 서술했으나, 카메라 수는 맵마다 다르다
    (기본 맵 `sim_01`=2대 → 8081·8082). `[HTTPServer.Listeners]`의 8081~8084는 상한 예약일 뿐이다.
  - **누락 보강**: `inventory.md`에 code-only git 전략(`.gitignore`), 서브모듈 운영 절차, `Scripts/`
    래퍼 4종(`build/run/package/common.ps1`), `.env`/`.env.example`, `docs/` 3문서, `Config/` 5파일,
    `.gitattributes` 라인엔딩 정책, 에디터 전용 플러그인이 2개가 아니라 3개(ModelingToolsEditorMode 포함)라는 점을 추가.
    낡은 TODO Notes(Target.cs "확인 필요", Build.cs 의존성 "반영 필요")는 이미 완료돼 삭제.
  - **구조 정리**: `dev_log.md` 엔트리를 최신순으로 통일(상단 3개만 최신순이고 나머지는 오래된순이라 뒤집혀 있었음),
    같은 날짜 구분을 위해 07-10에 (오전)/(오후) 표기 추가. `plan.md`는 규약대로 **앞으로 할 일만** 남기고
    완료 [x] 17항목을 제거(전부 이 dev_log에 보존됨), 열린 7항목을 능동/조건부로 재배열, Linux 보류는
    Structure decisions로 이동. 아래 2026-07-03 엔트리(플러그인 이관)는 이력 공백이라 소급 신설했다.
- 2026-07-10 (오후): **Windows 전용 최고 안정 품질 복원 + Shipping 배포판 일반 창모드 확정.**
  - Linux 버전은 보류. `Config/Linux/LinuxEngine.ini`, Linux/Mac 타깃 RHI, Linux staging 예외, 빌드·패키징의 Linux 선택지를 제거하고 `.uproject`/Target.cs/스크립트를 Windows·Win64 전용으로 제한했다. 프로젝트 아래 Linux 생성 산출물(Binaries/Build/Intermediate/Packaged/Saved 약 9GB)도 정확한 플랫폼 하위 경로만 정리했다.
  - **Windows 품질 수정은 보존**: 플러그인 v0.1.3의 `bOverrideVirtualTextureThrottle=true`(주차선 VT 결정성), DX12/SM6, Lumen GI/Reflection, VSM, RT, QHD 2560×1440 q92, 노출 -0.7/대비 1.2를 유지했다. 배포 품질은 검증된 Epic(3) + `sg.ResolutionQuality=100`; Cinematic(4)은 다중 SceneCapture/8GB VRAM에서 품질 역저하 가능성이 있어 강제하지 않는다.
  - `Config/DefaultGameUserSettings.ini` 신설: `FullscreenMode=2`, 960×540 일반 창, Epic 품질. Alt+Enter/F11 전체화면 전환도 비활성화했다. 메인 창 해상도는 SceneCapture 해상도와 독립이다.
  - `Scripts/package.ps1`는 패키징 전 검증된 `Packaged/Win64`만 비워 구 `Saved/GameUserSettings.ini`, Development EXE, 크래시 로그가 섞이지 않게 한다. Win64 Shipping `-Clean` 빌드·쿡 성공(2분13초, 1,704 packages, 2.96GB); 실행 전 아카이브는 `Saved/` 0, Linux/Vulkan 파일 0.
  - **직접 EXE 검증(명령행 `-windowed` 없음) 2회 통과**: client 960×540, caption+resize frame, non-maximized/non-popup. D3D12 로드, 8081/8082/8091/8092/8095 listen, `/scene/catalog` 200(plugin 0.1.3), JPEG 2560×1440(주차선·톤·선명도 육안 정상). 과거 로컬 `FullscreenMode=1` 파일은 `%LOCALAPPDATA%/.../GameUserSettings.ini.pre-windowed-20260710-112103.bak`으로 보존했다.
- 2026-07-10 (오전): **주차면 라인 데칼 미렌더 사건 해결(Windows 완치) + 시연용 Shipping 빌드 + Linux(gb_210) 배포. 플러그인 v0.1.3.**
  - **시연 빌드(보존본)**: `Packaged/Win64_Shipping_demo-20260710/baro_unreal.exe` — Shipping, 독립실행, 라인 렌더·차종 카탈로그·5포트 검증 완료. 이후 패키징이 `Packaged/Win64`를 덮어써도 이 폴더는 안전.
  - **진범 = SceneCapture 전용 렌더의 VT(버추얼 텍스처) 페이지 스로틀.** 주차면 라인 데칼(`MI_Decal_Line_Road_White_02`, Megascans `M_MS_Decal_Material_VT` 계열)만 SVT 텍스처 3장을 샘플. VT 페이지는 렌더 픽셀 피드백으로 스트리밍되는데 이 sim은 `bDisableWorldRendering`+캡처 전용이라 피드백이 스로틀에 막힘 → 쿡 빌드에서 부팅 복불복(-RenderOffscreen은 상시)으로 라인만 투명. **수정 = `PTZCaptureComponent` 캡처 컴포넌트에 `bOverrideVirtualTextureThrottle=true`** (플러그인 0.1.2→**0.1.3**). 클린부팅 Dev 4/4 + Shipping 2/2 라인 정상으로 결정성 확인.
  - **판별 결정타 2개**: ① Windows에서 `-dpcvars=r.VirtualTextures=0`으로 켜면 증상 100% 재현(=VT 인과 확정). ② 같은 pak 두 부팅에서 라인 유/무가 갈림(=쿡 아닌 런타임 확정). 수사 중 "7/7 umap 재저장이 고쳤다"는 결론은 **부팅 복권에 속은 오판**이었음(재저장 자체는 무해, 백업 `LV_Park_sim_01.umap.bak-20260709` 잔존 — 확인 후 삭제 가능).
  - **디버깅 함정(중요)**: Shipping 게임 자식 프로세스명은 `baro_unreal-Win64-Shipping.exe` — `Stop-Process -Name baro_unreal`은 런처만 죽여 구 인스턴스가 포트(8081+/8095)를 쥔 채 테스트를 오염시킴(아침 "시뮬 4개" 사건). 부팅 테스트 전 `netstat` 리스너 0 확인 필수.
  - **Linux(gb_210, 192.168.0.210)**: 0.1.3 pak 배포·가동 중(`~/baro_sim`, `nohup ./baro_unreal.sh -RenderOffscreen -log &`, 종료는 `pkill -f '[b]aro_unreal'`). 스폰·PTZ·MJPEG·카탈로그 전부 원격 정상. **단 주차면 라인만 여전히 미표시** — Vulkan 오프스크린에선 VT 피드백 자체가 무동작(VeryVerbose 로그 0줄). bindless 가설(`BaseLinuxEngine.ini`의 VULKAN_SM6 `BindlessConfiguration=All`)은 오버라이드 실험으로 **반증**됨. 쿡타임 VT-off(`r.VirtualTextures=0`) 우회는 VT 샘플러 불일치로 데칼이 흰 판이 되어 기각.
  - **이어서 할 일(플랜 B, ~30분)**: Vulkan에서도 라인을 원하면 라인 데칼을 비-VT로 교체 — 근거: 같은 슬롯 BP의 장애인 아이콘 데칼(`M_장애인`, 부모 `/Game/M_Auto/M_Decal`, 비-VT, MD_DeferredDecal)은 전 플랫폼 정상. 절차: 라인 텍스처 3장(`T_Decal_Line_Road_White_02_{D,DpRA,N}`) VirtualTextureStreaming=False + 비-VT 데칼 머티리얼 신설(D=베이스컬러, DpRA 채널=오패시티) + `MI_Decal_Line_Road_White_02` 부모 교체. Megascans 공용 `_VT` 마스터는 건드리지 말 것(횡단보도 등 공유).
  - **이번에 남긴 설정**: `DefaultGame.ini [Staging] +AllowedDirectories=..._RYU_Portable/.../Upper/Windows`(폴더명 "Windows"가 UAT 제한폴더에 걸려 Linux 스테이징 실패하던 것), `DefaultEngine.ini [HTTPServer.Listeners]` 8081~8084·8095 포트별 `BindAddress=any`(원격 제어용, MCP :8000은 localhost 유지), `Config/Linux/LinuxEngine.ini [ShaderPlatformConfig VULKAN_SM6] BindlessConfiguration=RayTracing`(반증된 실험 부산물이나 무해). ※ 이 중 Linux/Staging 관련 설정은 같은 날 (오후) 엔트리에서 **제거**됐다 — 현재 유효한 것은 `[HTTPServer.Listeners]` 뿐이다.
  - **Linux 빌드 인프라(재현 절차)**: 툴체인 `v26_clang-20.1.8-rockylinux8` 설치됨(`LINUX_MULTIARCH_ROOT` 머신 env, 단 기존 셸엔 미반영이라 `$env:` 수동 지정) + 런처에서 UE5.8 Linux 타깃 컴포넌트 설치됨. `./Scripts/package.ps1 -Platform Linux` → `tar -C Packaged/Linux -cf - . | ssh gb_210 'tar -xf - -C ~/baro_sim'`. Zen 스토어가 `[::1]:8558 연결 거부`로 간헐 실패하면 그냥 재실행. 상세는 전역 메모리 `baro-unreal-packaging-cli`, `ue-scenecapture-streaming-lod`. ※ (오후) 엔트리에서 `-Platform` 파라미터는 제거됐다.
- 2026-07-08: **씬 제어 슬롯 라벨 계약 + 플러그인 v0.1.1 반영.**
  - `baroCCTVSimulator` 플러그인의 `/scene/slots` 응답에 안정 ID(`id=GetName()`)와 에디터 표시명(`label=GetActorLabel()`)을 함께 내려주도록 정리. 웹 UI는 `label || id`를 표시하고 숫자 인식 natural sort로 `BP_ParkingSlot1,2,3,...10` 순서를 유지한다.
  - 프론트가 `BP_ParkingSlot_C_*` 런타임 이름을 가공하거나 하드코딩하지 않도록 계약을 문서화. 관련 API 문서 `docs/scene-control-api.md` 기준 버전을 플러그인 **v0.1.1**로 갱신.
  - 에디터 종료 후 `Build.bat baro_unrealEditor Win64 Development -Project=...\baro_unreal.uproject -WaitMutex -NoHotReload`로 플러그인 포함 빌드 성공 확인.
- 2026-07-06 (저녁): **CLI 패키징(sim_01 단독) + 실행검증 성공 + 우분투 빌드 가능성 판정.** (이교수님 "레벨01만 넣은 상태로 최적 빌드" 요청)
  - **패키징 스크립트 `Scripts/package.ps1`**(RunUAT BuildCookRun 래퍼): 파라미터 `-Platform Win64|Linux`, `-Config Development|Shipping`, `-Map`(기본 `/Game/simulator/LV_Park_sim_01`), `-Clean`. 엔진=`C:\Program Files\Epic Games\UE_5.8`. **실행 전 에디터 닫기 필수**(파일락/DDC).
  - **최적화 2세팅**: ①맵 격리 — `-map=/Game/simulator/LV_Park_sim_01`로 sim_01만 쿡(sim_02/03 제외, 로그 `HasMapsToCook`로 확인). ②플러그인 슬림 — `baro_unreal.uproject`의 MCP 플러그인(`ModelContextProtocol`,`AllToolsets`)에 `"TargetAllowList":["Editor"]` 추가(게임 타깃 제외, 에디터 MCP 원격제어는 유지). RYU는 uproject Plugins 배열에서 제거했고 `Plugins/RYUKoreaBuilidngCreator` 소스는 잔존해 컴파일된다. ※ 당시 이를 "비활성"으로 기록했으나 **오판**이다 — 프로젝트 로컬 플러그인은 기본 활성이라 실제로는 계속 enabled였다(2026-07-10 문서 정리 엔트리에서 정정).
  - **Win64/Development 빌드 성공**: 13분35초, sim_01 단독 1704 패키지, 산출물 **~3.4GB**(`Packaged/Win64/baro_unreal.exe` 런처).
  - **실행검증 통과**: 맵로드 9.2초, Hucoms 채널 **2/2** 기동, 포트 **4/4** 리슨(제어 8081·8082=`127.0.0.1`, MJPEG 8091·8092=`0.0.0.0`), HTTP 404 응답(라우터 정상). ※실 게임/소켓은 **자식 프로세스**(`Binaries/Win64/baro_unreal.exe`)가 소유 — netstat은 **포트 기준**으로 봐야 함(루트 런처 PID엔 소켓 없음). ⚠️ **제어포트가 `127.0.0.1` 바인딩** → baro_calory 원격(다른 호스트) 제어 시 `0.0.0.0` 코드 조정 필요(스트림은 이미 0.0.0.0). ※ 2026-07-09 `[HTTPServer.Listeners]` `BindAddress=any`로 해결됨.
  - **우분투(Linux) 크로스컴파일 — 코드/플러그인은 준비완료, 툴체인만 설치하면 됨**: 소켓 서버가 크로스플랫폼 UE API(`FTcpListener`/`FSocket`/`ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)`), Winsock/WSAStartup/windows.h 직접호출 **無**(`MjpegStreamServer.cpp` 근거). `baroCCTVSimulator.uplugin` 플랫폼 제한 無. **필요 툴체인 = `v26_clang-20.1.8-rockylinux8`**(쿡 로그 PlatformValidate에서 확인, 미설치). 설치 후 `LINUX_MULTIARCH_ROOT` 세팅 → `./Scripts/package.ps1 -Platform Linux`. ⚠️ 실행엔 **Vulkan/GPU 필요**(SceneCapture 캡처 — 헤드리스 CPU 서버 불가, GPU+nvidia 도커 전제). (전역 메모리 `baro-unreal-packaging-cli`.)
- 2026-07-06: **고정 카메라 모드(PTZ 특수케이스) + sim_02/03 CCTV 배치 + sim_03 빌딩 베이크.**
  - **고정형 CCTV = 별도 플러그인 대신 `APTZCamera::bFixedMode` 옵션**(이교수님 방향 승인). 액터층 `Tick()`이 bFixedMode면 모터 보간 스킵(설치자세 고정). 서버층 `HucomsServerSubsystem`: `FHucomsChannel::bFixed`(=`Cam->bFixedMode`, BuildChannels 복사), Tick 슬루 스킵(Cur=Tgt), `ApplyGoPtz`/`ApplySetCenter` early-return(명령 무시), `HandleCapabilityPtz` 고정이면 Pan/Tilt/Zoom Supported=No 광고. **`getptzfpos`는 고정값 그대로 반환**(baro_calory 라운드트립 유지), **스트림/스냅샷은 고정 모드에서도 정상**. 새 UPROPERTY라 Live Coding 불가 → 에디터 닫고 **CLI 풀 리빌드**(성공). (전역 메모리 `barosim-fixed-camera-mode`.)
  - **CCTV 폴 자식 배치(정답 규약, 이교수님 확정)**: sim 레벨 CCTV(`APTZCamera`)는 카메라 폴(`BP_Pole`)의 **자식**으로 배치 — RelativeLocation **z+600**(암 높이), **pitch −20** 하방, yaw는 폴 heading 상속. **폴 개수 = 카메라 개수**(폴 하나에 1대). 이전 세션의 공중 4모서리 배치는 **틀림** → 제거하고 폴 자식으로 교체. sim_01(폴2/카메라2), sim_02(폴2/카메라2), sim_03(폴4/카메라4 `PTZ_Pole_C1~4`, 기존 `BP_Camera` 2개 제거). 전부 저장. (전역 메모리 `barosim-cctv-pole-placement`.)
  - **sim_03 빌딩 베이크(RYU-프리, sim_01 레시피 반복)**: `LV_Park_03`→sim_03, RYU 빌딩 Merge Actors(Method=Merge, **Merge Materials=off**) → `/Game/simulator/SM_sim03_buildings`(409만tris, **144 머티슬롯**, non-Nanite, `/Game/_RYU_Portable` 참조). 병합 결과 피벗=첫 선택액터 위치라 offset 재정렬(P=(5465.47, −1834.99, 0), baked vs orig bounds max축 일치로 검증, 이교수님 "복구됐다" 확인). RYU 참조 0 확정.
  - **sim_03 성능(에디터 느림 진단)**: `Saved/Logs`에서 병목 = 병합메시 내비 콜리전 **327만tris export 경고** + VSM Non-Nanite 마킹 큐 오버플로 + PSO 히치. **`remove_collisions` 성공**(내비 부하 해소 = 체감 개선 핵심). **Nanite는 144섹션 > 한계로 실패**(Merge Materials=off 여파: "Unsupported number of sections: 144") → off 원복. VSM 경고는 잔존(치명 아님, CCTV 좁은 화각엔 영향 적음). 잔여: 필요 시 **Merge Materials=on 재병합**해야 섹션 줄어 Nanite 가능.
- 2026-07-04: **RYU 플러그인-프리 이관 + `LV_Park_sim_01` 빌딩 스태틱 베이크 (이교수님 최우선 목표 "플러그인 없이 이식 가능한 맵" — sim_01 달성).**
  - **성능 선작업(sim_01)**: 이전팀 잔재 삭제(매프레임 풀씬 렌더 `SceneCapture2D_1` + 시네 `BP_Camera`×2). 유리/IES 반투명 Nanite 폴백 9에셋 정리. 무거운 불투명 메시 Nanite 전환(`현대_쏘나타` 121만tris=씬 삼각형 44% + 펜스/가로등/건물 등 19개). **UDS `Disable All Runtime Updating`=true**(정적 씬 전용 프리즈 — 매프레임 시간/구름/스카이라이트 갱신 0). 측정은 standalone -game(에디터 백그라운드 스로틀 회피).
  - **RYU 콘텐츠 이관**: 프로젝트 유일 외부 플러그인=RYUKoreaBuilidngCreator. 7레벨이 참조하는 RYU 자산 **transitive closure 1,096개 → `/Game/_RYU_Portable/`로 폴더단위 이동**(`AssetTools.move`). move는 구경로에 리다이렉터 남기고 참조 자동수정 안 함 → 에디터 우클릭 **"Update Redirector References"**(구 Fix Up Redirectors)로 6레벨+ExternalActors 참조 /Game 재작성. 검증: 7레벨(sim_01/02, LV_Park_01/04/07_A, Unity/LV_Park_01_U/04_U) 직접·transitive RYU참조 **0**.
  - **초기 오판 정정 — RYU는 "콘텐츠 전용"이 아니었다.** `BP_RYUBuilding` 직접 deps엔 /Script/RYU 없지만, **PCG 빌딩 생성기(`BP_GenerateUpperDistributionByGrammar`/`BP_GenerateModuleDistribution` + 구조체 `SymbolMeshVariation` in `BP_RYUUpperHorizontalPreset`)가 `/Script/RYUKoreaBuilidngCreator` C++ 사용**. 빌딩 상세 지오메트리는 PCG가 매 로드 생성하는 **transient** — 플러그인 disable 시 상세 소실(컴포넌트 ~68 → 8 = 기초박스+옥상만). ⇒ 이런 PCG 빌딩은 **베이크가 먼저, 플러그인 제거가 나중**이 올바른 순서(내가 반대로 해서 한번 헛돎, 백업으로 복구).
  - **sim_01 베이크 성공(올바른 순서)**: ①플러그인 재활성화+에디터 재시작 ②빌딩 PCG `generationTrigger`를 GenerateOnLoad로 되돌리고 `save_assets([])`+`load_level` 리로드 → 상세 재생성(8→~68컴포넌트/빌딩) ③6빌딩 전체선택 → **Merge Actors**(Method=Merge, **Merge Materials=off**, Replace=off) → 스태틱메시. **함정: 결과 피벗=첫 선택액터(C_1) 위치**라 배치 후 C_1 world location으로 이동해야 정렬(baked vs orig의 X-max·Z-max 정확일치로 검증, min 2~3m차=원본 스플라인/디버그 컴포넌트 인플레). ④원본 BP 6개 `remove_from_scene` ⑤저장+전이검증 = **RYU콘텐츠 0·/Script/RYU 0 확정**. 결과물 `/Game/simulator/SM_SM_sim01_Buildings`(409만tris, non-Nanite, 129머티슬롯, /Game/_RYU_Portable 참조). **sim_01 = 완전 플러그인-프리(콘텐츠+C++ 둘 다).**
  - **현재 상태 / 이어작업**: 플러그인은 다시 **ENABLED**(나머지 레벨이 아직 씀). 나머지 RYU빌딩 레벨(`LV_Park_01/04/07_A`, `Unity/LV_Park_01_U/04_U`)도 **동일 레시피**(memo `RYU 플러그인-프리 베이크` 참조) 반복 → **전부 끝난 뒤에만** uproject서 RYU 최종 disable = 프로젝트 전체 완료. sim_02는 원래 RYU無(이미 베이크 흔적 `/Game/_GENERATED/SM_Bake1`).
  - **잔여(선택)**: 병합메시 409만tris non-Nanite라 -game 캡처엔 다소 무거움 — Nanite 켤 땐 **유리(반투명) 슬롯 렌더 주의**. sim_01 간판 하나 절차생성 삐져나옴(베이크돼 개별수정 불가, CCTV 배경엔 무시 가능 — 이교수님 "그냥 둬"). standalone -game 최종 확인 권장. (도구/함정 상세는 전역 메모리 `barosim-park-scene-optimization`, `unreal-mcp-toolset-quirks`.)
- 2026-07-03: **CCTV 시뮬 C++를 `baroCCTVSimulator` 플러그인(서브모듈)으로 이관 — 3프로젝트 단일 소스 통일.** (커밋 `8eff2c5`; 이 엔트리는 2026-07-10 문서 정리 때 이력 공백을 메우려 소급 작성)
  - 게임모듈 `Source/baro_unreal/`의 CCTV C++ **19파일 삭제** → `Plugins/baroCCTVSimulator`(git submodule, 최초 핀 `88d6c6b`)가 소유. `baroCCTVSimulator` / `baroQuantum` / `baro_unreal` 세 프로젝트가 같은 플러그인 소스를 공유한다.
  - `baro_unreal.Build.cs`에서 CCTV 의존성(HTTP/Json/HTTPServer/Sockets/Networking) 제거 — 플러그인이 자체 보유하고, **호스트 모듈은 부팅만 담당**한다(현재 Core/CoreUObject/Engine/InputCore/EnhancedInput).
  - **CoreRedirects 필수**: 기존 레벨(`.umap`)/BP가 `/Script/baro_unreal.*`를 참조하므로 `DefaultEngine.ini [CoreRedirects]`에
    ClassRedirects **8개**(`PTZCamera`, `PTZCaptureComponent`, `PTZPlayerController`, `CenteringClientComponent`,
    `HucomsServerSubsystem`, `BaroSimGameMode`, `BaroSimHUD`, `BaroSimPlayerController`) +
    StructRedirects 1개(`CenteringPlate`) + EnumRedirects 1개(`ECenteringState`)를 추가했다.
    **이걸 빠뜨리면 레벨의 CCTV 액터가 통째로 사라진다.**
  - Config 경로 이관: `GlobalDefaultGameMode`와 Hucoms/SceneControl ini 섹션을 `/Script/baroCCTVSimulator.*`로 변경.
  - `.gitignore`: `Plugins/*` 제외 + `!Plugins/baroCCTVSimulator` 예외(RYU는 계속 제외). `.gitmodules`에 GitHub 원격 URL 설정.
- 2026-07-02 (저녁): **스트림 30fps + 미니멀 sim 실행 모드 + 줌 인식 setcenter + 원거리 화질 (baro_calory v0.2.0 대응).**
  - **스트림 파이프라인 30fps 실측 달성**: `StreamFps=30`(DefaultGame.ini — 코드 기본 15는 24fps 목표 미달). Tick accumulator `=0` 리셋→**잔여 보존+1프레임 클램프**(틱 양자화로 목표 미달하던 것). `FMjpegStreamServer`를 **프레임 시퀀스 게이트 + auto-reset FEvent 대기**로 전환(중복 재전송·고정 sleep 제거 — 페이싱은 producer가 결정, 송신 시간이 주기를 안 깎음). **송신 중 ClientsLock 해제**(블로킹 SendAll이 락을 물면 게임스레드 `HasClients()`가 같이 멈춰 sim 전체 프리즈 — 느린 원격 브라우저로 재현 가능한 major, 적대 리뷰 발견). 수렴 실측 29.9~30.2fps(720p q80).
  - **BaroSim 미니멀 실행 모드**(신규 `BaroSimGameMode`/`BaroSimPlayerController`/`BaroSimHUD`): standalone은 카메라 서버가 목적 — SpectatorPawn(구체 폰 제거), **`bDisableWorldRendering=true`**(메인 뷰포트 월드 렌더 OFF; SceneCapture는 자체 씬 렌더라 CCTV 무영향 — "오프스크린 전용"의 실현), `t.MaxFPS 60`, 커서 항상 표시(`DefaultInput.ini` NoCapture/DoNotLock), **ESC 종료**, HUD에 타이틀·채널별 실측 스트림 fps·클라이언트 수·게임 틱 fps 표시. `GameDefaultMap=/Game/simulator/LV_Park_sim_01`, `GlobalDefaultGameMode` 지정. PIE는 월드 렌더 유지(게이트: WorldType==Game).
  - **원거리 화질(줌 시 텍스처 뭉개짐) 원인 확정+수정**: UE5.8 엔진 소스 확인 결과 **텍스처 스트리머는 게임 뷰포트 뷰만 시점으로 등록**(GameViewportClient.cpp:1913→AddStreamingViewInfo), SceneCapture는 미등록 → mip 기준이 "투명 스펙테이터 90° 뷰"였다. 수정: Tick에서 채널마다 `IStreamingManager::AddViewInformation`(카메라 위치+**현재 줌 FOV**, 폭=max(Stream,Snapshot)) 등록 + `CaptureComp->LODDistanceFactor=현재HFOV/광각HFOV`(거리 기반 폴리지 컬링/페이드는 FOV 무시라 줌 보정 필요). 신규 줌 지점은 mip 스트리밍 1~2초 지연 정상.
  - **setcenter 줌 인식**: `ApplySetCenter`가 `Ch.CurZoom` 무시하고 광각 상수로 델타 환산 → 줌 시 배율만큼 과이동(10x에서 10배). `ZoomPosToHFov`로 현재 실효 FOV 환산(VFOV는 tan 비례). 검증: 줌 6000 클릭 → 반환 델타(pan -2.61°/tilt -1.24°)가 계산값과 소수점 일치, 표적 중앙 안착 ±0.4°. **호밍(줌인 반복 센터링) 안정성에도 직결.** baro_calory fake mock도 동일 모델로 정렬(`fov-convert.zoomPosToHFov` — HucomsProtocol.h와 같은 표, 동기화 유지).
  - **톤 "탄 느낌" 실측**: 구운 `대비 1.6`이 흰 차+직사광에서 하이라이트 클리핑 1.9%/암부 뭉개짐 12.0%로 세피아 톤(대비 1.0: 0.3%/2.5%). **`CaptureContrast=1.2`로 ini 베이크**(노출 -0.7은 mean~150 적정, 무죄). 차 옆면 잔여 누런 얼룩은 Lumen 바운스+에셋 먼지 레이어(물리적 타당, 유지). ※ 2026-07-02 오전의 "대비 1.6 확정"을 실측으로 **개정**.
  - **함정 2건**: ① 에디터 백그라운드 스로틀(Use Less CPU when in Background)로 포커스 잃으면 게임 틱 ~3.3fps → 스트림도 3.3fps. 성능 테스트는 standalone `-game` 필수(스탠드얼론은 백그라운드 스로틀 없음, 실측 확인). ② Live Coding 활성(에디터/게임 실행) 중엔 CLI 빌드 거부 — 새 UCLASS는 어차피 풀 리빌드 필요.
- 2026-07-02: **SceneCapture(CCTV JPEG) 화질 — 선명도 + 톤 실측 개선 (적대적 검증으로 두 이론 반증).**
  - 증상(이교수님): 게임 뷰포트는 선명한데 CCTV 캡처가 뿌옇고 "희게" 뜸(휴컴스 4K급 화질 요구). "혼자 자위 말고 적대적 검증하라" — 이후 전 과정 **실측**으로 진행.
  - **선명도** — 내 직감 두 개가 실측으로 반증됨:
    - `ShowFlags.SetTemporalAA(false)`(내 첫 수정)는 프로젝트 AA가 TSR이면 **TSR→FXAA 다운그레이드**를 강제해 오히려 풀프레임 블러(SceneView.cpp `SetupAntiAliasingMethod`). = 더 뿌옇게 만든 오답.
    - 멀티프레임 워밍업(연속 `CaptureScene()` N회로 TSR 히스토리 수렴 기대)도 **실측상 더 뿌옇다**(LapVar N=0:1358 > N=8:1174, 0.86×). 정지 프레임 재블렌딩=소프트닝일 뿐 초해상 이득 없음 + GPU (N+1)배 낭비.
    - **진짜 해법**: `SetTemporalAA(true)`+`SetAntiAliasing(true)`(FXAA 경로 제거) + SceneCapture 기본 GI/Reflection=None이므로 **Lumen 명시 오버라이드**(`bOverride_DynamicGlobalIlluminationMethod`/`ReflectionMethod`) + **단발 CaptureScene 1회**. 4K는 VRAM 굶음 경고("RT geometry >20% budget") 시 mip/Lumen 저하로 오히려 흐려 → **QHD(2560×1440)가 더 선명**. LapVar 878→1358.
  - **톤("희게"=과노출+저대비)** — 뷰포트를 `CaptureViewport`(EditorAppToolset)로 실렌더해 **FOV 맞춰(뷰포트 90° 중앙 70% 크롭≈캡처 70°)** 휘도 히스토그램 비교(FOV 안 맞추면 톤 비교 오염). 캡처가 뷰포트보다 밝고(mean 159 vs 122) 밋밋(std 47 vs 69). `AutoExposureBias=-0.7`+`ColorContrast=FVector4(1.6,1.6,1.6,1.0)` 오버라이드 → mean 123(뷰포트 122 정합), std 59, black p1=1. 최종 LapVar 2475(원본 대비 2.8×).
  - 확정: 전부 **코드 기본값**으로 베이크(TAA-on·Lumen·QHD 2560×1440·JpegQuality 92·노출-0.7·대비1.6·워밍업0). 튜닝 중엔 config UPROPERTY→DefaultGame.ini로 리빌드 없이 스윕, 값 확정 후 ini 오버라이드 제거(코드가 진실의 출처). ※ 대비 1.6은 같은 날 저녁 실측으로 1.2로 개정됨.
  - 검증 자산: `baro_calory/apps/backend-core/public/compare.html`(before/after 슬라이더 + 선명도/톤 지표표 + 뷰포트 기준). 여정: 원본(878)→선명수정(1358)→최종(2475).
  - **교훈(핵심)**: 선명도/톤은 **반드시 실측**하라 — 라플라시안 분산(선명) + 휘도 히스토그램(톤), PowerShell+System.Drawing(이 환경의 Python은 Store 스텁이라 못 씀, ImageMagick 없음). "선명해 보인다"(자축)도, 그럴듯한 이론(워밍업 수렴)도 데이터로 반증됐다. 뷰포트 대비 시 **FOV 정합 필수**. 전역 메모리 `ue-scenecapture-sharpness`에 확정 기록.
- 2026-07-02: **LV_Park_sim_01 클린 시뮬 레벨 — 4모서리 CCTV + 기존 주차기능 제거.** 이교수님이 레벨을 복제(`LV_Park_sim_01`)해 "에러 빼고 CCTV 4대(주차장 각 모서리), 기존 주차 관련 기능은 드러내고 순수 레벨만" 요청. 처리:
  - 정리(HUD 없이 클린 실행 선택): Level Blueprint EventGraph 전체(18노드) 제거 — 원인은 `Create Widget`가 클래스 미지정으로 컴파일 실패(`WBP_ParkingTool` 제거 여파). 잔존 `WBP`/`BP_Camera` 변수도 BlueprintTools로 삭제. 인엔진 HUD 없음 = UI는 baro_calory 웹이 담당.
  - CCTV 4대: 주차장 4모서리에 `APTZCamera` 배치, 채널/포트 매핑(8081~8084). standalone `-game` 기동 검증(4대 라이브 + baro_calory 백엔드 8080). ※ 이 4모서리 배치는 2026-07-06에 **폴 자식 2대 배치로 교체**됨(공중 배치는 틀린 규약).
- 2026-07-01: **PTZ 회전 2대 버그 수정 — 롤 + 상하반전 (적대적 검증으로 확정, 실렌더 검증 완료).**
  - 증상(이교수님 보고): (1) 피치 후 팬 시 두 회전이 꼬여 **지평선이 롤**됨. (2) 클릭/패드 상하가 **거꾸로**(상단 클릭 시 아래로).
  - 근본 원인:
    - **롤**: `APTZCamera::ApplyToComponents`가 팬을 **액터 로컬축**(`SetRelativeRotation`)으로 돌려, 액터 pitch가 팬 축에 새어들어 롤 유발. (코드 주석은 "액터를 똑바로 설치"로 회피 — CCTV로선 잘못.)
    - **상하반전**: sim `ApplySetCenter`가 **렌더 없는 `fake-camera-client.mjs`의 미검증 부호**(`tiltpos - tiltDelta`)를 그대로 답습. 진실의 출처는 field-validated `fov-convert.mjs`(**higher tiltpos = 아래**). → 아래 규약 섹션 참조. **이게 여러 곳에 반복 전파된 근본 원인.**
  - 수정(sim, `baro_unreal`): 팬을 **월드 수직축**(`SetWorldRotation` yaw-only)으로 → 어떤 설치각에서도 지평선 유지. 설치 Pitch를 **tilt로 이관**(`BuildChannels`, 롤 없이 화각 보존). `ApplySetCenter` `- Δ`→`+ Δ`. `TiltToPitchSign = -1` **유지**(실기 절대방향 충실도 — 렌더 부호는 건드리지 않음).
  - 수정(root, `baro_calory`): **fake-camera mock `+ tiltDelta`**(뿌리) + 그 테스트 assertion 정정(틀린 규약을 "통과"로 고정하던 것) + 공유 패드 `web-ui/ptz-controls.mjs` ▲=tiltpos↓ + `public/simple.html` 패드. Node 테스트 57/57 통과.
  - 검증(실렌더): standalone `-game` + baro_calory 스냅샷 — 상단클릭→**위**, 하단클릭→**아래**, 패드▲→**위**, 팬 0/45/90°에서 지평선 **수평**. tiltpos 리드백 **−1101**(신코드; 구코드는 +1101).
  - 방법론(모범): **적대적 검증 워크플로우**가 내 첫 직감("`TiltToPitchSign`을 +1로")을 **반증**함 — 그건 실기와 상하 반대 렌더를 초래(절대 tiltpos 명령/프리셋/호밍 전부 뒤집힘). 진짜 버그는 setcenter/패드 부호. **부호 문제는 확신하지 말고 반증하라.**
  - Live Coding 함정: `UWorldSubsystem` 클래스 변경은 PIE 중 hot-reload 불가(`ensure !bInitialized`). "succeeded"가 떠도 **미적용**(구 코드 계속 돎) → 에디터 닫고 **CLI 풀 리빌드** 필요. 판별은 런타임 readback으로(추측 금지). (전역 메모리에 기록.)
- 2026-07-01: **CCTV 시뮬레이터 이식 + 주차장 환경 이관 완료.**
  - CCTV C++: baro_world 5.8의 13개 소스(Hucoms 서버·프로토콜, PTZ 카메라·캡처·컨트롤러, MJPEG, centering) → `Source/baro_unreal/`.
    `BARO_WORLD_API`→`BARO_UNREAL_API`(5곳), config 섹션 `[/Script/baro_unreal.HucomsServerSubsystem]`, Build.cs에 HTTP/Json/HTTPServer/Sockets/Networking 추가.
    빌드 성공 → MCP로 클래스 5종(`/Script/baro_unreal.{HucomsServerSubsystem,PTZCamera,PTZCaptureComponent,PTZPlayerController,CenteringClientComponent}`) 라이브 검증. ※ 2026-07-03에 전부 플러그인으로 이관됨.
  - 레벨 이관: `parking_area`(UE5.7) `Content` 30GB(15,063파일) → `baro_unreal/Content` robocopy(이미 있는 파일 건너뜀). 주차장 레벨 16개(`LV_Park_01~08`+Unity 변형) 레지스트리 등록 확인.
  - 플러그인 갭 해소: `RYUKoreaBuilidngCreator`(한국 건물팩, 2.6GB, plugin content) → `Plugins/`로 복사, 5.7 바이너리 제거 후 **5.8 재컴파일**(모듈 빌드 성공). uproject에 PCG·Niagara·DatasmithContent·VariantManager·CineCameraSceneCapture·GeometryScripting·RYU 활성. `.uplugin` EngineVersion 5.7→5.8(호환 경고 제거).
  - 검증: `LV_Park_01` MCP 로드 → RYU 건물·주차면 24개·기존 BP_Camera·번호판(BP_Plate) 등 ~185 액터 정상 스폰(깨진 참조 없음).
  - PTZ 배치: 기존 `BP_Camera` 4대 위치에 이식한 `APTZCamera` 4대(`PTZ_Cam_01~04`) 배치 → 레벨 저장(5.7→5.8 업그레이드 확정).
  - 미검증(다음): PIE 실행으로 Hucoms 서버 기동/포트(8081 CGI, 8082 MJPEG) 확인, PTZ↔서버 미러링.
- 2026-07-01: `_forAI/` 문서 세트 초기 생성 (forai-scaffold). `baro_unreal`은 신규 UE 5.8 C++ 프로젝트
  (Source에 CCTV 코드 없음, 기본 레벨 test01, MCP 서버 활성).
  `plan.md`에 목표 기록 — parking_area 주차장 레벨(LV_Park_*) 이관 + baro_world 5.8 CCTV 시뮬레이터 이식.
  `inventory.md`/`memo.md`의 상세 항목 일부는 TODO(이식 진행 또는 명시 요청 시 소스 분석해 채움).

## PTZ 좌표·부호 규약 (Canonical — 진실의 출처)

> ⚠️ **다른 세션 필독.** 틸트 부호는 과거 여러 번 반복해서 틀렸다(mock→sim→UI로 전파). **새 코드는 `fake-camera` mock을 베끼지 말고 이 표를 따를 것.**
> 유일한 field-validated 출처: `baro_calory/packages/cctv-client/src/fov-convert.mjs` (cam-001, 2026-06-13, 번호판 12/12 in-frame). **이걸 다른 코드에 맞추려 뒤집지 말 것 — 실기 검증됨.**

| 축 | 와이어(raw) | 방향 규약 | 근거 |
|---|---|---|---|
| pan (`panpos`) | 0..35999 centi-deg | higher = **우측(시계, 위에서 봄)** | fov-convert `panPxSign=+1` |
| tilt (`tiltpos`) | −2000..9000 centi-deg | **higher = 카메라 아래를 봄** | fov-convert `tiltPxSign=+1`, `ptzToWidePixel` |
| zoom (`zoompos`) | 0..65535 tick | higher = 망원(줌인) | `ZoomPosToHFov` |

**파생 규칙 (전부 위 규약에서 나옴):**
- **setcenter(픽셀→PTZ)**: 화면 아래(y+) 클릭 → 그 대상을 중앙으로 = 아래로 조준 = **tiltpos↑**. `TgtTilt = Cur + ((y−cy)/H)·vfov·100`. pan도 동일 부호(우측 클릭 → panpos↑).
- **UI 패드**: ▲(위 보기) → **tiltpos↓** (dTilt 음수). ▼ → tiltpos↑. (`control-api.applyNudge`는 `tiltpos += dTilt`로 부호 중립 — 방향은 UI가 결정.)
- **sim 렌더(UE)**: UE는 +pitch=위. higher tiltpos(=아래)를 렌더하려면 pitch=−tiltpos → **`TiltToPitchSign = -1`**. **렌더 부호는 절대 뒤집지 말 것**(뒤집으면 절대 tiltpos 명령/프리셋/호밍이 전부 상하 반전). 조작 방향 문제는 setcenter/UI 층에서 고친다.
- **팬 축**: 항상 **월드 수직(중력) 기준**. 카메라 액터를 기울여 설치해도 롤 금지 — 상하 조준은 tilt로만.

**이 규약을 따라야 하는 동기화 지점:**
- sim(`baro_unreal`): `Plugins/baroCCTVSimulator/Source/baroCCTVSimulator/Private/HucomsServerSubsystem.cpp`(`ApplySetCenter`/`MirrorChannel`/`BuildChannels`), 같은 경로의 `PTZCamera.cpp`(`ApplyToComponents`). (2026-07-03 플러그인 이관 전에는 `Source/baro_unreal/`에 있었다.)
- `baro_calory`: `fake-camera-client.mjs`(`centerPoint`), `web-ui/ptz-controls.mjs`(패드), `public/simple.html`(패드), `control-api.mjs`(`applyNudge` 부호중립).
- **진실의 출처**: `fov-convert.mjs`.
