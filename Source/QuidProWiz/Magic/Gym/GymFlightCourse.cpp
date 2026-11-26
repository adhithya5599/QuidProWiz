// Fill out your copyright notice in the Description page of Project Settings.

#include "GymFlightCourse.h"
#include "GymSubsystem.h"
#include "GymFacingTextComponent.h"
#include "Broom.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

namespace GymCourseTags
{
	static const FName Course(TEXT("GymCourse"));
	static const FName Mesh(TEXT("GymCourseMesh"));
	static const FName Trigger(TEXT("GymCourseTrigger"));

	static FName GateTag(int32 Index) { return FName(*FString::Printf(TEXT("GymGate%d"), Index)); }

	static int32 FindGateIndex(const UActorComponent* Component)
	{
		for (const FName& Tag : Component->ComponentTags)
		{
			const FString Text = Tag.ToString();
			if (Text.StartsWith(TEXT("GymGate")))
			{
				return FCString::Atoi(*Text.RightChop(7));
			}
		}
		return INDEX_NONE;
	}
}

AGymFlightCourse::AGymFlightCourse()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// A small starter course so a freshly placed actor is usable straight away.
	Gates = {
		FTransform(FVector(0.f, 0.f, 300.f)),
		FTransform(FVector(1500.f, 0.f, 500.f)),
		FTransform(FRotator(0.f, 90.f, 0.f), FVector(2500.f, 1000.f, 700.f)),
	};
}

void AGymFlightCourse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGymFlightCourse, GatesPassed);
	DOREPLIFETIME(AGymFlightCourse, bRunning);
	DOREPLIFETIME(AGymFlightCourse, LapStartServerTime);
	DOREPLIFETIME(AGymFlightCourse, LapCount);
	DOREPLIFETIME(AGymFlightCourse, LastLapSeconds);
	DOREPLIFETIME(AGymFlightCourse, BestLapSeconds);
}

// --- Layout --------------------------------------------------------------------------------------

template <typename T>
T* AGymFlightCourse::AddCourseComponent(USceneComponent* Parent)
{
	T* Component = NewObject<T>(this, NAME_None, RF_Transient);
	Component->CreationMethod = HasActorBegunPlay() || IsActorBeginningPlay()
		? EComponentCreationMethod::Instance
		: EComponentCreationMethod::UserConstructionScript;
	Component->ComponentTags.Add(GymCourseTags::Course);
	Component->SetupAttachment(Parent);
	Component->RegisterComponent();
	return Component;
}

void AGymFlightCourse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildGates();
}

void AGymFlightCourse::BuildGates()
{
	TInlineComponentArray<UActorComponent*> Existing(this);
	for (UActorComponent* Component : Existing)
	{
		if (Component && Component->ComponentHasTag(GymCourseTags::Course))
		{
			Component->DestroyComponent();
		}
	}

	for (int32 Index = 0; Index < Gates.Num(); ++Index)
	{
		const FName IndexTag = GymCourseTags::GateTag(Index);

		USceneComponent* Anchor = AddCourseComponent<USceneComponent>(RootComponent);
		Anchor->SetRelativeTransform(Gates[Index]);

		UStaticMeshComponent* Ring = AddCourseComponent<UStaticMeshComponent>(Anchor);
		Ring->ComponentTags.Append({ GymCourseTags::Mesh, IndexTag });
		Ring->SetStaticMesh(GateMesh);
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetCastShadow(false);

		UBoxComponent* Trigger = AddCourseComponent<UBoxComponent>(Anchor);
		Trigger->ComponentTags.Append({ GymCourseTags::Trigger, IndexTag });
		Trigger->SetBoxExtent(FVector(40.f, GateRadius, GateRadius));
		Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
		Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Trigger->SetGenerateOverlapEvents(true);

		UGymFacingTextComponent* Number = AddCourseComponent<UGymFacingTextComponent>(Anchor);
		Number->SetRelativeLocation(FVector(0.f, 0.f, GateRadius + 110.f));
		Number->SetText(FText::AsNumber(Index + 1));
		Number->SetWorldSize(110.f);
		Number->SetTextRenderColor(NumberColor.ToFColor(true));
	}

	GatherGates();
	UpdateGateVisuals();
}

void AGymFlightCourse::GatherGates()
{
	GateMeshes.Reset();
	GateTriggers.Reset();
	GateMeshes.SetNum(Gates.Num());
	GateTriggers.SetNum(Gates.Num());

	TInlineComponentArray<UActorComponent*> Components(this);
	for (UActorComponent* Component : Components)
	{
		const int32 Index = Component ? GymCourseTags::FindGateIndex(Component) : INDEX_NONE;
		if (!GateMeshes.IsValidIndex(Index)) continue;

		if (Component->ComponentHasTag(GymCourseTags::Mesh))
		{
			GateMeshes[Index] = Cast<UStaticMeshComponent>(Component);
		}
		else if (Component->ComponentHasTag(GymCourseTags::Trigger))
		{
			GateTriggers[Index] = Cast<UBoxComponent>(Component);
		}
	}
}

FTransform AGymFlightCourse::GetGateWorldTransform(int32 GateIndex) const
{
	return Gates.IsValidIndex(GateIndex) ? Gates[GateIndex] * GetActorTransform() : GetActorTransform();
}

// --- Play ----------------------------------------------------------------------------------------

void AGymFlightCourse::BeginPlay()
{
	Super::BeginPlay();

	GatherGates();
	const bool bMissing = GateTriggers.ContainsByPredicate([](const TObjectPtr<UBoxComponent>& Trigger) { return Trigger == nullptr; });
	if (bMissing && Gates.Num() > 0)
	{
		BuildGates();
	}

	if (HasAuthority())
	{
		for (UBoxComponent* Trigger : GateTriggers)
		{
			if (Trigger)
			{
				Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGymFlightCourse::OnGateOverlap);
			}
		}
	}

	if (UGymSubsystem* Gym = UGymSubsystem::Get(this))
	{
		Gym->RegisterFlightCourse(this);
	}

	UpdateGateVisuals();
}

void AGymFlightCourse::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGymSubsystem* Gym = UGymSubsystem::Get(this))
	{
		Gym->UnregisterFlightCourse(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AGymFlightCourse::OnGateOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ABroom* Broom = Cast<ABroom>(OtherActor))
	{
		NotifyGatePassed(GymCourseTags::FindGateIndex(OverlappedComponent), Broom);
	}
}

int32 AGymFlightCourse::GetNextGate() const
{
	if (!bRunning || Gates.Num() == 0) return 0;
	return GatesPassed >= Gates.Num() ? 0 : GatesPassed; // a full loop finishes back at gate 1
}

void AGymFlightCourse::NotifyGatePassed(int32 GateIndex, ABroom* Broom)
{
	if (!HasAuthority() || !Gates.IsValidIndex(GateIndex) || GateIndex != GetNextGate()) return;

	if (!bRunning)
	{
		StartLap();
	}
	else if (bLoop && GatesPassed >= Gates.Num())
	{
		FinishLap();
		StartLap(); // gate 1 is also the start of the next lap
	}
	else
	{
		++GatesPassed;
		if (!bLoop && GatesPassed >= Gates.Num())
		{
			FinishLap();
		}
	}

	UpdateGateVisuals();
}

void AGymFlightCourse::StartLap()
{
	bRunning = true;
	GatesPassed = 1;
	LapStartServerTime = GetServerTime();
}

void AGymFlightCourse::FinishLap()
{
	const float Seconds = GetServerTime() - LapStartServerTime;
	++LapCount;
	LastLapSeconds = Seconds;
	BestLapSeconds = BestLapSeconds > 0.f ? FMath::Min(BestLapSeconds, Seconds) : Seconds;
	bRunning = false;
	GatesPassed = 0;
	OnLap.Broadcast(Seconds);
}

void AGymFlightCourse::ResetRun()
{
	if (!HasAuthority()) return;

	bRunning = false;
	GatesPassed = 0;
	UpdateGateVisuals();
}

float AGymFlightCourse::GetServerTime() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : (World ? World->GetTimeSeconds() : 0.f);
}

float AGymFlightCourse::GetCurrentLapSeconds() const
{
	return bRunning ? FMath::Max(0.f, GetServerTime() - LapStartServerTime) : 0.f;
}

void AGymFlightCourse::OnRep_Progress()
{
	UpdateGateVisuals();
}

void AGymFlightCourse::UpdateGateVisuals()
{
	const int32 Next = GetNextGate();
	for (int32 Index = 0; Index < GateMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Ring = GateMeshes[Index];
		if (!Ring) continue;

		const bool bPassed = bRunning && Index < GatesPassed && Index != Next;
		UMaterialInterface* Material = Index == Next ? NextMaterial : (bPassed ? PassedMaterial : IdleMaterial);
		if (Material)
		{
			for (int32 Slot = 0; Slot < FMath::Max(1, Ring->GetNumMaterials()); ++Slot)
			{
				Ring->SetMaterial(Slot, Material);
			}
		}
	}
}
