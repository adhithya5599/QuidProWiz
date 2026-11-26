// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GymSign.generated.h"

class UGymFacingTextComponent;

/** A floating gym label (title plus optional subtitle) that always faces the player. */
UCLASS()
class QUIDPROWIZ_API AGymSign : public AActor
{
	GENERATED_BODY()

public:

	AGymSign();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign", meta = (MultiLine = "true"))
	FText Subtitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign")
	FLinearColor TitleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign")
	FLinearColor SubtitleColor = FLinearColor(0.8f, 0.8f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign", meta = (ClampMin = "1"))
	float TitleSize = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sign", meta = (ClampMin = "1"))
	float SubtitleSize = 50.f;

	UPROPERTY(VisibleAnywhere, Category = "Sign")
	TObjectPtr<UGymFacingTextComponent> TitleText;

	UPROPERTY(VisibleAnywhere, Category = "Sign")
	TObjectPtr<UGymFacingTextComponent> SubtitleText;
};
