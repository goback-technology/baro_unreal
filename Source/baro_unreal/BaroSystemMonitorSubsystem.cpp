#include "BaroSystemMonitorSubsystem.h"

#include "DynamicRHI.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHIStats.h"
#include "RHIGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaroHealth, Log, All);

namespace
{
	constexpr double BytesToMB = 1.0 / (1024.0 * 1024.0);

	TAutoConsoleVariable<float> CVarBaroHealthSampleInterval(
		TEXT("baro.Health.SampleInterval"), 1.0f,
		TEXT("System health sampling interval in seconds."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBaroHealthLogInterval(
		TEXT("baro.Health.LogInterval"), 30.0f,
		TEXT("System health UE log/CSV interval in seconds."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBaroHealthTrendWindow(
		TEXT("baro.Health.TrendWindow"), 120.0f,
		TEXT("Process physical-memory trend window in seconds."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBaroHealthWatchSlope(
		TEXT("baro.Health.WatchMBPerMin"), 5.0f,
		TEXT("Memory growth slope that changes health to WATCH (MB/min)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBaroHealthLeakSlope(
		TEXT("baro.Health.LeakMBPerMin"), 20.0f,
		TEXT("Memory growth slope that raises LEAK_SUSPECTED (MB/min)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBaroHealthResetJump(
		TEXT("baro.Health.ResetJumpMB"), 256.0f,
		TEXT("A one-sample positive memory jump treated as a workload transition (MB)."), ECVF_Default);
}

bool UBaroSystemMonitorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->WorldType == EWorldType::Game;
}

void UBaroSystemMonitorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StartSeconds = FPlatformTime::Seconds();
	LastSampleSeconds = StartSeconds - CVarBaroHealthSampleInterval.GetValueOnGameThread();
	LastLogSeconds = StartSeconds - CVarBaroHealthLogInterval.GetValueOnGameThread();

	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
	CsvLogPath = FPaths::Combine(FPaths::ProjectLogDir(), FString::Printf(TEXT("BaroHealth-%s.csv"), *Stamp));
	IFileManager::Get().MakeDirectory(*FPaths::ProjectLogDir(), true);

	const FString Header = TEXT("timestamp_utc,uptime_s,physical_mb,virtual_mb,cpu_pct,cpu_one_core_pct,gpu_pct,gpu_frame_ms,vram_used_mb,vram_budget_mb,fps,memory_slope_mb_min,trend_window_s,state") LINE_TERMINATOR;
	if (!FFileHelper::SaveStringToFile(Header, *CsvLogPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogBaroHealth, Warning, TEXT("[Health] CSV 생성 실패: %s"), *CsvLogPath);
	}
	UE_LOG(LogBaroHealth, Display, TEXT("[Health] 모니터 시작 sample=%.1fs log=%.1fs csv=%s"),
		CVarBaroHealthSampleInterval.GetValueOnGameThread(),
		CVarBaroHealthLogInterval.GetValueOnGameThread(), *CsvLogPath);
}

void UBaroSystemMonitorSubsystem::Deinitialize()
{
	const double Now = FPlatformTime::Seconds();
	TakeSample(Now);
	WritePeriodicRecord(Now, true);
	UE_LOG(LogBaroHealth, Display, TEXT("[Health] 모니터 종료 uptime=%.0fs"), LatestSnapshot.UptimeSeconds);

	MemoryTrend.Reset();
	Super::Deinitialize();
}

void UBaroSystemMonitorSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	const double SampleInterval = FMath::Max(0.25, static_cast<double>(CVarBaroHealthSampleInterval.GetValueOnGameThread()));
	if (Now - LastSampleSeconds < SampleInterval)
	{
		return;
	}

	LastSampleSeconds = Now;
	TakeSample(Now);
	WritePeriodicRecord(Now, false);
}

TStatId UBaroSystemMonitorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBaroSystemMonitorSubsystem, STATGROUP_Tickables);
}

void UBaroSystemMonitorSubsystem::TakeSample(double NowSeconds)
{
	LatestSnapshot.TimestampUtc = FDateTime::UtcNow().ToIso8601();
	LatestSnapshot.UptimeSeconds = static_cast<float>(NowSeconds - StartSeconds);

	const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
	LatestSnapshot.ProcessPhysicalMB = static_cast<float>(Memory.UsedPhysical * BytesToMB);
	LatestSnapshot.ProcessVirtualMB = static_cast<float>(Memory.UsedVirtual * BytesToMB);
	LatestSnapshot.TotalPhysicalMB = static_cast<float>(Memory.TotalPhysical * BytesToMB);

	const FCPUTime Cpu = FPlatformTime::GetCPUTime();
	LatestSnapshot.CpuPercent = Cpu.CPUTimePct;
	LatestSnapshot.CpuSingleCorePercent = Cpu.CPUTimePctRelative;

	LatestSnapshot.Fps = GetWorld() ? 1.0f / FMath::Max(GetWorld()->GetDeltaSeconds(), 0.0001f) : 0.0f;
	LatestSnapshot.GpuFrameMs = GDynamicRHI
		? FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles())
		: 0.0f;

	LatestSnapshot.bGpuUsageAvailable = GDynamicRHI && GRHISupportsGPUUsage && RHIGetGPUUsage != nullptr;
	LatestSnapshot.GpuPercent = 0.0f;
	if (LatestSnapshot.bGpuUsageAvailable)
	{
		const FRHIGPUUsageFractions Usage = RHIGetGPUUsage(0);
		LatestSnapshot.GpuPercent = FMath::Clamp(Usage.CurrentProcessMHz * 100.0f, 0.0f, 100.0f);
	}

	LatestSnapshot.VramUsedMB = 0.0f;
	LatestSnapshot.VramBudgetMB = 0.0f;
	if (GDynamicRHI)
	{
		FRHIMemoryStats RhiMemory;
		RHIGetMemoryStats(RhiMemory);
		if (RhiMemory.IsValid())
		{
			LatestSnapshot.VramUsedMB = static_cast<float>(RhiMemory.UsedLocal * BytesToMB);
			LatestSnapshot.VramBudgetMB = static_cast<float>(RhiMemory.BudgetLocal * BytesToMB);
		}
	}

	// 첫 SceneCapture/Lumen 활성화처럼 수 GB가 한 번에 할당되는 정상적인 workload 전환을
	// 연속 누수로 오인하지 않는다. 실제 결함의 +1~2 MB/s 증가는 이 임계값보다 훨씬 작아
	// 추세 창이 유지되고, 기본 60초 뒤 그대로 검출된다.
	if (MemoryTrend.Num() > 0)
	{
		const float JumpMB = LatestSnapshot.ProcessPhysicalMB - static_cast<float>(MemoryTrend.Last().PhysicalMB);
		if (JumpMB >= FMath::Max(64.0f, CVarBaroHealthResetJump.GetValueOnGameThread()))
		{
			UE_LOG(LogBaroHealth, Display,
				TEXT("[Health] workload transition ramJump=%+.1fMB — memory trend reset"), JumpMB);
			MemoryTrend.Reset();
		}
	}
	MemoryTrend.Add({NowSeconds, LatestSnapshot.ProcessPhysicalMB});
	UpdateMemoryTrend(NowSeconds);
}

void UBaroSystemMonitorSubsystem::UpdateMemoryTrend(double NowSeconds)
{
	const double TrendWindow = FMath::Max(30.0, static_cast<double>(CVarBaroHealthTrendWindow.GetValueOnGameThread()));
	const double Cutoff = NowSeconds - TrendWindow;
	int32 RemoveCount = 0;
	while (RemoveCount < MemoryTrend.Num() && MemoryTrend[RemoveCount].TimeSeconds < Cutoff)
	{
		++RemoveCount;
	}
	if (RemoveCount > 0)
	{
		MemoryTrend.RemoveAt(0, RemoveCount, EAllowShrinking::No);
	}

	LatestSnapshot.TrendWindowSeconds = MemoryTrend.Num() > 1
		? static_cast<float>(MemoryTrend.Last().TimeSeconds - MemoryTrend[0].TimeSeconds)
		: 0.0f;

	LatestSnapshot.MemorySlopeMBPerMinute = 0.0f;
	if (MemoryTrend.Num() >= 3)
	{
		double MeanTime = 0.0;
		double MeanMemory = 0.0;
		for (const FMemoryTrendPoint& Point : MemoryTrend)
		{
			MeanTime += Point.TimeSeconds;
			MeanMemory += Point.PhysicalMB;
		}
		MeanTime /= MemoryTrend.Num();
		MeanMemory /= MemoryTrend.Num();

		double TimeVariance = 0.0;
		double Covariance = 0.0;
		for (const FMemoryTrendPoint& Point : MemoryTrend)
		{
			const double Dt = Point.TimeSeconds - MeanTime;
			TimeVariance += Dt * Dt;
			Covariance += Dt * (Point.PhysicalMB - MeanMemory);
		}
		if (TimeVariance > UE_DOUBLE_SMALL_NUMBER)
		{
			LatestSnapshot.MemorySlopeMBPerMinute = static_cast<float>((Covariance / TimeVariance) * 60.0);
		}
	}

	const float MinimumTrendSeconds = FMath::Min(60.0f, static_cast<float>(TrendWindow * 0.5));
	if (LatestSnapshot.TrendWindowSeconds < MinimumTrendSeconds)
	{
		LatestSnapshot.HealthState = EBaroHealthState::WarmingUp;
	}
	else if (LatestSnapshot.MemorySlopeMBPerMinute >= CVarBaroHealthLeakSlope.GetValueOnGameThread())
	{
		LatestSnapshot.HealthState = EBaroHealthState::LeakSuspected;
	}
	else if (LatestSnapshot.MemorySlopeMBPerMinute >= CVarBaroHealthWatchSlope.GetValueOnGameThread())
	{
		LatestSnapshot.HealthState = EBaroHealthState::Watch;
	}
	else
	{
		LatestSnapshot.HealthState = EBaroHealthState::Healthy;
	}

	if (LatestSnapshot.HealthState != LastReportedState)
	{
		if (LatestSnapshot.HealthState == EBaroHealthState::LeakSuspected)
		{
			UE_LOG(LogBaroHealth, Error,
				TEXT("[Health] LEAK_SUSPECTED physical=%.1fMB slope=%+.1fMB/min window=%.0fs"),
				LatestSnapshot.ProcessPhysicalMB, LatestSnapshot.MemorySlopeMBPerMinute,
				LatestSnapshot.TrendWindowSeconds);
		}
		else
		{
			UE_LOG(LogBaroHealth, Display, TEXT("[Health] 상태 변경 %s -> %s (slope=%+.1fMB/min)"),
				HealthStateName(LastReportedState), HealthStateName(LatestSnapshot.HealthState),
				LatestSnapshot.MemorySlopeMBPerMinute);
		}
		LastReportedState = LatestSnapshot.HealthState;
	}
}

void UBaroSystemMonitorSubsystem::WritePeriodicRecord(double NowSeconds, bool bForce)
{
	const double LogInterval = FMath::Max(1.0, static_cast<double>(CVarBaroHealthLogInterval.GetValueOnGameThread()));
	if (!bForce && NowSeconds - LastLogSeconds < LogInterval)
	{
		return;
	}
	LastLogSeconds = NowSeconds;

	const float GpuPercentForLog = LatestSnapshot.bGpuUsageAvailable ? LatestSnapshot.GpuPercent : -1.0f;
	UE_LOG(LogBaroHealth, Display,
		TEXT("[Health] state=%s uptime=%.0fs ram=%.1fMB virtual=%.1fMB slope=%+.1fMB/min cpu=%.1f%% cpu1=%.1f%% gpu=%.1f%% gpuFrame=%.2fms vram=%.0f/%.0fMB fps=%.1f"),
		HealthStateName(LatestSnapshot.HealthState), LatestSnapshot.UptimeSeconds,
		LatestSnapshot.ProcessPhysicalMB, LatestSnapshot.ProcessVirtualMB,
		LatestSnapshot.MemorySlopeMBPerMinute, LatestSnapshot.CpuPercent,
		LatestSnapshot.CpuSingleCorePercent, GpuPercentForLog, LatestSnapshot.GpuFrameMs,
		LatestSnapshot.VramUsedMB, LatestSnapshot.VramBudgetMB, LatestSnapshot.Fps);

	const FString CsvLine = FString::Printf(
		TEXT("%s,%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%s") LINE_TERMINATOR,
		*LatestSnapshot.TimestampUtc, LatestSnapshot.UptimeSeconds,
		LatestSnapshot.ProcessPhysicalMB, LatestSnapshot.ProcessVirtualMB,
		LatestSnapshot.CpuPercent, LatestSnapshot.CpuSingleCorePercent,
		GpuPercentForLog, LatestSnapshot.GpuFrameMs,
		LatestSnapshot.VramUsedMB, LatestSnapshot.VramBudgetMB, LatestSnapshot.Fps,
		LatestSnapshot.MemorySlopeMBPerMinute, LatestSnapshot.TrendWindowSeconds,
		HealthStateName(LatestSnapshot.HealthState));

	if (!FFileHelper::SaveStringToFile(CsvLine, *CsvLogPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
	{
		UE_LOG(LogBaroHealth, Warning, TEXT("[Health] CSV append 실패: %s"), *CsvLogPath);
	}
}

const TCHAR* UBaroSystemMonitorSubsystem::HealthStateName(EBaroHealthState State)
{
	switch (State)
	{
	case EBaroHealthState::Healthy: return TEXT("HEALTHY");
	case EBaroHealthState::Watch: return TEXT("WATCH");
	case EBaroHealthState::LeakSuspected: return TEXT("LEAK_SUSPECTED");
	default: return TEXT("WARMING_UP");
	}
}
