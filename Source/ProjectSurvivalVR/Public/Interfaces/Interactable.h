// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"


UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
    GENERATED_BODY()
};


class PROJECTSURVIVALVR_API IInteractable
{
    GENERATED_BODY()


public:
    virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation,
        bool bIsLeftHand = false, ECollisionChannel HandChannel = ECC_Pawn) = 0;
    virtual void OnRelease(USkeletalMeshComponent* InComponent) = 0;

    // Getters
    virtual UPrimitiveComponent* GetGrabCollisionComponent() = 0;
    virtual UPrimitiveComponent* GetPhysicsComponent() = 0;
    virtual float GetObjectWeight() const = 0;

    // Sets render custom depth to the receiving value, can be overridden for further customization.
    virtual void ShowCanBeGrabbed(bool bIsGrabbable) = 0;

};
