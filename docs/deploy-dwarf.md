# 배포 가이드 — DWarf API 로 배포기에 올리고 pm2 로 띄우기

이 문서는 `package.ps1 -Zip` 으로 만든 배포본을 **배포기에 전송·설치·기동**하는 절차다.
zip 을 만드는 데까지는 [windows_build_run.md](windows_build_run.md) 가 담당하고, 이 문서는 그 다음이다.

전 과정이 **HTTP API 하나(DWarf)** 로 끝난다 — SSH·원격데스크톱 없이 업로드·압축해제·프로세스 기동을 한다.

## 목차

- [배포기와 접속 정보](#배포기와-접속-정보)
- [DWarf API 한눈에](#dwarf-api-한눈에)
- [배포 절차](#배포-절차)
  - [1. 사전 점검](#1-사전-점검)
  - [2. 폴더 만들기](#2-폴더-만들기)
  - [3. 청크 업로드](#3-청크-업로드)
  - [4. 압축 해제](#4-압축-해제)
  - [5. pm2 등록·기동](#5-pm2-등록기동)
  - [6. 검증](#6-검증)
- [재배포(버전 올리기)](#재배포버전-올리기)
- [함정 — 실측으로 확인된 것](#함정--실측으로-확인된-것)
- [문제 해결](#문제-해결)

## 배포기와 접속 정보

접속 정보는 저장소에 넣지 않고 프로젝트 루트 `.env` 에 둔다(Git 제외 대상).

```text
RELEASE_ADDRESS=http://<배포기IP>:10822/help
RELEASE_AUTHORIZE=<토큰>
```

- 인증: `/help` 를 제외한 모든 요청에 **`auth-token` 헤더**가 필요하다.
- **`GET /help` 가 이 API 의 단일 진실 소스다.** 서버가 요청 시점에 파일에서 읽어 주므로 항상 최신이고,
  토큰 없이 읽을 수 있다. 절차가 안 맞으면 이 문서보다 `/help` 를 믿는다.
- 경로는 전부 서버의 `WORK_DIR_ROOT` 안에서만 해석된다. `..`·절대경로·밖을 가리키는 심볼릭 링크는 거부된다.

## DWarf API 한눈에

| 목적 | 호출 |
|---|---|
| 사용법 문서(무인증) | `GET /help` |
| 상태·버전 | `GET /api/v1` |
| 환경·허용 명령 | `GET /api/v1/fc/env` |
| 파일 목록 | `GET /api/v1/fc/filelist?cwd=<상대경로>` |
| 디렉터리 생성 | `POST /api/v1/fc/mkdir` `{path, dir}` |
| 삭제 | `POST /api/v1/fc/delete` `{path, file}` |
| 청크 업로드(대용량) | `POST /api/v1/uploader/chunk` (헤더로 메타 전달) |
| 명령 실행(압축 해제 등) | `POST /api/v1/fc/exec` `{cmd, args, paths, cwd}` |
| pm2 목록·등록·제어 | `GET/POST /api/v1/pm2/*` |

**pm2 는 `/fc/exec` 가 아니라 전용 통로 `/api/v1/pm2` 를 쓴다**(DWarf 0.2.3~). 예전에는 `/fc/exec` 로
불렀고 앱 이름을 서버 화이트리스트에 등록해야 했지만, 지금은 `pm2 jlist` 가 진실의 출처라 그 유지가 없다.

## 배포 절차

아래는 실제로 v0.2.4 를 올린 순서 그대로다. `BOX`·`TOKEN` 은 `.env` 에서 읽어 쓴다.

### 1. 사전 점검

```bash
curl -s "http://$BOX:10822/api/v1" -H "auth-token: $TOKEN"          # 살아있는지·버전
curl -s "http://$BOX:10822/api/v1/fc/filelist?cwd=" -H "auth-token: $TOKEN"   # 루트 상태
curl -s "http://$BOX:10822/api/v1/pm2/list" -H "auth-token: $TOKEN"           # 지금 도는 앱
```

`GET /api/v1/fc/env` 로 `exec.enabled` 와 허용 명령(`tar` 필요)을 확인한다. `pm2` 통로는 `GET /api/v1/pm2`.

### 2. 폴더 만들기

**배포 폴더는 `baro_unreal_sim/` 로 고정한다.** 버전을 폴더명에 넣지 않는다 — pm2 등록의 `script`
경로가 고정이어야 재배포 때 등록을 건드리지 않는다.

```bash
curl -s -X POST "http://$BOX:10822/api/v1/fc/mkdir" -H "auth-token: $TOKEN" \
  -H "content-type: application/json" -d '{"path":"","dir":"baro_unreal_sim"}'
```

### 3. 청크 업로드

일반 업로드는 1GB 제한이라 **2.8GB 배포본은 반드시 청크 업로드**를 쓴다(단일 청크 100MB 이하).
`base-path` 를 **`baro_unreal_sim`** 으로 준다 — 루트에 올리면 4번에서 풀 수 없다([함정](#함정--실측으로-확인된-것)).

헤더:

```http
POST /api/v1/uploader/chunk
content-type: application/octet-stream
auth-token: <토큰>
upload-id: <이 업로드 고유 id>        # 동시 업로드 간 청크 섞임 방지
upload-name: <URI 인코딩한 파일명>
base-path: baro_unreal_sim
base-path-encoding: uri
file-size: <전체 바이트>
chunk-start: <오프셋>
chunk-size: <이 본문 바이트>
```

- 마지막 청크를 받으면 서버가 `0`부터 연속성을 검증한 뒤 병합하고 `{file, size}` 를 준다.
- **네트워크 오류에 재시도를 넣는다.** 오프셋 기반이라 같은 청크를 다시 보내면 그만이다(실측: 서버
  재시작과 겹쳐 한 청크가 실패했지만 재시도로 무손실 복구).
- 실측 2.78GB / 80MB×36청크 / LAN **약 4.7분**.

업로드 후 크기를 반드시 대조한다.

```bash
curl -s "http://$BOX:10822/api/v1/fc/filelist?cwd=baro_unreal_sim" -H "auth-token: $TOKEN"
```

### 4. 압축 해제

Windows 의 `tar.exe`(bsdtar)가 `.zip` 을 읽는다. 별도 zip 도구가 필요 없다.

```bash
# 먼저 내용 확인 (선택)
curl -s -X POST "http://$BOX:10822/api/v1/fc/exec" -H "auth-token: $TOKEN" \
  -H "content-type: application/json" \
  -d '{"cmd":"tar","args":["-t"],"paths":["baro_unreal_sim_v0.2.4_20260806.zip"],"cwd":"baro_unreal_sim"}'

# 풀기 — cwd 안에 풀린다
curl -s -X POST "http://$BOX:10822/api/v1/fc/exec" -H "auth-token: $TOKEN" \
  -H "content-type: application/json" \
  -d '{"cmd":"tar","args":["-x"],"paths":["baro_unreal_sim_v0.2.4_20260806.zip"],"cwd":"baro_unreal_sim"}'
```

- **아카이브는 `paths` 로 준다.** 서버가 검증한 절대경로를 `-f` 로 붙이므로 `args` 에 `-f` 를 넣으면 `400`.
- **푸는 위치는 `-C` 가 아니라 `cwd`** 다. `-C` 는 차단돼 있다.
- 기존 파일은 덮어쓴다. 실측 2.8GB 해제 **약 7초**.

zip 루트 구조(= 해제 결과):

```text
baro_unreal.exe          ← pm2 가 띄울 실행 파일(런처)
README.md                ← 에이전트용 배포 안내(docs/deploy-readme.md 가 실린 것)
Engine/ , baro_unreal/   ← 엔진·게임 콘텐츠
Manifest_*.txt , NOTICES.txt
```

### 5. pm2 등록·기동

`PM2_ALLOW_REGISTER=true` 면 `start` 에 `script` 를 실어 **등록과 기동을 한 번에** 한다.

```bash
curl -s -X POST "http://$BOX:10822/api/v1/pm2/start" -H "auth-token: $TOKEN" \
  -H "content-type: application/json" -d '{
    "name": "baro_unreal",
    "script": "baro_unreal_sim/baro_unreal.exe",
    "interpreter": "none",
    "args": ["-game", "-windowed", "-resx=1280", "-resy=720"]
  }'

# 재부팅 후에도 유지 — 이걸 빼면 등록이 사라진다
curl -s -X POST "http://$BOX:10822/api/v1/pm2/save" -H "auth-token: $TOKEN" \
  -H "content-type: application/json" -d '{}'
```

- `script` 는 **작업 루트 기준 상대 경로**, `.exe` 라 `interpreter: "none"`.
- `args` 는 배열. 서버가 `--` 뒤에 붙여 pm2 옵션으로 오해되지 않게 한다.
- 이미 같은 이름이 있으면 `400` → 재배포는 [아래](#재배포버전-올리기) 절차를 쓴다.

### 6. 검증

pm2 가 `online` 인 것만으로는 부족하다. **실제 API 가 응답해야 성공**이다.
UE 부팅(맵 로드·Lumen 셋업)에 시간이 걸리니 기동 직후 실패는 정상일 수 있다 — 기다렸다 다시 친다.

```bash
curl -s -o /dev/null -w "%{http_code}\n" "http://$BOX:8095/scene/help"    # 200
curl -s "http://$BOX:8095/scene/catalog"                                   # pluginVersion 확인
curl -s "http://$BOX:8095/scene/cameras"                                   # 시작 시 빈 목록
# 렌더까지 보려면 카메라를 하나 세웠다 지운다(포트 명시 필수)
curl -s -X POST "http://$BOX:8095/scene/cameras" -H "content-type: application/json" \
  -d '{"location":{"x":73,"y":-2015,"z":1000},"yawDeg":90,"pitchDeg":-30,"httpPort":8287,"mjpegPort":8297}'
curl -s -o snap.jpg -w "%{size_download}\n" "http://$BOX:8287/cgi-bin/image/jpeg.cgi"   # 실렌더
curl -s -X DELETE "http://$BOX:8095/scene/cameras/8287"
```

기능까지 확인하려면 차량을 하나 스폰해 보고 지운다(`POST /scene/cars` → `POST /scene/reset`).
**`/scene/reset` 같은 빈 본문 POST 에도 `-d '{}'` 를 붙인다** — UE HTTP 서버가 `Content-Length` 를 요구한다.

## 재배포(버전 올리기)

폴더명이 고정이라 pm2 등록은 그대로 두고 내용만 갈아끼운다.

1. `POST /api/v1/pm2/stop` `{"name":"baro_unreal"}` — 실행 중이면 파일이 잠겨 덮어쓰기가 실패한다.
2. 새 zip 청크 업로드 → `tar -x`(기존 파일 덮어씀).
3. `POST /api/v1/pm2/restart` `{"name":"baro_unreal"}` (또는 `start`).
4. [검증](#6-검증) 반복. 옛 zip 은 `POST /fc/delete` 로 지워 디스크를 회수한다.

## 함정 — 실측으로 확인된 것

- **zip 은 처음부터 배포 폴더 안으로 올린다.** `tar` 는 `cwd` 안에서만 풀고 `..` 도 `-C` 도 막혀 있어,
  루트에 올린 zip 을 하위 폴더에 풀 방법이 없다. 실제로 루트에 올렸다가 2.8GB 를 **다시 업로드**했다.
- **pm2 가 보는 메모리(7MB)로 시뮬 상태를 판단하지 말 것.** `baro_unreal.exe` 는 런처라 실제 엔진은
  자식 프로세스다. 부모만 보면 "안 떴다"로 오해한다. 판단은 **API 응답**으로 한다.
- **pm2 stdout 에 서버 기동 로그가 안 보인다**(버퍼링). 진짜 로그는 패키지 안
  `baro_unreal_sim/baro_unreal/Saved/Logs/baro_unreal.log` 다. 원격에서 `findstr` 로 읽는다:
  ```bash
  curl -s -X POST "http://$BOX:10822/api/v1/fc/exec" -H "auth-token: $TOKEN" \
    -H "content-type: application/json" \
    -d '{"cmd":"findstr","args":["/C:Hucoms"],"paths":["baro_unreal_sim/baro_unreal/Saved/Logs/baro_unreal.log"],"cwd":""}'
  ```
- **시뮬 포트(Hucoms CGI·씬 제어)는 전 인터페이스(0.0.0.0)에 바인드된다.** 리스너를 여는 코드가
  포트별로 그렇게 선언한다(플러그인 `HttpListenerBind.h`) — 런타임에 스폰한 카메라의 포트도 포함이다.
  로그에 찍히는 주소 문자열이 아니라 **원격에서 실제로 응답하는지**로 판정한다.
  판정 도구: `node tools/scene-test/lan-bind-contract.mjs --host <배포기IP>`.
- **`/fc/exec` 는 짧은 명령용**이다(`EXEC_TIMEOUT_MS` 초과 시 SIGKILL, 환경변수 미상속, 출력 버퍼링).
  상주 프로세스는 반드시 pm2 통로로 다룬다.
- **`pm2 kill`·일괄 `all` 은 라우트 자체가 없다.** DWarf 자신이 pm2 아래에 있어 함께 죽기 때문이다.
- 업로드 중 **DWarf 가 재시작될 수 있다**(운영자가 API 를 올릴 때). 청크 재시도가 있으면 무손실이다.

## 문제 해결

| 증상 | 원인 / 해결 |
|---|---|
| 업로드 후 `tar` 가 파일을 못 찾음 | `cwd` 와 `paths` 조합 확인. zip 이 그 `cwd` 안에 있어야 한다 |
| `tar` 가 `400` | `args` 에 `-f`·`-C`·`-P` 를 넣었을 때. 경로는 `paths`, 위치는 `cwd` |
| pm2 `start` 가 `400` | 이름이 이미 등록됨 → `restart` 를 쓰거나 `delete` 후 재등록 |
| pm2 online 인데 API 무응답 | 대개 UE 부팅 중. 1~2분 뒤 재시도, 그래도 안 되면 패키지 안 `Saved/Logs` 확인 |
| 덮어쓰기 실패 | 실행 중이라 파일 잠김 → `pm2 stop` 후 다시 |
| 재부팅하니 시뮬이 없음 | `pm2 save` 를 안 했다 |
| `404` on pm2 경로 | `PM2_ENABLED=false` 이거나 지원하지 않는 하위 명령 |
