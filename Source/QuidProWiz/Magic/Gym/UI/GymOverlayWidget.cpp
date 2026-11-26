// Fill out your copyright notice in the Description page of Project Settings.

#include "GymOverlayWidget.h"
#include "GymTuningRowWidget.h"
#include "GymActionButtonWidget.h"
#include "GymSubsystem.h"
#include "GymSnitchTrainer.h"
#include "GymFlightCourse.h"
#include "UI/ZoneUIManager.h"
#include "QuidditchWorldSubsystem.h"
#include "QuidProWizGameStateBase.h"
#include "Broom.h"
#include "GoalRing.h"
#include "GoalTargetingComponent.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "Gym"

void UGymOverlayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &UGymOverlayWidget::HandlePossessedPawnChanged);
	}
}

void UGymOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToPawn(GetOwningPlayerPawn());

	if (HintText)
	{
		HintText->SetText(FText::Format(LOCTEXT("Hint", "{0}: cursor for the sliders / back to flying   {1}: hide panel"),
			CursorKey.GetDisplayName(), ToggleKey.GetDisplayName()));
	}

	// Show the current zone (the player may spawn inside one before the widget exists).
	const UZoneUIManager* ZoneUI = BoundZoneUI.Get();
	ShowZone(ZoneUI ? ZoneUI->GetCurrentZone() : EZoneType::None);
}

void UGymOverlayWidget::NativeDestruct()
{
	if (UZoneUIManager* ZoneUI = BoundZoneUI.Get())
	{
		ZoneUI->OnZoneChanged.RemoveDynamic(this, &UGymOverlayWidget::HandleZoneChanged);
	}
	BoundZoneUI.Reset();

	Super::NativeDestruct();
}

void UGymOverlayWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UGymOverlayWidget::BindToPawn(APawn* Pawn)
{
	UZoneUIManager* NewZoneUI = nullptr;
	if (const ABroom* Broom = Cast<ABroom>(Pawn))
	{
		NewZoneUI = Broom->GetZoneUI();
	}

	if (NewZoneUI == BoundZoneUI.Get()) return;

	if (UZoneUIManager* OldZoneUI = BoundZoneUI.Get())
	{
		OldZoneUI->OnZoneChanged.RemoveDynamic(this, &UGymOverlayWidget::HandleZoneChanged);
	}

	BoundZoneUI = NewZoneUI;
	if (NewZoneUI)
	{
		NewZoneUI->OnZoneChanged.AddUniqueDynamic(this, &UGymOverlayWidget::HandleZoneChanged);
	}
}

void UGymOverlayWidget::HandleZoneChanged(EZoneType NewZone)
{
	ShowZone(NewZone);
}

// --- Building the panel ----------------------------------------------------------------------

TArray<FGymActionEntry> UGymOverlayWidget::GetActionsFor(const FGymZoneInfo* Info) const
{
	TArray<FGymActionEntry> Actions;
	if (!Info) return Actions;

	Actions.Append(Info->Actions);

	// Every zone with sliders gets Reset (and Keep, in the editor) without listing them in data.
	if (Info->Tunables.Num() > 0)
	{
		FGymActionEntry Reset;
		Reset.Action = EGymAction::ResetTuning;
		Reset.Label = LOCTEXT("ResetValues", "Reset values");
		Actions.Add(Reset);

#if WITH_EDITOR
		FGymActionEntry Keep;
		Keep.Action = EGymAction::KeepTuning;
		Keep.Label = LOCTEXT("KeepValues", "Keep values");
		Actions.Add(Keep);
#endif
	}

	return Actions;
}

void UGymOverlayWidget::ShowZone(EZoneType Zone)
{
	ShownZone = Zone;
	const FGymZoneInfo* Info = GymData ? GymData->FindZone(Zone) : nullptr;

	if (ZoneTitle)
	{
		ZoneTitle->SetText(Info ? Info->Title : UEnum::GetDisplayValueAsText(Zone));
	}
	if (ZoneDescription)
	{
		ZoneDescription->SetText(Info ? Info->Description : FText::GetEmpty());
		ZoneDescription->SetVisibility(Info && !Info->Description.IsEmpty()
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (AccentBar && Info)
	{
		AccentBar->SetColorAndOpacity(Info->AccentColor);
	}

	// Sliders.
	Rows.Reset();
	if (TuningList)
	{
		TuningList->ClearChildren();
		if (Info && RowClass)
		{
			for (const FGymTunable& Tunable : Info->Tunables)
			{
				UGymTuningRowWidget* Row = CreateWidget<UGymTuningRowWidget>(this, RowClass);
				if (!Row) continue;
				TuningList->AddChild(Row);
				Row->Setup(Tunable);
				Rows.Add(Row);
			}
		}
		TuningList->SetVisibility(Rows.Num() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Buttons.
	if (ActionList)
	{
		ActionList->ClearChildren();
		const TArray<FGymActionEntry> Actions = GetActionsFor(Info);
		if (ButtonClass)
		{
			for (const FGymActionEntry& Entry : Actions)
			{
				UGymActionButtonWidget* Button = CreateWidget<UGymActionButtonWidget>(this, ButtonClass);
				if (!Button) continue;
				ActionList->AddChild(Button);
				Button->Setup(Entry);
				Button->OnActionClicked.BindUObject(this, &UGymOverlayWidget::HandleActionClicked);
			}
		}
		ActionList->SetVisibility(Actions.Num() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	TimeUntilStats = 0.f;
}

void UGymOverlayWidget::RefreshRows()
{
	for (UGymTuningRowWidget* Row : Rows)
	{
		if (Row)
		{
			Row->RefreshFromTarget();
		}
	}
}

void UGymOverlayWidget::HandleActionClicked(const FGymActionEntry& Entry)
{
	UGymSubsystem* Gym = UGymSubsystem::Get(this);
	if (!Gym) return;

	ShowStatus(Gym->ExecuteAction(Entry));

	// Values may have changed under the sliders (Reset, AI preset).
	RefreshRows();
}

void UGymOverlayWidget::ShowStatus(const FText& Message)
{
	if (!StatusText) return;

	StatusText->SetText(Message);
	StatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	StatusTimeLeft = 4.f;
}

void UGymOverlayWidget::SetCursorMode(bool bInCursorMode)
{
	bCursorMode = bInCursorMode;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	PC->SetShowMouseCursor(bCursorMode);
	if (bCursorMode)
	{
		// Cursor for the panel; the keyboard still flies the broom.
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
	else
	{
		// Mouse captured: it steers the broom.
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UGymOverlayWidget::SetCollapsed(bool bInCollapsed)
{
	bCollapsed = bInCollapsed;
	if (Body)
	{
		Body->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

// --- Live stats ------------------------------------------------------------------------------

void UGymOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Start in flying mode once the level has finished its own setup (it may show the cursor).
	if (!bAppliedInitialMode)
	{
		bAppliedInitialMode = true;
		SetCursorMode(false);
	}

	// Read through the player's input so the keys work while flying (the panel never takes focus).
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (PC->WasInputKeyJustPressed(ToggleKey))
		{
			SetCollapsed(!bCollapsed);
		}
		if (PC->WasInputKeyJustPressed(CursorKey))
		{
			SetCursorMode(!bCursorMode);
		}
	}

	// Late possession (the pawn can arrive after the widget is created).
	if (!BoundZoneUI.IsValid())
	{
		BindToPawn(GetOwningPlayerPawn());
	}

	if (StatusTimeLeft > 0.f)
	{
		StatusTimeLeft -= InDeltaTime;
		if (StatusTimeLeft <= 0.f && StatusText)
		{
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	TimeUntilStats -= InDeltaTime;
	if (TimeUntilStats > 0.f || !StatsText) return;
	TimeUntilStats = StatsInterval;

	const FText Stats = BuildStats();
	StatsText->SetText(Stats);
	StatsText->SetVisibility(Stats.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

FText UGymOverlayWidget::BuildStats() const
{
	const ABroom* Broom = Cast<ABroom>(GetOwningPlayerPawn());
	const UWorld* World = GetWorld();
	if (!World) return FText::GetEmpty();

	FNumberFormattingOptions Whole;
	Whole.MaximumFractionalDigits = 0;
	FNumberFormattingOptions OneDecimal;
	OneDecimal.MinimumFractionalDigits = 1;
	OneDecimal.MaximumFractionalDigits = 1;

	const AQuidProWizGameStateBase* GameState = World->GetGameState<AQuidProWizGameStateBase>();

	switch (ShownZone)
	{
	case EZoneType::BroomMovement:
	case EZoneType::None:
		if (Broom)
		{
			const FText Flight = FText::Format(LOCTEXT("MoveStats", "Speed {0} / {1} cm/s\nHeight {2} cm"),
				FText::AsNumber(Broom->GetCurrentSpeed(), &Whole),
				FText::AsNumber(Broom->GetCurrentMaxSpeed(), &Whole),
				FText::AsNumber(Broom->GetActorLocation().Z, &Whole));

			const UGymSubsystem* Gym = UGymSubsystem::Get(this);
			const AGymFlightCourse* Course = Gym ? Gym->GetFlightCourse() : nullptr;
			if (ShownZone != EZoneType::BroomMovement || !Course || Course->GetNumGates() == 0)
			{
				return Flight;
			}

			auto Seconds = [&OneDecimal](float Value) { return FText::Format(LOCTEXT("SecsC", "{0} s"), FText::AsNumber(Value, &OneDecimal)); };
			const FText Lap = Course->IsRunning()
				? FText::Format(LOCTEXT("LapRunning", "Lap: gate {0}/{1}  {2}"), Course->GetNextGate() + 1, Course->GetNumGates(), Seconds(Course->GetCurrentLapSeconds()))
				: LOCTEXT("LapIdle", "Lap: fly through gate 1 to start");
			const FText Times = Course->GetLapCount() > 0
				? FText::Format(LOCTEXT("LapTimes", "Last {0}   Best {1}   Laps {2}"), Seconds(Course->GetLastLapSeconds()), Seconds(Course->GetBestLapSeconds()), Course->GetLapCount())
				: FText::GetEmpty();

			return Times.IsEmpty()
				? FText::Format(LOCTEXT("MoveLap", "{0}\n{1}"), Flight, Lap)
				: FText::Format(LOCTEXT("MoveLapTimes", "{0}\n{1}\n{2}"), Flight, Lap, Times);
		}
		break;

	case EZoneType::QuaffleAndGoals:
		if (Broom)
		{
			const AGoalRing* Target = Broom->GetGoalTargeting() ? Broom->GetGoalTargeting()->GetCurrentTargetGoal() : nullptr;
			return FText::Format(LOCTEXT("QuaffleStats", "Quaffle: {0}\nAim assist: {1}"),
				Broom->IsHoldingQuaffle() ? LOCTEXT("Holding", "holding") : LOCTEXT("NotHolding", "not holding"),
				Target ? FText::Format(LOCTEXT("RingWorth", "ring worth {0}"), Target->GetScoreValue()) : LOCTEXT("NoTarget", "no ring in range"));
		}
		break;

	case EZoneType::Bludger:
		if (Broom)
		{
			float Distance = 0.f;
			const UQuidditchWorldSubsystem* Quidditch = UQuidditchWorldSubsystem::Get(this);
			const bool bHasBludger = Quidditch && Quidditch->FindNearestBludger(Broom->GetActorLocation(), &Distance);
			return FText::Format(LOCTEXT("BludgerStats", "Nearest Bludger: {0}\nStunned: {1}"),
				bHasBludger ? FText::Format(LOCTEXT("Cm", "{0} cm"), FText::AsNumber(Distance, &Whole)) : LOCTEXT("None", "none"),
				Broom->IsStunned() ? LOCTEXT("Yes", "yes") : LOCTEXT("No", "no"));
		}
		break;

	case EZoneType::FullMatch:
		if (GameState)
		{
			return FText::Format(LOCTEXT("MatchStats", "{0}\nTeam A {1}  -  Team B {2}\nSnitch: {3}"),
				UEnum::GetDisplayValueAsText(GameState->GetMatchPhase()),
				GameState->GetTeamAScore(), GameState->GetTeamBScore(),
				GameState->IsSnitchReleased() ? LOCTEXT("Released", "released") : LOCTEXT("NotYet", "not released"));
		}
		break;

	case EZoneType::Snitch:
		if (const UGymSubsystem* Gym = UGymSubsystem::Get(this))
		{
			if (const AGymSnitchTrainer* Trainer = Gym->GetSnitchTrainer())
			{
				const float Chase = Trainer->GetCurrentChaseSeconds();
				return FText::Format(LOCTEXT("SnitchStats", "Chase: {0}\nLast catch: {1}   Best: {2}\nCatches: {3}"),
					Chase > 0.f ? FText::Format(LOCTEXT("Secs", "{0} s"), FText::AsNumber(Chase, &OneDecimal)) : LOCTEXT("Waiting", "next release soon"),
					Trainer->GetCatchCount() > 0 ? FText::Format(LOCTEXT("Secs2", "{0} s"), FText::AsNumber(Trainer->GetLastCatchSeconds(), &OneDecimal)) : LOCTEXT("Dash", "-"),
					Trainer->GetCatchCount() > 0 ? FText::Format(LOCTEXT("Secs3", "{0} s"), FText::AsNumber(Trainer->GetBestCatchSeconds(), &OneDecimal)) : LOCTEXT("Dash2", "-"),
					Trainer->GetCatchCount());
			}
			return LOCTEXT("NoTrainer", "No Snitch trainer in this level.");
		}
		break;

	case EZoneType::Art:
		break;
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
