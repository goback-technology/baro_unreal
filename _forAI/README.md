# _forAI Guide

## 목차

- [한 줄 요약](#한-줄-요약)
- [읽는 순서](#읽는-순서)
- [문서 역할](#문서-역할)
- [프로젝트 문서(팀 공유, docs/)](#프로젝트-문서팀-공유-docs)
- [외부 소비자와 협업 컨텍스트](#외부-소비자와-협업-컨텍스트)
- [현재 스냅샷](#현재-스냅샷)
- [유지 규칙](#유지-규칙)

## 한 줄 요약

이 디렉터리는 `baro_unreal` 작업을 이어받을 때 필요한 AI 작업 문맥을 정리해 두는 곳이다.

## 읽는 순서

1. `README.md`
2. `inventory.md`
3. `memo.md`
4. `dev_log.md`
5. `plan.md`

## 문서 역할

- `inventory.md`: 저장소에 실제로 있는 구조, 엔트리포인트, 빌드/검증 명령을 기록한다.
- `plan.md`: 앞으로 진행할 개발 계획과 우선순위만 기록한다.
- `memo.md`: 프로토콜, 핀맵, 기본값, 디버깅 교훈 같은 참고 메모를 모은다.
- `dev_log.md`: 날짜별 작업 이력과 `_forAI` 정리 내역을 남긴다.

## 프로젝트 문서(팀 공유, docs/)

`_forAI/`는 AI 세션용이고, 사람/팀에게 주는 문서는 `docs/`에 있다. 대응되는 작업을 하면 해당 문서의 정합도 함께 확인할 것.

- `docs/scene-control-api.md` — **`/scene/*` REST 계약 문서(외부 소비자에게 주는 레퍼런스).**
  씬 제어 API·3D→2D 투영·에러 규약. 플러그인의 API 표면(엔드포인트·필드·기본값)을 바꾸면 반드시 같이 갱신한다.
- `docs/windows_build_run.md` — 팀원 온보딩(준비물·서브모듈·빌드/실행·배포 zip·트러블슈팅).
  `Scripts/*.ps1`의 파라미터나 배포 규약이 바뀌면 같이 갱신한다.
- `docs/sangmyung_team_request.md` — 상명대 에셋팀 협업 요청서(에셋/레벨 정리 기준). 발신용 문서라 이쪽 사정으로 고치지 않는다.

## 외부 소비자와 협업 컨텍스트

이 시뮬레이터의 산출물(API·스트림·에셋)을 소비하거나 공급하는 상대. 요청/피드백이 오면 여기와 `plan.md`에 반영한다.

- **baro_calory** (로컬 Node 백엔드, `C:\works\baro_calory`): `/api/simulator/*`가 `/scene/*`를 프록시하고
  웹 `/simulator`·CLI(`pnpm sim:*`)가 그 위에 있다. JS 쪽 계약 진실의 출처는
  `packages/cctv-client/src/scene-control-client.mjs`. 실기(휴컴스 CCTV)와 동일 계약으로 소비한다.
- **응용 소프트웨어 팀** (비전/파인튜닝): sim을 **GT(그라운드-트루스) 오라클**로 사용한다 —
  `/scene/cameras`·`/scene/slots`로 카메라 외부 파라미터·지면을 재구성하고 `/scene/project`로 투영 정합을
  검증(2026-07, 특정 검증점 0.00px·pan/tilt/zoom 전체 검증 최대 0.03px). 차량 크기 라벨은 아직
  제조사 공표 스펙을 쓰고 있어 sim 메시 실측 치수가 아니다.
  **접수된 API 확장 요청(2026-07-23, 상세는 `plan.md` 「/scene API 확장 계획」)**: ① 차종별 실제 메시
  바운딩박스 노출(요청측 "필수급") ③ 카메라 높이·화각표 노출(이교수님 제안) — **둘 다 플러그인 v0.1.7로
  구현 완료**(`boundsCm`·`groundReference`·`heightAboveReferenceGroundCm`·`intrinsics.zoomHfov`).
  ② 차량별 가시성/가림 GT 플래그는 별도 과제로 유보.
- **상명대 에셋팀**: 주차장 원본 에셋·레벨 저작(원본 `Parking_Project.uproject`, UE 5.7).
  요청서는 `docs/sangmyung_team_request.md`.

## 현재 스냅샷

- 저장소 경로: `C:\works\ue_prjs\baro_unreal`
- 대상 플랫폼: Unreal Engine 5.8 / **Windows 11·Win64 전용**. DX12/SM6 고품질 렌더 경로를 사용하며 Linux 버전은 보류한다.
- 현재 버전: 앱 **v0.2.0** + `baroCCTVSimulator` v0.1.7 작업본. 기본 배포 맵은 `/Game/simulator/LV_Park_sim_01`이다.
  배포 zip 이름은 앱 버전에서 자동 생성한다(`./Scripts/package.ps1 -Zip`). 0.2.0 이전 수동 zip은 **플러그인** 버전으로 이름이 붙어 있어 숫자를 직접 비교하면 안 된다.
- 메인 엔트리포인트: `UHucomsServerSubsystem` + `APTZCamera` + `USceneControlSubsystem`(전부 `baroCCTVSimulator` 플러그인). 패키지 기본값은 960×540 창모드, Epic 품질·해상도 품질 100이다.
- 노출 포트 기본값: Hucoms CGI `8081+i` / MJPEG `8091+i`(카메라 인덱스별, 액터별 명시 포트 우선),
  씬 제어 REST `8095`(`/scene/*`, ini·런치 인자로 변경 가능 — 계약 문서 `docs/scene-control-api.md`).
  전부 무인증·내부망 전용(의도된 결정, `memo.md`).
- 저장소는 **code-only**다 — `Content/`(30GB)와 RYU 플러그인은 git에 없다. 신규 클론은 에셋 재취득 + `git submodule update --init --recursive`가 필요하다(루트 `readme.md`).
- MCP: Unreal MCP 서버 활성(`:8000`, `bAutoStartServer=True`), Claude Code 전역(user) 등록 연결됨

## 유지 규칙

- 계획이 아닌 참고 정보는 `plan.md`가 아니라 `memo.md`에 둔다.
- 저장소 구조나 실행 명령이 바뀌면 `inventory.md`를 먼저 갱신한다.
- 작업 이력은 날짜를 붙여 `dev_log.md`에만 남긴다.
- 새 작업을 시작할 때는 `inventory.md`와 `memo.md`를 먼저 읽고, 실제 할 일은 `plan.md`에서 확인한다.
- 모든 문서에는 제목 바로 아래에 `## 목차` 섹션을 둔다.
- 사용자 동의 없이 git commit을 하지 않는다.
- 사용자 동의 없이 `_forAI/` 문서를 수정하지 않는다.
