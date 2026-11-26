// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZoneType.h"
#include "Data/GymDataAsset.h"
#include "GymOverlayWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UBorder;
class UImage;
class UGymTuningRowWidget;
class UGymActionButtonWidget;
class UZoneUIManager;
class APawn;

/**
 * The gym's side panel. Follows the player's zone (via UZoneUIManager), shows that zone's
 * title, help text, live stats, sliders and buttons from UGymDataAsset, and runs the buttons
 * through UGymSubsystem. All logic is here; WBP_GymPanel is layout and style only.
 *
 * Two modes: flying (mouse steers the broom, no cursor) and tuning (cursor shown to use the
 * sliders; keyboard flight still works). CursorKey (Tab) switches; HideKey (H) collapses the
 * panel to its header.
 */
UCLASS(Abstract)
class QUIDPROWIZ_API UGymOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Show a zone's panel (called automatically when the player enters a zone). */
	UFUNCTION(BlueprintCallable, Category = "Gym")
	void ShowZone(EZoneType Zone);

	UFUNCTION(BlueprintCallable, Category = "Gym")
	void SetCollapsed(bool bInCollapsed);

	UFUNCTION(BlueprintPure, Category = "Gym")
	bool IsCollapsed() const { return bCollapsed; }

	/** Cursor shown for the panel (true) or captured for flying (false). */
	UFUNCTION(BlueprintCallable, Category = "Gym")
	void SetCursorMode(bool bInCursorMode);

	EZoneType GetShownZone() const { return ShownZone; }
	int32 GetNumRows() const { return Rows.Num(); }

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	TObjectPtr<UGymDataAsset> GymData;

	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	TSubclassOf<UGymTuningRowWidget> RowClass;

	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	TSubclassOf<UGymActionButtonWidget> ButtonClass;

	/** Key that collapses/expands the panel. */
	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	FKey ToggleKey = EKeys::H;

	/** Key that switches between flying (mouse steers) and using the panel (cursor shown). */
	UPROPERTY(EditDefaultsOnly, Category = "Gym")
	FKey CursorKey = EKeys::Tab;

	/** How often the stats line refreshes. */
	UPROPERTY(EditDefaultsOnly, Category = "Gym", meta = (ClampMin = "0.02"))
	float StatsInterval = 0.1f;

	// --- Bound widgets (names must match in WBP_GymOverlay) ----------------------------------

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneTitle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneDescription;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TuningList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ActionList;

	/** Everything below the header; hidden when collapsed. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> Body;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Tinted with the zone's accent colour. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AccentBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

private:

	EZoneType ShownZone = EZoneType::None;
	bool bCollapsed = false;
	bool bCursorMode = false;
	bool bAppliedInitialMode = false;
	float TimeUntilStats = 0.f;
	float StatusTimeLeft = 0.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGymTuningRowWidget>> Rows;

	TWeakObjectPtr<UZoneUIManager> BoundZoneUI;

	UFUNCTION()
	void HandleZoneChanged(EZoneType NewZone);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindToPawn(APawn* Pawn);
	void HandleActionClicked(const FGymActionEntry& Entry);
	void RefreshRows();
	void ShowStatus(const FText& Message);
	FText BuildStats() const;
	TArray<FGymActionEntry> GetActionsFor(const FGymZoneInfo* Info) const;
};
