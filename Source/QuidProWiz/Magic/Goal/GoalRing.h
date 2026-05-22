// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DataAsset/GoalRingDataAsset.h"
#include "GoalRing.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoalScored, AGoalRing*, int32);

class UBoxComponent;
class UStaticMeshComponent;
class AQuaffle;
class USceneComponent;

UENUM(BlueprintType)
enum class EGoalTeam : uint8
{
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B")
};

UCLASS()
class QUIDPROWIZ_API AGoalRing : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGoalRing();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FVector GetAimDirection() const;
	bool IsGoalEnabled() const { return bGoalEnabled; }

	void SetLocallyHighlighted(bool bHighlighted);

	static const TArray<TWeakObjectPtr<AGoalRing>>& GetRegisteredGoals()
	{
		PurgeStaleGoals();
		return RegisteredGoals;
	}

	static FOnGoalScored OnGoalScored;

	static void PurgeStaleGoals();

	EGoalTeam GetOwningTeam() const { return OwningTeam; }

	int32 GetScoreOverride() const { return ScoreOverride; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	UStaticMeshComponent* RingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	UBoxComponent* LockOnVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	UBoxComponent* ScoreVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal | Components")
	USceneComponent* AimDirectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	bool bGoalEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Data")
	UGoalRingDataAsset* GoalDataAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	EGoalTeam OwningTeam = EGoalTeam::TeamA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goal | Gameplay")
	int32 ScoreOverride = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnScoreVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult);

	bool IsValidGoalScore(AQuaffle* Quaffle) const;
	void RegisterScore(AQuaffle* Quaffle);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	bool bLocallyHighlighted = false;

	static TArray<TWeakObjectPtr<AGoalRing>> RegisteredGoals;

	float PulseTimer = 0.f;
	bool bIsPulsing = false;

	UPROPERTY(EditAnywhere, Category = "Goal | Highlight")
	float PulseSpeed = 3.f;

	UPROPERTY(EditAnywhere, Category = "Goal | Highlight")
	float PulseIntensity = 0.3f;

	void UpdatePulse(float DeltaTime);

	void TriggerScoreFlash();
	void ResetAfterScoreFlash();

	FTimerHandle ScoreFlashTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Goal | Highlight")
	float ScoreFlashDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Goal | Highlight")
	UMaterialInterface* ScoreFlashMaterial = nullptr;
};
