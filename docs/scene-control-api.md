# 씬 제어 API (/scene/*) 레퍼런스

UE 시뮬레이터의 씬(주차면·차량·카메라 파라미터)을 **실행 중에** 편집·조회하는 HTTP REST API.
`baroCCTVSimulator` 플러그인의 `USceneControlSubsystem`이 구현하며, baro_calory(Node 백엔드)의
`/api/simulator/*`가 이 API를 그대로 프록시한다. 웹UI·CLI·(향후) MCP가 전부 이 하나의 제어면을 공유한다.

> 문서 기준: 플러그인 v0.1.6 (2026-07-22). 응답 예시는 전부 실측 캡처.

## 목차

1. [개요와 설계 원칙](#개요와-설계-원칙)
2. [아키텍처 (소비 스택)](#아키텍처-소비-스택)
3. [서버 기동과 포트 설정](#서버-기동과-포트-설정)
4. [엔드포인트 레퍼런스](#엔드포인트-레퍼런스)
   - [GET /scene/catalog](#get-scenecatalog)
   - [GET /scene/slots](#get-sceneslots)
   - [GET /scene/cameras](#get-scenecameras)
   - [POST /scene/project](#post-sceneproject)
   - [GET /scene/cars · POST /scene/cars](#get-scenecars--post-scenecars)
   - [GET·PATCH·DELETE /scene/cars/:id](#getpatchdelete-scenecarsid)
   - [POST /scene/reset](#post-scenereset)
5. [데이터 모델과 카탈로그 캐논](#데이터-모델과-카탈로그-캐논)
6. [에러 규약](#에러-규약)
7. [카메라 파라미터와 3D→2D 오버레이 투영](#카메라-파라미터와-3d2d-오버레이-투영)
8. [사용 예시](#사용-예시)
9. [버전·소스 위치](#버전소스-위치)

## 개요와 설계 원칙

- **씬 상태의 진실의 출처는 UE sim(런타임)이다.** 이 API는 상태를 저장하지 않고 살아있는 월드를 조회·조작한다.
  차량 스폰/삭제는 실제 액터 스폰/파괴이고, 슬롯 점유는 서브시스템이 추적한다.
- **단일 제어면**: 웹UI(`/simulator`)·CLI(`sim.mjs`)·AI 에이전트가 전부 같은 엔드포인트를 호출한다.
  사람이 화면에서 보는 것과 자동화가 조작하는 것이 항상 같은 대상이다(재현 가능성 원칙).
- **UE 없이도 개발 가능**: baro_calory 쪽 `FakeSceneClient`(인메모리)가 동일 메서드 표면을 구현한다.
  계약의 JS 쪽 진실의 출처는 `packages/cctv-client/src/scene-control-client.mjs`.
- **카메라 정보는 실기 방식으로 분해**: sim은 실카메라가 알 수 없는 것(설치 위치·방위 = 외부 파라미터)만 주고,
  실카메라도 아는 것(PTZ·FOV)은 Hucoms CGI에서 가져온다. 상세는
  [카메라 파라미터 절](#카메라-파라미터와-3d2d-오버레이-투영) 참조.

## 아키텍처 (소비 스택)

```text
웹UI /simulator        CLI (pnpm sim:*)        AI 에이전트/MCP
      |                      |                        |
      +----------- baro_calory 백엔드 (:8080) --------+
      |   /api/simulator/*  (simulator-api.mjs — 범위 검증, 400)
      |   SceneControlClient (scene-control-client.mjs — JSON 프록시)
      |        (config.simulator.host:port 미설정 시 FakeSceneClient 폴백)
      v
UE sim  USceneControlSubsystem  (:8095, /scene/*)
      |   FHttpServerModule 라우터, 월드 전역 1포트
      v
살아있는 월드: BP_ParkingSlot 액터(슬롯), BP_Car 스폰(차량 — Change_Car/Color/Plate/Text
ProcessEvent 호출), APTZCamera(카메라 파라미터)
```

## 서버 기동과 포트 설정

- **수명**: Game/PIE 월드의 `BeginPlay`에 리스너 시작, 월드 종료(`Deinitialize`)에 정지 + 스폰 차량 정리.
  에디터를 열어만 둔 상태(월드 미실행)에서는 응답하지 않는다. standalone `-game`에서 정상 동작.
- **포트 기본값 8095** — `UCLASS(config=Game)`라 `Config/DefaultGame.ini`로 변경(리빌드 불필요, 재시작 필요):

```ini
[/Script/baroCCTVSimulator.SceneControlSubsystem]
; 씬 제어 HTTP 포트 (기본 8095). baro_calory config.json 의 simulator.port 와 일치시킬 것.
ScenePort=8095
; 스폰할 차량 블루프린트 generated class 경로.
CarBlueprintPath=/Game/BP/BP_Car.BP_Car_C
; 주차면 액터 클래스명 접두(BP_ParkingSlot_5m/6m/BUS/... 공통).
ParkingSlotClassPrefix=BP_ParkingSlot
```

- 런치별 오버라이드(패키징 빌드 포함, ini 수정 없이):

```text
-ini:Game:[/Script/baroCCTVSimulator.SceneControlSubsystem]:ScenePort=21030
```

- 주석은 반드시 별도 줄(`;` 시작)에 쓴다 — 값 뒤 인라인 주석은 UE ini 규약이 아니다(값으로 읽힘).

## 엔드포인트 레퍼런스

공통: 요청/응답 본문은 `application/json`(UTF-8). 빈 본문 POST는 `{}`로 취급. 실패 시 `{"error":"메시지"}`.

### GET /scene/catalog

정적 카탈로그 + 레벨/플러그인 버전. 웹UI가 셀렉트 박스를 채우고 버전을 표시하는 데 쓴다.

```json
{
  "level": "LV_Park_sim_01",
  "pluginVersion": "0.1.6",
  "carCount": 23,
  "cars": [ { "index": 0, "name": "BMW 1시리즈", "asset": "BMW_1시리즈" }, ... carCount 개 ],
  "colors": [ { "index": 0, "name": "화이트", "rgb": [0.94, 0.94, 0.90] }, ... 8종 ],
  "plateTypes": [ { "index": 0, "name": "일반" }, { "index": 1, "name": "영업용" }, { "index": 2, "name": "전기차" } ],
  "korList": ["가", "나", "다", "라", "마"]
}
```

- `pluginVersion`은 `.uplugin`의 `VersionName`을 `IPluginManager`로 런타임 조회한 값 — 배포된 빌드가
  어느 플러그인인지 즉시 식별(웹 `/simulator` 씬 카드에 표시됨).
- `cars[]`는 **v0.1.2부터** `BP_Car.Mesh_List` CDO 리플렉션으로 채워진다. `carCount`는 그 길이이며
  구 계약을 그대로 유지한다(리플렉션 실패 시에만 과거 상수 23으로 폴백하고 `cars[]`는 비어 나간다).
  차종을 하드코딩하지 말고 이 배열을 쓸 것 — 에셋이 늘면 자동으로 따라온다.

### GET /scene/slots

레벨에 배치된 주차면(`BP_ParkingSlot*` 액터) 목록. 점유 상태 포함.

```json
{
  "slots": [
    {
      "id": "BP_ParkingSlot_C_0",
      "label": "BP_ParkingSlot1",
      "type": "slot",
      "transform": {
        "location": { "x": -2313.99, "y": 1449.99, "z": 0.03 },
        "rotation": { "pitch": 0, "yaw": 90, "roll": 0 }
      },
      "occupied": false,
      "carId": null
    }
  ]
}
```

- `id` = 배치 액터의 오브젝트명(쿠킹 후에도 안정). 에디터 라벨이 아니다.
- `label` = 에디터 Outliner의 Actor Label. 에디터 빌드에서는 `GetActorLabel()` 값이고, 런타임/패키지 빌드에서는 `id`로 폴백한다. UI 표시·사람용 정렬에 사용한다.
- `type` = 클래스명에서 접두(`BP_ParkingSlot`)를 뗀 나머지(`5m`/`6m`/`BUS`/...; 무접미 변형은 `slot`).
- `transform.location`은 UE 월드 좌표(cm) — 오버레이 투영의 입력점이 된다.

### GET /scene/cameras

레벨의 `APTZCamera`들의 **설치 외부 파라미터**(오버레이 투영용). PTZ·FOV는 여기 없다 — 실카메라와
동일하게 Hucoms CGI(`getptzfpos`)에서 가져온다(설계 이유는 아래 투영 절).

```json
{
  "cameras": [
    {
      "id": "PTZCamera_4",
      "hucomsPort": 8081,
      "mjpegPort": 8091,
      "fixed": false,
      "mount": {
        "location": { "x": -1344.19, "y": 1237.93, "z": 585.0 },
        "baseYaw": 0.0
      },
      "wideHFovDeg": 69.88
    }
  ]
}
```

- `hucomsPort`/`mjpegPort` = 이 카메라 채널이 **실제로 바인딩한** 실효 포트
  (`UHucomsServerSubsystem::GetCameraPorts` — 자동부여 규칙 재계산이 아니라 채널값 조회).
  baro_calory `devices[].port`와 조인 키로 쓴다.
- `mount.location` = 광학중심 월드 위치(cm). PTZ 피벗들이 루트와 동일 위치(레버암 0)라 pan/tilt에 불변.
- `mount.baseYaw` = 설치 방위(pan=0일 때의 월드 yaw). 광학 yaw = `baseYaw + pan`.
- `wideHFovDeg` = 1x(zoompos 0) 수평 FOV. zoom→FOV 곡선의 스케일 기준.
- `fixed` = `bFixedMode`(고정형 카메라 — PTZ 명령 무시, 스트림은 정상).

### POST /scene/project

월드 점들을 지정 카메라의 화면 픽셀로 투영한 **그라운드-트루스**. UE가 실제 렌더에 쓰는
뷰·투영행렬(`FMinimalViewInfo` + `GetViewProjectionMatrix`)로 계산하므로, 클라이언트측 투영
구현(JS `projectWorldToPixel`)의 정합 검증 오라클로 쓴다. 운영 경로가 아니라 검증 도구다.

요청:

```json
{
  "cameraId": "PTZCamera_4",          // 또는 "hucomsPort": 8081 (둘 다 없으면 첫 카메라)
  "points": [ { "x": 0, "y": 0, "z": 0 }, ... ],
  "resolution": { "width": 1920, "height": 1080 }   // 생략 시 1920x1080
}
```

응답 (`points`는 요청 순서 1:1):

```json
{
  "cameraId": "PTZCamera_4",
  "fovDeg": 69.88,
  "resolution": { "width": 1920, "height": 1080 },
  "points": [
    { "x": 615.0, "y": 386.9, "visible": true,  "behind": false },
    { "x": -653.4, "y": 458.5, "visible": false, "behind": false }
  ]
}
```

- `visible` = 프레임 사각형 안 + 카메라 앞. `behind` = 상 평면 뒤(그때 x/y는 무의미).
- 프레임 밖 점도 좌표를 그대로 반환(클램프하지 않음) — 잘라낼지는 호출자가 결정.

### GET /scene/cars · POST /scene/cars

GET = 현재 배치된 차량 목록:

```json
{ "cars": [ { "id": "car-01", "slotId": "BP_ParkingSlot_C_15", "transform": { ... },
              "carType": 3, "color": 4,
              "plate": { "type": 0, "city": "서울", "prefix": "123", "kor": "가", "number": "4567" } } ] }
```

POST = 스폰. `slotId`(슬롯 배치, 트랜스폼은 슬롯 것) 또는 `transform`(자유 좌표) 중 하나 필수:

```json
{
  "slotId": "BP_ParkingSlot_C_15",
  "carType": 3,
  "color": 4,
  "plate": { "type": 0, "city": "서울", "prefix": "123", "kor": "가", "number": "4567" },
  "force": false
}
```

- 점유된 슬롯이면 `409`(`force: true`로 덮어쓰기 허용). 없는 슬롯이면 `404`.
- 응답 = `{ "car": { ...스폰된 차량 상태... } }`. `id`는 `car-01`, `car-02`... 순번.
- 값 범위는 서버가 클램프하지만, baro_calory 라우터가 프록시 전에 `400`으로 먼저 거른다
  (carType 0..carCount-1, color 0..7, plate.type 0..2).
- **번호판 정규화**: 한국 신형(앞 3자리 + 한글 1자 + 뒤 4자리, 예 `123가4567`)으로 자릿수를 서버가
  정규화한다(`NormalizeKoreanPlate` — BP_Plate의 파싱이 이 자릿수를 전제). 저장값 = 렌더값.
  `city`는 별도 TextRender라 임의 문자열.

### GET·PATCH·DELETE /scene/cars/:id

- GET → `{ "car": { ... } }`. 없으면 `404`.
- PATCH = 부분 갱신(넘긴 필드만). `plate`는 필드 단위 병합. `slotId` 변경 = 슬롯 이동(이전 슬롯 해제,
  새 슬롯 점유·트랜스폼 적용; 점유 시 `409`, `force`로 무시). `"slotId": ""` = 슬롯에서 분리.
- DELETE → 액터 파괴 + 슬롯 해제, `{ "removed": "car-01" }`.

### POST /scene/reset

스폰된 차량 전부 삭제(시나리오 초기화). 레벨에 저작된 액터는 건드리지 않는다.

```json
{ "cleared": 2 }
```

## 데이터 모델과 카탈로그 캐논

BP_Car / BP_Plate 실측(2026-07-07)에서 도출한 캐논. JS 쪽 `SIM_CATALOG`(scene-control-client.mjs)와
UE 쪽 상수(SceneControlSubsystem.cpp)가 동일해야 한다 — 한쪽을 바꾸면 다른 쪽도 맞출 것.

| 필드 | 범위 | 의미 |
|---|---|---|
| `carType` | 0..carCount-1 | `/scene/catalog` 의 `cars[]` 인덱스 (v0.1.2부터 동적, 현재 23종) |
| `color` | 0..7 (8색) | BP_Car `selected_Color` (화이트/블랙/실버/그레이/레드/옐로/그린/블루) |
| `plate.type` | 0..2 | BP_Plate 메시 (일반/영업용/전기차) |
| `plate.prefix` | 숫자 3자리 | 번호판 앞자리 (정규화됨) |
| `plate.kor` | 한글 1자 | korList는 팔레트일 뿐 — TextRender라 임의 글자 렌더 가능 |
| `plate.number` | 숫자 4자리 | 번호판 뒷자리 (정규화됨) |
| `plate.city` | 임의 문자열 | 지역 표기(선택), 별도 TextRender |

트랜스폼 규약: UE 좌표계(왼손, +Z up, 단위 cm, 각도 deg).
`{ "location": {x,y,z}, "rotation": {pitch,yaw,roll} }`.

## 에러 규약

sim의 HTTP status를 baro_calory가 코드로 매핑해 그대로 되돌린다(계층 간 의미 보존):

| HTTP | 의미 | baro_calory 코드 |
|---|---|---|
| 400 | 잘못된 입력(JSON 파싱 실패, slotId/transform 둘 다 없음 등) | `BAD_INPUT` |
| 404 | 슬롯/차량/카메라 없음 | `NOT_FOUND` |
| 409 | 슬롯 점유됨(`force`로 무시 가능) | `OCCUPIED` |
| 500 | 차량 BP 로드/스폰 실패 | — |
| (그 외) | 연결 불가/타임아웃 등은 baro_calory가 502로 | — |

## 카메라 파라미터와 3D→2D 오버레이 투영

이 API의 핵심 설계 하나: **카메라 정보를 실기(실제 CCTV)와 같은 방식으로 분해**한다.

| 정보 | 실카메라에서의 출처 | sim에서의 출처 |
|---|---|---|
| PTZ (panpos/tiltpos/zoompos) | Hucoms `getptzfpos` | 동일 (sim의 Hucoms 서버, 카메라별 포트) |
| FOV (내부 파라미터) | zoompos → 화각 캘리브레이션 표 | 동일 (`hfovFromZoomPos`) |
| 설치 위치·방위 (외부 파라미터) | 측량/캘리브레이션 값 | **`/scene/cameras`의 `mount`** |

즉 `/scene/cameras`는 실카메라가 스스로 알 수 없는 것(설치 외부 파라미터)만 준다. 나머지는 실기와
똑같은 경로(Hucoms)로 읽으므로, **실기 이식 시 mount만 측량값으로 채우면 오버레이 코드가 동일**하다.

클라이언트(웹) 재구성 공식 — `packages/web-ui/src/camera-projection.mjs`의 `ptzCamera()`:

```text
pitch = -tiltpos / 100          (tiltpos 증가 = 아래를 봄, field-validated)
yaw   = baseYaw + panpos / 100
roll  = 0
hfov  = hfovFromZoomPos(zoompos, wideHFovDeg)
```

이 합성이 맞는 근거(플러그인 내부 구조): `APTZCamera`는 PanPivot(yaw-only 월드 회전) →
TiltPivot(pitch) 계층으로 짐벌 커플링을 제거하고, 피벗들이 루트와 동일 위치라 **레버암 0** —
pan/tilt를 움직여도 광학중심(`mount.location`)이 이동하지 않는다.

투영은 **tan 원근(핀홀)** 을 쓴다 — 렌더가 표준 원근이기 때문. Hucoms `setcenter` 재현에 쓰는
선형(linear) 각도-픽셀 모델과 **섞으면 안 된다**(가장자리로 갈수록 마커가 밀림; 두 모델은 용도가 다름).

정합 검증: 위 재구성 + tan 핀홀(JS)을 `/scene/project` 오라클과 비교해 pan/tilt/zoom 전역에서
**최대 오차 0.03px** (2026-07-07, 카메라 2대 × 슬롯 24면 × 이동 상태 포함).

## 사용 예시

sim에 직접 (기본 포트 8095):

```bash
# 카탈로그(레벨·플러그인 버전 포함)
curl http://127.0.0.1:8095/scene/catalog

# 주차면 목록
curl http://127.0.0.1:8095/scene/slots

# 슬롯에 차량 스폰 (차종 3, 레드, 번호판 123가4567)
curl -X POST http://127.0.0.1:8095/scene/cars \
  -H "content-type: application/json" \
  -d '{"slotId":"BP_ParkingSlot_C_15","carType":3,"color":4,"plate":{"type":0,"prefix":"123","kor":"가","number":"4567"}}'

# 번호판만 교체
curl -X PATCH http://127.0.0.1:8095/scene/cars/car-01 \
  -H "content-type: application/json" -d '{"plate":{"number":"7777"}}'

# 투영 오라클 (월드 원점이 PTZCamera_4 화면 어디에 찍히나)
curl -X POST http://127.0.0.1:8095/scene/project \
  -H "content-type: application/json" \
  -d '{"cameraId":"PTZCamera_4","points":[{"x":0,"y":0,"z":0}]}'

# 전체 초기화
curl -X POST http://127.0.0.1:8095/scene/reset
```

baro_calory 경유(동일 계약, `/scene/*` → `/api/simulator/*`):

```bash
curl http://127.0.0.1:8080/api/simulator/slots
pnpm sim:catalog && pnpm sim:slots && pnpm sim:cars     # CLI 하네스
```

## 버전·소스 위치

- 버전 규칙: 플러그인 `.uplugin` `VersionName` = **0.1.0 시작, 수정 시 끝자리 +1**. 현재 문서 기준은 **0.1.6**.
  런타임 확인 = `/scene/catalog.pluginVersion`, sim HUD 제목줄, 웹 `/simulator` 씬 카드.
- UE 구현: `Plugins/baroCCTVSimulator/Source/baroCCTVSimulator/{Public,Private}/SceneControlSubsystem.{h,cpp}`
  (포트 조회는 `HucomsServerSubsystem::GetCameraPorts`).
- JS 계약(진실의 출처): `baro_calory/packages/cctv-client/src/scene-control-client.mjs`
  (`SceneControlClient` 실 프록시 + `FakeSceneClient` 인메모리 — 동일 표면).
- 라우터(검증층): `baro_calory/apps/backend-core/src/simulator-api.mjs`.
- 오버레이 투영: `baro_calory/packages/web-ui/src/{camera-projection,camera-intrinsics}.mjs`.
- 주의: `USceneControlSubsystem`은 `UWorldSubsystem`이라 **Live Coding 핫리로드가 안 된다** —
  수정 시 에디터를 닫고 풀 리빌드(`Build.bat baro_unrealEditor ...`).
