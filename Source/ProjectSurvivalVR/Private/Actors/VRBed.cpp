// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRBed.h"
#include "Components/SphereComponent.h"
#include "Components/SurvivalComponent.h"
#include "Characters/VRCharacterBase.h"
#include "Environment/DayNightManager.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

AVRBed::AVRBed()
{
	PrimaryActorTick.bCanEverTick = false;

	SleepInteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SleepInteractionSphere"));
	SleepInteractionSphere->SetupAttachment(ActorMesh);
	SleepInteractionSphere->SetSphereRadius(150.0f);
	SleepInteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SleepInteractionSphere->SetHiddenInGame(true);

	// Bed dont simulate physics or be grabbable 
	ActorMesh->SetSimulatePhysics(false);
	ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ActorMesh->SetCollisionObjectType(ECC_WorldStatic);

	// Set grab behavior to none so it acts as an interaction object
	GrabPointBehavior = EGrabPointBehavior::None;
}

void AVRBed::BeginPlay()
{
	Super::BeginPlay();

	// Finds the DayNightManager
	DayNightManager = ADayNightManager::GetInstance(GetWorld());
	if (!DayNightManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: No DayNightManager found in world"));
	}

	// Bind overlap events
	if (SleepInteractionSphere)
	{
		SleepInteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AVRBed::OnSleepInteractionBeginOverlap);
		//SleepInteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AVRBed::OnSleepInteractionEndOverlap);
	}

}

void AVRBed::OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation, bool bIsLeftHand, ECollisionChannel HandChannel)
{
	// Gets the actor that's grabbing
	if (InComponent && InComponent->GetOwner())
	{
		AActor* GrabbingActor = InComponent->GetOwner()->GetOwner();
		if (GrabbingActor)
		{
			AttemptSleep(GrabbingActor);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No grabbing actor found"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid InComponent or owner"));
	}
}

void AVRBed::OnSleepInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's the player character
	if (AVRCharacterBase* Player = Cast<AVRCharacterBase>(OtherActor))
	{
		if (bShowDebugMessages && GEngine)
		{
			if (IsAvailable() && IsNightTime())
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Press grab to sleep"));
			}
			else if (!IsNightTime())
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("You can only sleep at night"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Bed is not available"));
			}
		}
	}
}

void AVRBed::AttemptSleep(AActor* SleepingActor)
{

	if (!SleepingActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: AttemptSleep called with null actor"));
		return;
	}

	// Checks if bed is available
	if (!IsAvailable())
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: Bed not available"));
		OnSleepFailed.Broadcast(TEXT("Bed is not available"));
		if (bShowDebugMessages && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Bed is not available"));
		}
		return;
	}

	// Checks if its night time
	if (!IsNightTime())
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: Not night time"));
		OnSleepFailed.Broadcast(TEXT("You can only sleep at night"));
		if (bShowDebugMessages && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("You can only sleep at night"));
		}
		return;
	}

	// Checks if actor can sleep
	if (!CanActorSleep(SleepingActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: Actor cannot sleep"));
		OnSleepFailed.Broadcast(TEXT("You cannot sleep right now"));
		if (bShowDebugMessages && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("You cannot sleep right now"));
		}
		return;
	}

	// Checks DayNightManager
	if (!DayNightManager)
	{
		UE_LOG(LogTemp, Error, TEXT("VRBed: No DayNightManager found!"));
		OnSleepFailed.Broadcast(TEXT("Time system not available"));
		return;
	}

	// Starts sleeping process
	bIsBeingUsed = true;
	CurrentSleepingActor = SleepingActor;
	OnSleepStarted.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("VRBed: %s started sleeping"), *SleepingActor->GetName());

	// Start the fade transition
	StartSleepFade();
}

void AVRBed::StartSleepFade()
{
	if (SleepFadeWidgetClass)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			CurrentFadeWidget = CreateWidget<UUserWidget>(PlayerController, SleepFadeWidgetClass);
			if (CurrentFadeWidget)
			{
				CurrentFadeWidget->AddToViewport(1000);
				UE_LOG(LogTemp, Warning, TEXT("VRBed: Fade widget created and added to viewport"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VRBed: No SleepFadeWidgetClass set, skipping fade"));
	}

	float TimeToSkip = FadeDuration * 0.5f; 
	GetWorld()->GetTimerManager().SetTimer(
		SleepTransitionTimer,
		this,
		&AVRBed::SkipTimeToMorning,
		TimeToSkip,
		false
	);

	// Sets timer to remove fade widget after full duration
	FTimerHandle RemoveFadeTimer;
	GetWorld()->GetTimerManager().SetTimer(
		RemoveFadeTimer,
		this,
		&AVRBed::CompleteSleepAndRemoveFade,
		FadeDuration,
		false
	);
}

void AVRBed::SkipTimeToMorning()
{
	UE_LOG(LogTemp, Warning, TEXT("VRBed: Skipping time to morning (during black screen)"));

	// Skips to morning
	bool bSleepSuccess = false;
	if (DayNightManager)
	{
		bSleepSuccess = DayNightManager->SleepUntilMorning();
		UE_LOG(LogTemp, Warning, TEXT("VRBed: SleepUntilMorning returned: %s"), bSleepSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
	}

	if (bSleepSuccess && CurrentSleepingActor)
	{
		// Apply sleep benefits
		ApplySleepBenefits(CurrentSleepingActor);
		UE_LOG(LogTemp, Warning, TEXT("VRBed: Sleep benefits applied"));
	}
}

void AVRBed::CompleteSleepAndRemoveFade()
{
	UE_LOG(LogTemp, Warning, TEXT("VRBed: Completing sleep and removing fade"));

	// Complete sleep
	OnSleepCompleted.Broadcast();

	if (bShowDebugMessages && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Good morning! You slept until dawn."));
	}

	RemoveFadeWidget();

	// Free up the bed
	bIsBeingUsed = false;
	CurrentSleepingActor = nullptr;
}

void AVRBed::ApplySleepBenefits(AActor* Player)
{
	if (!Player)
		return;

	USurvivalComponent* SurvivalComp = Player->FindComponentByClass<USurvivalComponent>();
	if (!SurvivalComp)
		return;

	// Apply temperature warming
	if (bWarmsPlayer)
	{
			// Directly increase temperature
			float NewTemperature = FMath::Clamp(
				SurvivalComp->Temperature + WarmthAmount,
				SurvivalComp->MinTemperature,
				SurvivalComp->MaxTemperature
			);
			SurvivalComp->Temperature = NewTemperature;

			// To add: HUD Hint that temperature was restored
	}
	if (bRestoreStamina)
	{
		// Restore stamina from sleep
		SurvivalComp->RestoreStaminaFromSleep();

		UE_LOG(LogTemp, Warning, TEXT("Sleep fully restored stamina"));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
				TEXT("You feel well-rested!"));
		}
		// To add: HUD Hint that stamina was restored
	}
}

bool AVRBed::CanActorSleep(AActor* Actor) const
{
	if (!Actor)
		return false;

	// Checks if its a player character
	if (!Cast<AVRCharacterBase>(Actor))
		return false;

	return true;
}

bool AVRBed::TriggerSleep(AActor* SleepingActor)
{
	AttemptSleep(SleepingActor);
	return !bIsBeingUsed;
}

bool AVRBed::IsNightTime() const
{
	return DayNightManager ? DayNightManager->IsNight() : false;
}

void AVRBed::RemoveFadeWidget()
{
	if (CurrentFadeWidget)
	{
		CurrentFadeWidget->RemoveFromParent();
		CurrentFadeWidget = nullptr;
	}
}