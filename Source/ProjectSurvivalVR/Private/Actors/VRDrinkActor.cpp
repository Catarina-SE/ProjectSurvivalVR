// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRDrinkActor.h"
#include "Components/SurvivalComponent.h"
#include "Characters/VRCharacterBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

AVRDrinkActor::AVRDrinkActor()
{
}

void AVRDrinkActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("VRDrinkActor BeginPlay - %s"), *GetName());
}

bool AVRDrinkActor::IsProperlyTiltedForDrinking() const
{
    if (!ActorMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("ActorMesh is null!"));
        return false;
    }

    // Get all the different transforms we can think of
    FVector ActorUp = GetActorUpVector();
    FVector MeshUp = ActorMesh->GetUpVector();
    FVector MeshForward = ActorMesh->GetForwardVector();
    FVector MeshRight = ActorMesh->GetRightVector();

    // Also check component transform
    FTransform MeshTransform = ActorMesh->GetComponentTransform();
    FVector ComponentUp = MeshTransform.GetUnitAxis(EAxis::Z);

    // Try relative transform too
    FTransform RelativeTransform = ActorMesh->GetRelativeTransform();
    FVector RelativeUp = RelativeTransform.GetUnitAxis(EAxis::Z);

    FVector WorldUp = FVector::UpVector;

    // Test all of them
    float ActorDot = FVector::DotProduct(ActorUp, WorldUp);
    float MeshDot = FVector::DotProduct(MeshUp, WorldUp);
    float ComponentDot = FVector::DotProduct(ComponentUp, WorldUp);
    float RelativeDot = FVector::DotProduct(RelativeUp, WorldUp);

    float ActorAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ActorDot, -1.0f, 1.0f)));
    float MeshAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(MeshDot, -1.0f, 1.0f)));
    float ComponentAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ComponentDot, -1.0f, 1.0f)));
    float RelativeAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(RelativeDot, -1.0f, 1.0f)));

    // Use the one that shows the most variation (probably component or relative)
    bool bIsTilted = ComponentAngle >= MinimumTiltAngleForDrinking;

    UE_LOG(LogTemp, Warning, TEXT("Using Component Angle: %.2f >= %.2f = %s"),
        ComponentAngle, MinimumTiltAngleForDrinking, bIsTilted ? TEXT("YES") : TEXT("NO"));

    return bIsTilted;
}

void AVRDrinkActor::Consume()
{
    if (!SurvivalComponent || !CharacterReference)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing SurvivalComponent or CharacterReference"));
        return;
    }

    // Store initial thirst value
    float InitialThirst = SurvivalComponent->Thirst;

    // Check all conditions
    bool bThirstNotFull = SurvivalComponent->Thirst < SurvivalComponent->MaxThirst;
    bool bHasWater = TotalWaterPercentage > 0.0f;
    bool bOverlappingMouth = CharacterReference->IsOverlappingMouth();
    bool bProperlyTilted = IsProperlyTiltedForDrinking();
    bool bCurrentlyHeld = IsBeingHeld();

    // If all conditions are met, consume and start/continue timer
    if (bCurrentlyHeld && bThirstNotFull && bHasWater && bOverlappingMouth && bProperlyTilted)
    {
        UE_LOG(LogTemp, Error, TEXT("*** ALL CONDITIONS MET - CONSUMING! ***"));

        // Consume the drink
        SurvivalComponent->ConsumeDrink(HydrationValue);

        // Adds stamina restoration from drink
        SurvivalComponent->RestoreStaminaFromDrink(StaminaRestorationValue);

        TotalWaterPercentage -= WaterDecreaseRate;

        // Call blueprint event
        OnConsumed();

        // Check if thirst actually changed
        float NewThirst = SurvivalComponent->Thirst;
        float ThirstChange = NewThirst - InitialThirst;

        // Check if empty
        if (TotalWaterPercentage <= 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Water bottle is empty"));

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Water bottle is empty"));
            }

            // Stop any ongoing consumption
            GetWorld()->GetTimerManager().ClearTimer(ConsumptionTimerHandle);
            return;
        }

        // Start timer to call Consume again in 1 second (for continuous drinking)
        if (!GetWorld()->GetTimerManager().IsTimerActive(ConsumptionTimerHandle))
        {
            UE_LOG(LogTemp, Warning, TEXT("Starting consumption timer"));
            GetWorld()->GetTimerManager().SetTimer(ConsumptionTimerHandle, this, &AVRDrinkActor::Consume, 1.0f, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Conditions not met - stopping consumption"));

        // Stop the consumption timer
        GetWorld()->GetTimerManager().ClearTimer(ConsumptionTimerHandle);
    }
}

void AVRDrinkActor::PrepareForDestroy()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ConsumptionTimerHandle);
    }
    Super::PrepareForDestroy();
}

void AVRDrinkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ConsumptionTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}