// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GymArtGallery.generated.h"

class UGymArtGalleryDataAsset;
class UStaticMesh;
class UMaterialInterface;
class USceneComponent;

/**
 * Lays out the exhibits from a UGymArtGalleryDataAsset as a row (or grid) of pedestals with
 * turntables and labels. Built in OnConstruction, so the layout is visible in the editor and
 * updates as soon as the data asset changes (move the actor or press Refresh to rebuild).
 */
UCLASS()
class QUIDPROWIZ_API AGymArtGallery : public AActor
{
	GENERATED_BODY()

public:

	AGymArtGallery();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

	/** Number of exhibits currently laid out. */
	int32 GetNumBuiltExhibits() const;

	/** Rebuild the layout from the data asset. */
	UFUNCTION(CallInEditor, Category = "Gallery")
	void Refresh();

protected:

	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery")
	TObjectPtr<UGymArtGalleryDataAsset> Gallery;

	/** Distance between pedestals along the actor's +Y. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (ClampMin = "100"))
	float Spacing = 700.f;

	/** Pedestals per row before starting a new row behind (along -X). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (ClampMin = "1"))
	int32 ExhibitsPerRow = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (ClampMin = "100"))
	float RowSpacing = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Pedestal")
	TObjectPtr<UStaticMesh> PedestalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Pedestal")
	TObjectPtr<UMaterialInterface> PedestalMaterial;

	/** Pedestal size in cm (diameter X/Y, height Z). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Pedestal")
	FVector PedestalSize = FVector(220.f, 220.f, 120.f);

	/** Turntable speed in degrees per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery")
	float TurntableSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Labels")
	FLinearColor TitleColor = FLinearColor(1.f, 0.85f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Labels")
	FLinearColor DescriptionColor = FLinearColor(0.85f, 0.85f, 0.9f);

	/** Height of the labels above the pedestal top. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Labels")
	float LabelHeight = 330.f;

	/** Spin in the editor viewport too (handy while reviewing art). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery")
	bool bSpinInEditor = false;

	virtual bool ShouldTickIfViewportsOnly() const override { return bSpinInEditor; }

private:

	/** Pivots that spin. Gathered by tag, so it survives PIE duplication and reloads. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Turntables;

	void BuildExhibits();
	void DestroyExhibits();
	void GatherTurntables();

	template <typename T>
	T* AddExhibitComponent(USceneComponent* Parent);
};
