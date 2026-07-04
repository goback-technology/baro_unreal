# Dev Log

## 목차

- [Entries](#entries)
- [PTZ 좌표·부호 규약 (Canonical — 진실의 출처)](#ptz-좌표부호-규약-canonical--진실의-출처)

## Entries

- 2026-07-01: `_forAI/` 문서 세트 초기 생성 (forai-scaffold). `baro_unreal`은 신규 UE 5.8 C++ 프로젝트
  (Source에 CCTV 코드 없음, 기본 레벨 test01, MCP 서버 활성).
  `plan.md`에 목표 기록 — parking_area 주차장 레벨(LV_Park_*) 이관 + baro_world 5.8 CCTV 시뮬레이터 이식.
  `inventory.md`/`memo.md`의 상세 항목 일부는 TODO(이식 진행 또는 명시 요청 시 소스 분석해 채움).
- 2026-07-01: **CCTV 시뮬레이터 이식 + 주차장 환경 이관 완료.**
  - CCTV C++: baro_world 5.8의 13개 소스(Hucoms 서버·프로토콜, PTZ 카메라·캡처·컨트롤러, MJPEG, centering) → `Source/baro_unreal/`.
    `BARO_WORLD_API`→`BARO_UNREAL_API`(5곳), config 섹션 `[/Script/baro_unreal.HucomsServerSubsystem]`, Build.cs에 HTTP/Json/HTTPServer/Sockets/Networking 추가.
    빌드 성공 → MCP로 클래스 5종(`/Script/baro_unreal.{HucomsServerSubsystem,PTZCamera,PTZCaptureComponent,PTZPlayerController,CenteringClientComponent}`) 라이브 검증.
  - 레벨 이관: `parking_area`(UE5.7) `Content` 30GB(15,063파일) → `baro_unreal/Content` robocopy(이미 있는 파일 건너뜀). 주차장 레벨 16개(`LV_Park_01~08`+Unity 변형) 레지스트리 등록 확인.
  - 플러그인 갭 해소: `RYUKoreaBuilidngCreator`(한국 건물팩, 2.6GB, plugin content) → `Plugins/`로 복사, 5.7 바이너리 제거 후 **5.8 재컴파일**(모듈 빌드 성공). uproject에 PCG·Niagara·DatasmithContent·VariantManager·CineCameraSceneCapture·GeometryScripting·RYU 활성. `.uplugin` EngineVersion 5.7→5.8(호환 경고 제거).
  - 검증: `LV_Park_01` MCP 로드 → RYU 건물·주차면 24개·기존 BP_Camera·번호판(BP_Plate) 등 ~185 액터 정상 스폰(깨진 참조 없음).
  - PTZ 배치: 기존 `BP_Camera` 4대 위치에 이식한 `APTZCamera` 4대(`PTZ_Cam_01~04`) 배치 → 레벨 저장(5.7→5.8 업그레이드 확정).
  - 미검증(다음): PIE 실행으로 Hucoms 서버 기동/포트(8081 CGI, 8082 MJPEG) 확인, PTZ↔서버 미러링.
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
- 2026-07-02: **LV_Park_sim_01 클린 시뮬 레벨 — 4모서리 CCTV + 기존 주차기능 제거.** 이교수님이 레벨을 복제(`LV_Park_sim_01`)해 "에러 빼고 CCTV 4대(주차장 각 모서리), 기존 주차 관련 기능은 드러내고 순수 레벨만" 요청. 처리:
  - 정리(HUD 없이 클린 실행 선택): Level Blueprint EventGraph 전체(18노드) 제거 — 원인은 `Create Widget`가 클래스 미지정으로 컴파일 실패(`WBP_ParkingTool` 제거 여파). 잔존 `WBP`/`BP_Camera` 변수도 BlueprintTools로 삭제. 인엔진 HUD 없음 = UI는 baro_calory 웹이 담당.
  - CCTV 4대: 주차장 4모서리에 `APTZCamera` 배치, 채널/포트 매핑(8081~8084). standalone `-game` 기동 검증(4대 라이브 + baro_calory 백엔드 8080).
- 2026-07-02: **SceneCapture(CCTV JPEG) 화질 — 선명도 + 톤 실측 개선 (적대적 검증으로 두 이론 반증).**
  - 증상(이교수님): 게임 뷰포트는 선명한데 CCTV 캡처가 뿌옇고 "희게" 뜸(휴컴스 4K급 화질 요구). "혼자 자위 말고 적대적 검증하라" — 이후 전 과정 **실측**으로 진행.
  - **선명도** — 내 직감 두 개가 실측으로 반증됨:
    - `ShowFlags.SetTemporalAA(false)`(내 첫 수정)는 프로젝트 AA가 TSR이면 **TSR→FXAA 다운그레이드**를 강제해 오히려 풀프레임 블러(SceneView.cpp `SetupAntiAliasingMethod`). = 더 뿌옇게 만든 오답.
    - 멀티프레임 워밍업(연속 `CaptureScene()` N회로 TSR 히스토리 수렴 기대)도 **실측상 더 뿌옇다**(LapVar N=0:1358 > N=8:1174, 0.86×). 정지 프레임 재블렌딩=소프트닝일 뿐 초해상 이득 없음 + GPU (N+1)배 낭비.
    - **진짜 해법**: `SetTemporalAA(true)`+`SetAntiAliasing(true)`(FXAA 경로 제거) + SceneCapture 기본 GI/Reflection=None이므로 **Lumen 명시 오버라이드**(`bOverride_DynamicGlobalIlluminationMethod`/`ReflectionMethod`) + **단발 CaptureScene 1회**. 4K는 VRAM 굶음 경고("RT geometry >20% budget") 시 mip/Lumen 저하로 오히려 흐려 → **QHD(2560×1440)가 더 선명**. LapVar 878→1358.
  - **톤("희게"=과노출+저대비)** — 뷰포트를 `CaptureViewport`(EditorAppToolset)로 실렌더해 **FOV 맞춰(뷰포트 90° 중앙 70% 크롭≈캡처 70°)** 휘도 히스토그램 비교(FOV 안 맞추면 톤 비교 오염). 캡처가 뷰포트보다 밝고(mean 159 vs 122) 밋밋(std 47 vs 69). `AutoExposureBias=-0.7`+`ColorContrast=FVector4(1.6,1.6,1.6,1.0)` 오버라이드 → mean 123(뷰포트 122 정합), std 59, black p1=1. 최종 LapVar 2475(원본 대비 2.8×).
  - 확정: 전부 **코드 기본값**으로 베이크(TAA-on·Lumen·QHD 2560×1440·JpegQuality 92·노출-0.7·대비1.6·워밍업0). 튜닝 중엔 config UPROPERTY→DefaultGame.ini로 리빌드 없이 스윕, 값 확정 후 ini 오버라이드 제거(코드가 진실의 출처).
  - 검증 자산: `baro_calory/apps/backend-core/public/compare.html`(before/after 슬라이더 + 선명도/톤 지표표 + 뷰포트 기준). 여정: 원본(878)→선명수정(1358)→최종(2475).
  - **교훈(핵심)**: 선명도/톤은 **반드시 실측**하라 — 라플라시안 분산(선명) + 휘도 히스토그램(톤), PowerShell+System.Drawing(이 환경의 Python은 Store 스텁이라 못 씀, ImageMagick 없음). "선명해 보인다"(자축)도, 그럴듯한 이론(워밍업 수렴)도 데이터로 반증됐다. 뷰포트 대비 시 **FOV 정합 필수**. 전역 메모리 `ue-scenecapture-sharpness`에 확정 기록.

- 2026-07-02 (저녁): **스트림 30fps + 미니멀 sim 실행 모드 + 줌 인식 setcenter + 원거리 화질 (baro_calory v0.2.0 대응).**
  - **스트림 파이프라인 30fps 실측 달성**: `StreamFps=30`(DefaultGame.ini — 코드 기본 15는 24fps 목표 미달). Tick accumulator `=0` 리셋→**잔여 보존+1프레임 클램프**(틱 양자화로 목표 미달하던 것). `FMjpegStreamServer`를 **프레임 시퀀스 게이트 + auto-reset FEvent 대기**로 전환(중복 재전송·고정 sleep 제거 — 페이싱은 producer가 결정, 송신 시간이 주기를 안 깎음). **송신 중 ClientsLock 해제**(블로킹 SendAll이 락을 물면 게임스레드 `HasClients()`가 같이 멈춰 sim 전체 프리즈 — 느린 원격 브라우저로 재현 가능한 major, 적대 리뷰 발견). 수렴 실측 29.9~30.2fps(720p q80).
  - **BaroSim 미니멀 실행 모드**(신규 `BaroSimGameMode`/`BaroSimPlayerController`/`BaroSimHUD`): standalone은 카메라 서버가 목적 — SpectatorPawn(구체 폰 제거), **`bDisableWorldRendering=true`**(메인 뷰포트 월드 렌더 OFF; SceneCapture는 자체 씬 렌더라 CCTV 무영향 — "오프스크린 전용"의 실현), `t.MaxFPS 60`, 커서 항상 표시(`DefaultInput.ini` NoCapture/DoNotLock), **ESC 종료**, HUD에 타이틀·채널별 실측 스트림 fps·클라이언트 수·게임 틱 fps 표시. `GameDefaultMap=/Game/simulator/LV_Park_sim_01`, `GlobalDefaultGameMode` 지정. PIE는 월드 렌더 유지(게이트: WorldType==Game).
  - **원거리 화질(줌 시 텍스처 뭉개짐) 원인 확정+수정**: UE5.8 엔진 소스 확인 결과 **텍스처 스트리머는 게임 뷰포트 뷰만 시점으로 등록**(GameViewportClient.cpp:1913→AddStreamingViewInfo), SceneCapture는 미등록 → mip 기준이 "투명 스펙테이터 90° 뷰"였다. 수정: Tick에서 채널마다 `IStreamingManager::AddViewInformation`(카메라 위치+**현재 줌 FOV**, 폭=max(Stream,Snapshot)) 등록 + `CaptureComp->LODDistanceFactor=현재HFOV/광각HFOV`(거리 기반 폴리지 컬링/페이드는 FOV 무시라 줌 보정 필요). 신규 줌 지점은 mip 스트리밍 1~2초 지연 정상.
  - **setcenter 줌 인식**: `ApplySetCenter`가 `Ch.CurZoom` 무시하고 광각 상수로 델타 환산 → 줌 시 배율만큼 과이동(10x에서 10배). `ZoomPosToHFov`로 현재 실효 FOV 환산(VFOV는 tan 비례). 검증: 줌 6000 클릭 → 반환 델타(pan -2.61°/tilt -1.24°)가 계산값과 소수점 일치, 표적 중앙 안착 ±0.4°. **호밍(줌인 반복 센터링) 안정성에도 직결.** baro_calory fake mock도 동일 모델로 정렬(`fov-convert.zoomPosToHFov` — HucomsProtocol.h와 같은 표, 동기화 유지).
  - **톤 "탄 느낌" 실측**: 구운 `대비 1.6`이 흰 차+직사광에서 하이라이트 클리핑 1.9%/암부 뭉개짐 12.0%로 세피아 톤(대비 1.0: 0.3%/2.5%). **`CaptureContrast=1.2`로 ini 베이크**(노출 -0.7은 mean~150 적정, 무죄). 차 옆면 잔여 누런 얼룩은 Lumen 바운스+에셋 먼지 레이어(물리적 타당, 유지). ※ 2026-07-02 오전의 "대비 1.6 확정"을 실측으로 **개정**.
  - **함정 2건**: ① 에디터 백그라운드 스로틀(Use Less CPU when in Background)로 포커스 잃으면 게임 틱 ~3.3fps → 스트림도 3.3fps. 성능 테스트는 standalone `-game` 필수(스탠드얼론은 백그라운드 스로틀 없음, 실측 확인). ② Live Coding 활성(에디터/게임 실행) 중엔 CLI 빌드 거부 — 새 UCLASS는 어차피 풀 리빌드 필요.

- 2026-07-04: **RYU 플러그인-프리 이관 + `LV_Park_sim_01` 빌딩 스태틱 베이크 (이교수님 최우선 목표 "플러그인 없이 이식 가능한 맵" — sim_01 달성).**
  - **성능 선작업(sim_01)**: 이전팀 잔재 삭제(매프레임 풀씬 렌더 `SceneCapture2D_1` + 시네 `BP_Camera`×2). 유리/IES 반투명 Nanite 폴백 9에셋 정리. 무거운 불투명 메시 Nanite 전환(`현대_쏘나타` 121만tris=씬 삼각형 44% + 펜스/가로등/건물 등 19개). **UDS `Disable All Runtime Updating`=true**(정적 씬 전용 프리즈 — 매프레임 시간/구름/스카이라이트 갱신 0). 측정은 standalone -game(에디터 백그라운드 스로틀 회피).
  - **RYU 콘텐츠 이관**: 프로젝트 유일 외부 플러그인=RYUKoreaBuilidngCreator. 7레벨이 참조하는 RYU 자산 **transitive closure 1,096개 → `/Game/_RYU_Portable/`로 폴더단위 이동**(`AssetTools.move`). move는 구경로에 리다이렉터 남기고 참조 자동수정 안 함 → 에디터 우클릭 **"Update Redirector References"**(구 Fix Up Redirectors)로 6레벨+ExternalActors 참조 /Game 재작성. 검증: 7레벨(sim_01/02, LV_Park_01/04/07_A, Unity/LV_Park_01_U/04_U) 직접·transitive RYU참조 **0**.
  - **초기 오판 정정 — RYU는 "콘텐츠 전용"이 아니었다.** `BP_RYUBuilding` 직접 deps엔 /Script/RYU 없지만, **PCG 빌딩 생성기(`BP_GenerateUpperDistributionByGrammar`/`BP_GenerateModuleDistribution` + 구조체 `SymbolMeshVariation` in `BP_RYUUpperHorizontalPreset`)가 `/Script/RYUKoreaBuilidngCreator` C++ 사용**. 빌딩 상세 지오메트리는 PCG가 매 로드 생성하는 **transient** — 플러그인 disable 시 상세 소실(컴포넌트 ~68 → 8 = 기초박스+옥상만). ⇒ 이런 PCG 빌딩은 **베이크가 먼저, 플러그인 제거가 나중**이 올바른 순서(내가 반대로 해서 한번 헛돎, 백업으로 복구).
  - **sim_01 베이크 성공(올바른 순서)**: ①플러그인 재활성화+에디터 재시작 ②빌딩 PCG `generationTrigger`를 GenerateOnLoad로 되돌리고 `save_assets([])`+`load_level` 리로드 → 상세 재생성(8→~68컴포넌트/빌딩) ③6빌딩 전체선택 → **Merge Actors**(Method=Merge, **Merge Materials=off**, Replace=off) → 스태틱메시. **함정: 결과 피벗=첫 선택액터(C_1) 위치**라 배치 후 C_1 world location으로 이동해야 정렬(baked vs orig의 X-max·Z-max 정확일치로 검증, min 2~3m차=원본 스플라인/디버그 컴포넌트 인플레). ④원본 BP 6개 `remove_from_scene` ⑤저장+전이검증 = **RYU콘텐츠 0·/Script/RYU 0 확정**. 결과물 `/Game/simulator/SM_SM_sim01_Buildings`(409만tris, non-Nanite, 129머티슬롯, /Game/_RYU_Portable 참조). **sim_01 = 완전 플러그인-프리(콘텐츠+C++ 둘 다).**
  - **현재 상태 / 이어작업**: 플러그인은 다시 **ENABLED**(나머지 레벨이 아직 씀). 나머지 RYU빌딩 레벨(`LV_Park_01/04/07_A`, `Unity/LV_Park_01_U/04_U`)도 **동일 레시피**(memo `RYU 플러그인-프리 베이크` 참조) 반복 → **전부 끝난 뒤에만** uproject서 RYU 최종 disable = 프로젝트 전체 완료. sim_02는 원래 RYU無(이미 베이크 흔적 `/Game/_GENERATED/SM_Bake1`).
  - **잔여(선택)**: 병합메시 409만tris non-Nanite라 -game 캡처엔 다소 무거움 — Nanite 켤 땐 **유리(반투명) 슬롯 렌더 주의**. sim_01 간판 하나 절차생성 삐져나옴(베이크돼 개별수정 불가, CCTV 배경엔 무시 가능 — 이교수님 "그냥 둬"). standalone -game 최종 확인 권장. (도구/함정 상세는 전역 메모리 `barosim-park-scene-optimization`, `unreal-mcp-toolset-quirks`.)

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
- sim(`baro_unreal`): `HucomsServerSubsystem.cpp`(`ApplySetCenter`/`MirrorChannel`/`BuildChannels`), `PTZCamera.cpp`(`ApplyToComponents`).
- `baro_calory`: `fake-camera-client.mjs`(`centerPoint`), `web-ui/ptz-controls.mjs`(패드), `public/simple.html`(패드), `control-api.mjs`(`applyNudge` 부호중립).
- **진실의 출처**: `fov-convert.mjs`.
