# Plan

## 목차

- [Current goal](#current-goal)
- [Near-term work](#near-term-work)
- [/scene API 확장 계획 (v0.1.7 구현 완료)](#scene-api-확장-계획-v017-구현-완료)
- [Structure decisions](#structure-decisions)
- [Risks](#risks)

> 이 문서는 **앞으로 할 일만** 담는다. 완료된 마일스톤(2026-07-01 ~ 07-10: CCTV 이식, 환경 이관,
> 스트림 30fps, SceneCapture 화질, sim 레벨 정비, 고정 카메라 모드, RYU sim_01/03 베이크,
> CLI 패키징, VT 스로틀 수정, Windows 전용 배포 복원)은 `dev_log.md`에 날짜별로 보존돼 있다.

## Current goal

**플러그인 없이 이식 가능한 주차장 CCTV 시뮬레이터**를 완성한다(이교수님 최우선 목표).
`baroCCTVSimulator` 플러그인(자체 소스, 서브모듈)만 남기고, 상용 `RYUKoreaBuilidngCreator` 의존을
전 레벨에서 제거하는 것이 남은 관문이다. 시뮬 자체(제어·스트림·씬 제어 API·Win64 Shipping 배포)는
이미 baro_calory와 실기 동일 계약으로 동작한다.

## Near-term work

### 능동 작업

- [x] **런타임 스폰 카메라의 Hucoms CGI 가 원격에서 안 열린다 (v0.1.13 결함) — 앱 0.2.5 / 플러그인 0.1.14 로
      수정·로컬 검증 완료(2026-08-06). 배포 미수행.** 원인은 스폰 코드가 아니라 바인드 주소였고,
      리스너를 여는 코드가 자기 포트의 노출을 선언하도록 바꿨다(`HttpListenerBind.h`). ini 포트 목록은 폐기.
      상세·실측은 `dev_log.md` 2026-08-06, 상시 규칙은 `memo.md` 「기본 설정값」.
      **배포기(192.168.0.22) 재배포까지 완료** — 원격 `lan-bind-contract.mjs` 전 항목 통과.
      남은 일은 커밋·push·서브모듈 핀 갱신뿐이다(배포본은 워킹트리로 구운 `-dirty` 빌드).
- [x] ~~**v0.2.0 48시간 soak 검증**~~ — **26h 중간 판정 합격**(2026-07-23 17:14 KST, 이교수님 선언).
      정상 상태 physical 기울기 0.0 MB/min, 캡처 26h 연속 가동 확인. 상세는 `dev_log.md` 2026-07-23 17:14
      엔트리. **잔여**: 48h 완주 시 `baro_unreal.log`·`Saved/Crashes/` 회수해 GameFeatureData·Crashes 최종 확정.
- [ ] **배포본을 Shipping 으로 전환 (이제 착수 가능, 다음 관문).** soak 가 누수 없음을 확인했으므로 로그·
      `BaroHealth-*.csv` 감시 목적의 Development 는 소임을 다했다. Shipping 은 `ensure` 가 컴파일 아웃돼
      이상을 숨기니, 48h 완주로 Crashes 공란까지 확정한 **뒤** `./Scripts/package.ps1 -Config Shipping -Zip`.
- [ ] **`baroQuantum` 플러그인 핀 갱신 검토** — 현재 `9a52a22`(v0.1.5 = 암부 화질 회귀가 있는 폐기 버전)에
      묶여 있다. v0.1.6 으로 올리려면 그쪽 `DefaultEngine.ini` 에도 `r.Lumen.HardwareRayTracing=True` 가
      필요하다(없으면 가드가 persist 를 꺼 v0.1.5 와 같은 화질이 된다 — 누수는 없음). 별건·이교수님 판단.

- [x] **/scene API 확장 — 플러그인 v0.1.7 구현 완료(응용 SW팀 요청 접수 2026-07-23)**:
      차종별 실제 치수, 카메라 기준면 높이, zoom→HFOV 화각표를 기존 응답에 하위 호환 필드로 추가한다.
      상세 계약·결정 항목·검증 결과는 아래 [전용 계획](#scene-api-확장-계획-v017-구현-완료)을 따른다.
      2026-07-23 에디터 빌드, UE 자동화 4개, JS 테스트 200개와 standalone 실응답 검증을 통과했다.
- [x] **BEVHeight 파인튜닝 확장 — 플러그인 v0.1.8 구현·검증 완료(2026-07-23, 응용 SW팀 요구서 `_localfiles/sim_camera_request.md`)**:
      ① config 카메라 스포너(레벨 무수정, 높이별 8/12/16/20m — `+SpawnCameras` in DefaultGame.ini),
      ② 차종 `class` 라벨(car/truck/van, 버스 없음), ③ 가시성/가림 GT(`/scene/cars?visibility=<cam>` →
      visibleRatio 0..1, 라인트레이스), ④ 연속 PTZ 미러(pt_control/zf_control velocity), ⑤ 핀홀 명시 필드
      (projection/distortion/rollDeg). 에디터 빌드 + 라이브 검증(6대·포트·높이·연속PTZ·가시성·렌더 육안) +
      baro_calory 207 테스트 통과. baro_calory 버전 범프(root/backend-core 0.5.7, cctv-client 0.1.9, sim 0.4.6).
      **미해결(이교수님 판단 필요)**: LV_Park_sim_01 은 좁은 폐쇄형(~21×27m)이라 실장비 cam-real-002 의
      16m/20°/36-49m **클리어 사이트라인 재현 불가** — 44m 남쪽은 단지 정원·건물 뒤라 주차장이 가려진다
      (실측: visibleRatio 0, 렌더가 잔디/건물). 현재는 남쪽 22m 에 높이만 8/12/16/20m 쌓고 각자 주차중심
      조준(PitchDeg 개별)해 클리어뷰 확보(틸트 20~42°, 슬롯 슬랜트 12~42m — 실장비 36~49m 와 far row 에서 겹침).
      실장비 원거리 저각이 필수면 더 개방된 레벨 필요(ini 만 고치면 됨·리빌드 불요). **커밋·push·배포 미수행.**
- [ ] **나머지 RYU 레벨 빌딩 베이크**: `LV_Park_01 / 04 / 07_A`, `Unity/LV_Park_01_U / 04_U`.
      절차는 `memo.md`의 「RYU 플러그인-프리 베이크」 레시피 그대로. (sim_01·sim_03 완료, sim_02는 RYU無.)
- [ ] **전 레벨 베이크 완료 후에만** uproject에 `{"Name":"RYUKoreaBuilidngCreator","Enabled":false}`를
      명시해 최종 비활성화 → 프로젝트 전체 플러그인-프리.
      ⚠️ 현재는 uproject에 항목이 없을 뿐 **여전히 활성**이다(프로젝트 로컬 플러그인 기본 enabled).
      그전에 끄면 미베이크 레벨의 빌딩 상세가 소실된다.
- [ ] **성능: 캡처 파이프라인 오프스레드화** — 4캠 동시 24fps가 필요해지면 착수.
      JPEG 인코딩 오프스레드(`UE::Tasks`) → 블로킹 `ReadPixels` → 비동기 `FRHIGPUTextureReadback`
      → `jpeg.cgi` 스냅샷 캐시. (근거: baro_calory dev_log 2026-07-02 저녁 진단)
      ※ 앞 두 항목(오프스레드 인코딩·비동기 리드백)은 **플러그인 v0.1.10 에서 구현됨**. 남은 것은
      `jpeg.cgi` 스냅샷 캐시인데, 소비자 쪽(baro_calory)이 프리뷰를 스트림 프레임으로 돌리면서
      급한 불은 껐다(2026-07-29, `preview-snapshot.mjs`). 캐시는 필요해지면 착수.

- [ ] **캡처 페이싱의 계단 양자화 제거 — 플러그인 1줄, 다음 플러그인 갱신 때 함께 넣을 것.**
      `HucomsServerSubsystem.cpp` Tick 의 스트림 페이싱이 **제출을 못 해도 누적기 예산을 먼저 깎는다**:
      ```cpp
      if (Ch.StreamAccum >= Interval) {
          Ch.StreamAccum = FMath::Min(Ch.StreamAccum - Interval, Interval);   // 먼저 깎고
          ...
          if (Ch.StreamCapState == EStreamCapState::Idle)                     // 그 다음 판정
              SubmitStreamCapture(Ch, bCold);                                 // InFlight 면 기회가 소멸
      }
      ```
      채널당 in-flight 가 1개라, GPU 왕복이 `Interval`(=1/StreamFps=33.3ms)을 넘으면 그 제출 기회가
      통째로 버려진다. 그래서 실효 fps 가 `StreamFps/k` 로만 나온다 — **30 / 15 / 10 / 7.5**.
      지연이 조금만 나빠져도 절벽으로 떨어지고, 반대로 개선해도 계단을 못 넘으면 숫자가 전혀
      안 움직여 **개선 여부를 측정할 수 없다**(이번에 RT 풀 튜닝이 그래서 판정이 어려웠다).
      **수정**: `Idle` 판정을 누적기 차감 **앞으로** 옮긴다 → `min(StreamFps, 1/지연)` 의 연속 저하가 된다.
      **실측 근거(2026-07-29, 배포기 192.168.0.22)**: v0.2.3(RT 풀 1600MB) 적용 후 25.9fps 인데,
      563장 중 377장만 33ms 이내로 오고 나머지는 한 칸 미끄러진다(간격이 33ms 와 62ms 사이 진동).
      이 한 줄이 남은 4fps 를 회수한다. 30fps 를 여유 있게 넘기려면 in-flight 다중화(링버퍼)가
      필요하지만 그건 별건이고 수명 관리 재검증이 따른다 — 우선 이 한 줄부터.
      ⚠ 공유 서브모듈이라 버전 범프 → 풀 리빌드 → push → baro_unreal·baroQuantum 서브모듈 갱신 사슬.

### 조건부 / 유보 (실제로 문제가 될 때만)

- [ ] PIE 일시정지·캡처 실패 시 스트림 keepalive가 없어 클라이언트가 무프레임 대기한다.
      소비자 read-timeout 이슈가 **실제로 생기면** idle 재전송을 추가한다.
- [ ] 나머지 `LV_Park` 레벨(02~08)에도 PTZ 배치 — **필요해지면**. (배치 규약: `BP_Pole` 자식, z+600, pitch −20)
- [ ] `sim_03` 병합메시 Nanite 불가(144 머티리얼 섹션 > 한계). **필요하면** Merge Materials=on으로 재병합.
      VSM 그림자 경고는 잔존하나 콜리전 제거로 실부하는 이미 해소됨.

## /scene API 확장 계획 (v0.1.7 구현 완료)

> 상태(2026-07-23): 코드·계약 문서·버전 반영과 로컬 검증 완료. 커밋·push·소비 저장소의
> 서브모듈 핀 갱신 및 배포는 이번 요청 범위에 포함하지 않아 수행하지 않았다.

### 목표와 범위

응용 SW팀이 제조사 공표 치수나 클라이언트 내부 상수 사본에 의존하지 않고, 실행 중인 sim의
`/scene/*` 응답만으로 차량 3D 크기와 카메라 모델을 구성할 수 있게 한다.

- 포함: 차종별 차량 bounds, 주차면에서 유도한 기준 지면과 카메라 높이, zoom→HFOV 화각표.
- 연동 포함: baro_calory 실 클라이언트·Fake 응답·입력 검증·계약 테스트.
- 제외: 현재 PTZ/current FOV의 `/scene/cameras` 중복 노출(계속 Hucoms `getptzfpos` 사용),
  차량별 가시성/가림 GT(카메라별 segmentation 계약이 필요한 별도 과제).
- 호환성: 기존 필드는 삭제·변경하지 않고 새 필드만 추가한다.

### 화각표 정의

화각표는 `zoompos`와 실측 수평 화각 HFOV(deg)의 앵커 목록이다. 현재 13개 앵커
(`0→57.14°` … `16384→2.39°`) 사이를 선형 보간하고 범위 밖은 양 끝에서 클램프한다.
망원 포화와 비선형 구간 때문에 `wideHFovDeg` 하나만으로 대체할 수 없다.

구현 시 `HucomsProtocol.h::ZoomPosToHFov` 내부의 표를 공용 `constexpr` 데이터로 분리해
계산과 JSON 직렬화가 같은 데이터를 사용하게 한다. JS는 API 응답을 우선 사용하고 내장 표는
실카메라·오프라인 폴백으로만 유지한다.

제안 응답(필드명은 구현 착수 전 최종 확정, 아래 `zoomHfov`는 첫/끝점만 보인 축약 예시):

```json
{
  "intrinsics": {
    "interpolation": "linear",
    "clamp": true,
    "zoomHfov": [
      { "zoomPos": 0, "hfovDeg": 57.14 },
      { "zoomPos": 16384, "hfovDeg": 2.39 }
    ]
  }
}
```

### 차량 치수

`/scene/catalog.cars[]`에 cm 단위 bounds를 추가한다. 축 의미를 검증하기 전에는
`length/width/height`로 추정하지 않고 UE 로컬 축 `x/y/z`를 그대로 노출한다.

```json
{
  "boundsCm": {
    "coordinateSpace": "actorLocal",
    "center": { "x": 2.1, "y": 0.0, "z": 72.3 },
    "size": { "x": 471.2, "y": 186.0, "z": 144.5 },
    "source": "renderedMeshAggregate"
  }
}
```

구현 결정:

1. `Mesh_List`의 주 `UStaticMesh::GetBounds()`만 사용하면 빠르고 결정적이지만 휠·번호판 등
   자식 컴포넌트를 제외할 수 있다.
2. 차종을 적용한 임시 `BP_Car`에서 표시 중인 `UMeshComponent`를 자식 액터까지 합치면 휠·번호판
   메시를 포함하면서 가변 `TextRender`는 제외할 수 있다. actor-local `center`도 함께 제공해야
   차량 피벗과 bounds 중심이 다른 에셋의 3D 박스를 정확히 배치할 수 있다.
3. 파인튜닝 3D 라벨 목적에 필요한 최종 렌더 형상을 보존하기 위해 2번을 채택했다. 실행 검증에서
   23종 모두 양수 bounds였고 크기 범위는 X `362.19..526.06`, Y `182.77..238.39`,
   Z `133.06..251.35` cm였다.

### 기준 지면과 카메라 높이

현재 장면은 슬롯 24개의 Z가 모두 10cm인 평면이지만 이를 영구 전제로 하드코딩하지 않는다.
`/scene/cameras`에 슬롯 Z 중앙값으로 만든 **기준면**과 광학중심의 기준면 대비 높이를 추가한다.
경사·다층 장면의 로컬 지면과 혼동하지 않도록 일반적인 `groundZ` 대신 출처가 드러나는 이름을 쓴다.

```json
{
  "groundReference": {
    "zCm": 10.0,
    "method": "parkingSlotPlacementOriginMedian",
    "sampleCount": 24,
    "maxDeviationCm": 0.0
  },
  "heightAboveReferenceGroundCm": 575.0
}
```

슬롯이 없으면 기준면과 파생 높이는 `null`로 반환한다. `maxDeviationCm`을 함께 제공해 소비자가
평면 가정을 적용해도 되는지 판단하게 한다. 향후 경사·다층 지원이 필요할 때만 카메라 하향
레이캐스트 기반 `heightAboveLocalGroundCm`을 별도 설계한다.

### 구현 순서와 검증

1. **차량 bounds 의미 확정**
   - 23종의 주 메시 bounds와 최종 액터 aggregate bounds를 비교하고 휠·번호판 포함 여부를 기록한다.
   - 검증: 모든 축이 양수이고 대표 차종의 UE 에디터 bounds와 허용오차 내 일치.
2. **화각표 단일 소스화**
   - C++ 앵커를 공용 상수로 분리하고 기존 `ZoomPosToHFov`가 이를 사용하도록 유지한다.
   - 검증: 13개 앵커, 중간 보간, 양끝 클램프 테스트가 변경 전 수치와 동일.
3. **카탈로그와 카메라 JSON 확장**
   - `cars[].boundsCm`, `cameras[].groundReference`,
     `cameras[].heightAboveReferenceGroundCm`, `cameras[].intrinsics` 추가.
   - 검증: 기존 필드 스냅샷 불변, 24슬롯 장면에서 기준 Z=10cm·카메라 높이=575cm.
4. **baro_calory 연동**
   - `FakeSceneClient`에 동일 shape를 추가하고 웹 투영이 API 화각표를 우선 사용하게 한다.
   - 하드코딩된 `CAR_TYPE_MAX=22`를 실제 카탈로그 `carCount-1` 검증으로 바꾼다.
   - 검증: 기존 계약 테스트 + 24번째 가상 차종 허용/범위 밖 400 테스트.
5. **회귀 검증**
   - UE 자동화 테스트, 에디터 풀 빌드, PIE/standalone API 실응답을 확인한다.
   - 기존 JS 재투영과 `/scene/project`의 pan/tilt/zoom 전체 최대 오차 0.03px 이하 유지.
6. **버전·문서·배포 사슬**
   - 플러그인 `0.1.7` 범프와 `docs/scene-control-api.md`/baro_calory 계약 문서 갱신 완료.
   - 커밋·push·소비 저장소 서브모듈 핀 갱신은 별도 명시 요청 시 수행한다.
   - `UWorldSubsystem` 변경이므로 Live Coding 없이 에디터 종료 후 풀 리빌드한다.

### 완료 조건

- 23개 모든 차종에 출처가 명시된 양수 bounds가 반환된다.
- 카메라별 기준 지면·높이와 13개 화각 앵커가 API만으로 해석 가능하다.
- 기존 클라이언트는 새 필드를 무시해도 그대로 동작한다.
- UE/JS 화각 계산과 투영 오라클의 수치가 회귀하지 않는다.
- 가시성/가림 GT는 이번 버전에 섞지 않는다.

## Structure decisions

- 에셋 이관: **파일 직접 복사(robocopy)** 채택 — parking_area가 5.7이고 Content 의존이 광범위해 통째 복사가 확실. `/Game` 경로 동일해 참조 자동 해소.
- CCTV 코드: 호스트 게임 모듈이 아니라 **`baroCCTVSimulator` 플러그인(git submodule)** 단일 소스.
  baroQuantum과 baro_unreal이 함께 소비한다. 이관 시 `CoreRedirects` 필수(2026-07-03).
- **플러그인은 "최소한의 카메라"**(2026-07-10 확정). 앱 고유 기능(HUD·버전 표기·서빙 주소)은 플러그인이 아니라
  호스트 게임 모듈에서 상속으로 붙인다. 공용 서브모듈을 건드리면 버전 범프·풀 리빌드·push·두 프로젝트
  서브모듈 갱신 사슬이 발생하고, 다른 소비 프로젝트에 불필요한 변경이 강제된다.
- **브랜치**: `main` = Windows 전용 기반. Linux/Vulkan 작업은 `dev/vulkan-port`에서만 한다(실험적).
- 레벨: parking_area 레벨은 **모놀리식 `.umap`(월드파티션 아님)** — 그대로 사용.
- 플러그인: RYU는 프로젝트 로컬 `Plugins/`(콘텐츠+모듈, 5.8 재빌드), 나머지는 엔진 내장 활성.
- **플랫폼: Windows·Win64 전용**(2026-07-10 확정, **2026-07-22 이교수님 재확인** — "당분간 윈도우 전용,
  리눅스는 화질 손상"). Linux/Vulkan은 보류다 —
  POC 배포는 성공했으나 Vulkan 오프스크린에서 VT 피드백이 동작하지 않아 주차면 라인 데칼이 렌더되지 않았다.
  Vulkan 지원을 재개하려면 라인 데칼을 비-VT 머티리얼로 교체해야 한다(플랜 B, `dev_log.md` 2026-07-10 오전).
  **별도 요청 전까지 Linux 경로는 범위 밖**이다.
- **제어 API 무인증**: 내부망 개발 보조 전용이라는 의도된 결정(`memo.md`).

## Risks

- parking_area의 Fab/마켓플레이스 에셋 라이선스·용량 제약(재배포 불가 → code-only git 전략의 근거).
- **RYU 빌딩은 PCG + `/Script/RYU` C++로 매 로드 생성되는 transient** — 콘텐츠 이관만으론 플러그인-프리 불가.
  반드시 **빌딩 스태틱 베이크 후** 플러그인 제거(순서를 뒤집으면 빌딩 상세 소실).
  현재 sim_01·sim_03만 베이크 완료 — 나머지 레벨 미베이크 상태에서 끄면 그 레벨들이 깨진다.
- 서브모듈 미초기화 클론에서 CCTV 클래스가 통째로 사라진 채 레벨이 열려, 참조 깨짐이 액터 소실로 오인될 수 있다.
