# baro_unreal

주차장 CCTV 시뮬레이터 — Unreal Engine 5.8 / C++. baro_calory(Node.js 관제 시스템)의
Hucoms PTZ 카메라를 가상으로 재현해, 실기 없이 온보딩·주차면 발견·번호판 호밍을 개발/검증한다.

## 목차

- [개요](#개요)
- [기술 스택](#기술-스택)
- [저장소 전략 (code-only)](#저장소-전략-code-only)
- [클론 후 셋업 — 에셋·플러그인 재취득](#클론-후-셋업--에셋플러그인-재취득)
- [빌드 / 실행](#빌드--실행)
- [관련 문서](#관련-문서)

## 개요

- 실제 Hucoms 카메라 프로토콜(CGI setcenter / MJPEG / PTZ status)을 UE 내에서 서버로 재현.
- `APTZCamera`(팬/틸트/줌 짐벌) + `PTZCaptureComponent`(SceneCapture→JPEG) + `HucomsServerSubsystem`(HTTP/MJPEG 서버).
- baro_calory 웹 UI가 이 시뮬레이터를 실기와 동일한 API로 제어(포트 8081~8084).

## 기술 스택

- Unreal Engine **5.8**, C++ (`Source/baro_unreal/`)
- 모듈 의존: HTTP / Json / HTTPServer / Sockets / Networking (`baro_unreal.Build.cs`)
- 상용 에셋: BlackAlder, Fab, CityKitBR, Road Creator Pro, UltraDynamicSky, RYU Korean Building Creator 등

## 저장소 전략 (code-only)

이 저장소는 **소스코드 · Config · 문서만** 버전관리한다. 이유:

- `Content/` 는 **약 30GB** 로 대부분 **상용 마켓플레이스/외부 임포트 에셋**이다 → 재배포 불가(라이선스) + git·GitHub 부적합(파일 100MB 하드리밋, 무료 LFS 1GB).
- `Plugins/RYUKoreaBuilidngCreator` 는 상용 플러그인(2.6GB) 이다.

따라서 `.gitignore` 로 `Content/`, `Plugins/`, 그리고 모든 빌드 산출물/캐시(`Binaries` `Intermediate` `Saved` `DerivedDataCache` `.vs` 등)를 제외한다.
에셋까지 버전관리가 필요해지면 **Perforce** 또는 **자체호스팅 Git LFS** 가 UE 표준 경로다.

## 클론 후 셋업 — 에셋·플러그인 재취득

> ⚠️ 신규 클론에는 `Content/`·`Plugins/` 가 **없다**. 복구 전에는 에디터가 정상적으로 열리지 않는다.

1. **에셋 복구**: `Content/` 를 백업(또는 원본 마켓플레이스/외부 프로젝트)에서 복사해 넣는다.
   상용 에셋(BlackAlder, CityKit, Road Creator Pro, UltraDynamicSky 등)은 Fab/Epic 계정으로 재취득.
2. **플러그인 복구**: `Plugins/RYUKoreaBuilidngCreator/` 를 배치하고, 5.8 미대응 시 소스에서 재컴파일.
3. C++ 빌드(아래) → `.uproject` 실행.

## 빌드 / 실행

- 프로젝트 파일 생성: `.uproject` 우클릭 → *Generate Visual Studio project files* (`.sln`/`.slnx` 는 생성물이라 git 제외).
- 빌드: 생성된 솔루션 또는 `Build.bat baro_unrealEditor Win64 Development`.
- 실행: 에디터(PIE) 또는 standalone `-game`. CCTV 서버 포트/톤/선명도 등 확정값은 코드 기본값에 베이크됨(`HucomsServerSubsystem.h`, `PTZCaptureComponent.cpp`).

## 관련 문서

- 개발 이력·설계 결정·교훈: [`_forAI/`](_forAI/) (`dev_log.md`, `plan.md`, `inventory.md`, `memo.md`).
- PTZ 좌표·부호 규약(진실의 출처)·SceneCapture 화질 교훈은 `_forAI/dev_log.md` 참조.
