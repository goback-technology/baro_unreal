# Plan

## 목차

- [Current goal](#current-goal)
- [Near-term work](#near-term-work)
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

- [ ] **나머지 RYU 레벨 빌딩 베이크**: `LV_Park_01 / 04 / 07_A`, `Unity/LV_Park_01_U / 04_U`.
      절차는 `memo.md`의 「RYU 플러그인-프리 베이크」 레시피 그대로. (sim_01·sim_03 완료, sim_02는 RYU無.)
- [ ] **전 레벨 베이크 완료 후에만** uproject에 `{"Name":"RYUKoreaBuilidngCreator","Enabled":false}`를
      명시해 최종 비활성화 → 프로젝트 전체 플러그인-프리.
      ⚠️ 현재는 uproject에 항목이 없을 뿐 **여전히 활성**이다(프로젝트 로컬 플러그인 기본 enabled).
      그전에 끄면 미베이크 레벨의 빌딩 상세가 소실된다.
- [ ] **성능: 캡처 파이프라인 오프스레드화** — 4캠 동시 24fps가 필요해지면 착수.
      JPEG 인코딩 오프스레드(`UE::Tasks`) → 블로킹 `ReadPixels` → 비동기 `FRHIGPUTextureReadback`
      → `jpeg.cgi` 스냅샷 캐시. (근거: baro_calory dev_log 2026-07-02 저녁 진단)

### 조건부 / 유보 (실제로 문제가 될 때만)

- [ ] PIE 일시정지·캡처 실패 시 스트림 keepalive가 없어 클라이언트가 무프레임 대기한다.
      소비자 read-timeout 이슈가 **실제로 생기면** idle 재전송을 추가한다.
- [ ] 나머지 `LV_Park` 레벨(02~08)에도 PTZ 배치 — **필요해지면**. (배치 규약: `BP_Pole` 자식, z+600, pitch −20)
- [ ] `sim_03` 병합메시 Nanite 불가(144 머티리얼 섹션 > 한계). **필요하면** Merge Materials=on으로 재병합.
      VSM 그림자 경고는 잔존하나 콜리전 제거로 실부하는 이미 해소됨.

## Structure decisions

- 에셋 이관: **파일 직접 복사(robocopy)** 채택 — parking_area가 5.7이고 Content 의존이 광범위해 통째 복사가 확실. `/Game` 경로 동일해 참조 자동 해소.
- CCTV 코드: 호스트 게임 모듈이 아니라 **`baroCCTVSimulator` 플러그인(git submodule)** 단일 소스.
  baroQuantum과 baro_unreal이 함께 소비한다. 이관 시 `CoreRedirects` 필수(2026-07-03).
- 레벨: parking_area 레벨은 **모놀리식 `.umap`(월드파티션 아님)** — 그대로 사용.
- 플러그인: RYU는 프로젝트 로컬 `Plugins/`(콘텐츠+모듈, 5.8 재빌드), 나머지는 엔진 내장 활성.
- **플랫폼: Windows·Win64 전용**(2026-07-10 확정). Linux/Vulkan은 보류다 —
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
