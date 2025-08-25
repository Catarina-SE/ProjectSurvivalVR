// Fill out your copyright notice in the Description page of Project Settings.

#include "Hands/VRHand.h"
#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Structures/FingerData.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Actors/VRClimbableActor.h"

TArray<AVRHand*> AVRHand::VRHands;

AVRHand::AVRHand()
{
    PrimaryActorTick.bCanEverTick = true;

    VROrigin = CreateDefaultSubobject<USceneComponent>("VROrigin");
    SetRootComponent(VROrigin);

    MotionController = CreateDefaultSubobject<UMotionControllerComponent>("MotionController");
    MotionController->SetupAttachment(VROrigin);

    WidgetInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>("WidgetInteraction");
    WidgetInteraction->SetupAttachment(MotionController);

    HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HandMesh");
    HandMesh->SetupAttachment(VROrigin);

    HandPhysicsConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>("HandPhysicsConstraint");
    HandPhysicsConstraint->SetupAttachment(MotionController);

    HandOriginPoint = CreateDefaultSubobject<UStaticMeshComponent>("HandOriginPoint");
    HandOriginPoint->SetupAttachment(MotionController);

    GrabConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>("GrabConstraint");
    GrabConstraint->SetupAttachment(MotionController);

    GrabSphere = CreateDefaultSubobject<USphereComponent>("GrabSphere");
    GrabSphere->SetupAttachment(HandMesh);

    Spline_Thumb = CreateDefaultSubobject<USplineComponent>("Spline_Thumb");
    Spline_Thumb->SetupAttachment(HandMesh);

    Spline_Index = CreateDefaultSubobject<USplineComponent>("Spline_Index");
    Spline_Index->SetupAttachment(HandMesh);

    Spline_Middle = CreateDefaultSubobject<USplineComponent>("Spline_Middle");
    Spline_Middle->SetupAttachment(HandMesh);

    Spline_Ring = CreateDefaultSubobject<USplineComponent>("Spline_Ring");
    Spline_Ring->SetupAttachment(HandMesh);

    Spline_Pinky = CreateDefaultSubobject<USplineComponent>("Spline_Pinky");
    Spline_Pinky->SetupAttachment(HandMesh);

    GrabPointIndicator = CreateDefaultSubobject<UStaticMeshComponent>("GrabPointIndicator");
    GrabPointIndicator->SetupAttachment(VROrigin);
    GrabPointIndicator->SetHiddenInGame(true);
    GrabPointIndicator->SetGenerateOverlapEvents(true);
    GrabPointIndicator->SetCollisionProfileName("NoCollision");

    SocketName = (HandType == EControllerHand::Left) ? FName("HandGripSocket_l") : FName("HandGripSocket_r");

    RootBoneName = (HandType == EControllerHand::Left) ? FName("hand_l") : FName("hand_r");
}

FFingerData AVRHand::GetFingerData()
{
    return FingerData;
}

bool AVRHand::IsGrabbing()
{
    return bIsGrabbing;
}

void AVRHand::BeginPlay()
{
    Super::BeginPlay();

    if (HandType != EControllerHand::Left && HandType != EControllerHand::Right) {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, FString::Printf(TEXT("Class %s: Wrong HandType"), *GetClass()->GetName()));
    }

    bIsGrabbing = false;
    SetupInputBindings();
    SetupFingerAnimationData();
    VRHands.AddUnique(this);

    if (GrabSphere)
    {
        GrabSphere->OnComponentBeginOverlap.AddDynamic(this, &AVRHand::OnGrabSphereBeginOverlap);
        GrabSphere->OnComponentEndOverlap.AddDynamic(this, &AVRHand::OnGrabSphereEndOverlap);
    }
}

void AVRHand::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    VRHands.Remove(this);
}

void AVRHand::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    switch (HandType)
    {
    case EControllerHand::Left:
        MotionController->MotionSource = "Left";
        break;
    case EControllerHand::Right:
        MotionController->MotionSource = "Right";
        break;
    default:
        break;
    }
}

void AVRHand::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateHoveredGrabbable();

    // Check if we are currently hovering over ANY interactable object.
    if (HoveredInteractable)
    {
        // Now, try to cast our generic interactable to the SPECIFIC class we need.
        AVRGrabbableActor* GrabbableActor = Cast<AVRGrabbableActor>(HoveredInteractable.GetObject());

        // If the cast is successful, it means we are hovering over a VRGrabbableActor.
        if (GrabbableActor && !bIsGrabbing)
        {
            // It is safe to call the grabbable-specific function now.
            FGrabPointInfo NearbyGrabPoint = GetClosestAvailableGrabPoint(GrabbableActor);

            if (NearbyGrabPoint.bIsAvailable)
            {
                // Show grab point indicator (for snap grabs)
                GrabPointIndicator->SetWorldLocation(NearbyGrabPoint.Location);
                GrabPointIndicator->SetHiddenInGame(false);
                CurrentTargetGrabPoint = NearbyGrabPoint.Type;
            }
            else
            {
                // No grab point available, hide indicator (outline will show via OnHoverChanged for free grabs)
                GrabPointIndicator->SetHiddenInGame(true);
                CurrentTargetGrabPoint = EGrabPointType::None;
            }
        }
        else // This means we are hovering over something else (like a VRClimbableActor) or are grabbing.
        {
            // Hide the grab point indicator because climbables don't use it.
            GrabPointIndicator->SetHiddenInGame(true);
            CurrentTargetGrabPoint = EGrabPointType::None;
        }
    }
    else // Not hovering over anything.
    {
        GrabPointIndicator->SetHiddenInGame(true);
        CurrentTargetGrabPoint = EGrabPointType::None;
    }
}

void AVRHand::SetupInputBindings()
{
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
        return;

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        EnhancedInputComponent->BindAction(GrabPressed, ETriggerEvent::Triggered, this, &AVRHand::GrabObject);
        EnhancedInputComponent->BindAction(GrabReleased, ETriggerEvent::Triggered, this, &AVRHand::ReleaseObject);
        EnhancedInputComponent->BindAction(OpenMenu, ETriggerEvent::Triggered, this, &AVRHand::ToggleMenu);
    }
}

FGrabPointInfo AVRHand::GetClosestAvailableGrabPoint(AVRGrabbableActor* Object)
{
    FGrabPointInfo Result;
    Result.bIsAvailable = false;

    if (!Object)
        return Result;

    // Check if this object should use grab points at all
    if (!Object->ShouldUseGrabPoints())
    {
        return Result; // Return empty result, will trigger free grab
    }

    // NEW: Check current grab state - if object is free grabbed, snap grabs are allowed
    // If object is snap grabbed, only allow snaps to available points or force switches
    EGrabState CurrentState = Object->GetCurrentGrabState();

    FVector HandLocation = HandOriginPoint->GetComponentLocation();
    bool bIsLeftHand = (HandType == EControllerHand::Left);

    TArray<FGrabPointInfo> AvailablePoints;

    // Check main grab point
    if (Object->ShouldUseMainGrabPoint() && Object->HasMainGrabSocket(bIsLeftHand))
    {
        bool bCanGrabMain = false;

        if (CurrentState == EGrabState::NotGrabbed || CurrentState == EGrabState::FreeGrab)
        {
            // Not grabbed or free grabbed - can snap to main if available
            bCanGrabMain = Object->IsMainGrabPointAvailable();
        }
        else if (CurrentState == EGrabState::SnapGrab)
        {
            // Snap grabbed - can grab main if available, or force switch if occupied
            bCanGrabMain = true; // Always allow attempt (force switch if needed)
        }

        if (bCanGrabMain)
        {
            FGrabPointInfo MainInfo;
            MainInfo.bIsAvailable = true;
            MainInfo.Location = Object->GetMainGrabPointTransform().GetLocation();
            MainInfo.Type = EGrabPointType::Main;
            MainInfo.Distance = FVector::Distance(HandLocation, MainInfo.Location);
            MainInfo.SocketTransform = Object->GetMainGrabSocketTransform(bIsLeftHand);
            AvailablePoints.Add(MainInfo);
        }
    }

    // Check secondary grab point
    if (Object->ShouldUseSecondaryGrabPoint() && Object->HasSecondaryGrabSocket(bIsLeftHand))
    {
        bool bCanGrabSecondary = false;

        if (CurrentState == EGrabState::NotGrabbed || CurrentState == EGrabState::FreeGrab)
        {
            // Not grabbed or free grabbed - can snap to secondary if available
            bCanGrabSecondary = Object->IsSecondaryGrabPointAvailable();
        }
        else if (CurrentState == EGrabState::SnapGrab)
        {
            // Snap grabbed - can grab secondary if available, or force switch if occupied
            bCanGrabSecondary = true; // Always allow attempt (force switch if needed)
        }

        if (bCanGrabSecondary)
        {
            FGrabPointInfo SecondInfo;
            SecondInfo.bIsAvailable = true;
            SecondInfo.Location = Object->GetSecondaryGrabPointTransform().GetLocation();
            SecondInfo.Type = EGrabPointType::Secondary;
            SecondInfo.Distance = FVector::Distance(HandLocation, SecondInfo.Location);
            SecondInfo.SocketTransform = Object->GetSecondaryGrabSocketTransform(bIsLeftHand);
            AvailablePoints.Add(SecondInfo);
        }
    }

    // Find closest point within snap range
    for (const auto& Point : AvailablePoints)
    {
        if (Point.Distance < SnapRange && Point.Distance < Result.Distance)
        {
            Result = Point;
        }
    }

    return Result;
}

void AVRHand::OnGrabSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->Implements<UInteractable>())
    {
        TScriptInterface<IInteractable> Interactable(OtherActor);
        if (!OverlappingInteractables.Contains(Interactable))
        {
            OverlappingInteractables.Add(Interactable);
            UpdateHoveredGrabbable();
        }
    }
}

void AVRHand::OnGrabSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor->Implements<UInteractable>())
    {
        OverlappingInteractables.Remove(TScriptInterface<IInteractable>(OtherActor));
        UpdateHoveredGrabbable();
    }
}

void AVRHand::UpdateHoveredGrabbable()
{
    TScriptInterface<IInteractable> ClosestInteractable = nullptr;

    if (OverlappingInteractables.Num() > 0)
    {
        float ClosestDistance = FLT_MAX;
        FVector HandOriginLocation = HandOriginPoint->GetComponentLocation();

        for (TScriptInterface<IInteractable> Interactable : OverlappingInteractables)
        {
            if (Interactable.GetInterface())
            {
                UPrimitiveComponent* CollisionComponent = Interactable->GetGrabCollisionComponent();
                if (CollisionComponent)
                {
                    FVector ClosestPoint;
                    CollisionComponent->GetClosestPointOnCollision(HandOriginLocation, ClosestPoint);
                    float Distance = FVector::Distance(HandOriginLocation, ClosestPoint);

                    if (Distance < ClosestDistance)
                    {
                        ClosestDistance = Distance;
                        ClosestInteractable = Interactable;
                    }
                }
            }
        }

        // Compare with the current HoveredInteractable, not itself
        if (ClosestInteractable != HoveredInteractable)
        {
            // Clear previous hover
            if (HoveredInteractable.GetInterface())
            {
                OnHoverCleared();
            }

            // Set new hover
            HoveredInteractable = ClosestInteractable;

            if (HoveredInteractable.GetInterface())
            {
                OnHoverChanged();
            }
        }
    }
    else
    {
        if (HoveredInteractable.GetInterface())
        {
            OnHoverCleared();
            HoveredInteractable = nullptr;
        }
    }
}

void AVRHand::GrabObject()
{
    if (GrabbedActor || !HoveredInteractable)
        return;

    // Handle climbing interactions
    AVRClimbableActor* ClimbableActor = Cast<AVRClimbableActor>(HoveredInteractable.GetObject());
    if (ClimbableActor)
    {
        bIsGrabbing = true;
        GrabbedActor = TScriptInterface<IInteractable>(ClimbableActor);
        GrabbedPrimitiveComponent = GrabbedActor->GetGrabCollisionComponent();
        GrabPointIndicator->SetHiddenInGame(true);
        OnGrab();
        OnGrabClimbable(ClimbableActor);

        // Freeze hand mesh at current position for climbing
        if (HandMesh)
        {
            FrozenHandTransform = HandMesh->GetComponentTransform();
            HandMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            HandMesh->SetAllBodiesBelowSimulatePhysics(RootBoneName, false);
            UE_LOG(LogTemp, Warning, TEXT("VRHand: Hand mesh frozen at climbing position"));
        }

        // Register climb interaction with character controller
        ClimbableActor->OnGrab(HandMesh, HandMesh->GetComponentLocation(),
            (HandType == EControllerHand::Left), HandMesh->GetCollisionObjectType());

        // Initialize distance-based auto-release monitoring
        GetWorldTimerManager().SetTimer(
            DistanceCheckTimerHandle,
            this,
            &AVRHand::CheckHandControllerDistance,
            DistanceCheckInterval,
            true
        );

        TraceFingerData();

        UE_LOG(LogTemp, Warning, TEXT("VRHand: Climbing grab setup complete"));
        return;
    }

    // Handle grabbable object interactions
    AVRGrabbableActor* TargetGrabbable = Cast<AVRGrabbableActor>(HoveredInteractable.GetObject());
    if (!IsValid(TargetGrabbable))
    {
        UE_LOG(LogTemp, Error, TEXT("TargetGrabbable is invalid!"));
        HoveredInteractable = nullptr;
        return;
    }

    TScriptInterface<IInteractable> InteractableObject(TargetGrabbable);
    UPrimitiveComponent* ObjectPhysicsComponent = InteractableObject->GetPhysicsComponent();
    if (!ObjectPhysicsComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No physics component found on grabbable object"));
        return;
    }

    // Determine grab mode based on proximity to available grab points
    FGrabPointInfo NearbyGrabPoint = GetClosestAvailableGrabPoint(TargetGrabbable);
    bool bWillSnap = NearbyGrabPoint.bIsAvailable;
    EGrabPointType GrabPointTypeForAttempt = bWillSnap ? NearbyGrabPoint.Type : EGrabPointType::None;

    // Validate grab attempt against object state rules
    EGrabState CurrentObjectState = TargetGrabbable->GetCurrentGrabState();

    UE_LOG(LogTemp, Warning, TEXT("Attempting grab - Will Snap: %s, Target Point: %d, Object State: %d"),
        bWillSnap ? TEXT("YES") : TEXT("NO"), (int32)GrabPointTypeForAttempt, (int32)CurrentObjectState);

    // Enforce grab state restrictions
    if (CurrentObjectState == EGrabState::SnapGrab && !bWillSnap)
    {
        UE_LOG(LogTemp, Warning, TEXT("Free grab rejected - object is snap grabbed"));
        return;
    }

    // For free grabs, determine logical grab point assignment for tracking
    if (!bWillSnap && TargetGrabbable->ShouldUseGrabPoints())
    {
        FVector HandLocation = HandOriginPoint->GetComponentLocation();
        float MainDistance = FLT_MAX;
        float SecondaryDistance = FLT_MAX;

        if (TargetGrabbable->ShouldUseMainGrabPoint())
        {
            MainDistance = FVector::Distance(HandLocation, TargetGrabbable->GetMainGrabPointTransform().GetLocation());
        }

        if (TargetGrabbable->ShouldUseSecondaryGrabPoint())
        {
            SecondaryDistance = FVector::Distance(HandLocation, TargetGrabbable->GetSecondaryGrabPointTransform().GetLocation());
        }

        // Assign closest logical grab point for state tracking
        GrabPointTypeForAttempt = (MainDistance < SecondaryDistance) ? EGrabPointType::Main : EGrabPointType::Secondary;
    }

    // Initialize grab state
    bIsGrabbing = true;
    GrabbedActor = InteractableObject;
    GrabbedPrimitiveComponent = GrabbedActor->GetGrabCollisionComponent();
    GrabPointIndicator->SetHiddenInGame(true);
    OnGrab();
    OnGrabGrabbable(TargetGrabbable);

    // Execute standard grab interface
    bool bIsLeftHand = (HandType == EControllerHand::Left);
    ECollisionChannel HandChannel = HandMesh->GetCollisionObjectType();

    InteractableObject->OnGrab(HandMesh, HandMesh->GetComponentLocation(), bIsLeftHand, HandChannel);

    // Execute unified grab with conflict resolution
    TargetGrabbable->OnUnifiedGrab(HandMesh, bIsLeftHand, bWillSnap, GrabPointTypeForAttempt);

    // Validate grab acceptance
    if (!TargetGrabbable->IsBeingHeld())
    {
        UE_LOG(LogTemp, Warning, TEXT("Grab was rejected by object - cleaning up"));

        // Cleanup failed grab attempt
        bIsGrabbing = false;
        GrabbedActor = nullptr;
        GrabbedPrimitiveComponent = nullptr;
        return;
    }

    // Setup physics constraints for free grabs only (snap grabs use direct attachment)
    if (!bWillSnap)
    {
        SetupGrabConstraint(ObjectPhysicsComponent, bWillSnap, GrabPointTypeForAttempt);
    }

    // Initialize monitoring systems
    GetWorldTimerManager().SetTimer(
        DistanceCheckTimerHandle,
        this,
        &AVRHand::CheckHandControllerDistance,
        DistanceCheckInterval,
        true
    );

    TraceFingerData();

    UE_LOG(LogTemp, Display, TEXT("Grab complete - Hand: %s, Snapping: %s, Type: %d"),
        *GetName(), bWillSnap ? TEXT("YES") : TEXT("NO"), (int32)GrabPointTypeForAttempt);
}

void AVRHand::SetupGrabConstraint(UPrimitiveComponent* ObjectPhysicsComponent, bool bIsSnapping, EGrabPointType GrabPointType)
{
    GrabConstraint->SetDisableCollision(true);

    FName HandBoneName = (HandType == EControllerHand::Left) ? FName("hand_l") : FName("hand_r");

    GrabConstraint->SetConstrainedComponents(
        HandMesh,
        HandBoneName,
        ObjectPhysicsComponent,
        NAME_None
    );

    // Lock all movements
    GrabConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    GrabConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    GrabConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    GrabConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    GrabConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    GrabConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);

    // Set drive values based on grab type
    float LinearStrength = bIsSnapping ? 1000.0f : 800.0f;  // Snap grabs slightly stronger
    float AngularStrength = bIsSnapping ? 1000.0f : 800.0f;
    float LinearDamping = bIsSnapping ? 100.0f : 80.0f;
    float AngularDamping = bIsSnapping ? 100.0f : 80.0f;

    GrabConstraint->SetLinearPositionDrive(true, true, true);
    GrabConstraint->SetLinearVelocityDrive(true, true, true);
    GrabConstraint->SetLinearDriveParams(LinearStrength, LinearDamping, 0.0f);

    GrabConstraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
    GrabConstraint->SetAngularOrientationDrive(true, true);
    GrabConstraint->SetAngularVelocityDrive(true, true);
    GrabConstraint->SetAngularDriveParams(AngularStrength, AngularDamping, 0.0f);

    UE_LOG(LogTemp, Display, TEXT("Grab constraint setup - Snapping: %s"), bIsSnapping ? TEXT("YES") : TEXT("NO"));
}

void AVRHand::ReleaseObject()
{
    if (!GrabbedActor.GetObject())
        return;

    UE_LOG(LogTemp, Warning, TEXT("VRHand: Releasing object with %s hand"),
        (HandType == EControllerHand::Left) ? TEXT("LEFT") : TEXT("RIGHT"));

    OnRelease();

    // Clear the distance check timer
    GetWorldTimerManager().ClearTimer(DistanceCheckTimerHandle);

    // Check what type of actor we're releasing
    AVRClimbableActor* ClimbableActor = Cast<AVRClimbableActor>(GrabbedActor.GetObject());
    AVRGrabbableActor* GrabbableActor = Cast<AVRGrabbableActor>(GrabbedActor.GetObject());

    GrabbedActor->OnRelease(HandMesh);

    if (ClimbableActor)
    {
        // For climbing, just re-enable physics on the hand mesh.
        // The hand will snap back to the motion controller's position.

        HandMesh->AttachToComponent(MotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale, RootBoneName);
        HandMesh->SetAllBodiesBelowSimulatePhysics(RootBoneName, true);
        HandMesh->SetAllBodiesBelowPhysicsBlendWeight(RootBoneName, PhysicsBlendWeight);
    }
    else if (GrabbableActor)
    {
        // Break physics constraint for grabbable objects
        UE_LOG(LogTemp, VeryVerbose, TEXT("VRHand: Breaking physics constraint for grabbable"));
        GrabConstraint->BreakConstraint();
    }

    // Reset hand state
    GrabbedActor = nullptr;
    GrabbedPrimitiveComponent = nullptr;
    bIsGrabbing = false;

    // Reset finger data
    FingerData.Thumb = 0.0f;
    FingerData.Index = 0.0f;
    FingerData.Middle = 0.0f;
    FingerData.Ring = 0.0f;
    FingerData.Pinky = 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("VRHand: Release complete"));
}

bool AVRHand::IsClimbing() const
{
    AVRClimbableActor* ClimbableActor = Cast<AVRClimbableActor>(GrabbedActor.GetObject());
    return ClimbableActor != nullptr && bIsGrabbing;
}

void AVRHand::CheckHandControllerDistance()
{
    if (GrabbedActor)
    {
        float Distance = FVector::Distance(HandMesh->GetComponentLocation(), MotionController->GetComponentLocation());
        float EffectiveThreshold = GrabReleaseThreshold;

        // Check if other hand is also grabbing the same object
        for (AVRHand* OtherHand : VRHands)
        {
            if (OtherHand != this && OtherHand->GrabbedActor.GetObject() == GrabbedActor.GetObject())
            {
                EffectiveThreshold *= 2.0f;
                break;
            }
        }

        if (Distance > EffectiveThreshold)
        {
            UE_LOG(LogTemp, Display, TEXT("Auto-release: %.2f > %.2f"), Distance, EffectiveThreshold);
            ReleaseObject();
        }
    }
}

TScriptInterface<IInteractable> AVRHand::GetGrabbedActor() const
{
    return GrabbedActor;
}

void AVRHand::OnHoverChanged_Implementation()
{
    if (HoveredInteractable)
    {
        // Check if this is a grabbable actor for smart visual feedback
        AVRGrabbableActor* GrabbableActor = Cast<AVRGrabbableActor>(HoveredInteractable.GetObject());
        if (GrabbableActor)
        {
            // Check if we can snap to a grab point
            FGrabPointInfo NearbyGrabPoint = GetClosestAvailableGrabPoint(GrabbableActor);
            bool bCanSnapToGrabPoint = NearbyGrabPoint.bIsAvailable;

            EGrabState CurrentState = GrabbableActor->GetCurrentGrabState();

            // If object is snap grabbed, only show indicators/outline based on available snap points
            if (CurrentState == EGrabState::SnapGrab)
            {
                if (bCanSnapToGrabPoint)
                {
                    // Show grab point indicator, no outline
                    HoveredInteractable->ShowCanBeGrabbed(false); // No outline
                }
                else
                {
                    // No available snap points, no visual feedback at all
                    HoveredInteractable->ShowCanBeGrabbed(false);
                }
            }
            else
            {
                // Object is not snap grabbed (free grab or not grabbed)
                if (bCanSnapToGrabPoint)
                {
                    // Show grab point indicator, no outline
                    HoveredInteractable->ShowCanBeGrabbed(false); // No outline
                }
                else
                {
                    // No grab point detected, show outline for free grab
                    HoveredInteractable->ShowCanBeGrabbed(true); // Show outline
                }
            }
        }
        else
        {
            // Not a grabbable actor (probably climbable), show outline normally
            HoveredInteractable->ShowCanBeGrabbed(true);
        }
    }
}

void AVRHand::OnHoverCleared_Implementation()
{
    if (HoveredInteractable)
    {
        HoveredInteractable->ShowCanBeGrabbed(false);
    }
}

FName AVRHand::GetHandGripSocketName() const
{
    return SocketName;
}

FName AVRHand::GetHandBoneName() const
{
    return RootBoneName;
}

void AVRHand::SetupFingerAnimationData()
{
    FingerCache_Thumb = GetFingerSteps(Spline_Thumb);
    FingerCache_Index = GetFingerSteps(Spline_Index);
    FingerCache_Middle = GetFingerSteps(Spline_Middle);
    FingerCache_Ring = GetFingerSteps(Spline_Ring);
    FingerCache_Pinky = GetFingerSteps(Spline_Pinky);

    Spline_Thumb->DestroyComponent();
    Spline_Index->DestroyComponent();
    Spline_Middle->DestroyComponent();
    Spline_Ring->DestroyComponent();
    Spline_Pinky->DestroyComponent();
}

TArray<FVector> AVRHand::GetFingerSteps(USplineComponent* FingerSpline)
{
    TArray<FVector> TempCache;
    float StepSize = 1.0f / static_cast<float>(FingerSteps);

    for (int32 i = 0; i <= FingerSteps; i++)
    {
        float Time = FMath::Clamp((i * StepSize), 0.0f, 1.0f);
        FVector LocationAtTime = FingerSpline->GetLocationAtTime(Time, ESplineCoordinateSpace::Local, false);
        FVector TransformLocation = UKismetMathLibrary::TransformLocation(FingerSpline->GetRelativeTransform(), LocationAtTime);
        TempCache.Add(TransformLocation);
    }

    return TempCache;
}

void AVRHand::TraceFingerData()
{
    FingerData.Thumb = TraceFingerSegment(FingerCache_Thumb);
    FingerData.Index = TraceFingerSegment(FingerCache_Index);
    FingerData.Middle = TraceFingerSegment(FingerCache_Middle);
    FingerData.Ring = TraceFingerSegment(FingerCache_Ring);
    FingerData.Pinky = TraceFingerSegment(FingerCache_Pinky);
}

float AVRHand::TraceFingerSegment(TArray<FVector> FingerCacheArray)
{
    if (!GrabbedPrimitiveComponent)
    {
        return 1.0f;
    }

    float BendValue = 0.0f;
    FTransform HandTransform = HandMesh->GetComponentTransform();

    for (int32 i = 0; i < FingerCacheArray.Num() - 1; i++)
    {
        FVector StartLocal = FingerCacheArray[i];
        FVector EndLocal = FingerCacheArray[i + 1];

        FVector Start = HandTransform.TransformPosition(StartLocal);
        FVector End = HandTransform.TransformPosition(EndLocal);

        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
        Params.bTraceComplex = true;

        FHitResult HitResult;

        bool bHit = GrabbedPrimitiveComponent->LineTraceComponent(
            HitResult,
            Start,
            End,
            Params
        );

        if (bHit)
        {
            BendValue = static_cast<float>(i) / static_cast<float>(FingerCacheArray.Num() - 1);
            float SegmentFraction = HitResult.Time;
            float SegmentContribution = 1.0f / static_cast<float>(FingerCacheArray.Num() - 1);
            BendValue += SegmentContribution * SegmentFraction;
            break;
        }
    }

    if (BendValue == 0.0f && FingerCacheArray.Num() > 1)
    {
        return 1.0f;
    }

    return BendValue;
}