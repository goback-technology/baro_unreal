// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class baro_unreal : ModuleRules
{
	public baro_unreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// baroCCTVSimulator: ABaroSimGameMode 를 상속하고 Hucoms/SceneControl 서브시스템을 읽는다.
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "baroCCTVSimulator" });

		// CCTV 시뮬 C++ 는 baroCCTVSimulator 플러그인으로 이관됨 (HTTP/Json/HTTPServer/Sockets/
		// Networking 등 관련 의존성도 플러그인이 자체 보유). 이 게임 모듈은 부트 + 앱 고유 HUD 만 담는다.
		// 플러그인의 Sockets/Projects 는 Private 의존이라 전이되지 않는다 — 여기서 직접 가져온다.
		//   Sockets  = ISocketSubsystem/FInternetAddr (HUD 의 LAN 주소 열거)
		//   Projects = IPluginManager (플러그인 버전 표시)
		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Projects" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
