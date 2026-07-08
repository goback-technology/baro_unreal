# Windows 빌드/실행 가이드

## 목차

- [목적](#목적)
- [준비물](#준비물)
- [처음 받은 뒤 확인할 것](#처음-받은-뒤-확인할-것)
- [로컬 환경 파일](#로컬-환경-파일)
- [프로젝트 파일 생성](#프로젝트-파일-생성)
- [에디터용 빌드](#에디터용-빌드)
- [에디터에서 실행](#에디터에서-실행)
- [CCTV 서버처럼 실행](#cctv-서버처럼-실행)
- [패키지 만들기](#패키지-만들기)
- [실행 확인](#실행-확인)
- [자주 막히는 경우](#자주-막히는-경우)

## 목적

이 문서는 Windows PC에서 `baro_unreal` 프로젝트를 열고, 빌드하고, 실행하는 순서를 설명한다.
팀원이 그대로 따라 할 수 있도록 클릭 위치와 PowerShell 명령을 함께 적었다.

프로젝트 경로는 아래를 기준으로 설명한다.

```text
C:\works\ue_prjs\baro_unreal
```

다른 폴더에 받은 경우에는 명령어의 경로만 자기 PC 경로로 바꾸면 된다.

## 준비물

1. Windows 10 또는 Windows 11
2. Unreal Engine **5.8**
   - 기본 설치 경로 기준: `C:\Program Files\Epic Games\UE_5.8`
3. Visual Studio 2022
   - Visual Studio Installer에서 **Game development with C++** 선택
   - 같이 설치되는 MSVC, Windows SDK도 필요
4. Git
5. 프로젝트 에셋
   - 이 저장소는 소스코드와 설정 중심이다.
   - `Content/` 같은 큰 상용 에셋은 Git에 포함하지 않는다.
   - 새로 받은 PC에서는 백업, 원본 프로젝트, Fab/Epic 계정에서 에셋을 복구해야 한다.

## 처음 받은 뒤 확인할 것

PowerShell을 열고 프로젝트 폴더로 이동한다.

```powershell
cd C:\works\ue_prjs\baro_unreal
```

플러그인 서브모듈을 받는다.

```powershell
git submodule update --init --recursive
```

다음 파일과 폴더가 있는지 확인한다.

```powershell
Test-Path .\baro_unreal.uproject
Test-Path .\Plugins\baroCCTVSimulator
Test-Path .\Content
```

세 명령이 모두 `True`면 기본 준비가 된 상태다.

`Content`가 `False`이면 에셋이 아직 없는 상태다. 이 경우 C++ 빌드는 될 수 있지만,
레벨을 열거나 실행할 때 에셋 누락 오류가 날 수 있다.

## 로컬 환경 파일

PC마다 다른 값은 `.env`에 둔다. 대표적으로 Unreal Engine 설치 경로가 여기에 들어간다.

처음 받은 PC에서는 예제 파일을 복사해서 만든다.

```powershell
Copy-Item .env.example .env
notepad .env
```

기본 내용은 아래처럼 생겼다.

```text
UE_PATH=C:\Program Files\Epic Games\UE_5.8
DEFAULT_MAP=/Game/simulator/LV_Park_sim_01
RUN_RESX=960
RUN_RESY=540
RUN_WINDOWED=true
```

`UE_PATH`만 자기 PC에 맞으면 대부분 그대로 써도 된다.

`.env`는 Git에 올리지 않는다. 각자 PC에서만 쓰는 파일이다.
팀에 공유할 기본값은 `.env.example`에 적는다.

## 프로젝트 파일 생성

Visual Studio 솔루션 파일은 Git에 올리지 않는다. 각자 PC에서 생성하면 된다.

가장 쉬운 방법:

1. 탐색기에서 `baro_unreal.uproject`를 찾는다.
2. 우클릭한다.
3. **Generate Visual Studio project files**를 누른다.
4. `baro_unreal.sln`이 생겼는지 확인한다.

우클릭 메뉴가 없으면 PowerShell에서 아래 명령을 실행한다.

```powershell
& "C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe" /projectfiles "C:\works\ue_prjs\baro_unreal\baro_unreal.uproject"
```

위 명령이 실패하면 Unreal Engine 5.8 또는 Epic Games Launcher 설치 상태를 먼저 확인한다.

## 에디터용 빌드

코드가 바뀌었거나 처음 받았다면 에디터용 빌드를 한 번 해준다.

```powershell
.\Scripts\build.ps1
```

성공하면 마지막에 `BUILD SUCCESSFUL`에 해당하는 메시지가 나온다.

게임 실행 파일 타깃만 빌드하고 싶으면:

```powershell
.\Scripts\build.ps1 -Target Game
```

Visual Studio로 빌드해도 된다.

1. `baro_unreal.sln`을 연다.
2. 상단 설정을 `Development Editor` / `Win64`로 맞춘다.
3. 시작 프로젝트 또는 빌드 대상이 `baro_unrealEditor`인지 확인한다.
4. **Build**를 실행한다.

새 C++ 클래스, `UCLASS`, `UPROPERTY` 같은 언리얼 반영 코드가 바뀐 경우에는
에디터를 닫고 위 PowerShell 빌드를 다시 하는 편이 가장 깔끔하다.

## 에디터에서 실행

작업 화면을 보고 확인할 때는 에디터 실행이 가장 편하다.

1. `baro_unreal.uproject`를 더블클릭한다.
2. Unreal Engine 5.8로 열린다.
3. 기본 시작 맵은 `/Game/simulator/LV_Park_sim_01`이다.
4. 상단 **Play** 버튼을 누른다.

Play를 누르면 레벨에 배치된 CCTV 카메라 수만큼 Hucoms 서버가 열린다.
HTTP 포트는 보통 `8081`부터, MJPEG 포트는 보통 `8091`부터 사용한다.

## CCTV 서버처럼 실행

관제 시스템이나 외부 프로그램에서 붙여 테스트할 때는 standalone 실행이 편하다.

```powershell
.\Scripts\run.ps1
```

기본 맵은 `/Game/simulator/LV_Park_sim_01`이라서 맵 이름을 따로 넣지 않아도 된다.
기본 실행은 UE 창을 띄운 뒤 PowerShell로 바로 돌아온다.
UE가 종료될 때까지 기다리고 싶으면 `.\Scripts\run.ps1 -WaitForExit`를 사용한다.

standalone 창이 검은 화면처럼 보여도 정상일 수 있다. 이 모드는 사람이 보는 메인 화면보다
CCTV 캡처와 HTTP/MJPEG 서버 실행이 목적이다. 카메라 영상은 별도 SceneCapture가 만들기 때문에
검은 창과 별개로 `jpeg.cgi`, `mjpeg.cgi`에서 확인할 수 있다.

종료는 실행 창에서 `ESC`를 누른다.

에디터만 열고 싶으면:

```powershell
.\Scripts\run.ps1 -Mode Editor
```

이미 패키징된 실행 파일을 켜고 싶으면:

```powershell
.\Scripts\run.ps1 -Mode Packaged
```

## 패키지 만들기

배포용 실행 파일이 필요하면 패키징 스크립트를 사용한다.

먼저 에디터를 닫는다. 에디터가 열린 상태에서 패키징하면 파일 잠금 때문에 실패할 수 있다.

```powershell
cd C:\works\ue_prjs\baro_unreal
.\Scripts\package.ps1
```

기본값은 다음과 같다.

- 플랫폼: `Win64`
- 설정: `Development`
- 맵: `.env`의 `DEFAULT_MAP`
- 결과 폴더: `Packaged/Win64`

결과 실행 파일:

```text
Packaged\Win64\baro_unreal.exe
```

배포용으로 더 가볍게 만들고 싶으면 Shipping 설정을 쓴다.

```powershell
.\Scripts\package.ps1 -Config Shipping
```

이전 산출물을 지우고 처음부터 다시 만들고 싶으면 `-Clean`을 붙인다.

```powershell
.\Scripts\package.ps1 -Clean
```

PowerShell에서 스크립트 실행이 막히면 현재 창에서만 정책을 풀고 다시 실행한다.

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\Scripts\package.ps1
```

같은 방식으로 `build.ps1`, `run.ps1`도 실행할 수 있다.

## 실행 확인

에디터 Play, standalone, 패키지 실행 중 하나를 켠 뒤 확인한다.

포트가 열렸는지 확인:

```powershell
Get-NetTCPConnection -State Listen -LocalPort 8081,8082,8083,8084,8091,8092,8093,8094
```

PTZ 상태 응답 확인:

```powershell
Invoke-WebRequest `
  "http://127.0.0.1:8081/cgi-bin/control/ptzf_status.cgi?action=getptzfpos" `
  -UseBasicParsing |
  Select-Object -ExpandProperty Content
```

JPEG 한 장 받아보기:

```powershell
Invoke-WebRequest `
  "http://127.0.0.1:8081/cgi-bin/image/jpeg.cgi" `
  -OutFile "$env:TEMP\baro_cam_8081.jpg"

Invoke-Item "$env:TEMP\baro_cam_8081.jpg"
```

응답이 오면 CCTV 서버가 정상 실행 중인 것이다.

## 자주 막히는 경우

### Unreal Engine 버전이 다르다고 나올 때

이 프로젝트는 UE **5.8** 기준이다. Epic Games Launcher에서 5.8이 설치되어 있는지 확인한다.
다른 버전으로 열면 에셋 변환이나 C++ 빌드 문제가 생길 수 있다.

### Missing modules 또는 rebuild 메시지가 나올 때

대부분 C++ 빌드가 아직 안 된 상태다.

1. Visual Studio 2022의 **Game development with C++** 워크로드가 설치되어 있는지 확인한다.
2. 프로젝트 파일을 다시 생성한다.
3. `.\Scripts\build.ps1`로 `baro_unrealEditor Win64 Development`를 빌드한다.

### baroCCTVSimulator 플러그인을 못 찾을 때

서브모듈이 없는 상태일 가능성이 높다.

```powershell
git submodule update --init --recursive
```

### 레벨이 비어 보이거나 에셋 오류가 많이 날 때

`Content/`가 빠진 상태일 가능성이 높다. 이 저장소는 큰 상용 에셋을 Git에 넣지 않는다.
백업, 원본 프로젝트, Fab/Epic 계정에서 필요한 에셋을 복구해야 한다.

### 8081 포트가 이미 사용 중일 때

이전에 실행한 에디터나 패키지 프로그램이 남아 있을 수 있다.

```powershell
Get-NetTCPConnection -State Listen -LocalPort 8081 | Select-Object LocalAddress,LocalPort,OwningProcess
```

작업 관리자에서 해당 PID의 프로그램을 확인한 뒤 종료한다.

### 패키징이 실패할 때

먼저 에디터를 닫고 다시 실행한다.

그래도 실패하면 처음부터 다시 만든다.

```powershell
.\Scripts\package.ps1 -Clean
```
