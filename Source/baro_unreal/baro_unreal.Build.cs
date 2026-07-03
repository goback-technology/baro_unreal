// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class baro_unreal : ModuleRules
{
	public baro_unreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// CCTV 시뮬 C++ 는 baroCCTVSimulator 플러그인으로 이관됨 (HTTP/Json/HTTPServer/Sockets/
		// Networking 등 관련 의존성도 플러그인이 자체 보유). 이 게임 모듈은 이제 부트만 담는다.
		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
