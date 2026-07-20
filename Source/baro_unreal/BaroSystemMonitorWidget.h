#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaroSystemMonitorWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UVerticalBox;

/**
 * 패키지에 바로 포함되는 네이티브 UMG 상태 패널.
 * 추후 WBP 자식 클래스로 교체할 수 있도록 Blueprintable로 둔다.
 */
UCLASS(Blueprintable)
class UBaroSystemMonitorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildNativeLayout();
	void RefreshValues();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CpuValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GpuValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MemoryValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VramValueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FooterText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> CpuBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> GpuBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> MemoryBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> VramBar;

	float RefreshAccumulator = 0.f;
};
