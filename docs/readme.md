# baro_unreal 문서 색인

이 폴더는 `baro_unreal`의 빌드·실행 절차와 런타임 Scene API 계약을 관리한다.
새로 작업을 시작할 때는 아래 색인에서 목적에 맞는 문서만 선택해 읽는다.

## 문서 색인

| 문서 | 분류 | 읽어야 하는 경우 | 내용 요약 |
|---|---|---|---|
| [readme.md](readme.md) | **시작점 / 색인** | `docs/`에서 필요한 문서를 찾을 때 | 문서별 역할, 중요도와 권장 열람 순서를 안내한다. |
| [windows_build_run.md](windows_build_run.md) | **필독 — 개발·배포 환경** | 새 PC 설정, 프로젝트 파일 생성, Editor 빌드·실행, CCTV 서버 실행, 패키징·zip 배포 시 | Windows 10/11, Unreal Engine 5.8, Visual Studio 환경을 기준으로 `.env` 설정부터 빌드·실행·패키징·배포 확인과 자주 발생하는 오류의 해결 방법까지 설명한다. |
| [scene-control-api.md](scene-control-api.md) | **필독 — Scene API 개발** | UE 시뮬레이터와 baro_calory 웹·Node·CLI를 연동하거나 `/scene/*` 계약을 수정할 때 | 차량·주차면·카메라 조회 및 차량 CRUD, 투영 오라클, 차량 bounds/클래스 라벨, 카메라 기준 높이·핀홀 내부 파라미터, zoom→HFOV 화각표, 가시성/가림 GT, config 카메라 스포너, Hucoms 연속 PTZ 미러, 오류 규약과 호출 예제를 정의하는 런타임 REST API 기준 문서다(플러그인 v0.1.8 기준). |
| [sangmyung_team_request.md](sangmyung_team_request.md) | **중요 문서 아님 · 읽을 필요 없음** | 기본적으로 읽지 않는다. 과거 상명대 에셋 작업팀 요청 내용을 확인해야 하는 예외적인 경우만 참고 | 원본 주차장 프로젝트의 에셋·레벨·조명·머티리얼 정리와 납품 형태에 관한 외부 협업 요청서다. 현재 빌드, 런타임 구현 및 API 계약의 기준 문서가 아니다. |

## 권장 열람 순서

1. 프로젝트를 처음 빌드하거나 실행한다면 [windows_build_run.md](windows_build_run.md)
2. Scene API 또는 baro_calory 연동을 작업한다면 [scene-control-api.md](scene-control-api.md)
3. [sangmyung_team_request.md](sangmyung_team_request.md)는 기본 열람 대상에서 제외

## 문서 관리 기준

- 빌드 도구, Unreal Engine 버전, 실행·패키징 절차가 바뀌면 `windows_build_run.md`를 갱신한다.
- `/scene/*` 필드, 계산 규칙, 버전 또는 클라이언트 소비 방식이 바뀌면 `scene-control-api.md`를 코드와 함께 갱신한다.
- 과거 협업 요청서는 현재 기술 기준으로 인용하지 않는다.
- 새 문서를 추가하면 이 색인에 분류, 대상 독자와 한 문단 요약을 함께 기록한다.
