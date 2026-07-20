# Inventory

## 목차

- [Repository](#repository)
- [Top-level structure](#top-level-structure)
- [Entrypoints and key modules](#entrypoints-and-key-modules)
- [Submodule 운영](#submodule-운영)
- [Build and validation commands](#build-and-validation-commands)
- [Tests](#tests)
- [Notes](#notes)

## Repository

- Name: `baro_unreal`
- Path: `C:\works\ue_prjs\baro_unreal`
- Summary: UE 5.8 C++ 프로젝트. 주차장 CCTV 시뮬레이터. parking_area 주차장 환경 + baro_world 5.8 CCTV 시뮬레이터 C++를 통합(2026-07-01), 이후 CCTV C++는 `baroCCTVSimulator` 플러그인(서브모듈)으로 분리(2026-07-03).
- 에셋 원저작: 상명대 협업팀(요청서 `docs/sangmyung_team_request.md`). 원본은 `Parking_Project.uproject`(UE 5.7, 런타임 모듈 `Parking_Tool`).
- **브랜치 정책**(2026-07-10): `main` = **Windows 전용 기반**(기본 브랜치). `dev/vulkan-port` = Linux/Vulkan 포팅 실험 브랜치(Linux 경로를 걷어내기 직전 커밋 `c03c6b9` 보존 — `Config/Linux/LinuxEngine.ini`, Linux 타깃 RHI, `package.ps1 -Platform Linux`, Staging 예외가 온전). Linux/Mac은 실험적 용도.
- **git 전략 = code-only**: 소스·Config·문서만 추적한다. `.gitignore`가 `Content/`(30GB 상용 에셋), `Plugins/*`(단 `baroCCTVSimulator`는 서브모듈로 추적), 빌드 산출물(`Binaries` `Build` `Intermediate` `Saved` `Packaged` `DerivedDataCache`), `docs/outputs/`, `.env`를 제외한다. **신규 클론에는 Content도 RYU 플러그인도 없어 에디터가 정상 열리지 않는다** — 재취득 절차는 루트 `readme.md` 참조.

## Top-level structure

- `baro_unreal.uproject` — EngineAssociation 5.8, `TargetPlatforms=["Windows"]`. 활성 플러그인:
  - **에디터 전용**(`TargetAllowList=["Editor"]` — 게임 패키지에서 제외): `ModelingToolsEditorMode`, `ModelContextProtocol`, `AllToolsets` **세 개 모두**.
  - 런타임: `PCG`, `Niagara`, `DatasmithContent`, `VariantManager`, `CineCameraSceneCapture`, `GeometryScripting`, **`baroCCTVSimulator`**.
  - ⚠️ **`RYUKoreaBuilidngCreator`는 uproject Plugins 배열에 항목이 없을 뿐 여전히 활성(enabled)이다.** UE는 프로젝트 로컬 플러그인(`<Project>/Plugins/`)의 `EnabledByDefault` 미지정 시 기본 enabled로 취급한다(`FPlugin::IsEnabledByDefault` — Unspecified → `LoadedFrom == Project`). 실제로 `Plugins/RYUKoreaBuilidngCreator/Binaries/Win64/UnrealEditor-RYUKoreaBuilidngCreator.dll`이 빌드돼 있다. 진짜로 끄려면 uproject에 `{"Name":"RYUKoreaBuilidngCreator","Enabled":false}`를 명시하거나 `.uplugin`에 `"EnabledByDefault": false`를 넣어야 한다. 미베이크 원본 레벨(`LV_Park_01/04/07_A`, `Unity/LV_Park_01_U/04_U`)이 아직 이 플러그인을 필요로 하므로 지금 끄면 안 된다(`plan.md`).
- `Source/` — 호스트 게임 모듈. CCTV 런타임 구현은 전부 플러그인에 있고, 여기엔 **부팅 + 앱 고유 표시**만 둔다.
  - `baro_unreal.Build.cs` — Public: Core/CoreUObject/Engine/InputCore/EnhancedInput + **`baroCCTVSimulator`**. Private: **`Sockets`**, **`Projects`**, **`UMG`**, **`Slate`**, **`SlateCore`**, **`RHI`**.
  - `BaroUnrealHUD.{h,cpp}` — 앱 HUD(`AHUD` 직접 상속). 기존 텍스트 HUD와 시스템 상태 위젯을 함께 표시한다.
  - `BaroSystemMonitorSubsystem.{h,cpp}` — 1초 주기 CPU/RAM/GPU/VRAM/FPS 샘플링, 30초 UE 로그·CSV 저장, RAM 증가율 기반 상태 판정.
  - `BaroSystemMonitorWidget.{h,cpp}` — 우상단 native UMG 시스템 상태 패널. Blueprintable이므로 향후 WBP에서 레이아웃/스타일을 교체할 수 있다.
  - `BaroUnrealGameMode.{h,cpp}` — `ABaroSimGameMode` 상속, 생성자에서 `HUDClass`만 앱 HUD로 교체. 나머지(SpectatorPawn·월드 렌더 OFF·ESC)는 부모 그대로.
- `Source/*.Target.cs` — `BuildSettingsVersion.V7` / `EngineIncludeOrderVersion.Unreal5_8`, `[SupportedPlatforms("Win64")]`.
- `Plugins/baroCCTVSimulator/` — **git submodule**(CCTV 런타임 C++ 단일 소스). 아래 [Submodule 운영](#submodule-운영) 참조.
- `Plugins/RYUKoreaBuilidngCreator/` — 한국 건물팩(Fab 상용, 콘텐츠 2.6GB + Runtime C++ 모듈, 5.8 재빌드됨). git 미추적.
- `Content/` — parking_area에서 이관한 30GB 환경. 시뮬 레벨 `simulator/LV_Park_sim_01~03`, 원본 `Levels/LV_Park_01~08`, 대형 에셋팩(BlackAlder/Fab/Cars/Road_Creator_Pro/UltraDynamicSky). git 미추적.
- `Config/` — 5개 파일: `DefaultEditor.ini`, `DefaultEngine.ini`(RHI·맵·게임모드·`[HTTPServer.Listeners]`), `DefaultGame.ini`(플러그인 서브시스템 설정 + `[GeneralProjectSettings] ProjectVersion` = **앱 버전**, 현 0.1.0), `DefaultGameUserSettings.ini`(배포 기본 960×540 일반 창모드 `FullscreenMode=2` + Epic 품질·`sg.ResolutionQuality=100`), `DefaultInput.ini`(창모드 고정을 위해 `bAltEnterTogglesFullscreen=False` / `bF11TogglesFullscreen=False`).
  - MCP 자동시작은 `Saved/Config/.../EditorPerProjectUserSettings.ini`(git 미추적).
- `Scripts/` — `common.ps1`(.env 로더 + `Get-BaroSetting`/`Assert-BaroFile` 헬퍼), `build.ps1`, `run.ps1`, `package.ps1`.
- `docs/` — `windows_build_run.md`(팀원 온보딩: 준비물·서브모듈·`.env`·빌드/실행·트러블슈팅), `scene-control-api.md`(`/scene/*` REST 레퍼런스 + 3D→2D 투영), `sangmyung_team_request.md`(상명대 에셋팀 협업 요청서), `outputs/`(생성 PDF, git 미추적).
- `.env` / `.env.example` — PC별 로컬 설정. `UE_PATH`, `DEFAULT_MAP=/Game/simulator/LV_Park_sim_01`, `RUN_RESX=960`, `RUN_RESY=540`, `RUN_WINDOWED=true`, (옵션) `PACKAGED_EXE`. `.env`는 git 미추적 — `.env.example`을 복사해 쓴다.
- `readme.md`(사람용 온보딩) / `_forAI/`(AI 작업 문맥).

## Entrypoints and key modules

호스트 모듈(`Source/baro_unreal/`)에는 CCTV 코드가 없다. 부팅과 앱 고유 표시(`BaroUnrealGameMode` → `BaroUnrealHUD`)만 있고, standalone 진입점은 `GlobalDefaultGameMode=/Script/baro_unreal.BaroUnrealGameMode`다. 아래 CCTV 구현은 모두 `Plugins/baroCCTVSimulator/Source/baroCCTVSimulator/` 하위다.

- `Private/HucomsServerSubsystem.cpp` — `UHucomsServerSubsystem`(UTickableWorldSubsystem). `OnWorldBeginPlay`에서 카메라마다 Hucoms HTTP CGI 라우터 + MJPEG TCP 서버 기동(Game/PIE). Tick에서 PTZ 모터 슬루 → 카메라 미러 → 텍스처 스트리머 뷰 등록 → 캡처/스트림.
- `Private/PTZCamera.cpp` — `APTZCamera`(Pan/Tilt/Zoom 짐벌 액터). `bFixedMode`면 고정형 CCTV. sim 레벨에서는 `BP_Pole`의 자식으로 배치한다.
- `Private/PTZCaptureComponent.cpp` — `UPTZCaptureComponent`: SceneCapture → JPEG. `bOverrideVirtualTextureThrottle=true`(캡처 전용 렌더의 VT 페이지 스로틀 해제).
- `Private/SceneControlSubsystem.cpp` — `USceneControlSubsystem`: `/scene/*` 런타임 씬 제어 REST(기본 포트 8095). 주차면 슬롯은 `id=GetName()`(안정 식별자) + `label=GetActorLabel()`(표시명)을 함께 노출.
- `Private/MjpegStreamServer.cpp` — `FMjpegStreamServer`(FRunnable): 프레임 시퀀스 게이트 + auto-reset FEvent.
- `Private/CenteringClientComponent.cpp` — baro_vla 추론 서버 관측 클라이언트(번호판 검출).
- `Private/BaroSim{GameMode,PlayerController,HUD}.cpp` — standalone 미니멀 실행 모드(월드 렌더 OFF, ESC 종료, fps HUD).
- 상세 프로토콜/단위/기하는 `dev_log.md`의 「PTZ 좌표·부호 규약」과 baro_calory `fov-convert.mjs`(진실의 출처)를 따른다.

## Submodule 운영

- 정의: `.gitmodules` → `Plugins/baroCCTVSimulator` = `https://github.com/gbox3d/baroCCTVSimulator.git`
- 현재 핀: `ea38976`(heads/main, `.uplugin` VersionName **0.1.3**)
- 클론 직후 필수: `git submodule update --init --recursive` (빠뜨리면 플러그인 없이 열려 CCTV 전부 실종)
- 갱신 흐름: 서브모듈에서 커밋/푸시 → 부모에서 포인터 bump → `chore: baroCCTVSimulator 서브모듈 갱신 → <sha>` 커밋.

## Build and validation commands

모든 스크립트는 `common.ps1`이 `.env`를 읽어 `UE_PATH`/`PROJECT_FILE`/`DEFAULT_MAP`을 정한다. 엔진 경로를 하드코딩하지 말 것.

- **빌드**: `./Scripts/build.ps1 [-Target Editor|Game] [-Config Debug|DebugGame|Development|Shipping] [-Clean]`
  (기본 `Editor`/`Development`. `Editor`+`Shipping` 조합은 스크립트가 거부.)
  - 원시 호출이 필요하면: `& "$UE_PATH\Engine\Build\BatchFiles\Build.bat" baro_unrealEditor Win64 Development -Project="...\baro_unreal.uproject" -WaitMutex`
  - 새 UCLASS/UPROPERTY는 Live Coding 불가 → 에디터 닫고 CLI 풀 리빌드.
- **실행**: `./Scripts/run.ps1 [-Mode Game|Editor|Packaged] [-Map ...] [-ResX/-ResY] [-Fullscreen] [-Log] [-WaitForExit]`
  (기본 `Game` = standalone `-game`. 성능 실측은 반드시 이 모드 — 에디터는 백그라운드 스로틀.)
- **패키징(sim_01 단독, Win64)** — 에디터를 반드시 닫고 실행:
  ```powershell
  ./Scripts/package.ps1                  # Development, sim_01만 쿡 → Packaged/Win64 (~3.4GB)
  ./Scripts/package.ps1 -Config Shipping # 배포용 최적화(심볼 제외)
  ```
  RunUAT BuildCookRun 래퍼. 실행 전 `Packaged/Win64`를 경로 확인 후 비워, 과거 `Saved/GameUserSettings.ini`와 구 바이너리가 배포본에 섞이지 않게 한다.
  UAT 출력은 `Saved/Logs/package-uat.log`에 티잉되고, **Zen oplog 오류일 때만 1회 자동 재시도**한다(zenserver sponsor 경쟁 조건 — `memo.md` 「반복 금지」). 컴파일·쿡 에러는 재시도 없이 즉시 실패한다.
  ⚠️ 실행 중인 패키지 인스턴스가 있으면 아카이브 정리 단계가 파일락에 걸린다 — 패키징 전에 닫을 것.
- **패키지 실행 검증**: `Packaged/Win64/baro_unreal.exe` 실행 → 로그 `[Hucoms] 시뮬레이터 서버 시작 — 채널 N/N` 확인. 실 게임/소켓은 **자식 프로세스**(Shipping은 `baro_unreal-Win64-Shipping.exe`)가 소유하므로 **포트 기준**으로 확인:
  ```powershell
  Get-NetTCPConnection -State Listen -LocalPort 8081,8082,8091,8092,8095
  ```
  제어 = `BaseHttpPort 8081`+카메라인덱스, MJPEG = `BaseMjpegPort 8091`+인덱스, 씬 제어 = 8095 고정. 제어 포트 8081~8084와 8095는 LAN 원격 제어를 위해 `[HTTPServer.Listeners]`에서 포트별 `BindAddress=any`.

## Tests

- 자동화 테스트 스위트는 없다. 검증은 빌드 + 실행 관측으로 한다.
- Win64 Shipping 클린 패키징: `./Scripts/package.ps1 -Config Shipping -Clean`.
- 배포 EXE는 추가 인자 없이 직접 실행해 client 960×540, caption/resize frame, non-maximized인지 확인한다. 실행 전 아카이브에 `baro_unreal/Saved`가 없어야 한다.
- 런타임 검증: D3D12 모듈 로드, 포트 리슨, `/scene/catalog` 200(`pluginVersion` 확인), `/cgi-bin/image/jpeg.cgi` 2560×1440. 2026-07-10 Shipping 검증 통과.
- 메모리 모니터 CSV: 패키지 기준 `Saved/Logs/BaroHealth-YYYYMMDD-HHMMSS.csv`. 1초 샘플, 30초 기록이며 `MemorySlopeMBPerMin`과 `State`를 확인한다.
- 2026-07-20 Development 패키지 검증: workload RAM jump를 trend에서 분리했고, 166초 동안 `LEAK_SUSPECTED` 없이 RAM 감소 추세 및 `HEALTHY`를 확인했다.

## Notes

- 라인엔딩: `.gitattributes`가 `* text=auto`(저장소 LF / 체크아웃 CRLF)로 정규화하고 `*.uasset`·`*.umap`·이미지류는 binary 선언. Git LFS 도입 지점은 주석으로 예약돼 있으나 code-only 전략상 미적용.
- 인증 없음: 씬 제어(8095)·Hucoms CGI(8081~8084)는 토큰/API키가 없고 LAN에 열려 있다. **내부망 개발 보조 전용**이라는 의도된 결정이다(`memo.md`).
