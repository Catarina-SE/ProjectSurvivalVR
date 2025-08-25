// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/VRCharacterBase.h"
#include "Camera/CameraComponent.h"
#include "Hands/VRHand.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AI/Navigation/NavAreaBase.h"
#include "MotionControllerComponent.h"
#include "Actors/VRConsumableActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Actors/VRClimbableActor.h"
#include "Blueprint/UserWidget.h"

AVRCharacterBase::AVRCharacterBase()
{
    PrimaryActorTick.bCanEverTick = true;

    VROrigin = CreateDefaultSubobject<USceneComponent>("VROrigin");
    VROrigin->SetupAttachment(GetMesh());

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(VROrigin);

    // Headlight Component Setup
    HeadlightComponent = CreateDefaultSubobject<USpotLightComponent>("HeadlightComponent");
    HeadlightComponent->SetupAttachment(Camera);

    MouthCollider = CreateDefaultSubobject<USphereComponent>("MouthCollider");
    MouthCollider->SetupAttachment(Camera);
    MouthCollider->SetSphereRadius(15.0f);
    MouthCollider->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    MouthCollider->OnComponentBeginOverlap.AddDynamic(this, &AVRCharacterBase::OnMouthBeginOverlap);
    MouthCollider->OnComponentEndOverlap.AddDynamic(this, &AVRCharacterBase::OnMouthEndOverlap);

    SurvivalComponent = CreateDefaultSubobject<USurvivalComponent>("SurvivalComponent");

    TeleportTraceNiagaraSystem = CreateDefaultSubobject<UNiagaraComponent>("TeleportTraceNiagaraSystem");
    TeleportTraceNiagaraSystem->SetupAttachment(GetRootComponent());
    TeleportTraceNiagaraSystem->SetVisibility(false);
}

void AVRCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    SpawnHands();
    SetupInputBindings();

    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = NormalMoveSpeed;
    }

    if (SurvivalComponent)
    {
        SurvivalComponent->OnStaminaDepleted.AddDynamic(this, &AVRCharacterBase::OnStaminaForcedStop);
    }

    if (GetCapsuleComponent())
    {
        bOriginalSimulatePhysics = GetCapsuleComponent()->IsSimulatingPhysics();
        bOriginalEnableGravity = GetCapsuleComponent()->IsGravityEnabled();
        OriginalCollisionEnabled = GetCapsuleComponent()->GetCollisionEnabled();
    }
}

void AVRCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bEnableHeadCollisionPrevention &&
        (CurrentMovementState == EVRMovementState::Climbing ||
            CurrentMovementState == EVRMovementState::Locomotion))
    {
        ProcessHeadCollisionPrevention(DeltaTime);
    }

    if (bEnableHeadMovementCompensation &&
        (CurrentMovementState == EVRMovementState::Climbing ||
            CurrentMovementState == EVRMovementState::Locomotion))
    {
        ProcessHeadMovementCompensation(DeltaTime);
    }

    switch (CurrentMovementState)
    {
    case EVRMovementState::Locomotion:
        ProcessLocomotionMovement();

        if (GetCharacterMovement()->IsFalling())
        {
            CurrentMovementState = EVRMovementState::Falling;
            GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        }
        break;

    case EVRMovementState::Climbing:
        ProcessClimbingMovement(DeltaTime);
        break;

    case EVRMovementState::Falling:
        // Check if we need height adjustment (only when falling)
        if (IsCharacterStuckInGeometry())
        {
            FixHeightAfterClimbing();
        }
        else if (!GetCharacterMovement()->IsFalling())
        {
            CurrentMovementState = EVRMovementState::Locomotion;
        }
        break;
    }

    if (bEnableFallDetection)
    {
        UpdateFallDetection();
    }
}

void AVRCharacterBase::SpawnHands()
{
    UWorld* World = GetWorld();

    // Spawn left hand
    if (LeftHandClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        LeftHand = World->SpawnActor<AVRHand>(
            LeftHandClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (LeftHand && VROrigin)
        {
            LeftHand->AttachToComponent(
                VROrigin,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                NAME_None
            );

            UPrimitiveComponent* HandPrimitive = Cast<UPrimitiveComponent>(LeftHand->GetRootComponent());
            if (HandPrimitive)
            {
                HandPrimitive->SetSimulatePhysics(true);
                HandPrimitive->WeldTo(VROrigin);
            }
        }
    }

    // Spawn right hand
    if (RightHandClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        RightHand = World->SpawnActor<AVRHand>(
            RightHandClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (RightHand && VROrigin)
        {
            RightHand->AttachToComponent(
                VROrigin,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                NAME_None
            );

            UPrimitiveComponent* HandPrimitive = Cast<UPrimitiveComponent>(RightHand->GetRootComponent());
            if (HandPrimitive)
            {
                HandPrimitive->SetSimulatePhysics(true);
                HandPrimitive->WeldTo(VROrigin);
            }
        }
    }
}

void AVRCharacterBase::SetupInputBindings()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
        return;

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        // Movement Input
        EnhancedInputComponent->BindAction(HorizontalMovementAction, ETriggerEvent::Triggered, this, &AVRCharacterBase::HorizontalMovementInput);
        EnhancedInputComponent->BindAction(VerticalMovementAction, ETriggerEvent::Triggered, this, &AVRCharacterBase::VerticalMovementInput);

        // Reset Movement On Joystick Release
        EnhancedInputComponent->BindAction(HorizontalMovementAction, ETriggerEvent::Completed, this, &AVRCharacterBase::ResetHorizontalMovement);
        EnhancedInputComponent->BindAction(VerticalMovementAction, ETriggerEvent::Completed, this, &AVRCharacterBase::ResetVerticalMovement);

        // Sprint Input
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AVRCharacterBase::Sprint);

        // Turn Input
        EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AVRCharacterBase::HandleTurnAction);
        EnhancedInputComponent->BindAction(TurnAction, ETriggerEvent::Completed, this, &AVRCharacterBase::CompleteTurnAction);
    }
}

void AVRCharacterBase::ProcessHeadCollisionPrevention(float DeltaTime)
{
    if (!Camera)
        return;

    FVector TargetRepulsionForce = CalculateHeadRepulsionForce();

    CurrentRepulsionForce = FMath::VInterpTo(
        CurrentRepulsionForce,
        TargetRepulsionForce,
        DeltaTime,
        RepulsionSmoothingSpeed
    );

    if (!CurrentRepulsionForce.IsNearlyZero())
    {
        if (CurrentMovementState == EVRMovementState::Climbing)
        {
            AddActorWorldOffset(CurrentRepulsionForce * DeltaTime, true);

            // Update climbing anchors
            if (ClimbingHand_Left && ClimbingHand_Left->GetMotionController())
            {
                LeftHandClimbAnchor = ClimbingHand_Left->GetMotionController()->GetComponentLocation();
            }
            if (ClimbingHand_Right && ClimbingHand_Right->GetMotionController())
            {
                RightHandClimbAnchor = ClimbingHand_Right->GetMotionController()->GetComponentLocation();
            }
            if (PrimaryClimbingHand && PrimaryClimbingHand->GetMotionController())
            {
                PrimaryHandClimbAnchor = PrimaryClimbingHand->GetMotionController()->GetComponentLocation();
            }
        }
        else
        {
            FVector NormalizedForce = CurrentRepulsionForce.GetSafeNormal();
            AddMovementInput(NormalizedForce, CurrentRepulsionForce.Size() / HeadRepulsionForce);
        }
    }
}

FVector AVRCharacterBase::CalculateHeadRepulsionForce() const
{
    FVector WallNormal;
    float DistanceToWall;

    if (!IsHeadNearWall(WallNormal, DistanceToWall))
    {
        return FVector::ZeroVector;
    }

    float RepulsionStrength = 0.0f;

    if (DistanceToWall < HeadCollisionDistance)
    {
        float DistanceRatio = 1.0f - (DistanceToWall / HeadCollisionDistance);
        RepulsionStrength = DistanceRatio * HeadRepulsionForce;
    }

    FVector RepulsionVector = WallNormal * RepulsionStrength;
    float MaxDistanceThisFrame = MaxRepulsionDistance * GetWorld()->GetDeltaSeconds();
    RepulsionVector = RepulsionVector.GetClampedToMaxSize(MaxDistanceThisFrame);

    return RepulsionVector;
}

bool AVRCharacterBase::IsHeadNearWall(FVector& OutWallNormal, float& OutDistance) const
{
    if (!Camera)
        return false;

    FVector CameraLocation = Camera->GetComponentLocation();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    // Sphere sweep to detect nearby walls
    FHitResult SweepResult;
    bool bHitFound = GetWorld()->SweepSingleByChannel(
        SweepResult,
        CameraLocation,
        CameraLocation,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeSphere(HeadCollisionDistance),
        QueryParams
    );

    if (bHitFound)
    {
        // Check if this is a grabbable (but not climbable) that we should ignore
        if (AActor* HitActor = SweepResult.GetActor())
        {
            // If it's a grabbable but NOT a climbable, ignore it
            if (Cast<AVRGrabbableActor>(HitActor) && !Cast<AVRClimbableActor>(HitActor))
            {
                // Skip this hit, it's a consumable/grabbable we should ignore
            }
            else
            {
                // Valid collision (wall, climbable, etc.)
                OutWallNormal = SweepResult.Normal;
                OutDistance = FVector::Distance(CameraLocation, SweepResult.Location);
                return true;
            }
        }
        else
        {
            // Hit something that's not an actor (likely static geometry)
            OutWallNormal = SweepResult.Normal;
            OutDistance = FVector::Distance(CameraLocation, SweepResult.Location);
            return true;
        }
    }

    // Multi-directional ray casting
    TArray<FVector> TestDirections = {
        FVector::ForwardVector,
        FVector::BackwardVector,
        FVector::RightVector,
        FVector::LeftVector,
        FVector::UpVector,
        FVector::DownVector
    };

    float ClosestDistance = FLT_MAX;
    FVector BestNormal = FVector::ZeroVector;
    bool bFoundWall = false;

    for (const FVector& Direction : TestDirections)
    {
        FVector TraceEnd = CameraLocation + (Direction * HeadCollisionDistance);

        FHitResult HitResult;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            CameraLocation,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (bHit)
        {
            // Check if this is a grabbable (but not climbable) that we should ignore
            if (AActor* HitActor = HitResult.GetActor())
            {
                // If it's a grabbable but NOT a climbable, skip it
                if (Cast<AVRGrabbableActor>(HitActor) && !Cast<AVRClimbableActor>(HitActor))
                {
                    continue; // Skip this hit
                }
            }

            float Distance = FVector::Distance(CameraLocation, HitResult.Location);
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                BestNormal = HitResult.Normal;
                bFoundWall = true;
            }
        }
    }

    if (bFoundWall)
    {
        OutWallNormal = BestNormal;
        OutDistance = ClosestDistance;
        return true;
    }

    return false;
}

void AVRCharacterBase::ProcessHeadMovementCompensation(float DeltaTime)
{
    if (!Camera)
        return;

    FVector CurrentHeadPosition = Camera->GetComponentLocation();

    if (!bHasValidLastHeadPosition)
    {
        LastHeadPosition = CurrentHeadPosition;
        bHasValidLastHeadPosition = true;
        return;
    }

    FVector HeadMovementThisFrame = CurrentHeadPosition - LastHeadPosition;
    HeadVelocity = HeadMovementThisFrame / DeltaTime;

    FVector WallPenetration = CalculateHeadWallPenetration();

    if (!WallPenetration.IsNearlyZero() || !HeadMovementThisFrame.IsNearlyZero())
    {
        FVector CompensationMovement = GetCompensatedMovement(HeadMovementThisFrame, WallPenetration);

        if (!CompensationMovement.IsNearlyZero())
        {
            AddActorWorldOffset(CompensationMovement, true);

            // Update climbing anchors
            if (ClimbingHand_Left && ClimbingHand_Left->GetMotionController())
            {
                LeftHandClimbAnchor = ClimbingHand_Left->GetMotionController()->GetComponentLocation();
            }
            if (ClimbingHand_Right && ClimbingHand_Right->GetMotionController())
            {
                RightHandClimbAnchor = ClimbingHand_Right->GetMotionController()->GetComponentLocation();
            }
            if (PrimaryClimbingHand && PrimaryClimbingHand->GetMotionController())
            {
                PrimaryHandClimbAnchor = PrimaryClimbingHand->GetMotionController()->GetComponentLocation();
            }
        }
    }

    LastHeadPosition = CurrentHeadPosition;
}

FVector AVRCharacterBase::CalculateHeadWallPenetration() const
{
    if (!Camera)
        return FVector::ZeroVector;

    FVector HeadPosition = Camera->GetComponentLocation();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    TArray<FVector> CheckDirections = {
        FVector::ForwardVector,
        FVector::BackwardVector,
        FVector::RightVector,
        FVector::LeftVector,
        FVector::UpVector,
        FVector::DownVector
    };

    FVector TotalPenetration = FVector::ZeroVector;
    int32 PenetrationCount = 0;

    for (const FVector& Direction : CheckDirections)
    {
        FVector TraceStart = HeadPosition;
        FVector TraceEnd = HeadPosition + (Direction * HeadWallDetectionDistance);

        FHitResult HitResult;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (bHit)
        {
            // Check if this is a grabbable (but not climbable) that we should ignore
            if (AActor* HitActor = HitResult.GetActor())
            {
                // If it's a grabbable but NOT a climbable, skip it
                if (Cast<AVRGrabbableActor>(HitActor) && !Cast<AVRClimbableActor>(HitActor))
                {
                    continue; // Skip this hit
                }
            }

            float DistanceToWall = FVector::Distance(HeadPosition, HitResult.Location);

            if (DistanceToWall < HeadCollisionRadius)
            {
                float PenetrationDepth = HeadCollisionRadius - DistanceToWall;
                FVector PenetrationVector = HitResult.Normal * PenetrationDepth;

                TotalPenetration += PenetrationVector;
                PenetrationCount++;
            }
        }
    }

    if (PenetrationCount > 0)
    {
        return TotalPenetration / PenetrationCount;
    }

    return FVector::ZeroVector;
}

FVector AVRCharacterBase::GetCompensatedMovement(const FVector& HeadMovement, const FVector& WallPenetration) const
{
    FVector CompensationVector = FVector::ZeroVector;

    if (!WallPenetration.IsNearlyZero())
    {
        CompensationVector += WallPenetration * CompensationStrength;
    }

    if (!HeadMovement.IsNearlyZero())
    {
        FVector PredictedHeadPosition = Camera->GetComponentLocation() + HeadMovement;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        TArray<FOverlapResult> OverlapResults;
        bool bWouldOverlap = GetWorld()->OverlapMultiByChannel(
            OverlapResults,
            PredictedHeadPosition,
            FQuat::Identity,
            ECC_WorldStatic,
            FCollisionShape::MakeSphere(HeadCollisionRadius),
            QueryParams
        );

        if (bWouldOverlap)
        {
            FVector AverageNormal = FVector::ZeroVector;
            int32 ValidNormals = 0;

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (Result.Component.IsValid())
                {
                    // Check if this overlap is with a grabbable (but not climbable) that we should ignore
                    if (AActor* OverlapActor = Result.GetActor())
                    {
                        // If it's a grabbable but NOT a climbable, skip it
                        if (Cast<AVRGrabbableActor>(OverlapActor) && !Cast<AVRClimbableActor>(OverlapActor))
                        {
                            continue; // Skip this overlap
                        }
                    }

                    FVector ComponentCenter = Result.Component->GetComponentLocation();
                    FVector DirectionToComponent = (ComponentCenter - PredictedHeadPosition).GetSafeNormal();

                    FHitResult NormalHit;
                    bool bGotNormal = GetWorld()->LineTraceSingleByChannel(
                        NormalHit,
                        PredictedHeadPosition,
                        PredictedHeadPosition + DirectionToComponent * HeadCollisionRadius * 2,
                        ECC_WorldStatic,
                        QueryParams
                    );

                    if (bGotNormal)
                    {
                        // Double-check the normal hit isn't from a grabbable we should ignore
                        if (AActor* NormalHitActor = NormalHit.GetActor())
                        {
                            if (Cast<AVRGrabbableActor>(NormalHitActor) && !Cast<AVRClimbableActor>(NormalHitActor))
                            {
                                continue; // Skip this normal
                            }
                        }

                        AverageNormal += NormalHit.Normal;
                        ValidNormals++;
                    }
                }
            }

            if (ValidNormals > 0)
            {
                AverageNormal /= ValidNormals;
                AverageNormal.Normalize();

                float MovementMagnitude = HeadMovement.Size();
                CompensationVector += AverageNormal * MovementMagnitude * CompensationStrength;
            }
        }
    }

    float DeltaTime = GetWorld()->GetDeltaSeconds();
    float MaxCompensationThisFrame = MaxCompensationSpeed * DeltaTime;
    CompensationVector = CompensationVector.GetClampedToMaxSize(MaxCompensationThisFrame);

    return CompensationVector;
}

bool AVRCharacterBase::IsCharacterAtEdge() const
{
    if (!GetCapsuleComponent())
        return false;

    FVector CharacterLocation = GetActorLocation();

    TArray<FVector> EdgeCheckDirections = {
        FVector::ForwardVector,
        FVector::BackwardVector,
        FVector::RightVector,
        FVector::LeftVector
    };

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (LeftHand) QueryParams.AddIgnoredActor(LeftHand);
    if (RightHand) QueryParams.AddIgnoredActor(RightHand);

    int32 EmptyDirections = 0;

    for (const FVector& Direction : EdgeCheckDirections)
    {
        FVector TraceStart = CharacterLocation;
        FVector TraceEnd = CharacterLocation + (Direction * EdgeDetectionDistance);

        FHitResult HitResult;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (!bHit)
        {
            EmptyDirections++;
        }
    }

    return EmptyDirections >= 2;
}

bool AVRCharacterBase::WouldCharacterGetPushedSideways() const
{
    if (!GetCapsuleComponent())
        return false;

    float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector CapsuleCenter = GetActorLocation();

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (LeftHand) QueryParams.AddIgnoredActor(LeftHand);
    if (RightHand) QueryParams.AddIgnoredActor(RightHand);

    TArray<FOverlapResult> OverlapResults;
    bool bIsOverlapping = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        CapsuleCenter,
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
        QueryParams
    );

    if (bIsOverlapping)
    {
        for (const FOverlapResult& Result : OverlapResults)
        {
            if (Result.Component.IsValid())
            {
                float ComponentZ = Result.Component->GetComponentLocation().Z;
                float CharacterFeetZ = GetActorLocation().Z - CapsuleHalfHeight;

                if (ComponentZ > CharacterFeetZ + 30.0f)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

FVector AVRCharacterBase::GetEdgeAvoidanceDirection() const
{
    if (!GetCapsuleComponent())
        return FVector::ZeroVector;

    FVector CharacterLocation = GetActorLocation();

    TArray<FVector> TestDirections = {
        FVector::ForwardVector,
        FVector::BackwardVector,
        FVector::RightVector,
        FVector::LeftVector
    };

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (LeftHand) QueryParams.AddIgnoredActor(LeftHand);
    if (RightHand) QueryParams.AddIgnoredActor(RightHand);

    for (const FVector& Direction : TestDirections)
    {
        FVector TraceStart = CharacterLocation;
        FVector TraceEnd = CharacterLocation + (Direction * EdgeDetectionDistance * 2.0f);

        FHitResult HitResult;
        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            TraceStart,
            TraceEnd,
            ECC_WorldStatic,
            QueryParams
        );

        if (bHit)
        {
            float Distance = FVector::Distance(CharacterLocation, HitResult.Location);
            if (Distance < EdgeDetectionDistance * 1.5f)
            {
                return Direction;
            }
        }
    }

    if (Camera)
    {
        FVector CameraForward = Camera->GetForwardVector();
        return FVector(CameraForward.X, CameraForward.Y, 0.0f).GetSafeNormal();
    }

    return FVector::ForwardVector;
}

void AVRCharacterBase::HorizontalMovementInput(const FInputActionValue& Value)
{
    float CurrentInput = Value.Get<float>();

    if (FMath::Abs(CurrentInput) < JoystickDeadzone)
    {
        HorizontalMovement = 0.0f;
    }
    else
    {
        HorizontalMovement = FMath::Clamp(CurrentInput / (1.0f - JoystickDeadzone), -1.0f, 1.0f);
    }
}

void AVRCharacterBase::VerticalMovementInput(const FInputActionValue& Value)
{
    float CurrentInput = Value.Get<float>();

    if (FMath::Abs(CurrentInput) < JoystickDeadzone)
    {
        VerticalMovement = 0.0f;
    }
    else
    {
        VerticalMovement = FMath::Clamp(CurrentInput / (1.0f - JoystickDeadzone), -1.0f, 1.0f);
    }
}

void AVRCharacterBase::ResetHorizontalMovement(const FInputActionValue& Value)
{
    HorizontalMovement = 0.0f;
}

void AVRCharacterBase::ResetVerticalMovement(const FInputActionValue& Value)
{
    VerticalMovement = 0.0f;
}

void AVRCharacterBase::Sprint()
{
    bIsSprinting = !bIsSprinting;

    if (SurvivalComponent && !SurvivalComponent->CanStartSprinting() && bIsSprinting)
    {
        bIsSprinting = false;
    }

    UpdateMovementSpeed();

    if (SurvivalComponent)
    {
        SurvivalComponent->SetSprintingState(bIsSprinting);
    }
}

void AVRCharacterBase::OnStaminaForcedStop()
{
    if (ClimbingHand_Left)
    {
        ClimbingHand_Left->ReleaseObject();
    }
    if (ClimbingHand_Right)
    {
        ClimbingHand_Right->ReleaseObject();
    }
}

void AVRCharacterBase::ProcessLocomotionMovement()
{
    if (!Camera || !IsJoystickMovementAllowed()) return;

    if (FMath::IsNearlyZero(HorizontalMovement, 0.01f) && FMath::IsNearlyZero(VerticalMovement, 0.01f))
    {
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
        AddMovementInput(FVector::ZeroVector);
        return;
    }

    const FVector RightDirection = Camera->GetRightVector();
    const FVector ForwardDirection = FVector::VectorPlaneProject(Camera->GetForwardVector(), FVector::UpVector).GetSafeNormal();
    FVector MovementVector = (ForwardDirection * VerticalMovement) + (RightDirection * HorizontalMovement);
    MovementVector = MovementVector.GetSafeNormal();
    AddMovementInput(MovementVector, 1.0f);
}

void AVRCharacterBase::ProcessClimbingMovement(float DeltaTime)
{
    if (PrimaryClimbingHand && PrimaryClimbingHand->GetMotionController())
    {
        FVector MovementDelta = PrimaryClimbingHand->GetMotionController()->GetComponentLocation() - PrimaryHandClimbAnchor;

        AddActorWorldOffset(-MovementDelta);

        PrimaryHandClimbAnchor = PrimaryClimbingHand->GetMotionController()->GetComponentLocation();

        if (ClimbingHand_Left && ClimbingHand_Left->GetMotionController())
        {
            LeftHandClimbAnchor = ClimbingHand_Left->GetMotionController()->GetComponentLocation();
        }
        if (ClimbingHand_Right && ClimbingHand_Right->GetMotionController())
        {
            RightHandClimbAnchor = ClimbingHand_Right->GetMotionController()->GetComponentLocation();
        }
    }
}

void AVRCharacterBase::UpdateMovementSpeed()
{
    GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintMoveSpeed : NormalMoveSpeed;
}

void AVRCharacterBase::HandleTurnAction(const FInputActionValue& Value)
{
    float TurnValue = Value.Get<float>();

    if (bSmoothTurnEnabled)
    {
        SmoothTurn(TurnValue);
    }
    else
    {
        bool RightTurn = TurnValue > 0.0f;
        if (FMath::Abs(TurnValue) > JoystickDeadzone && !bSnapTurnExecuted)
        {
            SnapTurn(RightTurn);
        }
    }
}

void AVRCharacterBase::CompleteTurnAction()
{
    bSnapTurnExecuted = false;
}

void AVRCharacterBase::SnapTurn(bool RightTurn)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
        return;

    float YawRotation = RightTurn ? SnapTurnDegrees : -SnapTurnDegrees;
    FRotator CurrentRotation = PlayerController->GetControlRotation();
    FRotator NewRotation = CurrentRotation + FRotator(0.0f, YawRotation, 0.0f);
    PlayerController->SetControlRotation(NewRotation);

    bSnapTurnExecuted = true;
}

void AVRCharacterBase::SmoothTurn(float TurnValue)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    float NormalizedInput = FMath::Clamp(TurnValue, -1.0f, 1.0f);

    if (FMath::Abs(NormalizedInput) <= JoystickDeadzone)
        return;

    float DeltaTime = World->GetDeltaSeconds();
    float YawDelta = NormalizedInput * SmoothTurnRate * DeltaTime;

    FRotator CurrentRotation = PlayerController->GetControlRotation();
    FRotator NewRotation = CurrentRotation + FRotator(0.0f, YawDelta, 0.0f);

    PlayerController->SetControlRotation(NewRotation);
}

void AVRCharacterBase::StartTeleport()
{
    if (!IsTeleportAllowed())
    {
        return;
    }

    bTeleportTraceActive = true;
    TeleportTraceNiagaraSystem->SetVisibility(true);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined,
        SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;

    if (TeleportVisualizerClass)
    {
        TeleportViasualizerReference = GetWorld()->SpawnActor<AActor>(
            TeleportVisualizerClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );
    }
}

TArray<FVector> AVRCharacterBase::TeleportTrace(FVector StartPosition, FVector ForwardVector)
{
    if (!IsTeleportAllowed())
    {
        if (TeleportViasualizerReference)
        {
            TeleportViasualizerReference->GetRootComponent()->SetVisibility(false, true);
        }
        return TArray<FVector>();
    }

    FVector LaunchVelocity = ForwardVector * TeleportLaunchSpeed;
    FHitResult HitResult;
    TArray<FVector> PathPositions;
    FVector LastTraceDestination;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(this);

    bool bHit = UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(
        GetWorld(),
        HitResult,
        PathPositions,
        LastTraceDestination,
        StartPosition,
        LaunchVelocity,
        true,
        TeleportProjectileRadius,
        ObjectTypes,
        true,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        0.0f,
        15.0f,
        2.0f,
        0.0f
    );

    bValidTeleportTrace = false;

    if (bHit)
    {
        FVector HitLocation = HitResult.Location;

        UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (NavSystem)
        {
            FNavLocation ProjectedLocation;

            bool bProjectionSuccess = NavSystem->ProjectPointToNavigation(
                HitLocation,
                ProjectedLocation,
                TeleportProjectPointToNavigationQueryExtend
            );

            if (bProjectionSuccess)
            {
                ProjectedTeleportLocation = FVector(
                    ProjectedLocation.Location.X,
                    ProjectedLocation.Location.Y,
                    ProjectedLocation.Location.Z - NavMeshCellHeight
                );

                bValidTeleportTrace = CanReachLocation(ProjectedTeleportLocation);
            }
            else
            {
                ProjectedTeleportLocation = HitLocation;
                bValidTeleportTrace = false;
            }
        }
        else
        {
            ProjectedTeleportLocation = HitLocation;
            bValidTeleportTrace = false;
        }
    }
    else
    {
        ProjectedTeleportLocation = LastTraceDestination;
        bValidTeleportTrace = false;
    }

    if (TeleportViasualizerReference)
    {
        TeleportViasualizerReference->SetActorLocation(ProjectedTeleportLocation);
        TeleportViasualizerReference->GetRootComponent()->SetVisibility(bValidTeleportTrace, true);
    }

    return PathPositions;
}

void AVRCharacterBase::TryTeleport()
{
	if (!IsTeleportAllowed())
	{
		bTeleportTraceActive = false;
		if (TeleportViasualizerReference)
		{
			GetWorld()->DestroyActor(TeleportViasualizerReference);
		}
		TeleportTraceNiagaraSystem->SetVisibility(false);
		return;
	}

	bTeleportTraceActive = false;

	if (TeleportViasualizerReference)
	{
		GetWorld()->DestroyActor(TeleportViasualizerReference);
	}

	TeleportTraceNiagaraSystem->SetVisibility(false);

	if (!bValidTeleportTrace)
	{
		return;
	}

	bValidTeleportTrace = false;

	FVector CameraRelativeLocation = FVector::ZeroVector;
	if (USceneComponent* CameraComponent = GetComponentByClass<UCameraComponent>())
	{
		CameraRelativeLocation = CameraComponent->GetRelativeLocation();
	}

	FVector AdjustedRelativeLocation = CameraRelativeLocation;
	AdjustedRelativeLocation.Z = 0.0f;

	FRotator ActorRotation = GetActorRotation();
	float Yaw = ActorRotation.Yaw;

	FRotator YawRotation = FRotator(0.0f, Yaw, 0.0f);
	FVector RotatedOffset = YawRotation.RotateVector(AdjustedRelativeLocation);

	FVector DestLocation = ProjectedTeleportLocation - RotatedOffset;
	FRotator DestRotation = FRotator(0.0f, ActorRotation.Yaw, 0.0f);

	FixHeightAfterClimbing();

	if (TeleportViasualizerReference)
	{
		SetActorLocationAndRotation(DestLocation, DestRotation, false, nullptr, ETeleportType::TeleportPhysics);
		bTeleportTraceActive = false;
		TeleportViasualizerReference->SetActorHiddenInGame(true);
	}
}

void AVRCharacterBase::OnMouthBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!SurvivalComponent || !OtherActor) return;

    AVRConsumableActor* Consumable = Cast<AVRConsumableActor>(OtherActor);
    if (Consumable && Consumable->IsBeingHeld())
    {
        bIsOverlappingMouth = true;
        Consumable->Consume();
    }
}

void AVRCharacterBase::OnMouthEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    bIsOverlappingMouth = false;
}

void AVRCharacterBase::UpdateFallDetection()
{
    if (bFallDetectionTriggered)
        return;

    bool bCurrentlyFalling = GetCharacterMovement()->IsFalling();
    float CurrentHeight = GetActorLocation().Z;

    if (bCurrentlyFalling && !bIsFalling)
    {
        bIsFalling = true;
        FallStartHeight = CurrentHeight;
    }
    else if (bIsFalling && bCurrentlyFalling)
    {
        float FallDistance = FallStartHeight - CurrentHeight;

        if (FallDistance >= FallDistanceThreshold)
        {
            TriggerDeathScreen();
        }
    }
    else if (bIsFalling && !bCurrentlyFalling)
    {
        ResetFallDetection();
    }
}

void AVRCharacterBase::TriggerDeathScreen()
{
    if (bFallDetectionTriggered)
        return;

    bFallDetectionTriggered = true;

    if (DeathScreenWidgetClass)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            DeathScreenWidget = CreateWidget<UUserWidget>(PC, DeathScreenWidgetClass);
            if (DeathScreenWidget)
            {
                DeathScreenWidget->AddToViewport(999);
            }
        }
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->DisableInput(PC);
    }

    GetCharacterMovement()->SetMovementMode(MOVE_None);
    OnDeath();
}

void AVRCharacterBase::ResetFallDetection()
{
    bIsFalling = false;
    FallStartHeight = 0.0f;
}

bool AVRCharacterBase::CanReachLocation(const FVector& TargetLocation)
{
    if (!bValidateReachability)
        return true;

    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSystem)
        return true;

    float Distance = FVector::Distance(GetActorLocation(), TargetLocation);
    if (Distance > MaxTeleportDistance)
    {
        return false;
    }

    FNavLocation PlayerNavLocation;
    FNavLocation TargetNavLocation;

    bool bPlayerOnNavMesh = NavSystem->ProjectPointToNavigation(GetActorLocation(), PlayerNavLocation);
    bool bTargetOnNavMesh = NavSystem->ProjectPointToNavigation(TargetLocation, TargetNavLocation);

    if (!bPlayerOnNavMesh || !bTargetOnNavMesh)
    {
        return false;
    }

    float HeightDiff = FMath::Abs(TargetNavLocation.Location.Z - PlayerNavLocation.Location.Z);
    bool bSameLevel = HeightDiff < 100.0f;

    return bSameLevel;
}

void AVRCharacterBase::StartClimbing(AVRHand* GrabbingHand)
{
    if (!GrabbingHand || !GrabbingHand->GetMotionController())
    {
        return;
    }

    if (SurvivalComponent && !SurvivalComponent->CanStartClimbing())
    {
        GrabbingHand->ReleaseObject();
        return;
    }

    if (CurrentMovementState != EVRMovementState::Climbing)
    {
        CurrentMovementState = EVRMovementState::Climbing;
        GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        GetCharacterMovement()->GravityScale = 0.0f;

        if (SurvivalComponent)
        {
            SurvivalComponent->SetClimbingState(true);
        }
    }

    PrimaryClimbingHand = GrabbingHand;
    PrimaryHandClimbAnchor = GrabbingHand->GetMotionController()->GetComponentLocation();

    if (GrabbingHand->GetHandType() == EControllerHand::Left)
    {
        ClimbingHand_Left = GrabbingHand;
        LeftHandClimbAnchor = GrabbingHand->GetMotionController()->GetComponentLocation();
    }
    else
    {
        ClimbingHand_Right = GrabbingHand;
        RightHandClimbAnchor = GrabbingHand->GetMotionController()->GetComponentLocation();
    }
}

void AVRCharacterBase::StopClimbing(AVRHand* ReleasingHand)
{
    if (!ReleasingHand)
        return;

    if (ReleasingHand == ClimbingHand_Left)
    {
        ClimbingHand_Left = nullptr;
    }
    else if (ReleasingHand == ClimbingHand_Right)
    {
        ClimbingHand_Right = nullptr;
    }

    if (ReleasingHand == PrimaryClimbingHand)
    {
        if (ClimbingHand_Left && ClimbingHand_Left != ReleasingHand)
        {
            PrimaryClimbingHand = ClimbingHand_Left;
            PrimaryHandClimbAnchor = ClimbingHand_Left->GetMotionController()->GetComponentLocation();
        }
        else if (ClimbingHand_Right && ClimbingHand_Right != ReleasingHand)
        {
            PrimaryClimbingHand = ClimbingHand_Right;
            PrimaryHandClimbAnchor = ClimbingHand_Right->GetMotionController()->GetComponentLocation();
        }
        else
        {
            PrimaryClimbingHand = nullptr;
        }
    }

    if (ClimbingHand_Left == nullptr && ClimbingHand_Right == nullptr)
    {
        if (SurvivalComponent)
        {
            SurvivalComponent->SetClimbingState(false);
        }

        // Simple edge avoidance
        bool bAtEdge = IsCharacterAtEdge();
        bool bWouldGetPushed = WouldCharacterGetPushedSideways();

        if (bAtEdge && bWouldGetPushed)
        {
            FVector EdgeAvoidanceDirection = GetEdgeAvoidanceDirection();
            if (!EdgeAvoidanceDirection.IsNearlyZero())
            {
                FVector PushVector = EdgeAvoidanceDirection * 50.0f;
                FVector NewLocation = GetActorLocation() + PushVector;
                SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
            }
        }

        // Set to falling and let height fixing handle the rest
        CurrentMovementState = EVRMovementState::Falling;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        GetCharacterMovement()->GravityScale = 1.0f;

        PrimaryClimbingHand = nullptr;
    }
}

// VRCharacterBase.cpp - Updated IsCharacterStuckInGeometry function

bool AVRCharacterBase::IsCharacterStuckInGeometry() const
{
    if (!GetCapsuleComponent() || !Camera)
        return false;

    float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector CameraLocation = Camera->GetComponentLocation();
    FVector ActorLocation = GetActorLocation();

    // Raycast from head down to bottom of capsule
    FVector TraceStart = CameraLocation;
    FVector TraceEnd = FVector(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FHitResult HitResult;
    bool bHit = GetWorld()->LineTraceSingleByProfile(
        HitResult,
        TraceStart,
        TraceEnd,
        "BlockAll",
        QueryParams
    );

    if (!bHit)
        return false; // No ground found, we're falling

    // Check if the hit component is actually blocking pawns
    if (AActor* HitActor = HitResult.GetActor())
    {
        // Filter out grabbables (except climbables)
        if (Cast<AVRGrabbableActor>(HitActor) && !Cast<AVRClimbableActor>(HitActor))
        {
            return false; // Ignore grabbables
        }

        // Only consider it "stuck" if the geometry actually blocks pawns
        if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
        {
            ECollisionResponse PawnResponse = HitComponent->GetCollisionResponseToChannel(ECC_Pawn);

            // If it's not blocking pawns, then we're not really "stuck"
            if (PawnResponse != ECR_Block)
            {
                UE_LOG(LogTemp, Verbose, TEXT("Geometry doesn't block pawns, not stuck: %s"), *HitActor->GetName());
                return false;
            }
        }
    }

    // Calculate where we should be standing on this BLOCKING geometry
    float GroundZ = HitResult.Location.Z;
    float DesiredCharacterZ = GroundZ + CapsuleHalfHeight;
    float CurrentCharacterZ = ActorLocation.Z;

    // If difference is significant, we're stuck and need adjustment
    bool bIsStuck = FMath::Abs(DesiredCharacterZ - CurrentCharacterZ) > 15.0f; // 15cm tolerance

    if (bIsStuck)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character stuck in BLOCKING geometry - Height difference: %.2fcm"),
            FMath::Abs(DesiredCharacterZ - CurrentCharacterZ));
    }

    return bIsStuck;
}

void AVRCharacterBase::FixHeightAfterClimbing()
{
    if (!Camera || !GetCapsuleComponent())
        return;

    float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector CameraLocation = Camera->GetComponentLocation();
    FVector ActorLocation = GetActorLocation();

    // Find ground position with head-to-bottom raycast
    FVector TraceStart = CameraLocation;
    FVector TraceEnd = FVector(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - CapsuleHalfHeight - 200.0f); // Extra 2m search

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FHitResult GroundHit;
    bool bFoundGround = GetWorld()->LineTraceSingleByProfile(
        GroundHit,
        TraceStart,
        TraceEnd,
        "BlockAll",
        QueryParams
    );

    if (!bFoundGround)
    {
        // No ground found, just enable falling
        CurrentMovementState = EVRMovementState::Falling;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        GetCharacterMovement()->GravityScale = 1.0f;
        return;
    }

    // Filter out grabbables (except climbables)
    if (AActor* HitActor = GroundHit.GetActor())
    {
        if (Cast<AVRGrabbableActor>(HitActor) && !Cast<AVRClimbableActor>(HitActor))
        {
            // Try again ignoring this grabbable
            QueryParams.AddIgnoredActor(HitActor);
            bFoundGround = GetWorld()->LineTraceSingleByProfile(
                GroundHit,
                TraceStart,
                TraceEnd,
                "BlockAll",
                QueryParams
            );

            if (!bFoundGround)
            {
                CurrentMovementState = EVRMovementState::Falling;
                GetCharacterMovement()->SetMovementMode(MOVE_Falling);
                GetCharacterMovement()->GravityScale = 1.0f;
                return;
            }
        }
    }

    // Calculate target position
    float GroundZ = GroundHit.Location.Z;
    float TargetCharacterZ = GroundZ + CapsuleHalfHeight;
    FVector TargetPosition = FVector(ActorLocation.X, ActorLocation.Y, TargetCharacterZ);

    // Check head clearance at target position
    FVector HeadCheckStart = FVector(ActorLocation.X, ActorLocation.Y, TargetCharacterZ);
    FVector HeadCheckEnd = HeadCheckStart + FVector(0, 0, MinHeadClearance);

    FHitResult HeadHit;
    bool bHeadBlocked = GetWorld()->LineTraceSingleByProfile(
        HeadHit,
        HeadCheckStart,
        HeadCheckEnd,
        "BlockAll",
        QueryParams
    );

    if (bHeadBlocked)
    {
        // Check if head hit is from a grabbable (except climbable) that we should ignore
        if (AActor* HeadHitActor = HeadHit.GetActor())
        {
            if (Cast<AVRGrabbableActor>(HeadHitActor) && !Cast<AVRClimbableActor>(HeadHitActor))
            {
                bHeadBlocked = false; // Ignore this collision
            }
        }
    }

    if (bHeadBlocked)
    {
        // Head would be blocked, can't place character here - fall instead
        UE_LOG(LogTemp, Warning, TEXT("Head would be blocked at target position, falling instead"));
        CurrentMovementState = EVRMovementState::Falling;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        GetCharacterMovement()->GravityScale = 1.0f;
        return;
    }

    // Instant snap to target position
    UE_LOG(LogTemp, Warning, TEXT("Instantly snapping character to ground position"));
    SetActorLocation(TargetPosition, false, nullptr, ETeleportType::TeleportPhysics);

    // Set to falling state to let physics take over
    CurrentMovementState = EVRMovementState::Falling;
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    GetCharacterMovement()->GravityScale = 1.0f;
}

#pragma region Locomotion Settings

bool AVRCharacterBase::ToggleJoystickMovement()
{
    bJoystickMovementEnabled = !bJoystickMovementEnabled;
    return bJoystickMovementEnabled;
}

bool AVRCharacterBase::ToggleTeleportMovement()
{
    bTeleportEnabled = !bTeleportEnabled;
    return bTeleportEnabled;
}

void AVRCharacterBase::LockMovement()
{
    bMovementLocked = true;
}

void AVRCharacterBase::UnlockMovement()
{
    bMovementLocked = false;
}

bool AVRCharacterBase::IsJoystickMovementAllowed() const
{
    return bJoystickMovementEnabled && !bMovementLocked;
}

bool AVRCharacterBase::IsTeleportAllowed() const
{
    return bTeleportEnabled && !bMovementLocked;
}

bool AVRCharacterBase::ToggleSmoothTurn()
{
    bSmoothTurnEnabled = !bSmoothTurnEnabled;
    bSnapTurnEnabled = !bSmoothTurnEnabled;
    return bSmoothTurnEnabled;
}

bool AVRCharacterBase::ToggleSnapTurn()
{
    bSmoothTurnEnabled = !bSmoothTurnEnabled;
    bSnapTurnEnabled = !bSmoothTurnEnabled;
    return !bSmoothTurnEnabled;
}

bool AVRCharacterBase::SetSnapTurnDegrees(ESnapTurnDegrees Degrees)
{
    CurrentSnapTurnDegrees = Degrees;
    SnapTurnDegrees = ConvertSnapTurnEnumToFloat(Degrees);
    return true;
}

float AVRCharacterBase::GetSnapTurnDegreesAsFloat() const
{
    return ConvertSnapTurnEnumToFloat(CurrentSnapTurnDegrees);
}

ESnapTurnDegrees AVRCharacterBase::GetSnapTurnDegreesAsEnum() const
{
    return CurrentSnapTurnDegrees;
}

float AVRCharacterBase::ConvertSnapTurnEnumToFloat(ESnapTurnDegrees EnumValue) const
{
    switch (EnumValue)
    {
    case ESnapTurnDegrees::Deg15:
        return 15.0f;
    case ESnapTurnDegrees::Deg30:
        return 30.0f;
    case ESnapTurnDegrees::Deg45:
        return 45.0f;
    default:
        return 30.0f;
    }
}

void AVRCharacterBase::UpdateSnapTurnSettings()
{
    SnapTurnDegrees = ConvertSnapTurnEnumToFloat(CurrentSnapTurnDegrees);
}

#pragma endregion

#pragma region Headlight System

void AVRCharacterBase::SetHeadlightEnabled(bool bEnabled)
{
    if (!HeadlightComponent)
        return;

    HeadlightComponent->SetIntensity(bEnabled ? 1200.0f : 0.0f);

    UE_LOG(LogTemp, Log, TEXT("Headlight %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
}

bool AVRCharacterBase::IsHeadlightEnabled() const
{
    return HeadlightComponent && HeadlightComponent->Intensity > 0.0f;
}

#pragma endregion