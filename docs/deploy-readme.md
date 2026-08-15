# baro_unreal CCTV 시뮬레이터 — 배포본 안내 (에이전트용)

> 플러그인 baroCCTVSimulator v0.1.16 기준 · Windows/Win64 · 헤드리스 (정확한 버전은 `/scene/catalog`)
>
> 이 파일은 배포 zip 루트에 실린다. **에이전트가 이 시뮬을 100% 활용해 작업을 스스로 계획·실행**하도록
> 전체 기능을 한 장으로 조망한다. 필드 단위의 최신 계약은 런타임 자기서술 API **`GET :8095/scene/help`**
> 가 항상 최신으로 준다 — 이 README 는 그 위의 지도이고, **먼저 `/scene/help` 를 읽는 것이 부트스트랩이다.**
> (씬 포트 기본 8095 — 인스턴스를 `-ScenePort=N` 으로 띄웠다면 N. 아래 「포트 맵」의 기동 계약 참조.)

## 목차

- [0. 30초 부트스트랩](#0-30초-부트스트랩)
- [1. 이게 무엇인가](#1-이게-무엇인가)
- [2. 능력 지도 — 두 축](#2-능력-지도--두-축)
- [3. 포트 맵](#3-포트-맵)
- [4. 좌표·단위 규약](#4-좌표단위-규약-라벨-만들-때-필수)
- [5. 카메라 자원 생명주기 — 동작 계약](#5-카메라-자원-생명주기--동작-계약-꼭-지킬-것)
- [6. 에이전트가 자율로 하는 작업](#6-에이전트가-자율로-하는-작업-100-활용-예시)
- [7. 빠른 시작 (curl)](#7-빠른-시작-curl)
- [8. 런타임·배포 메모](#8-런타임배포-메모)

## 0. 30초 부트스트랩

```bash
BOX=<배포기 IP>
curl http://$BOX:8095/scene/help      # 전체 read+write 계약(자기서술, 항상 최신) — 여기서 시작
curl http://$BOX:8095/scene/catalog   # 차종·색·번호판 어휘 + 버전
curl http://$BOX:8095/scene/cameras   # 카메라 목록 + 각자의 hucomsPort·mjpegPort·화각표
```

`/scene/help` 하나로 전 엔드포인트·규약·라이브 상태를 **소스 없이** 파악할 수 있다. 이 README 는 큰 그림,
`/scene/help` 는 필드 단위 계약이다. 둘이 어긋나면 `/scene/help`(파일에서 요청 시점에 읽음)가 진실이다.

## 1. 이게 무엇인가

헤드리스 언리얼엔진 5.8 주차장 CCTV 시뮬레이터. 실기(Hucoms) CCTV 와 표면 호환되는 카메라 서버와
런타임 씬 제어 API 를 노출한다. 스폰 API 가 **무한 정답 라벨**(3D 박스·2D 투영·가림 정답)을 주므로,
합성 학습 데이터를 무제한 생성할 수 있다. pm2 로 상주 실행한다.

> standalone 메인 창은 **검게** 보이는 게 정상이다(헤드리스 카메라 서버). 영상은 창이 아니라 카메라
> API(`jpeg.cgi`·MJPEG)로 나온다.

## 2. 능력 지도 — 두 축

에이전트가 다루는 표면은 두 개다.

### A. 씬 API — 월드를 읽고 쓴다 (`:8095/scene/*`, 자기서술)

| 목적 | 엔드포인트 |
|---|---|
| 자기서술 도움말(전 계약·라이브 상태) | `GET /scene/help` (= `GET /scene`) |
| 어휘(차종·색·번호판) + 버전 | `GET /scene/catalog` |
| 주차면 목록 | `GET /scene/slots` |
| 카메라 목록·파라미터·화각표 | `GET /scene/cameras` |
| 카메라 **런타임 스폰**(v0.1.13, 레벨·ini 무수정) | `POST /scene/cameras` · `PATCH`·`DELETE /scene/cameras/:id` |
| 차량 스폰(**비틀어 배치 = offset**) | `POST /scene/cars` |
| 차량 조회·편집·삭제 | `GET`·`PATCH`·`DELETE /scene/cars/:id` |
| 3D→2D 투영 오라클(라벨 검증) | `POST /scene/project` |
| 가시성·가림 정답(GT) | `GET /scene/cars?visibility=<cameraId 또는 hucomsPort>` |
| 스냅샷(라벨용 정지 프레임) | `GET`·`POST /scene/snapshot` |
| 전부 정리 | `POST /scene/reset` |

**읽기(상태 조회) + 쓰기(스폰·편집·리셋)가 한 API 다.** 에이전트가 씬을 자유롭게 구성한다.
차량 배치는 **기준(Base: 주차면 또는 자유 transform) + 로컬 변형(offset)** 으로, 최종 = `Offset * Base`.
`offset` 으로 자리는 주차면에 붙인 채 위치·회전을 흔들어 삐뚤·라인 물기·후진 포즈를 만든다(값이지 누적 아님).

### B. 카메라 제어·영상 — Hucoms CGI (카메라별 포트)

각 카메라의 실효 포트는 `/scene/cameras` 의 `hucomsPort`(CGI)·`mjpegPort`(스트림)로 조인한다.

| 목적 | 호출 |
|---|---|
| 현재 PTZ 조회 | `GET :{hucomsPort}/cgi-bin/control/ptzf_status.cgi?action=getptzfpos` |
| 절대 이동(팬·틸트·줌) | `...ptzf_status.cgi?action=goptzfpos&panpos=&tiltpos=&zoompos=` |
| 클릭 센터링(1920×1080 프레임) | `...ptz_centering.cgi?action=setcenter&...` |
| 스냅샷 1장 | `GET :{hucomsPort}/cgi-bin/image/jpeg.cgi` |
| 연속 영상(MJPEG) | `:{mjpegPort}` 스트림 |
| PTZ 능력 광고 | `...capabilityptz.cgi?action=getPTZ` |

## 3. 포트 맵

- **씬 제어: 기본 `8095`** (`/scene/*`) — 기동 시 `-ScenePort=N` 으로 바꿀 수 있다(아래 기동 계약).
- **카메라는 0 대로 시작한다** — 그래서 열려 있는 카메라 포트도 없다. `POST /scene/cameras` 로
  포트를 명시해 스폰하면 그때 그 포트의 CGI·MJPEG 가 살아난다. `DELETE` 하면 응답은 중단되지만
  **CGI HTTP 포트의 리스너는 프로세스 종료까지 리슨 상태로 남는다**(UE 엔진 제약) — 같은 인스턴스는
  그 포트를 재사용할 수 있지만 **다른 인스턴스·프로세스는 쓸 수 없다.**
- 자동 부여를 쓰면 **카메라 CGI 는 `8081`부터, 연속 MJPEG 는 `8091`부터** 순서대로 나간다.
- 포트를 하드코딩하지 말고 항상 `/scene/cameras` 로 실제 포트를 조인하라.

### 인스턴스 기동 계약 — 운용 에이전트용 (v0.1.16~, 다중 실행)

시뮬 인스턴스의 기동·포트 부여·헬스체크·재시작은 **운용 에이전트가 관리**한다. 자동 포트 탐색은 없다.

- **포트 결정 순서**: 커맨드라인 `-ScenePort=8096`(카메라 자동부여 시작값도 `-BaseHttpPort=` /
  `-BaseMjpegPort=`) → 없으면 ini 기본값(8095/8081/8091). Shipping 포함 전 빌드 구성에서 동작.
- **충돌 = 즉시 종료**: 지정한 `ScenePort` 가 이미 점유돼 있으면 인스턴스는 에러 로그를 남기고 스스로
  종료한다. 따라서 **프로세스가 살아 있으면 포트는 보장된다** — 엉뚱한 포트·localhost 반쪽 기동 같은
  제3의 상태는 없다.
- **운용 사이클**: 기동 `baro_unreal.exe -ScenePort=N` → 준비 확인 `GET :N/scene/catalog` 200 →
  실패/종료 감지 = 프로세스 exit.
- **다중 인스턴스 규칙**: 인스턴스마다 `ScenePort` 와 **카메라 포트 블록을 전부 분리**할 것(위 DELETE
  리스너 존속 제약 때문에 블록 간 재사용 불가). 로그·설정 분리가 필요하면 인스턴스별 `-UserDir=<경로>`.
- **자원 계획**(실측): 인스턴스당 유휴 RAM 4.0GB / VRAM 2.2GB — 8GB GPU 기기는 **2개가 실용 한도**.

## 4. 좌표·단위 규약 (라벨 만들 때 필수)

- world = **UE cm · 왼손**, 지면 z ≈ 10 cm
- `panpos`·`tiltpos` = 1/100°, **`tiltpos` 증가 = 아래를 봄**, `panpos` 증가 = 우측
- `zoompos` = 불투명 눈금 → 반드시 `/scene/cameras` 의 화각표(`zoomHfov`)로만 해석
- 핀홀 광학(왜곡 0, principal point = 프레임 중앙): **focal(px) = 0.5 × width / tan(hfov/2)**
- 센터링 픽셀 프레임은 항상 논리 **1920 × 1080**(스냅샷 실제 해상도와 무관)

## 5. 카메라 자원 생명주기 — 동작 계약 (꼭 지킬 것)

**수요 기반이다(v0.1.9~). 쓰는 카메라만 켜진다.**

- 캡처(`jpeg.cgi`·MJPEG)·PTZ 이동 = 수요. **상태 폴링(`getptzfpos`·`capabilityptz`)은 수요가 아니다.**
- **안 쓰는 카메라를 상시 폴링하지 마라** — 여러 대를 keep-alive 로 계속 폴링하면 전부 warm 으로 붙잡혀
  오히려 무거워진다. 실제로 쓰는 카메라만 열어라.
- 유휴 10초 후 해제 → 다시 켠 첫 캡처는 콜드(약 3초), warm 재요청은 약 0.3초.
- 동시에 여러 대를 warm 으로 붙잡으려면 서버 config `MaxActiveCameras` 를 올린다(GPU 한계 내에서).

## 6. 에이전트가 자율로 하는 작업 (100% 활용 예시)

- **라벨된 3D/2D 데이터셋 생성**: `POST /scene/reset` → 차량을 다양한 포즈로 스폰(`offset` 으로 비틀기·
  라인 물기·후진) → `POST /scene/project` 로 2D 박스 정답 + `?visibility=` 로 가림 정답 → 임의 카메라·PTZ
  포즈에서 `snapshot`/`jpeg.cgi` → 반복. 정답 라벨이 무한이라 사람 라벨링이 필요 없다.
- **카메라 기하 다양화**: `POST /scene/cameras` 로 새 높이·각도 카메라를 런타임 스폰하거나, 한 대에서
  PTZ 틸트·줌을 스윕해 여러 화각을 생성(BEV/원근 일반화).
- **시나리오 구성·재현**: `catalog` 어휘로 차종·색·번호판을 조합, `slots` 로 배치, `reset` 으로 초기화.
- 모든 계약의 진실 소스는 `/scene/help`(항상 최신). 계획을 세울 땐 이 README 로 능력을 파악하고,
  실행 직전 `/scene/help` 로 정확한 필드·제약을 확인하라.

## 7. 빠른 시작 (curl)

```bash
BOX=<배포기 IP>; P=http://$BOX:8095
curl $P/scene/help                       # 전체 계약(먼저 읽기)
curl $P/scene/slots                      # 주차면 id 목록
# 주차면에 스폰 — 우측 12cm·뒤 16cm 비껴, 7도 틀어서
curl -X POST $P/scene/cars -H "content-type: application/json" \
  -d '{"slotId":"<slots 의 id>","carType":3,"color":4,
       "offset":{"location":{"x":-16,"y":12},"rotation":{"yaw":7}}}'
# 카메라는 0 대로 시작하니 먼저 하나 세운다(포트는 명시 필수)
curl -X POST $P/scene/cameras -H "content-type: application/json" \
  -d '{"location":{"x":73,"y":-2015,"z":1000},"yawDeg":90,"pitchDeg":-30,
       "httpPort":8287,"mjpegPort":8297}'
curl "$P/scene/cars?visibility=8287"     # 그 카메라 기준 차량별 가림 정답
curl -o snap.jpg "http://$BOX:8287/cgi-bin/image/jpeg.cgi"   # 그 카메라 실렌더 스냅샷
curl -X DELETE "$P/scene/cameras/8287"   # 카메라 제거(응답 중단 — 단 CGI 리스너는 프로세스 종료까지 남는다)
curl -X POST $P/scene/reset              # 전부 정리
```

투영 오라클·스냅샷·카메라 스폰의 정확한 요청 필드는 `GET $P/scene/help` 를 참조하라(여기 예시는 진입용).

## 8. 런타임·배포 메모

- 실행: pm2 앱 `baro_unreal`. 상태 `pm2 jlist`, 재시작 `pm2 restart baro_unreal`.
  다중 인스턴스는 pm2 앱 이름과 `-ScenePort`(필요시 `-UserDir`)를 인스턴스별로 달리 해서 등록한다.
- 이 빌드는 **Development** 라 로그/콘솔이 열려 있다(오류·안정성 분석용).
- 이 README 와 `/scene/help` 원문(`baro_unreal/Plugins/baroCCTVSimulator/docs/scene-help.md`)은 pak 밖
  **루즈 파일**이라, 배포기에서 직접 고치면 리빌드 없이 즉시 반영된다.
- **포트가 원격 에이전트에서 닿아야** 쓸 수 있다 — 배포기 방화벽·바인딩을 확인하라.
- 좌표계·포트·단위 규약을 바꾸면 소비 에이전트가 전부 깨진다. 변경 시 반드시 공지할 것.
