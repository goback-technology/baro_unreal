// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class baro_unreal : ModuleRules
{
	public baro_unreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// HTTP/Json: baro_vla 추론 서버 통신 + 응답 파싱 (CenteringClientComponent)
		// HTTPServer: 인엔진 Hucoms HTTP CGI 서버 (UHucomsServerSubsystem) - 실기 행세
		// Sockets/Networking: 연속 MJPEG 스트리밍 서버 (FMjpegStreamServer) - RTSP 브리지 입력
		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP", "Json", "HTTPServer", "Sockets", "Networking" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
