// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRActor.h"
#include "Interfaces/Interactable.h"
#include "VRClimbableActor.generated.h"

class AVRHand;

UENUM(BlueprintType)
enum class EClimbType : uint8
{
    Surface,
    Point
};

UCLASS()
class PROJECTSURVIVALVR_API AVRClimbableActor : public AVRActor, public IInteractable
{
    GENERATED_BODY()

public:
    AVRClimbableActor();

protected:
    virtual void BeginPlay() override;

#pragma region IInteractable

public:

    virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation, bool bIsLeftHand = false, ECollisionChannel HandChannel = ECC_Pawn) override;
    virtual void OnRelease(USkeletalMeshComponent* InComponent) override;
    virtual UPrimitiveComponent* GetGrabCollisionComponent() override;
    virtual UPrimitiveComponent* GetPhysicsComponent() override;
    virtual float GetObjectWeight() const override;
    
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|Interaction")
    void ShowCanBeGrabbed(bool bIsGrabbable) override;

#pragma endregion
    
protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Climbing|Setup")
    EClimbType ClimbType = EClimbType::Surface;

    // Just track which hands are currently grabbing this climbable
    UPROPERTY()
    TArray<USkeletalMeshComponent*> GrabbingHands;

public:
    // Getter for climb type
    UFUNCTION(BlueprintPure, Category = "Climbing")
    EClimbType GetClimbType() const { return ClimbType; }
    
    FName GetClosestSocketToHand(const FVector& HandLocation) const;
};