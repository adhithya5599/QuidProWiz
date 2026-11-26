// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "QuidditchTestWorld.h"
#include "QuidditchWorldSubsystem.h"
#include "QuidProWizGameStateBase.h"
#include "GymSubsystem.h"
#include "GymSnitchTrainer.h"
#include "GymArtGallery.h"
#include "GymFlightCourse.h"
#include "Data/GymDataAsset.h"
#include "Data/GymArtGalleryDataAsset.h"
#include "Broom.h"
#include "Quaffle.h"
#include "Bludger.h"
#include "Snitch.h"
#include "AIBroomController.h"
#include "DataAsset/BludgerDataAsset.h"
#include "DataAsset/SnitchDataAsset.h"
#include "DataAsset/BroomAIDataAsset.h"
#include "Engine/StaticMesh.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The gym: live tuning through reflection (and putting assets back afterwards), the panel's
 * commands, the Snitch practice station and the art gallery layout.
 */
BEGIN_DEFINE_SPEC(FQuidditchGymSpec, "QuidProWiz.Gym",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	TUniquePtr<FQuidditchTestWorld> TestWorld;

	UGymSubsystem* Gym() const { return UGymSubsystem::Get(TestWorld->GetWorld()); }

	static FGymTunable MakeTunable(EGymTuningTarget Target, FName Property, float Min, float Max)
	{
		FGymTunable Tunable;
		Tunable.Target = Target;
		Tunable.PropertyName = Property;
		Tunable.Min = Min;
		Tunable.Max = Max;
		return Tunable;
	}

	static FGymActionEntry MakeAction(EGymAction Action)
	{
		FGymActionEntry Entry;
		Entry.Action = Action;
		return Entry;
	}

	AGymSnitchTrainer* SpawnTrainer(const FVector& Location, USnitchDataAsset* PracticeData)
	{
		const FTransform Transform(Location);
		AGymSnitchTrainer* Trainer = TestWorld->GetWorld()->SpawnActorDeferred<AGymSnitchTrainer>(
			AGymSnitchTrainer::StaticClass(), Transform);
		FQuidditchTestWorld::SetObjectProperty(Trainer, TEXT("PracticeData"), PracticeData);
		FQuidditchTestWorld::SetPropertyValue(Trainer, TEXT("ReleaseDelay"), 1.f);
		Trainer->FinishSpawning(Transform);
		return Trainer;
	}

	/** A course with gates at Points, each facing the next one. */
	AGymFlightCourse* SpawnCourse(const TArray<FVector>& Points, bool bLoop)
	{
		TArray<FTransform> GateTransforms;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector Next = Points[(Index + 1) % Points.Num()];
			const FVector Facing = (Index + 1 < Points.Num() || bLoop) ? Next - Points[Index] : Points[Index] - Points[Index - 1];
			GateTransforms.Add(FTransform(FRotator(0.f, Facing.Rotation().Yaw, 0.f), Points[Index]));
		}

		const FTransform Transform = FTransform::Identity;
		AGymFlightCourse* Course = TestWorld->GetWorld()->SpawnActorDeferred<AGymFlightCourse>(AGymFlightCourse::StaticClass(), Transform);
		FQuidditchTestWorld::SetPropertyValue(Course, TEXT("Gates"), GateTransforms);
		FQuidditchTestWorld::SetPropertyValue(Course, TEXT("bLoop"), bLoop);
		Course->FinishSpawning(Transform);
		return Course;
	}

	static USnitchDataAsset* MakePracticeData()
	{
		USnitchDataAsset* Data = NewObject<USnitchDataAsset>();
		Data->ArenaRadius = 1000.f;
		Data->MinHeightOffset = 200.f;
		Data->MaxHeightOffset = 800.f;
		return Data;
	}
END_DEFINE_SPEC(FQuidditchGymSpec)

void FQuidditchGymSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = MakeUnique<FQuidditchTestWorld>();
	});

	AfterEach([this]()
	{
		TestWorld.Reset();
	});

	Describe("Tuning", [this]()
	{
		It("writes a live value that the object reads straight away", [this]()
		{
			ABludger* Bludger = TestWorld->SpawnBludger(FVector(0.f, 0.f, 500.f));
			const FGymTunable Tunable = MakeTunable(EGymTuningTarget::Bludger, TEXT("ChaseSpeed"), 100.f, 2000.f);

			TestTrue(TEXT("Set succeeds"), Gym()->SetTunableValue(Tunable, 1234.f));
			TestEqual(TEXT("Data asset changed"), Bludger->GetBludgerData()->ChaseSpeed, 1234.f);

			float Read = 0.f;
			TestTrue(TEXT("Get succeeds"), Gym()->GetTunableValue(Tunable, Read));
			TestEqual(TEXT("Reads back"), Read, 1234.f);
		});

		It("clamps to the slider range", [this]()
		{
			ABludger* Bludger = TestWorld->SpawnBludger(FVector(0.f, 0.f, 500.f));
			Gym()->SetTunableValue(MakeTunable(EGymTuningTarget::Bludger, TEXT("ChaseSpeed"), 100.f, 2000.f), 99999.f);
			TestEqual(TEXT("Clamped to max"), Bludger->GetBludgerData()->ChaseSpeed, 2000.f);
		});

		It("puts every changed value back on Reset", [this]()
		{
			ABludger* Bludger = TestWorld->SpawnBludger(FVector(0.f, 0.f, 500.f));
			const float Original = Bludger->GetBludgerData()->ChaseSpeed;
			const FGymTunable Tunable = MakeTunable(EGymTuningTarget::Bludger, TEXT("ChaseSpeed"), 100.f, 2000.f);

			Gym()->SetTunableValue(Tunable, 1500.f);
			Gym()->SetTunableValue(Tunable, 1700.f); // the first original is the one kept
			TestEqual(TEXT("One value changed"), Gym()->GetNumChangedValues(), 1);

			Gym()->ResetTuning();
			TestEqual(TEXT("Back to the original"), Bludger->GetBludgerData()->ChaseSpeed, Original);
			TestEqual(TEXT("Nothing left to reset"), Gym()->GetNumChangedValues(), 0);
		});

		It("puts shared data assets back when the world ends", [this]()
		{
			// Like an editor asset: it outlives the play session.
			TStrongObjectPtr<UBludgerDataAsset> SharedData(NewObject<UBludgerDataAsset>(GetTransientPackage()));
			const float Original = SharedData->ChaseSpeed;

			ABludger* Bludger = TestWorld->SpawnBludger(FVector(0.f, 0.f, 500.f));
			FQuidditchTestWorld::SetObjectProperty(Bludger, TEXT("BludgerDataAsset"), SharedData.Get());

			Gym()->SetTunableValue(MakeTunable(EGymTuningTarget::Bludger, TEXT("ChaseSpeed"), 100.f, 2000.f), 1900.f);
			TestEqual(TEXT("Changed during play"), SharedData->ChaseSpeed, 1900.f);

			TestWorld.Reset();
			TestEqual(TEXT("Restored after play"), SharedData->ChaseSpeed, Original);
		});

		It("rounds integer properties", [this]()
		{
			AQuidProWizGameStateBase* GameState = TestWorld->SpawnGameState(/*ScoreToWin*/ 0, /*Countdown*/ 1);
			Gym()->SetTunableValue(MakeTunable(EGymTuningTarget::GameState, TEXT("ScoreToWin"), 0.f, 300.f), 50.4f);
			TestEqual(TEXT("Integer written"), GameState->GetScoreToWin(), 50);
		});

		It("fails cleanly for a missing target or property", [this]()
		{
			TestWorld->SpawnBludger(FVector(0.f, 0.f, 500.f));
			float Value = 0.f;
			TestFalse(TEXT("Unknown property"), Gym()->GetTunableValue(MakeTunable(EGymTuningTarget::Bludger, TEXT("NotAProperty"), 0.f, 1.f), Value));
			TestFalse(TEXT("No Quaffle in the level"), Gym()->SetTunableValue(MakeTunable(EGymTuningTarget::Quaffle, TEXT("PickupRadius"), 0.f, 1.f), 0.5f));
			TestEqual(TEXT("Nothing recorded"), Gym()->GetNumChangedValues(), 0);
		});
	});

	Describe("Commands", [this]()
	{
		It("sends Quaffles back to their spawn points", [this]()
		{
			const FVector Spawn(100.f, 200.f, 300.f);
			AQuaffle* Quaffle = TestWorld->SpawnQuaffle(Spawn);
			Quaffle->SetActorLocation(FVector(3000.f, 0.f, 50.f));

			Gym()->ExecuteAction(MakeAction(EGymAction::ResetQuaffles));
			TestEqual(TEXT("Back at spawn"), Quaffle->GetActorLocation(), Spawn, 1.f);
			TestTrue(TEXT("Free to pick up"), Quaffle->CanBePickedUp());
		});

		It("sends Bludgers back to their spawn points", [this]()
		{
			const FVector Spawn(-500.f, 0.f, 400.f);
			ABludger* Bludger = TestWorld->SpawnBludger(Spawn);
			TestWorld->Tick(1.f); // let it wander off
			Bludger->SetActorLocation(FVector(2000.f, 2000.f, 900.f));

			Gym()->ExecuteAction(MakeAction(EGymAction::ResetBludgers));
			TestEqual(TEXT("Back at spawn"), Bludger->GetActorLocation(), Spawn, 1.f);
		});

		It("applies an AI difficulty preset to every AI broom", [this]()
		{
			ABroom* Bot = TestWorld->SpawnBroom(FVector(0.f, 0.f, 500.f));
			TestWorld->Possess(Bot);
			const AAIBroomController* AI = Cast<AAIBroomController>(Bot->GetController());
			if (!TestNotNull(TEXT("Bot has an AI controller"), AI)) return;

			UBroomAIDataAsset* Hard = NewObject<UBroomAIDataAsset>();
			FGymActionEntry Entry = MakeAction(EGymAction::ApplyAIPreset);
			Entry.AIPreset = Hard;
			Gym()->ExecuteAction(Entry);

			TestTrue(TEXT("Preset applied"), AI->GetAIData() == Hard);
		});
	});

	Describe("Snitch practice", [this]()
	{
		It("releases a practice Snitch that doesn't decide the match", [this]()
		{
			AQuidProWizGameStateBase* GameState = TestWorld->SpawnGameState(0, 1);
			TestWorld->Tick(1.1f); // countdown

			AGymSnitchTrainer* Trainer = SpawnTrainer(FVector(0.f, 0.f, 100.f), MakePracticeData());
			ASnitch* Snitch = Trainer->GetSnitch();
			if (!TestNotNull(TEXT("Trainer spawned a Snitch"), Snitch)) return;

			TestTrue(TEXT("Released straight away"), Snitch->IsActive());
			TestFalse(TEXT("Practice Snitch"), Snitch->EndsMatchWhenCaught());
			TestTrue(TEXT("Registered with the gym"), Gym()->GetSnitchTrainer() == Trainer);

			TestWorld->SpawnBroom(Snitch->GetActorLocation());
			TestTrue(TEXT("Caught"), TestWorld->TickUntil(0.5f, [Trainer]() { return Trainer->GetCatchCount() == 1; }));
			TestTrue(TEXT("Match still running"), GameState->GetMatchPhase() == EMatchPhase::InProgress);
			TestFalse(TEXT("Match Snitch not released"), GameState->IsSnitchReleased());
		});

		It("times each catch and releases the Snitch again", [this]()
		{
			AGymSnitchTrainer* Trainer = SpawnTrainer(FVector(0.f, 0.f, 100.f), MakePracticeData());
			ASnitch* Snitch = Trainer->GetSnitch();
			if (!TestNotNull(TEXT("Trainer spawned a Snitch"), Snitch)) return;

			TestWorld->Tick(0.5f);
			ABroom* Broom = TestWorld->SpawnBroom(Snitch->GetActorLocation());
			TestWorld->TickUntil(0.5f, [Trainer]() { return Trainer->GetCatchCount() == 1; });

			TestTrue(TEXT("Catch time recorded"), Trainer->GetLastCatchSeconds() >= 0.5f);
			TestEqual(TEXT("Best equals the only catch"), Trainer->GetBestCatchSeconds(), Trainer->GetLastCatchSeconds());
			TestTrue(TEXT("Waiting after the catch"), Snitch->GetSnitchState() == ESnitchState::Caught);

			Broom->SetActorLocation(FVector(5000.f, 5000.f, 500.f)); // out of the way
			TestTrue(TEXT("Released again after the delay"),
				TestWorld->TickUntil(1.5f, [Snitch]() { return Snitch->IsActive(); }));
		});

		It("is skipped when the match releases its own Snitch", [this]()
		{
			AQuidProWizGameStateBase* GameState = TestWorld->SpawnGameState(0, 1);
			FQuidditchTestWorld::SetPropertyValue(GameState, TEXT("SnitchReleaseTimeMax"), 0.f);
			TestWorld->SpawnGoal(FVector(-2000.f, 0.f, 500.f), FRotator::ZeroRotator, EGoalTeam::TeamA);
			AGymSnitchTrainer* Trainer = SpawnTrainer(FVector(0.f, 3000.f, 100.f), MakePracticeData());
			TestWorld->Tick(1.1f);

			GameState->ReleaseSnitch();

			const TArray<ASnitch*> Snitches = TestWorld->GetSubsystem()->GetSnitches();
			TestEqual(TEXT("A separate match Snitch was spawned"), Snitches.Num(), 2);
			for (const ASnitch* Snitch : Snitches)
			{
				TestTrue(TEXT("Both flying"), Snitch->IsActive());
			}
			TestTrue(TEXT("Released by the match"), GameState->IsSnitchReleased());
			TestFalse(TEXT("Trainer kept its practice Snitch"), Trainer->GetSnitch()->EndsMatchWhenCaught());
		});
	});

	Describe("Flight course", [this]()
	{
		It("times a lap through the gates in order and ignores gates out of order", [this]()
		{
			AGymFlightCourse* Course = SpawnCourse({ FVector(1000.f, 0.f, 500.f), FVector(2000.f, 0.f, 500.f), FVector(3000.f, 0.f, 500.f) }, /*bLoop*/ false);
			ABroom* Broom = TestWorld->SpawnBroom(FVector(0.f, 3000.f, 500.f));

			auto FlyThrough = [this, Broom, Course](int32 Gate)
			{
				Broom->SetActorLocation(FVector(0.f, 3000.f, 500.f));
				TestWorld->Tick(0.05f);
				Broom->SetActorLocation(Course->GetGateWorldTransform(Gate).GetLocation());
				TestWorld->Tick(0.05f);
			};

			FlyThrough(1);
			TestFalse(TEXT("Gate 2 first doesn't start a lap"), Course->IsRunning());

			FlyThrough(0);
			TestTrue(TEXT("Gate 1 starts the lap"), Course->IsRunning());
			TestEqual(TEXT("Next is gate 2"), Course->GetNextGate(), 1);

			FlyThrough(2);
			TestEqual(TEXT("Skipping a gate doesn't count"), Course->GetNextGate(), 1);

			TestWorld->Tick(1.f);
			FlyThrough(1);
			FlyThrough(2);

			TestEqual(TEXT("One lap"), Course->GetLapCount(), 1);
			TestFalse(TEXT("Lap over"), Course->IsRunning());
			AddInfo(FString::Printf(TEXT("Lap time %.2fs"), Course->GetLastLapSeconds()));
			TestTrue(TEXT("Lap time recorded"), Course->GetLastLapSeconds() > 1.f);
			TestEqual(TEXT("Best is the only lap"), Course->GetBestLapSeconds(), Course->GetLastLapSeconds());
		});

		It("can be flown by a broom steering with the player controls", [this]()
		{
			// A loop with right-angle and climbing/diving turns, like the gym course.
			AGymFlightCourse* Course = SpawnCourse({
				FVector(0.f, 0.f, 400.f), FVector(2000.f, 0.f, 800.f), FVector(2600.f, 1800.f, 400.f),
				FVector(1000.f, 2600.f, 1200.f), FVector(-800.f, 1500.f, 600.f) }, /*bLoop*/ true);

			ABroom* Broom = TestWorld->SpawnBroom(FVector(-1500.f, 0.f, 400.f));
			TestWorld->Possess(Broom);
			AController* Controller = Broom->GetController();

			// Steer like a player: point the aim at the next gate and hold full throttle.
			const bool bFinished = TestWorld->TickUntil(40.f, [Course, Broom, Controller]()
			{
				const FVector Target = Course->GetGateWorldTransform(Course->GetNextGate()).GetLocation();
				Controller->SetControlRotation((Target - Broom->GetActorLocation()).Rotation());
				Broom->PerformMove(1.f);
				return Course->GetLapCount() >= 1;
			});

			AddInfo(FString::Printf(TEXT("Lap finished: %s, time %.1fs"), bFinished ? TEXT("yes") : TEXT("no"), Course->GetLastLapSeconds()));
			TestTrue(TEXT("Completed a lap"), bFinished);
			TestTrue(TEXT("At a sensible pace"), Course->GetLastLapSeconds() < 20.f);
		});

		It("resets a lap from the panel command", [this]()
		{
			AGymFlightCourse* Course = SpawnCourse({ FVector(1000.f, 0.f, 500.f), FVector(2000.f, 0.f, 500.f) }, true);
			Course->NotifyGatePassed(0, nullptr);
			TestTrue(TEXT("Running"), Course->IsRunning());

			Gym()->ExecuteAction(MakeAction(EGymAction::ResetFlightCourse));
			TestFalse(TEXT("Reset"), Course->IsRunning());
			TestEqual(TEXT("Back to gate 1"), Course->GetNextGate(), 0);
		});
	});

	Describe("Art gallery", [this]()
	{
		It("puts each exhibit on a spinning pedestal", [this]()
		{
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UGymArtGalleryDataAsset* Data = NewObject<UGymArtGalleryDataAsset>();
			for (int32 Index = 0; Index < 3; ++Index)
			{
				FGymExhibit& Exhibit = Data->Exhibits.AddDefaulted_GetRef();
				Exhibit.Title = FText::FromString(FString::Printf(TEXT("Exhibit %d"), Index));
				Exhibit.StaticMesh = Cube;
			}

			const FTransform Transform(FVector(0.f, 0.f, 0.f));
			AGymArtGallery* Gallery = TestWorld->GetWorld()->SpawnActorDeferred<AGymArtGallery>(AGymArtGallery::StaticClass(), Transform);
			FQuidditchTestWorld::SetObjectProperty(Gallery, TEXT("Gallery"), Data);
			Gallery->FinishSpawning(Transform);

			TestEqual(TEXT("Three exhibits"), Gallery->GetNumBuiltExhibits(), 3);

			TArray<USceneComponent*> Pivots;
			TInlineComponentArray<USceneComponent*> Components(Gallery);
			for (USceneComponent* Component : Components)
			{
				if (Component->ComponentHasTag(TEXT("GymTurntable"))) Pivots.Add(Component);
			}
			if (!TestEqual(TEXT("Three turntables"), Pivots.Num(), 3)) return;

			const float StartYaw = Pivots[0]->GetRelativeRotation().Yaw;
			TestWorld->Tick(1.f);
			const float Turned = FMath::Abs(FMath::FindDeltaAngleDegrees(StartYaw, Pivots[0]->GetRelativeRotation().Yaw));
			AddInfo(FString::Printf(TEXT("Turned %.1f degrees in 1s"), Turned));
			TestTrue(TEXT("Turntable spins (30 deg/s)"), FMath::IsNearlyEqual(Turned, 30.f, 3.f));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
