// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GymArtGalleryDataAsset.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UAnimationAsset;
class UMaterialInterface;

/** One exhibit: a static mesh, or a skeletal mesh with an optional looping animation. */
USTRUCT(BlueprintType)
struct FGymExhibit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit", meta = (MultiLine = "true"))
	FText Description;

	/** Shown when set (takes priority over SkeletalMesh). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Looped on the skeletal mesh, e.g. animations made in Blender. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	TObjectPtr<UAnimationAsset> Animation;

	/** Optional material override for every slot (e.g. to preview a new material). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	FVector DisplayScale = FVector::OneVector;

	/** Extra lift above the pedestal top (meshes whose pivot is at their centre need some). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	float DisplayHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	FRotator DisplayRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhibit")
	bool bTurntable = true;
};

/**
 * Contents of the gym's art gallery. Adding a model to the gallery is one new entry here;
 * AGymArtGallery lays the exhibits out on pedestals.
 */
UCLASS(BlueprintType)
class QUIDPROWIZ_API UGymArtGalleryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (TitleProperty = "Title"))
	TArray<FGymExhibit> Exhibits;
};
