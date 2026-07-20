#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BaroUnrealHUD.generated.h"

class UBaroSystemMonitorWidget;

/**
 * ABaroUnrealHUD — baro_unreal 앱 전용 standalone HUD.
 *
 * 플러그인의 ABaroSimHUD 를 대체한다(상속이 아니라 형제 — 플러그인은 "최소한의 카메라"로 두고
 * 앱 고유 표시는 전부 이쪽에서 한다). 플러그인이 공개한 것만 읽는다:
 *   UHucomsServerSubsystem::{GetChannelCount, BaseHttpPort, BaseMjpegPort, GetChannelStatusLines}
 *   USceneControlSubsystem::ScenePort
 *
 * ABaroSimHUD 와 다른 점:
 *  - 제목줄이 플러그인 버전이 아니라 앱 버전(DefaultGame.ini ProjectVersion).
 *  - 외부에서 실제로 접속 가능한 LAN 주소를 표시한다. 바인딩은 이미 전 인터페이스이고
 *    (HTTP 리스너 BindAddress=any, MJPEG FIPv4Address::Any) 주소를 몰라서 못 붙었을 뿐이다.
 *
 * PIE 에서는 아무것도 그리지 않는다(에디터 작업 화면 유지).
 */
UCLASS()
class ABaroUnrealHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

private:
	/** 게임 틱 fps 지수이동평균(표시 안정화용). */
	float FpsEma = 0.f;

	/** 어댑터 주소는 프레임마다 바뀌지 않는다 — 첫 DrawHUD 에서 한 번만 열거해 캐시. */
	TArray<FString> CachedAddresses;
	bool bAddressesResolved = false;

	/** 우측 상단 시스템 상태 UMG. 좌측 Canvas HUD와 독립적으로 갱신된다. */
	UPROPERTY(Transient)
	TObjectPtr<UBaroSystemMonitorWidget> SystemMonitorWidget;

	/** 루프백이 아닌 IPv4 어댑터 주소. [0] = OS 기본 경로 어댑터(판별되면). */
	static TArray<FString> GatherLanAddresses();
};
