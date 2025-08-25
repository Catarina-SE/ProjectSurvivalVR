// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRClimbableActor.h"
#include "Characters/VRCharacterBase.h"
#include "Hands/VRHand.h"

AVRClimbableActor::AVRClimbableActor()
{
    ActorMesh->SetSimulatePhysics(false);
    ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ActorMesh->SetCollisionObjectType(ECC_WorldStatic);
}

void AVRClimbableActor::BeginPlay()
{
    Super::BeginPlay();
}

void AVRClimbableActor::OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation, bool bIsLeftHand, ECollisionChannel HandChannel)
{
    AVRHand* Hand = Cast<AVRHand>(InComponent->GetOwner());
    if (!Hand)
        return;

    AVRCharacterBase* Character = Cast<AVRCharacterBase>(Hand->GetOwner());
    if (!Character)
        return;

    // Temporarily disable collision with player and this specific hand during climbing
    if (GrabbingHands.Num() == 0)
    {
        // First hand grabbing - disable Pawn collision
        ActorMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        ShowCanBeGrabbed(false);
    }
    ActorMesh->SetCollisionResponseToChannel(HandChannel, ECR_Ignore);

    // Track this hand
    if (!GrabbingHands.Contains(InComponent))
    {
        GrabbingHands.Add(InComponent);
    }

    bool bGrabSucceeded = true;
    if (ClimbType == EClimbType::Surface)
    {
        bGrabSucceeded = true;
    }
    else // EClimbType::Point
    {
        if (ActorMesh->GetAllSocketNames().Num() > 0)
        {
            bGrabSucceeded = true;
        }
    }

    if (bGrabSucceeded)
    {
        // Tell the character to enter the climbing state
        Character->StartClimbing(Hand);

        UE_LOG(LogTemp, Display, TEXT("%s: Climbing grab by %s hand"),
            *GetName(), bIsLeftHand ? TEXT("LEFT") : TEXT("RIGHT"));
    }
}

void AVRClimbableActor::OnRelease(USkeletalMeshComponent* InComponent)
{
    AVRHand* Hand = Cast<AVRHand>(InComponent->GetOwner());
    if (!Hand)
        return;

    // Get hand channel and restore collision with this specific hand
    ECollisionChannel HandChannel = InComponent->GetCollisionObjectType();
    ActorMesh->SetCollisionResponseToChannel(HandChannel, ECR_Block);

    // Remove this hand from tracking
    GrabbingHands.Remove(InComponent);

    AVRCharacterBase* Character = Cast<AVRCharacterBase>(Hand->GetOwner());
    if (Character)
    {
        Character->StopClimbing(Hand);

        UE_LOG(LogTemp, Display, TEXT("%s: Climbing release by %s hand"),
            *GetName(), Hand->GetHandType() == EControllerHand::Left ? TEXT("LEFT") : TEXT("RIGHT"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get VRCharacterBase from hand owner"));
    }

    // If no hands are grabbing, restore Pawn collision
    if (GrabbingHands.Num() == 0)
    {
        ActorMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        UE_LOG(LogTemp, Display, TEXT("%s: All climbing hands released - Pawn collision restored"), *GetName());
    }
}

UPrimitiveComponent* AVRClimbableActor::GetGrabCollisionComponent()
{
    return ActorMesh;
}

UPrimitiveComponent* AVRClimbableActor::GetPhysicsComponent()
{
    return nullptr;
}

float AVRClimbableActor::GetObjectWeight() const
{
    return 0.0f;
}

void AVRClimbableActor::ShowCanBeGrabbed_Implementation(bool bIsGrabbable)
{
    ActorMesh->SetRenderCustomDepth(bIsGrabbable);
}

FName AVRClimbableActor::GetClosestSocketToHand(const FVector& HandLocation) const
{
    if (!ActorMesh)
    {
        return NAME_None;
    }

    TArray<FName> SocketNames = ActorMesh->GetAllSocketNames();
    if (SocketNames.Num() == 0)
    {
        return NAME_None;
    }

    FName ClosestSocket = NAME_None;
    float ClosestDistance = FLT_MAX;

    for (const FName& SocketName : SocketNames)
    {
        FVector SocketLocation = ActorMesh->GetSocketLocation(SocketName);
        float Distance = FVector::Distance(HandLocation, SocketLocation);

        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestSocket = SocketName;
        }
    }

    return ClosestSocket;
}