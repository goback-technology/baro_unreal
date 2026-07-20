#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BaroSystemMonitorSubsystem.generated.h"

UENUM(BlueprintType)
enum class EBaroHealthState : uint8
{
	WarmingUp,
	Healthy,
	Watch,
	LeakSuspected
};

/** 한 번의 시스템 상태 샘플. WBP에서도 같은 데이터를 사용할 수 있다. */
USTRUCT(BlueprintType)
struct FBaroSystemSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	FString TimestampUtc;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float UptimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float ProcessPhysicalMB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float ProcessVirtualMB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float TotalPhysicalMB = 0.f;

	/** 전체 CPU 용량 기준 현재 프로세스 사용률(Task Manager 방식). */
	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float CpuPercent = 0.f;

	/** 단일 코어 하나를 100%로 본 현재 프로세스 사용률. */
	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float CpuSingleCorePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	bool bGpuUsageAvailable = false;

	/** RHI/드라이버가 지원할 때만 유효한 현재 프로세스 GPU 사용률. */
	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float GpuPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float GpuFrameMs = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float VramUsedMB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float VramBudgetMB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float Fps = 0.f;

	/** 최근 추세선의 분당 프로세스 물리 메모리 변화량. */
	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float MemorySlopeMBPerMinute = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	float TrendWindowSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Baro|Health")
	EBaroHealthState HealthState = EBaroHealthState::WarmingUp;
};

/**
 * UI 수명과 무관하게 시스템 상태를 수집하고 Saved/Logs에 CSV/UE 로그를 남긴다.
 * 기본값: 1초 샘플, 30초 기록, 120초 메모리 추세 분석.
 */
UCLASS()
class UBaroSystemMonitorSubsystem final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintPure, Category="Baro|Health")
	FBaroSystemSnapshot GetLatestSnapshot() const { return LatestSnapshot; }

	UFUNCTION(BlueprintPure, Category="Baro|Health")
	FString GetCsvLogPath() const { return CsvLogPath; }

private:
	struct FMemoryTrendPoint
	{
		double TimeSeconds = 0.0;
		double PhysicalMB = 0.0;
	};

	void TakeSample(double NowSeconds);
	void UpdateMemoryTrend(double NowSeconds);
	void WritePeriodicRecord(double NowSeconds, bool bForce);
	static const TCHAR* HealthStateName(EBaroHealthState State);

	UPROPERTY(Transient)
	FBaroSystemSnapshot LatestSnapshot;

	TArray<FMemoryTrendPoint> MemoryTrend;
	FString CsvLogPath;
	double StartSeconds = 0.0;
	double LastSampleSeconds = 0.0;
	double LastLogSeconds = 0.0;
	EBaroHealthState LastReportedState = EBaroHealthState::WarmingUp;
};
