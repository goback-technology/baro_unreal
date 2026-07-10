#pragma once

#include "CoreMinimal.h"
#include "BaroSimGameMode.h"
#include "BaroUnrealGameMode.generated.h"

/**
 * ABaroUnrealGameMode — 플러그인의 ABaroSimGameMode 에서 HUD 만 앱 HUD 로 갈아끼운다.
 *
 * SpectatorPawn, 월드 렌더 OFF(standalone), t.MaxFPS, ESC 종료는 전부 부모 동작 그대로다.
 * DefaultEngine.ini [/Script/EngineSettings.GameMapsSettings] GlobalDefaultGameMode 로 지정.
 */
UCLASS()
class ABaroUnrealGameMode : public ABaroSimGameMode
{
	GENERATED_BODY()

public:
	ABaroUnrealGameMode();
};
