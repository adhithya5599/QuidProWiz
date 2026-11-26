// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GymSnitchTrainer.generated.h"

class ASnitch;
class ABroom;
class USnitchDataAsset;
class UBillboardComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPracticeCatch, float /*CatchSeconds*/);

/**
 * Gym station for Snitch practice. Spawns its own Snitch that doesn't decide the match, releases
 * it around this actor, times each catch and releases it again after a short pause.
 *
 * The server owns the Snitch and the timing; catch stats replicate so any player's panel can
 * show them.
 */
UCLASS()
class QUIDPROWIZ_API AGymSnitchTrainer : public AActor
{
	GENERATED_BODY()

public:

	AGymSnitchTrainer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: put the Snitch back and release it now (the clock restarts). */
	void RestartPractice();

	ASnitch* GetSnitch() const { return Snitch; }

	int32 GetCatchCount() const { return CatchCount; }
	float GetLastCatchSeconds() const { return LastCatchSeconds; }
	float GetBestCatchSeconds() const { return BestCatchSeconds; }

	/** Seconds since the current release, or 0 while waiting to release again. */
	float GetCurrentChaseSeconds() const;

	/** Raised on the server for each catch. */
	FOnPracticeCatch OnPracticeCatch;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Snitch to spawn (usually BP_Snitch for its mesh). */
	UPROPERTY(EditAnywhere, Category = "Snitch Practice")
	TSubclassOf<ASnitch> SnitchClass;

	/** Tuning for the practice Snitch, usually with a smaller arena than a match. */
	UPROPERTY(EditAnywhere, Category = "Snitch Practice")
	TObjectPtr<USnitchDataAsset> PracticeData;

	/** Pause between a catch and the next release. */
	UPROPERTY(EditAnywhere, Category = "Snitch Practice", meta = (ClampMin = "0"))
	float ReleaseDelay = 3.f;

	/** The Snitch appears this far above the trainer. */
	UPROPERTY(EditAnywhere, Category = "Snitch Practice")
	float SpawnHeight = 400.f;

	UPROPERTY(VisibleAnywhere, Category = "Snitch Practice")
	TObjectPtr<USceneComponent> Root;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif

private:

	UPROPERTY(Replicated)
	TObjectPtr<ASnitch> Snitch;

	UPROPERTY(Replicated)
	int32 CatchCount = 0;

	UPROPERTY(Replicated)
	float LastCatchSeconds = 0.f;

	UPROPERTY(Replicated)
	float BestCatchSeconds = 0.f;

	/** Server world time of the current release; negative while waiting. Replicated for the chase clock. */
	UPROPERTY(Replicated)
	float ReleaseServerTime = -1.f;

	FTimerHandle ReleaseTimerHandle;
	FDelegateHandle SnitchCaughtHandle;

	void SpawnSnitch();
	void ReleaseSnitch();
	void HandleSnitchCaught(ASnitch* CaughtSnitch, ABroom* Catcher);
	float GetServerTime() const;
};
