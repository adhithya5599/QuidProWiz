// Fill out your copyright notice in the Description page of Project Settings.

#include "GymSign.h"
#include "GymFacingTextComponent.h"

AGymSign::AGymSign()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TitleText = CreateDefaultSubobject<UGymFacingTextComponent>(TEXT("TitleText"));
	TitleText->SetupAttachment(RootComponent);

	SubtitleText = CreateDefaultSubobject<UGymFacingTextComponent>(TEXT("SubtitleText"));
	SubtitleText->SetupAttachment(RootComponent);
}

void AGymSign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TitleText->SetText(Title);
	TitleText->SetTextRenderColor(TitleColor.ToFColor(true));
	TitleText->SetWorldSize(TitleSize);

	SubtitleText->SetText(Subtitle);
	SubtitleText->SetTextRenderColor(SubtitleColor.ToFColor(true));
	SubtitleText->SetWorldSize(SubtitleSize);
	SubtitleText->SetRelativeLocation(FVector(0.f, 0.f, -(TitleSize * 0.5f + SubtitleSize)));
	SubtitleText->SetVisibility(!Subtitle.IsEmpty());
}
