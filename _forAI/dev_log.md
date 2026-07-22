# Dev Log

## 목차

- [Entries](#entries)
- [PTZ 좌표·부호 규약 (Canonical — 진실의 출처)](#ptz-좌표부호-규약-canonical--진실의-출처)

## Entries

> **최신순(내림차순)**. 새 엔트리는 이 목록 맨 위에 추가한다.
> 각 엔트리는 그 시점의 스냅샷이다 — 나중에 사실이 바뀌어도 과거 엔트리는 고쳐 쓰지 않고,
> 최신 엔트리에서 정정한다.

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
