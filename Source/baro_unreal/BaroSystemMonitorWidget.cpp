#include "BaroSystemMonitorWidget.h"

#include "BaroSystemMonitorSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor PanelBackground(0.012f, 0.024f, 0.036f, 0.94f);
	const FLinearColor TextPrimary(0.90f, 0.95f, 1.00f, 1.0f);
	const FLinearColor TextMuted(0.46f, 0.58f, 0.68f, 1.0f);
	const FLinearColor HealthyColor(0.20f, 0.88f, 0.56f, 1.0f);
	const FLinearColor WatchColor(1.00f, 0.72f, 0.20f, 1.0f);
	const FLinearColor DangerColor(1.00f, 0.25f, 0.25f, 1.0f);
	const FLinearColor AccentColor(0.16f, 0.68f, 1.00f, 1.0f);

	FString MemoryText(float Megabytes)
	{
		return Megabytes >= 1024.0f
			? FString::Printf(TEXT("%.2f GB"), Megabytes / 1024.0f)
			: FString::Printf(TEXT("%.0f MB"), Megabytes);
	}

	FString UptimeText(float Seconds)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));
		const int32 Hours = TotalSeconds / 3600;
		const int32 Minutes = (TotalSeconds / 60) % 60;
		const int32 RemainingSeconds = TotalSeconds % 60;
		return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, RemainingSeconds);
	}

	FLinearColor LoadColor(float Fraction)
	{
		return Fraction >= 0.90f ? DangerColor : (Fraction >= 0.70f ? WatchColor : AccentColor);
	}
}

bool UBaroSystemMonitorWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	// WBP 자식이 자체 WidgetTree를 제공하면 그 디자인을 존중한다.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildNativeLayout();
	}
	return true;
}

void UBaroSystemMonitorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshValues();
}

void UBaroSystemMonitorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= 0.5f)
	{
		RefreshAccumulator = 0.f;
		RefreshValues();
	}
}

void UBaroSystemMonitorWidget::BuildNativeLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MonitorRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MonitorPanel"));
	Panel->SetBrushColor(PanelBackground);
	Panel->SetPadding(FMargin(18.f, 15.f, 18.f, 14.f));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(-24.f, 24.f));
	PanelSlot->SetSize(FVector2D(430.f, 354.f));
	PanelSlot->SetZOrder(100);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MonitorColumn"));
	Panel->SetContent(Column);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(Header);
	HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("SYSTEM HEALTH")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 17));
	Title->SetColorAndOpacity(FSlateColor(TextPrimary));
	UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title);
	FSlateChildSize FillSize;
	FillSize.SizeRule = ESlateSizeRule::Fill;
	TitleSlot->SetSize(FillSize);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("수집 중")));
	StatusText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
	StatusText->SetColorAndOpacity(FSlateColor(WatchColor));
	UHorizontalBoxSlot* StatusSlot = Header->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetHorizontalAlignment(HAlign_Right);
	StatusSlot->SetVerticalAlignment(VAlign_Center);

	auto AddMetric = [this, Column, FillSize](const TCHAR* Name, const TCHAR* Label,
		TObjectPtr<UTextBlock>& OutValue, TObjectPtr<UProgressBar>& OutBar)
	{
		UVerticalBox* Metric = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("%sMetric"), Name)));
		UVerticalBoxSlot* MetricSlot = Column->AddChildToVerticalBox(Metric);
		MetricSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 9.f));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("%sRow"), Name)));
		Metric->AddChildToVerticalBox(Row);

		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%sLabel"), Name)));
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
		LabelText->SetColorAndOpacity(FSlateColor(TextMuted));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
		LabelSlot->SetSize(FillSize);

		OutValue = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("%sValue"), Name)));
		OutValue->SetText(FText::FromString(TEXT("--")));
		OutValue->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
		OutValue->SetColorAndOpacity(FSlateColor(TextPrimary));
		UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue);
		ValueSlot->SetHorizontalAlignment(HAlign_Right);

		OutBar = WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), FName(*FString::Printf(TEXT("%sBar"), Name)));
		OutBar->SetPercent(0.f);
		OutBar->SetFillColorAndOpacity(AccentColor);
		UVerticalBoxSlot* BarSlot = Metric->AddChildToVerticalBox(OutBar);
		BarSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	};

	AddMetric(TEXT("Cpu"), TEXT("CPU PROCESS"), CpuValueText, CpuBar);
	AddMetric(TEXT("Gpu"), TEXT("GPU PROCESS / FRAME"), GpuValueText, GpuBar);
	AddMetric(TEXT("Memory"), TEXT("PROCESS MEMORY / TREND"), MemoryValueText, MemoryBar);
	AddMetric(TEXT("Vram"), TEXT("VRAM / DRIVER BUDGET"), VramValueText, VramBar);

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailText"));
	DetailText->SetText(FText::FromString(TEXT("FPS --")));
	DetailText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
	DetailText->SetColorAndOpacity(FSlateColor(TextMuted));
	UVerticalBoxSlot* DetailSlot = Column->AddChildToVerticalBox(DetailText);
	DetailSlot->SetPadding(FMargin(0.f, 1.f, 0.f, 6.f));

	FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FooterText"));
	FooterText->SetText(FText::FromString(TEXT("1s sample · 30s CSV/UE log")));
	FooterText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
	FooterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.42f, 0.50f, 1.f)));
	Column->AddChildToVerticalBox(FooterText);
}

void UBaroSystemMonitorWidget::RefreshValues()
{
	if (!StatusText || !CpuValueText || !GpuValueText || !MemoryValueText || !VramValueText ||
		!CpuBar || !GpuBar || !MemoryBar || !VramBar || !DetailText || !FooterText)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UBaroSystemMonitorSubsystem* Monitor = World ? World->GetSubsystem<UBaroSystemMonitorSubsystem>() : nullptr;
	if (!Monitor)
	{
		return;
	}

	const FBaroSystemSnapshot S = Monitor->GetLatestSnapshot();
	FString StateLabel;
	FLinearColor StateColor;
	switch (S.HealthState)
	{
	case EBaroHealthState::Healthy:
		StateLabel = TEXT("안정");
		StateColor = HealthyColor;
		break;
	case EBaroHealthState::Watch:
		StateLabel = TEXT("관찰");
		StateColor = WatchColor;
		break;
	case EBaroHealthState::LeakSuspected:
		StateLabel = TEXT("누수 의심");
		StateColor = DangerColor;
		break;
	default:
		StateLabel = TEXT("추세 수집 중");
		StateColor = WatchColor;
		break;
	}
	StatusText->SetText(FText::FromString(StateLabel));
	StatusText->SetColorAndOpacity(FSlateColor(StateColor));

	const float CpuFraction = FMath::Clamp(S.CpuPercent / 100.f, 0.f, 1.f);
	CpuValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f%%  ·  1 core %.0f%%"),
		S.CpuPercent, S.CpuSingleCorePercent)));
	CpuBar->SetPercent(CpuFraction);
	CpuBar->SetFillColorAndOpacity(LoadColor(CpuFraction));

	const float GpuFraction = S.bGpuUsageAvailable
		? FMath::Clamp(S.GpuPercent / 100.f, 0.f, 1.f)
		: FMath::Clamp(S.GpuFrameMs / 16.667f, 0.f, 1.f);
	GpuValueText->SetText(FText::FromString(S.bGpuUsageAvailable
		? FString::Printf(TEXT("%.1f%%  ·  %.2f ms"), S.GpuPercent, S.GpuFrameMs)
		: FString::Printf(TEXT("N/A  ·  %.2f ms"), S.GpuFrameMs)));
	GpuBar->SetPercent(GpuFraction);
	GpuBar->SetFillColorAndOpacity(LoadColor(GpuFraction));

	const float RamFraction = S.TotalPhysicalMB > 0.f
		? FMath::Clamp(S.ProcessPhysicalMB / S.TotalPhysicalMB, 0.f, 1.f)
		: 0.f;
	MemoryValueText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  %+.1f MB/min"),
		*MemoryText(S.ProcessPhysicalMB), S.MemorySlopeMBPerMinute)));
	MemoryBar->SetPercent(RamFraction);
	MemoryBar->SetFillColorAndOpacity(StateColor);

	const float VramFraction = S.VramBudgetMB > 0.f
		? FMath::Clamp(S.VramUsedMB / S.VramBudgetMB, 0.f, 1.f)
		: 0.f;
	VramValueText->SetText(FText::FromString(S.VramBudgetMB > 0.f
		? FString::Printf(TEXT("%s / %s"), *MemoryText(S.VramUsedMB), *MemoryText(S.VramBudgetMB))
		: TEXT("N/A")));
	VramBar->SetPercent(VramFraction);
	VramBar->SetFillColorAndOpacity(LoadColor(VramFraction));

	DetailText->SetText(FText::FromString(FString::Printf(
		TEXT("FPS %.1f  ·  trend %.0fs  ·  virtual %s"),
		S.Fps, S.TrendWindowSeconds, *MemoryText(S.ProcessVirtualMB))));
	FooterText->SetText(FText::FromString(FString::Printf(
		TEXT("1s sample · 30s CSV/UE log · uptime %s"), *UptimeText(S.UptimeSeconds))));
}
