#include "BaroUnrealHUD.h"

#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "HucomsServerSubsystem.h"
#include "IPAddress.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "SceneControlSubsystem.h"
#include "SocketSubsystem.h"
#include "SocketTypes.h"

namespace
{
	FString AppVersion()
	{
		FString Version;
		GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni);
		return Version.IsEmpty() ? TEXT("?") : Version;
	}

	FString PluginVersion()
	{
		if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("baroCCTVSimulator")))
		{
			return Plugin->GetDescriptor().VersionName;
		}
		return TEXT("?");
	}
}

TArray<FString> ABaroUnrealHUD::GatherLanAddresses()
{
	TArray<FString> Result;

	ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!Sockets)
	{
		return Result;
	}

	TArray<TSharedPtr<FInternetAddr>> Adapters;
	Sockets->GetLocalAdapterAddresses(Adapters);
	for (const TSharedPtr<FInternetAddr>& Addr : Adapters)
	{
		if (!Addr.IsValid() || Addr->GetProtocolType() != FNetworkProtocolTypes::IPv4)
		{
			continue;
		}
		const FString Text = Addr->ToString(/*bAppendPort=*/false);

		// 루프백은 외부에서 못 붙는다. 169.254/16 은 APIPA 링크로컬 — DHCP 를 못 받은
		// 미연결 어댑터(끊긴 Wi-Fi, 블루투스 PAN, 가상 스위치)가 남기는 쓰레기 주소라
		// 여기 실려 나가면 사용자가 붙지 못하는 주소를 보게 된다. (이 개발 PC 실측:
		// 169.254 가 4개, 실제 LAN 은 이더넷 192.168.0.211 하나뿐.)
		if (Text.StartsWith(TEXT("127.")) || Text.StartsWith(TEXT("169.254.")))
		{
			continue;
		}
		Result.AddUnique(Text);
	}

	// OS 가 기본 경로로 고르는 어댑터를 맨 앞으로 — Hyper-V/WSL/Docker 의 vEthernet 이
	// 섞여도 사람이 실제로 브라우저에 칠 주소가 첫 줄에 오게 한다. GetLocalHostAddr 가
	// 링크로컬을 돌려주는 경우도 있어(위 실측 참조) 필터를 통과할 때만 채택한다.
	bool bCanBindAll = false;
	const TSharedRef<FInternetAddr> Host = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
	const int32 PrimaryIndex = Result.IndexOfByKey(Host->ToString(/*bAppendPort=*/false));
	if (PrimaryIndex > 0)
	{
		const FString Primary = Result[PrimaryIndex];
		Result.RemoveAt(PrimaryIndex);
		Result.Insert(Primary, 0);
	}

	return Result;
}

void ABaroUnrealHUD::DrawHUD()
{
	Super::DrawHUD();

	UWorld* World = GetWorld();
	if (!World || World->WorldType != EWorldType::Game || !GEngine)
	{
		return;
	}

	if (!bAddressesResolved)
	{
		CachedAddresses = GatherLanAddresses();
		bAddressesResolved = true;
	}

	const UHucomsServerSubsystem* Hucoms = World->GetSubsystem<UHucomsServerSubsystem>();
	const USceneControlSubsystem* Scene = World->GetSubsystem<USceneControlSubsystem>();
	const int32 ChannelCount = Hucoms ? Hucoms->GetChannelCount() : 0;
	const int32 HttpBase = Hucoms ? Hucoms->BaseHttpPort : 8081;
	const int32 MjpegBase = Hucoms ? Hucoms->BaseMjpegPort : 8091;
	const int32 ScenePort = Scene ? Scene->ScenePort : 8095;

	const float Dt = FMath::Max(World->GetDeltaSeconds(), 0.0001f);
	FpsEma = (FpsEma <= 0.f) ? (1.f / Dt) : (FpsEma * 0.95f + (1.f / Dt) * 0.05f);

	const float X = 40.f;
	float Y = 40.f;

	DrawText(FString::Printf(TEXT("%s v%s — CCTV 시뮬레이터"), FApp::GetProjectName(), *AppVersion()),
		FLinearColor::White, X, Y, GEngine->GetLargeFont(), 1.6f);
	Y += 44.f;
	DrawText(FString::Printf(TEXT("plugin baroCCTVSimulator v%s"), *PluginVersion()),
		FLinearColor(0.55f, 0.6f, 0.65f), X, Y, GEngine->GetMediumFont(), 1.1f);
	Y += 36.f;

	const bool bHasLan = CachedAddresses.Num() > 0;
	const FString Primary = bHasLan ? CachedAddresses[0] : TEXT("(LAN 주소 없음)");
	DrawText(FString::Printf(TEXT("서빙 주소  %s"), *Primary),
		FLinearColor(1.f, 0.9f, 0.4f), X, Y, GEngine->GetLargeFont(), 1.2f);
	Y += 34.f;

	if (CachedAddresses.Num() > 1)
	{
		TArray<FString> Others(CachedAddresses);
		Others.RemoveAt(0);
		DrawText(FString::Printf(TEXT("다른 어댑터  %s"), *FString::Join(Others, TEXT("   "))),
			FLinearColor(0.5f, 0.5f, 0.5f), X, Y, GEngine->GetMediumFont(), 1.1f);
		Y += 26.f;
	}

	if (bHasLan)
	{
		DrawText(FString::Printf(TEXT("스냅샷 http://%s:%d/cgi-bin/image/jpeg.cgi   ·   스트림 http://%s:%d   ·   씬제어 http://%s:%d/scene/catalog"),
			*Primary, HttpBase, *Primary, MjpegBase, *Primary, ScenePort),
			FLinearColor(0.7f, 0.85f, 1.f), X, Y, GEngine->GetMediumFont(), 1.1f);
		Y += 32.f;
	}

	DrawText(FString::Printf(TEXT("Hucoms 채널 %d개 서빙 중  ·  HTTP %d~  ·  MJPEG %d~  ·  씬 제어 %d"),
		ChannelCount, HttpBase, MjpegBase, ScenePort), FLinearColor(0.7f, 0.85f, 1.f), X, Y, GEngine->GetMediumFont(), 1.2f);
	Y += 28.f;
	DrawText(FString::Printf(TEXT("게임 틱 %.0f fps  ·  월드 렌더링 OFF — CCTV SceneCapture 만 렌더 중"), FpsEma),
		FLinearColor(0.6f, 0.6f, 0.6f), X, Y, GEngine->GetMediumFont(), 1.2f);
	Y += 36.f;

	if (Hucoms)
	{
		for (const FString& Line : Hucoms->GetChannelStatusLines())
		{
			DrawText(Line, FLinearColor(0.55f, 1.f, 0.65f), X, Y, GEngine->GetMediumFont(), 1.2f);
			Y += 26.f;
		}
	}

	Y += 10.f;
	DrawText(TEXT("ESC: 종료"), FLinearColor(1.f, 0.85f, 0.4f), X, Y, GEngine->GetMediumFont(), 1.2f);
}
