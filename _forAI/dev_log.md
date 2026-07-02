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
