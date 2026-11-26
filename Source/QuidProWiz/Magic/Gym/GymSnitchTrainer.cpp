// Fill out your copyright notice in the Description page of Project Settings.

#include "GymSnitchTrainer.h"
#include "GymSubsystem.h"
#include "Snitch.h"
#include "QuidditchWorldSubsystem.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AGymSnitchTrainer::AGymSnitchTrainer()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	if (Billboard)
	{
		Billboard->SetupAttachment(Root);
	}
#endif

	SnitchClass = ASnitch::StaticClass();
}

void AGymSnitchTrainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGymSnitchTrainer, Snitch);
	DOREPLIFETIME(AGymSnitchTrainer, CatchCount);
	DOREPLIFETIME(AGymSnitchTrainer, LastCatchSeconds);
	DOREPLIFETIME(AGymSnitchTrainer, BestCatchSeconds);
	DOREPLIFETIME(AGymSnitchTrainer, ReleaseServerTime);
}

void AGymSnitchTrainer::BeginPlay()
{
	Super::BeginPlay();

	if (UGymSubsystem* Gym = UGymSubsystem::Get(this))
	{
		Gym->RegisterSnitchTrainer(this);
	}

	if (!HasAuthority()) return;

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		SnitchCaughtHandle = Subsystem->OnSnitchCaught.AddUObject(this, &AGymSnitchTrainer::HandleSnitchCaught);
	}

	SpawnSnitch();
	ReleaseSnitch();
}

void AGymSnitchTrainer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGymSubsystem* Gym = UGymSubsystem::Get(this))
	{
		Gym->UnregisterSnitchTrainer(this);
	}

	if (UQuidditchWorldSubsystem* Subsystem = UQuidditchWorldSubsystem::Get(this))
	{
		Subsystem->OnSnitchCaught.Remove(SnitchCaughtHandle);
	}

	GetWorldTimerManager().ClearTimer(ReleaseTimerHandle);

	// The world tears its own actors down; only clean up when the trainer alone is removed.
	if (EndPlayReason == EEndPlayReason::Destroyed && HasAuthority() && IsValid(Snitch))
	{
		Snitch->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

void AGymSnitchTrainer::SpawnSnitch()
{
	if (!SnitchClass) return;

	// Deferred so the practice settings are in place before the Snitch's BeginPlay.
	const FTransform SpawnTransform(GetActorLocation() + FVector(0.f, 0.f, SpawnHeight));
	Snitch = GetWorld()->SpawnActorDeferred<ASnitch>(SnitchClass, SpawnTransform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Snitch) return;

	Snitch->SetEndsMatchWhenCaught(false);
	if (PracticeData)
	{
		Snitch->SetSnitchData(PracticeData);
	}
	Snitch->FinishSpawning(SpawnTransform);
}

void AGymSnitchTrainer::ReleaseSnitch()
{
	if (!HasAuthority() || !IsValid(Snitch)) return;

	GetWorldTimerManager().ClearTimer(ReleaseTimerHandle);
	Snitch->ResetToSpawn();
	Snitch->Release(GetActorLocation());
	ReleaseServerTime = GetServerTime();
}

void AGymSnitchTrainer::RestartPractice()
{
	ReleaseSnitch();
}

void AGymSnitchTrainer::HandleSnitchCaught(ASnitch* CaughtSnitch, ABroom* Catcher)
{
	if (!HasAuthority() || CaughtSnitch != Snitch || ReleaseServerTime < 0.f) return;

	const float Seconds = GetServerTime() - ReleaseServerTime;
	ReleaseServerTime = -1.f;

	++CatchCount;
	LastCatchSeconds = Seconds;
	BestCatchSeconds = BestCatchSeconds > 0.f ? FMath::Min(BestCatchSeconds, Seconds) : Seconds;
	OnPracticeCatch.Broadcast(Seconds);

	GetWorldTimerManager().SetTimer(ReleaseTimerHandle, this, &AGymSnitchTrainer::ReleaseSnitch,
		FMath::Max(ReleaseDelay, KINDA_SMALL_NUMBER), false);
}

float AGymSnitchTrainer::GetServerTime() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : GetWorld()->GetTimeSeconds();
}

float AGymSnitchTrainer::GetCurrentChaseSeconds() const
{
	return ReleaseServerTime >= 0.f ? FMath::Max(0.f, GetServerTime() - ReleaseServerTime) : 0.f;
}
