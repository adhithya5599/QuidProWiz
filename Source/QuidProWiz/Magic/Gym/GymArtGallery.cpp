// Fill out your copyright notice in the Description page of Project Settings.

#include "GymArtGallery.h"
#include "Data/GymArtGalleryDataAsset.h"
#include "GymFacingTextComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "UObject/ConstructorHelpers.h"

namespace GymGalleryTags
{
	static const FName Exhibit(TEXT("GymExhibit"));
	static const FName Turntable(TEXT("GymTurntable"));
}

AGymArtGallery::AGymArtGallery()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		PedestalMesh = Cylinder.Object;
	}
}

void AGymArtGallery::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildExhibits();
}

void AGymArtGallery::BeginPlay()
{
	Super::BeginPlay();

	// Cooked or duplicated actors may arrive without the construction-time components.
	if (GetNumBuiltExhibits() == 0)
	{
		BuildExhibits();
	}
	GatherTurntables();
}

void AGymArtGallery::Refresh()
{
	BuildExhibits();
}

int32 AGymArtGallery::GetNumBuiltExhibits() const
{
	TInlineComponentArray<USceneComponent*> Components(this);
	int32 Count = 0;
	for (const USceneComponent* Component : Components)
	{
		if (Component->ComponentHasTag(GymGalleryTags::Exhibit) && Component->GetAttachParent() == RootComponent)
		{
			++Count;
		}
	}
	return Count;
}

template <typename T>
T* AGymArtGallery::AddExhibitComponent(USceneComponent* Parent)
{
	T* Component = NewObject<T>(this, NAME_None, RF_Transient);
	// Construction-time components are cleaned up and rebuilt whenever the construction script
	// reruns; ones built during play are ordinary instance components.
	Component->CreationMethod = HasActorBegunPlay() || IsActorBeginningPlay()
		? EComponentCreationMethod::Instance
		: EComponentCreationMethod::UserConstructionScript;
	Component->ComponentTags.Add(GymGalleryTags::Exhibit);
	Component->SetupAttachment(Parent);
	Component->RegisterComponent();
	return Component;
}

void AGymArtGallery::DestroyExhibits()
{
	TInlineComponentArray<UActorComponent*> Components(this);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->ComponentHasTag(GymGalleryTags::Exhibit))
		{
			Component->DestroyComponent();
		}
	}
	Turntables.Reset();
}

void AGymArtGallery::BuildExhibits()
{
	DestroyExhibits();
	if (!Gallery) return;

	const int32 PerRow = FMath::Max(1, ExhibitsPerRow);
	const float PedestalHeight = PedestalSize.Z;

	for (int32 Index = 0; Index < Gallery->Exhibits.Num(); ++Index)
	{
		const FGymExhibit& Exhibit = Gallery->Exhibits[Index];
		const int32 Column = Index % PerRow;
		const int32 Row = Index / PerRow;

		// Centre each row on the actor along Y; further rows step back along -X.
		const int32 InThisRow = FMath::Min(PerRow, Gallery->Exhibits.Num() - Row * PerRow);
		const float Y = (Column - (InThisRow - 1) * 0.5f) * Spacing;
		const FVector BaseLocation(-Row * RowSpacing, Y, 0.f);

		// Exhibit anchor (everything for this exhibit hangs off it).
		USceneComponent* Anchor = AddExhibitComponent<USceneComponent>(RootComponent);
		Anchor->SetRelativeLocation(BaseLocation);

		// Pedestal: engine cylinder is 100 cm wide and tall, pivot in the middle.
		UStaticMeshComponent* Pedestal = AddExhibitComponent<UStaticMeshComponent>(Anchor);
		Pedestal->SetStaticMesh(PedestalMesh);
		Pedestal->SetRelativeLocation(FVector(0.f, 0.f, PedestalHeight * 0.5f));
		Pedestal->SetRelativeScale3D(PedestalSize / 100.f);
		Pedestal->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Pedestal->SetCollisionResponseToAllChannels(ECR_Block);
		if (PedestalMaterial)
		{
			Pedestal->SetMaterial(0, PedestalMaterial);
		}

		// Spinning pivot on top of the pedestal.
		USceneComponent* Pivot = AddExhibitComponent<USceneComponent>(Anchor);
		Pivot->SetRelativeLocation(FVector(0.f, 0.f, PedestalHeight + Exhibit.DisplayHeight));
		if (Exhibit.bTurntable)
		{
			Pivot->ComponentTags.Add(GymGalleryTags::Turntable);
		}

		if (Exhibit.StaticMesh)
		{
			UStaticMeshComponent* Display = AddExhibitComponent<UStaticMeshComponent>(Pivot);
			Display->SetStaticMesh(Exhibit.StaticMesh);
			Display->SetRelativeRotation(Exhibit.DisplayRotation);
			Display->SetRelativeScale3D(Exhibit.DisplayScale);
			Display->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (Exhibit.MaterialOverride)
			{
				for (int32 Slot = 0; Slot < Display->GetNumMaterials(); ++Slot)
				{
					Display->SetMaterial(Slot, Exhibit.MaterialOverride);
				}
			}
		}
		else if (Exhibit.SkeletalMesh)
		{
			USkeletalMeshComponent* Display = AddExhibitComponent<USkeletalMeshComponent>(Pivot);
			Display->SetSkeletalMesh(Exhibit.SkeletalMesh);
			Display->SetRelativeRotation(Exhibit.DisplayRotation);
			Display->SetRelativeScale3D(Exhibit.DisplayScale);
			Display->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (Exhibit.MaterialOverride)
			{
				for (int32 Slot = 0; Slot < Display->GetNumMaterials(); ++Slot)
				{
					Display->SetMaterial(Slot, Exhibit.MaterialOverride);
				}
			}
			if (Exhibit.Animation)
			{
				Display->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				Display->PlayAnimation(Exhibit.Animation, true);
			}
		}

		// Labels.
		UGymFacingTextComponent* Title = AddExhibitComponent<UGymFacingTextComponent>(Anchor);
		Title->SetRelativeLocation(FVector(0.f, 0.f, PedestalHeight + LabelHeight));
		Title->SetText(Exhibit.Title);
		Title->SetWorldSize(55.f);
		Title->SetTextRenderColor(TitleColor.ToFColor(true));

		if (!Exhibit.Description.IsEmpty())
		{
			UGymFacingTextComponent* Description = AddExhibitComponent<UGymFacingTextComponent>(Anchor);
			Description->SetRelativeLocation(FVector(0.f, 0.f, PedestalHeight + LabelHeight - 55.f));
			Description->SetText(Exhibit.Description);
			Description->SetWorldSize(28.f);
			Description->SetTextRenderColor(DescriptionColor.ToFColor(true));
		}
	}

	GatherTurntables();
}

void AGymArtGallery::GatherTurntables()
{
	Turntables.Reset();

	TInlineComponentArray<USceneComponent*> Components(this);
	for (USceneComponent* Component : Components)
	{
		if (Component && Component->ComponentHasTag(GymGalleryTags::Turntable))
		{
			Turntables.Add(Component);
		}
	}
}

void AGymArtGallery::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FMath::IsNearlyZero(TurntableSpeed)) return;

	for (USceneComponent* Pivot : Turntables)
	{
		if (Pivot)
		{
			Pivot->AddLocalRotation(FRotator(0.f, TurntableSpeed * DeltaTime, 0.f));
		}
	}
}
