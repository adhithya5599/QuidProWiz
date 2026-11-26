// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GymFlightCourse.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UBoxComponent;
class UStaticMeshComponent;
class ABroom;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCourseLap, float /*LapSeconds*/);

/**
 * A timed ring course for testing flight. Gates are flown in order; the next gate glows. Flying
 * through gate 1 starts the clock, and the lap ends back at gate 1 (or at the last gate when
 * the course isn't a loop), which immediately starts the next lap.
 *
 * Gates are edited in the level as transforms (drag the diamond widgets); the actor builds the
 * ring meshes, triggers and numbers in OnConstruction. The server judges the gates and the
 * times; progress and times replicate so the panel can show them.
 */
UCLASS()
class QUIDPROWIZ_API AGymFlightCourse : public AActor
{
	GENERATED_BODY()

public:

	AGymFlightCourse();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: abandon the current lap. */
	void ResetRun();

	/** Server: a broom flew through gate GateIndex (normally called by the gate triggers). */
	void NotifyGatePassed(int32 GateIndex, ABroom* Broom);

	int32 GetNumGates() const { return Gates.Num(); }

	/** Index of the gate to fly through next. */
	int32 GetNextGate() const;

	/** Gates passed on the current lap (0 when not running). */
	int32 GetGatesPassed() const { return bRunning ? GatesPassed : 0; }

	bool IsRunning() const { return bRunning; }
	int32 GetLapCount() const { return LapCount; }
	float GetLastLapSeconds() const { return LastLapSeconds; }
	float GetBestLapSeconds() const { return BestLapSeconds; }
	float GetCurrentLapSeconds() const;

	/** World transform of a gate (its +X is the flying direction). */
	FTransform GetGateWorldTransform(int32 GateIndex) const;

	/** Raised on the server when a lap is completed. */
	FOnCourseLap OnLap;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Gate placements relative to this actor; +X of each is the flying direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course", meta = (MakeEditWidget))
	TArray<FTransform> Gates;

	/** The lap finishes back at the first gate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course")
	bool bLoop = true;

	/** Ring mesh, modelled facing +X with the opening at its centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look")
	TObjectPtr<UStaticMesh> GateMesh;

	/** Inner radius of the opening (cm, at scale 1); the trigger fills it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look", meta = (ClampMin = "10"))
	float GateRadius = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look")
	TObjectPtr<UMaterialInterface> IdleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look")
	TObjectPtr<UMaterialInterface> NextMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look")
	TObjectPtr<UMaterialInterface> PassedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course|Look")
	FLinearColor NumberColor = FLinearColor(1.f, 0.85f, 0.35f);

private:

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 GatesPassed = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	bool bRunning = false;

	UPROPERTY(Replicated)
	float LapStartServerTime = 0.f;

	UPROPERTY(Replicated)
	int32 LapCount = 0;

	UPROPERTY(Replicated)
	float LastLapSeconds = 0.f;

	UPROPERTY(Replicated)
	float BestLapSeconds = 0.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GateMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> GateTriggers;

	UFUNCTION()
	void OnRep_Progress();

	UFUNCTION()
	void OnGateOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void BuildGates();
	void GatherGates();
	void UpdateGateVisuals();
	void StartLap();
	void FinishLap();
	float GetServerTime() const;

	template <typename T>
	T* AddCourseComponent(USceneComponent* Parent);
};
