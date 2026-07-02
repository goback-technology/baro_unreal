# Memo

## 목차

- [제품 기준선](#제품-기준선)
- [기본 설정값](#기본-설정값)
- [런타임 구조 메모](#런타임-구조-메모)
- [동작 규칙](#동작-규칙)
- [반복 금지](#반복-금지)

## 제품 기준선

- 엔진: Unreal Engine **5.8** (Windows 11 / PowerShell)
- 프로젝트: `baro_unreal` — C++ 게임 모듈(에디터 타깃 `baro_unrealEditor`)
- 통합 출처: 레벨=`parking_area`(Parking_Project), CCTV 시뮬=`baro_world 5.8`
- TODO: 최종 타깃 런타임(standalone/PIE)·배포 형태 확정

## 기본 설정값

- MCP 서버(에디터 내장): `http://127.0.0.1:8000/mcp`, `bAutoStartServer=True`
- (이식 예정) baro_world 5.8 포트: HTTP CGI **8081**, 연속 MJPEG **8082**, RTSP 브리지 554
- TODO: baro_unreal에서 실제 사용할 포트/FOV/모터 기본값 확정

## 런타임 구조 메모

- (이식 예정) `UHucomsServerSubsystem`(UTickableWorldSubsystem): `OnWorldBeginPlay`에서 HTTP 라우터 기동,
  Tick에서 PTZ 모터 슬루 시뮬 + 카메라 미러 + MJPEG 프레임 캡처. Game/PIE 월드에서만.
- TODO: 이식 후 초기화 순서/스레드(‌MJPEG 워커 스레드) 구조 확정.

## 동작 규칙

- TODO: setcenter LINEAR 픽셀→각도 모델, getptzfpos 라운드트립 등 baro_world 계약 유지 여부.

## 반복 금지

- **UE 버전업/신규 프로젝트 "컴파일 에러"는 대개 코드가 아니라 빌드환경(Target.cs V6→V7) 불일치** — Shared 환경 확인.
- MCP는 프로젝트별 옵트인(플러그인+AutoStart), 클라이언트 등록은 전역 1회. (상세: `ready_unreal` readme 부록 A/B)
- TODO: 이식 중 겪은 실제 함정 누적.
