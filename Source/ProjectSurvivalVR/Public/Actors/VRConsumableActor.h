// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRGrabbableActor.h"
#include "VRConsumableActor.generated.h"

class USurvivalComponent;
class AVRCharacterBase;

UCLASS(Abstract)
class PROJECTSURVIVALVR_API AVRConsumableActor : public AVRGrabbableActor
{
    GENERATED_BODY()

public: 
    AVRConsumableActor();

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void PrepareForDestroy() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // References to character and survival component
    UPROPERTY()
    AVRCharacterBase* CharacterReference;

    UPROPERTY()
    USurvivalComponent* SurvivalComponent;

public:
    // Array for Reusable Consumables
    static TArray<AVRConsumableActor*> ReusableConsumables;

    static AVRConsumableActor* GetOrCreate(UWorld* World, TSubclassOf<AVRConsumableActor> ConsumableClass, FVector Location);
    void Deactivate();
    void Reactivate(FVector NewLocation);

    // Pure virtual function - must be implemented by children
    virtual void Consume() PURE_VIRTUAL(AVRConsumableActor::Consume, );

    UFUNCTION(BlueprintImplementableEvent, Category = "VR|Consumption")
    void OnConsumed();
};