// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/FireplaceActor.h"
#include "Actors/WoodLog.h"
#include "Components/SurvivalComponent.h"
#include "Engine/Engine.h"

AFireplaceActor::AFireplaceActor()
{
	PrimaryActorTick.bCanEverTick = true;

	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	
	FireplaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireplaceMesh"));
	FireplaceMesh->SetupAttachment(RootComponent);

	// Wood log mesh components
	WoodLog1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WoodLog1"));
	WoodLog1->SetupAttachment(FireplaceMesh);

	WoodLog2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WoodLog2"));
	WoodLog2->SetupAttachment(FireplaceMesh);

	WoodLog3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WoodLog3"));
	WoodLog3->SetupAttachment(FireplaceMesh);

	// Fireplace detection sphere (for log placement)
	FireplaceDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("FireplaceDetectionSphere"));
	FireplaceDetectionSphere->SetupAttachment(FireplaceMesh);
	FireplaceDetectionSphere->SetSphereRadius(100.0f);
	FireplaceDetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// Heat zone sphere (for warmth when fireplace complete)
	HeatZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HeatZoneSphere"));
	HeatZoneSphere->SetupAttachment(FireplaceMesh);
	HeatZoneSphere->SetSphereRadius(150.0f);
	HeatZoneSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// Sheltered zone box (for shelter)
	ShelteredZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ShelteredZoneBox"));
	ShelteredZoneBox->SetupAttachment(FireplaceMesh);
	ShelteredZoneBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	ShelteredZoneBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// Niagara for Fire
	FireEffects = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffects"));
	FireEffects->SetupAttachment(FireplaceMesh);
}

void AFireplaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// FIREPLACE VISIBILITY

	// Hides the main fireplace mesh if fireplace is disabled
	if (FireplaceMesh)
	{
		FireplaceMesh->SetVisibility(bEnableFireplace);
		FireplaceMesh->SetHiddenInGame(!bEnableFireplace);
	}

	// Hides the wood logs if fireplace is disabled
	if (WoodLog1) WoodLog1->SetVisibility(bEnableFireplace);
	if (WoodLog2) WoodLog2->SetVisibility(bEnableFireplace);
	if (WoodLog3) WoodLog3->SetVisibility(bEnableFireplace);

	// Hides fireplace detection collider
	if (FireplaceDetectionSphere)
	{
		FireplaceDetectionSphere->SetVisibility(bEnableFireplace, false); 
		FireplaceDetectionSphere->SetHiddenInGame(true);
	}

	// HEAT ZONE VISIBILITY 
	
	// Hide heat zone collider 
	if (HeatZoneSphere)
	{
		HeatZoneSphere->SetVisibility(bEnableHeatZone, false); 
		HeatZoneSphere->SetHiddenInGame(true); 
	}

	// SHELTERED ZONE VISIBILITY
	
	// Hide sheltered zone collider 
	if (ShelteredZoneBox)
	{
		ShelteredZoneBox->SetVisibility(bEnableShelteredZone, false);
		ShelteredZoneBox->SetHiddenInGame(true); 
	}

	// === FIRE EFFECTS ===
	if (FireEffects)
	{
		FireEffects->SetVisibility(false);
		FireEffects->SetHiddenInGame(true);
	}

}

void AFireplaceActor::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableFireplace)
	{
		InitializeLogArrays();

		// Hide all wood logs initially
		for (int32 i = 0; i < LogMeshes.Num(); i++)
		{
			SetLogVisibilityAndMaterial(i, false, nullptr);
		}

		// Bind fireplace overlap events
		FireplaceDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AFireplaceActor::OnFireplaceDetectionBeginOverlap);
		FireplaceDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &AFireplaceActor::OnFireplaceDetectionEndOverlap);
	}
	else
	{
		// Disable fireplace collision 
		if (FireplaceDetectionSphere)
		{
			FireplaceDetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Setup heat zone 
	if (bEnableHeatZone)
	{
		// Heat zone starts inactive
		if (!bEnableFireplace)
		{
			ActivateHeatZone();
		}
	}
	else
	{
		if (HeatZoneSphere)
		{
			HeatZoneSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Setup sheltered zone if enabled
	if (bEnableShelteredZone)
	{
		ShelteredZoneBox->OnComponentBeginOverlap.AddDynamic(this, &AFireplaceActor::OnShelteredZoneBeginOverlap);
		ShelteredZoneBox->OnComponentEndOverlap.AddDynamic(this, &AFireplaceActor::OnShelteredZoneEndOverlap);
	}
	else
	{
		if (ShelteredZoneBox)
		{
			ShelteredZoneBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AFireplaceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableFireplace && CurrentDetectedLog)
	{
		// Check if log was released
		if (!CurrentDetectedLog->IsBeingHeld())
		{
			PlaceLog();
		}
	}
}

#pragma region Fireplace Implementation

void AFireplaceActor::InitializeLogArrays()
{
	if (!bEnableFireplace) return;

	// Clears and resize arrays
	LogMeshes.Empty();
	LogSlotStates.Empty();

	LogMeshes.SetNum(RequiredLogsCount);
	LogSlotStates.SetNum(RequiredLogsCount);

	// Populates with references to existing components
	LogMeshes[0] = WoodLog1;
	LogMeshes[1] = WoodLog2;
	LogMeshes[2] = WoodLog3;

	// Initializes all slots as empty
	for (int32 i = 0; i < RequiredLogsCount; i++)
	{
		LogSlotStates[i] = false;
	}

}

void AFireplaceActor::OnFireplaceDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bEnableFireplace) return;

	AWoodLog* WoodLog = Cast<AWoodLog>(OtherActor);
	if (WoodLog && WoodLog->IsBeingHeld())
	{
		ShowGhostEffect(WoodLog);
	}
}

void AFireplaceActor::OnFireplaceDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bEnableFireplace) return;

	AWoodLog* WoodLog = Cast<AWoodLog>(OtherActor);
	if (WoodLog && WoodLog == CurrentDetectedLog)
	{
		HideGhostEffect();
	}
}

void AFireplaceActor::ShowGhostEffect(AWoodLog* WoodLog)
{
	// Don't show ghost effect if fireplace is already complete
	if (IsFireplaceComplete())
	{
		return;
	}

	// If we're already showing a ghost effect, don't change it
	if (CurrentDetectedLog != nullptr)
	{
		return;
	}

	// Find the next available slot
	int32 NextSlot = GetNextAvailableSlot();
	if (NextSlot == -1)
	{
		return; 
	}

	// Store references
	CurrentDetectedLog = WoodLog;
	CurrentGhostSlot = NextSlot;

	// Show the ghost effect
	SetLogVisibilityAndMaterial(NextSlot, true, GhostMaterial);
}

void AFireplaceActor::HideGhostEffect()
{
	if (CurrentGhostSlot != -1)
	{
		// Check if slot is already placed
		bool bSlotIsPlaced = LogSlotStates.IsValidIndex(CurrentGhostSlot) && LogSlotStates[CurrentGhostSlot];

		if (!bSlotIsPlaced)
		{
			SetLogVisibilityAndMaterial(CurrentGhostSlot, false, nullptr);
		}
	}

	// Reset references
	CurrentDetectedLog = nullptr;
	CurrentGhostSlot = -1;
}

void AFireplaceActor::PlaceLog()
{
	if (CurrentDetectedLog && CurrentGhostSlot != -1)
	{
		int32 PlacingSlot = CurrentGhostSlot;

		// Change ghost to wood material
		SetLogVisibilityAndMaterial(PlacingSlot, true, WoodMaterial);

		// Update array state
		LogSlotStates[PlacingSlot] = true;
		PlacedLogsCount++;

		// Store reference and cleanup
		AWoodLog* LogToDestroy = CurrentDetectedLog;
		CurrentDetectedLog = nullptr;
		CurrentGhostSlot = -1;

		// Safely destroy the held log
		LogToDestroy->NotifyPlaced();

		// Safety check that fireplace log stays visible
		SetLogVisibilityAndMaterial(PlacingSlot, true, WoodMaterial);

		// Check if fireplace is complete
		if (IsFireplaceComplete())
		{
			OnFireplaceComplete();
		}
	}
}

void AFireplaceActor::OnFireplaceComplete()
{
	bFireplaceComplete = true;

	// Activate fire effects
	if (FireEffects)
	{
		FireEffects->SetVisibility(true);
		FireEffects->SetHiddenInGame(false);
		FireEffects->Activate();
	}

	// Activate heat zone if enabled
	if (bEnableHeatZone)
	{
		ActivateHeatZone();
	}

	OnFireComplete.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("Fireplace construction complete - heat zone activated!"));
}

int32 AFireplaceActor::GetNextAvailableSlot() const
{
	for (int32 i = 0; i < LogSlotStates.Num(); i++)
	{
		if (!LogSlotStates[i])
		{
			return i;
		}
	}
	return -1;
}

UStaticMeshComponent* AFireplaceActor::GetLogMeshByIndex(int32 Index) const
{
	if (LogMeshes.IsValidIndex(Index))
	{
		return LogMeshes[Index];
	}
	return nullptr;
}

void AFireplaceActor::SetLogVisibilityAndMaterial(int32 LogIndex, bool bVisible, UMaterialInterface* Material)
{
	UStaticMeshComponent* LogMesh = GetLogMeshByIndex(LogIndex);
	if (LogMesh)
	{
		LogMesh->SetVisibility(bVisible);
		LogMesh->SetHiddenInGame(!bVisible);

		if (Material && bVisible)
		{
			LogMesh->SetMaterial(0, Material);
		}
	}
}

bool AFireplaceActor::IsFireplaceComplete() const
{
	if (!bEnableFireplace) return false;

	for (bool SlotFilled : LogSlotStates)
	{
		if (!SlotFilled)
		{
			return false;
		}
	}
	return true;
}

#pragma endregion

#pragma region Heat Zone Implementation

void AFireplaceActor::ActivateHeatZone()
{
	if (!bEnableHeatZone) return;

	bHeatZoneActive = true;

	// Enable heat zone collision
	if (HeatZoneSphere)
	{
		HeatZoneSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HeatZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFireplaceActor::OnHeatZoneBeginOverlap);
		HeatZoneSphere->OnComponentEndOverlap.AddDynamic(this, &AFireplaceActor::OnHeatZoneEndOverlap);

		// Checks for actors already inside the heat zone
		TArray<AActor*> OverlappingActors;
		HeatZoneSphere->GetOverlappingActors(OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (Actor)
			{
				USurvivalComponent* SurvivalComp = Actor->FindComponentByClass<USurvivalComponent>();
				if (SurvivalComp)
				{
					SurvivalComp->SetIsInIntenseHeatZone(true);
					SurvivalComp->SetIntenseHeatRecoveryRate(IntenseHeatRecoveryRate);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Heat zone activated"));
}

void AFireplaceActor::DeactivateHeatZone()
{
	bHeatZoneActive = false;

	if (HeatZoneSphere)
	{
		HeatZoneSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AFireplaceActor::OnHeatZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bHeatZoneActive) return;

	USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
	if (SurvivalComp)
	{
		SurvivalComp->SetIsInIntenseHeatZone(true);
		SurvivalComp->SetIntenseHeatRecoveryRate(IntenseHeatRecoveryRate);
	}
}

void AFireplaceActor::OnHeatZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bHeatZoneActive) return;

	USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
	if (SurvivalComp)
	{
		SurvivalComp->SetIsInIntenseHeatZone(false);
	}
}

void AFireplaceActor::OnShelteredZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bEnableShelteredZone) return;

	USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
	if (SurvivalComp)
	{
		SurvivalComp->SetIsInShelteredZone(true);
		SurvivalComp->SetShelteredHeatRecoveryRate(ShelteredHeatRecoveryRate);
	}
}

void AFireplaceActor::OnShelteredZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bEnableShelteredZone) return;

	USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
	if (SurvivalComp)
	{
		SurvivalComp->SetIsInShelteredZone(false);
	}
}

#pragma endregion