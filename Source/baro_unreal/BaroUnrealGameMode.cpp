#include "BaroUnrealGameMode.h"

#include "BaroUnrealHUD.h"

ABaroUnrealGameMode::ABaroUnrealGameMode()
{
	// 플러그인 기본 HUD 를 앱 HUD 로 교체. 나머지(SpectatorPawn, 월드 렌더 OFF, ESC 종료)는
	// ABaroSimGameMode 생성자/BeginPlay 가 이미 세팅한 것을 그대로 쓴다.
	HUDClass = ABaroUnrealHUD::StaticClass();
}
