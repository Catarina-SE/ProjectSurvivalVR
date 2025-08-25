// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/VRGrabbableActor.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Hands/VRHand.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

AVRGrabbableActor::AVRGrabbableActor()
{
    GrabPointMain = CreateDefaultSubobject<UStaticMeshComponent>("GrabPoint_Main");
    GrabPointMain->SetupAttachment(ActorMesh);
    GrabPointMain->SetCollisionProfileName(TEXT("NoCollision"));
    GrabPointMain->SetHiddenInGame(true);

    GrabPointSecond = CreateDefaultSubobject<UStaticMeshComponent>("GrabPoint_Second");
    GrabPointSecond->SetupAttachment(ActorMesh);
    GrabPointSecond->SetCollisionProfileName(TEXT("NoCollision"));
    GrabPointSecond->SetHiddenInGame(true);
}

void AVRGrabbableActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SetupPhysics();
}

void AVRGrabbableActor::BeginPlay()
{
    Super::BeginPlay();

    SetupPhysics();
    bWasSimulatingPhysics = ActorMesh->IsSimulatingPhysics();

    // Initialize grip transform
    InitialGripTransform = FTransform::Identity;
    SecondaryGripTransform = FTransform::Identity;

    // Grabbables never collide with Pawn (player character)
    ActorMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void AVRGrabbableActor::SetupPhysics()
{
    if (!ActorMesh)
        return;

    if (bStartSimulatePhysics)
    {
        ActorMesh->SetSimulatePhysics(true);
        ActorMesh->SetEnableGravity(true);
        ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        // Force bSimulatePhysicsOnGrab to true when bStartSimulatePhysics is true
        bSimulatePhysicsOnGrab = true;
    }
    else
    {
        ActorMesh->SetSimulatePhysics(false);
        ActorMesh->SetEnableGravity(false);
        ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // bSimulatePhysicsOnGrab can be either true or false
    }
}

void AVRGrabbableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // If we have both hands grabbing, update rotation
    if (bIsHeld && MainGrabPointHand && SecondaryGrabPointHand)
    {
        UpdateTwoHandedRotation();
    }
}

void AVRGrabbableActor::OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation, bool bIsLeftHand, ECollisionChannel HandChannel)
{
    bIsHeld = true;

    // Apply physics settings on grab
    if (bSimulatePhysicsOnGrab && !ActorMesh->IsSimulatingPhysics())
    {
        ActorMesh->SetSimulatePhysics(true);
        ActorMesh->SetEnableGravity(true);
        ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    // Just ignore collision with this specific hand channel
    ActorMesh->SetCollisionResponseToChannel(HandChannel, ECR_Ignore);

    ShowCanBeGrabbed(false);
}

void AVRGrabbableActor::OnRelease(USkeletalMeshComponent* InComponent)
{
    if (!InComponent)
        return;

    // Get hand channel and restore collision with this specific hand
    ECollisionChannel HandChannel = InComponent->GetCollisionObjectType();
    ActorMesh->SetCollisionResponseToChannel(HandChannel, ECR_Block);

    // Remove from free grabbing hands if present
    bool bWasInFreeGrabbingHands = FreeGrabbingHands.Remove(InComponent) > 0;

    // Only handle grab point releases for actual snap grabs (not free grabs)
    if (!bWasInFreeGrabbingHands)
    {
        if (InComponent == MainGrabPointHand)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Main grab point hand released"), *GetName());
            bMainGrabPointOccupied = false;
            MainGrabPointHand = nullptr;
        }
        else if (InComponent == SecondaryGrabPointHand)
        {
            UE_LOG(LogTemp, Display, TEXT("%s: Secondary grab point hand released"), *GetName());
            bSecondaryGrabPointOccupied = false;
            SecondaryGrabPointHand = nullptr;
        }

        // Clean up second hand constraint if it exists
        if (SecondHandConstraints.Contains(InComponent))
        {
            UPhysicsConstraintComponent* ConstraintToRemove = SecondHandConstraints[InComponent];
            if (ConstraintToRemove)
            {
                ConstraintToRemove->BreakConstraint();
                ConstraintToRemove->DestroyComponent();
                UE_LOG(LogTemp, Display, TEXT("Destroyed second hand constraint"));
            }
            SecondHandConstraints.Remove(InComponent);
        }
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Free grabbing hand released (hands remaining: %d)"), *GetName(), FreeGrabbingHands.Num());
    }

    // Update grab state based on what's left
    bool bHasSnapGrabs = (MainGrabPointHand != nullptr || SecondaryGrabPointHand != nullptr);
    bool bHasFreeGrabs = (FreeGrabbingHands.Num() > 0);

    if (!bHasSnapGrabs && !bHasFreeGrabs)
    {
        // Completely released
        CurrentGrabState = EGrabState::NotGrabbed;
        bIsHeld = false;

        // Detach from any hand if attached
        if (ActorMesh->GetAttachParent())
        {
            ActorMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        }

        // Restore original physics state if not using bSimulatePhysicsOnGrab
        if (!bSimulatePhysicsOnGrab && !bStartSimulatePhysics)
        {
            ActorMesh->SetSimulatePhysics(false);
            ActorMesh->SetEnableGravity(false);
            ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
        else if (!ActorMesh->IsSimulatingPhysics())
        {
            ActorMesh->SetSimulatePhysics(true);
        }

        UE_LOG(LogTemp, Display, TEXT("%s: Fully released"), *GetName());
    }
    else if (bHasSnapGrabs)
    {
        // Still have snap grabs
        CurrentGrabState = EGrabState::SnapGrab;
        UE_LOG(LogTemp, Display, TEXT("%s: Still snap grabbed after release"), *GetName());
    }
    else if (bHasFreeGrabs)
    {
        // Only free grabs remaining
        CurrentGrabState = EGrabState::FreeGrab;
        UE_LOG(LogTemp, Display, TEXT("%s: Still free grabbed after release (hands remaining: %d)"), *GetName(), FreeGrabbingHands.Num());
    }
}

void AVRGrabbableActor::UpdateTwoHandedRotation()
{
    if (!MainGrabPointHand || !SecondaryGrabPointHand)
        return;

    // Get current transforms of both hands
    FTransform CurrentMainTransform = MainGrabPointHand->GetComponentTransform();
    FTransform CurrentSecondTransform = SecondaryGrabPointHand->GetComponentTransform();

    // Get the current direction vector between hands
    FVector CurrentMainToSecond = CurrentSecondTransform.GetLocation() - CurrentMainTransform.GetLocation();
    CurrentMainToSecond.Normalize();

    // Get the initial direction vector from stored transforms
    FVector InitialMainToSecond = SecondaryGripTransform.GetLocation() - InitialGripTransform.GetLocation();
    InitialMainToSecond.Normalize();

    // Calculate rotation between initial and current direction vectors
    FQuat RotationBetweenVectors = FQuat::FindBetweenVectors(InitialMainToSecond, CurrentMainToSecond);

    // Apply the rotation if object is attached
    if (ActorMesh->GetAttachParent())
    {
        // Get the current relative transform
        FTransform CurrentRelativeTransform = ActorMesh->GetRelativeTransform();

        // Calculate rotation strength
        float RotationStrength = 1.0f;
        float DeltaTime = GetWorld()->GetDeltaSeconds();

        // Apply the rotation smoothly
        FQuat CurrentRotation = CurrentRelativeTransform.GetRotation();
        FQuat TargetRotation = RotationBetweenVectors * CurrentRotation;
        FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, RotationStrength * DeltaTime * 10.0f);

        // Apply the new rotation
        FTransform NewRelativeTransform = CurrentRelativeTransform;
        NewRelativeTransform.SetRotation(NewRotation);
        ActorMesh->SetRelativeTransform(NewRelativeTransform);

        // Update the initial transforms for next frame
        InitialGripTransform = CurrentMainTransform;
        SecondaryGripTransform = CurrentSecondTransform;
    }
}

void AVRGrabbableActor::SetupGrabPointOccupancy(EGrabPointType GrabPointType, USkeletalMeshComponent* HandMesh)
{
    if (GrabPointType == EGrabPointType::Main)
    {
        bMainGrabPointOccupied = true;
        MainGrabPointHand = HandMesh;
    }
    else if (GrabPointType == EGrabPointType::Secondary)
    {
        bSecondaryGrabPointOccupied = true;
        SecondaryGrabPointHand = HandMesh;
    }
}

FTransform AVRGrabbableActor::GetGrabSocketTransformForType(EGrabPointType GrabPointType, bool bIsLeftHand) const
{
    if (GrabPointType == EGrabPointType::Main)
    {
        return GetMainGrabSocketTransform(bIsLeftHand);
    }
    else if (GrabPointType == EGrabPointType::Secondary)
    {
        return GetSecondaryGrabSocketTransform(bIsLeftHand);
    }
    return GetActorTransform();
}

void AVRGrabbableActor::CreateSecondHandConstraint(USkeletalMeshComponent* HandMesh, UStaticMeshComponent* GrabPointComponent, bool bIsLeftHand)
{
    if (!HandMesh || !GrabPointComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateSecondHandConstraint: Invalid parameters"));
        return;
    }

    // Get hand reference to obtain socket and bone names
    AVRHand* Hand = Cast<AVRHand>(HandMesh->GetOwner());
    if (!Hand)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateSecondHandConstraint: Could not get hand reference"));
        return;
    }

    // Initialize constraint component
    UPhysicsConstraintComponent* SecondHandConstraint = NewObject<UPhysicsConstraintComponent>(this);
    if (!SecondHandConstraint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create second hand constraint"));
        return;
    }

    SecondHandConstraint->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
    SecondHandConstraint->RegisterComponent();

    // Get socket and bone names from hand
    FName HandSocketName = Hand->GetHandGripSocketName();
    FName HandBoneName = Hand->GetHandBoneName();

    SecondHandConstraint->SetConstrainedComponents(
        HandMesh,
        HandBoneName,
        ActorMesh,
        NAME_None
    );

    // Calculate precise alignment transforms
    FTransform HandSocketWorldTransform = HandMesh->GetSocketTransform(HandSocketName);
    FTransform HandBoneWorldTransform = HandMesh->GetBoneTransform(HandBoneName);

    // Use Blueprint-overridable grab point transforms
    FTransform GrabPointWorldTransform;
    if (GrabPointComponent == GrabPointMain)
    {
        GrabPointWorldTransform = GetMainGrabPointTransform();
    }
    else if (GrabPointComponent == GrabPointSecond)
    {
        GrabPointWorldTransform = GetSecondaryGrabPointTransform();
    }
    else
    {
        GrabPointWorldTransform = GrabPointComponent->GetComponentTransform();
        UE_LOG(LogTemp, Warning, TEXT("Using fallback transform for unknown grab point component"));
    }

    FTransform ObjectWorldTransform = ActorMesh->GetComponentTransform();

    // Calculate constraint reference frames for proper alignment
    FTransform SocketRelativeToBone = HandSocketWorldTransform.GetRelativeTransform(HandBoneWorldTransform);
    FTransform Frame1 = SocketRelativeToBone;

    FTransform GrabPointRelativeToObject = GrabPointWorldTransform.GetRelativeTransform(ObjectWorldTransform);
    FTransform Frame2 = GrabPointRelativeToObject;

    SecondHandConstraint->SetConstraintReferenceFrame(EConstraintFrame::Frame1, Frame1);
    SecondHandConstraint->SetConstraintReferenceFrame(EConstraintFrame::Frame2, Frame2);

    // Configure constraint limits for rigid attachment
    SecondHandConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    SecondHandConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    SecondHandConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    SecondHandConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    SecondHandConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    SecondHandConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);

    // Configure drive parameters for stable attachment
    SecondHandConstraint->SetLinearPositionDrive(true, true, true);
    SecondHandConstraint->SetLinearVelocityDrive(true, true, true);
    SecondHandConstraint->SetLinearDriveParams(1200.0f, 120.0f, 0.0f);

    SecondHandConstraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
    SecondHandConstraint->SetAngularOrientationDrive(true, true);
    SecondHandConstraint->SetAngularVelocityDrive(true, true);
    SecondHandConstraint->SetAngularDriveParams(1200.0f, 120.0f, 0.0f);

    // Register constraint for lifecycle management
    SecondHandConstraints.Add(HandMesh, SecondHandConstraint);

    UE_LOG(LogTemp, Display, TEXT("Created second hand constraint for %s hand using bone %s"),
        bIsLeftHand ? TEXT("LEFT") : TEXT("RIGHT"), *HandBoneName.ToString());
}

bool AVRGrabbableActor::CanAcceptGrab(bool bIncomingIsSnap, EGrabPointType IncomingGrabPoint, USkeletalMeshComponent* IncomingHand) const
{
    // Allow initial grab on ungrabbed objects
    if (CurrentGrabState == EGrabState::NotGrabbed)
    {
        return true;
    }

    // Prevent duplicate grabs from same hand
    if (MainGrabPointHand == IncomingHand || SecondaryGrabPointHand == IncomingHand)
    {
        return false;
    }

    // Allow multiple free grabs, prevent duplicate free grabs from same hand
    if (!bIncomingIsSnap && FreeGrabbingHands.Contains(IncomingHand))
    {
        return false;
    }

    // Handle grab attempts on free-grabbed objects
    if (CurrentGrabState == EGrabState::FreeGrab)
    {
        return true; // Both snap and free grabs allowed
    }

    // Handle grab attempts on snap-grabbed objects
    if (CurrentGrabState == EGrabState::SnapGrab)
    {
        if (bIncomingIsSnap)
        {
            // Allow snap to available points or force-switching to occupied points
            return true;
        }
        else
        {
            // Prohibit free grabs on snap-grabbed objects
            return false;
        }
    }

    return false;
}

void AVRGrabbableActor::HandleGrabConflicts(USkeletalMeshComponent* IncomingHand, bool bIncomingIsSnap, EGrabPointType IncomingGrabPoint)
{
    UE_LOG(LogTemp, Warning, TEXT("%s: Handling grab conflicts - Incoming: %s, Current State: %d"),
        *GetName(), bIncomingIsSnap ? TEXT("SNAP") : TEXT("FREE"), (int32)CurrentGrabState);

    // Snap grabs override all free grabs
    if (CurrentGrabState == EGrabState::FreeGrab && bIncomingIsSnap)
    {
        UE_LOG(LogTemp, Warning, TEXT("Snap grab on free grabbed object - releasing all free grabs"));
        ForceReleaseAllFreeGrabs();
    }
    // Handle force-switching between snap grabs
    else if (CurrentGrabState == EGrabState::SnapGrab && bIncomingIsSnap)
    {
        if (IncomingGrabPoint == EGrabPointType::Main && bMainGrabPointOccupied)
        {
            UE_LOG(LogTemp, Warning, TEXT("Force switching to main grab point"));
            ForceReleaseHand(MainGrabPointHand);
        }
        else if (IncomingGrabPoint == EGrabPointType::Secondary && bSecondaryGrabPointOccupied)
        {
            UE_LOG(LogTemp, Warning, TEXT("Force switching to secondary grab point"));
            ForceReleaseHand(SecondaryGrabPointHand);
        }
    }
}

void AVRGrabbableActor::ForceReleaseHand(USkeletalMeshComponent* HandToRelease)
{
    if (!HandToRelease)
        return;

    UE_LOG(LogTemp, Warning, TEXT("Force releasing hand: %s"), *HandToRelease->GetOwner()->GetName());

    // Find the VRHand actor and call ReleaseObject
    if (AVRHand* Hand = Cast<AVRHand>(HandToRelease->GetOwner()))
    {
        Hand->ReleaseObject();
    }
}

void AVRGrabbableActor::ForceReleaseAllFreeGrabs()
{
    UE_LOG(LogTemp, Warning, TEXT("Force releasing all free grabs (%d hands)"), FreeGrabbingHands.Num());

    // Make a copy since ReleaseObject will modify the array
    TArray<USkeletalMeshComponent*> HandsToRelease = FreeGrabbingHands;

    for (USkeletalMeshComponent* Hand : HandsToRelease)
    {
        ForceReleaseHand(Hand);
    }
}

EGrabPointType AVRGrabbableActor::GetHandGrabPointType(USkeletalMeshComponent* Hand) const
{
    if (Hand == MainGrabPointHand)
    {
        return EGrabPointType::Main;
    }
    else if (Hand == SecondaryGrabPointHand)
    {
        return EGrabPointType::Secondary;
    }

    return EGrabPointType::None;
}

bool AVRGrabbableActor::ShouldUseGrabPoints() const
{
    return GrabPointBehavior != EGrabPointBehavior::None;
}

bool AVRGrabbableActor::ShouldUseMainGrabPoint() const
{
    return GrabPointBehavior == EGrabPointBehavior::MainOnly ||
        GrabPointBehavior == EGrabPointBehavior::DualHanded;
}

bool AVRGrabbableActor::ShouldUseSecondaryGrabPoint() const
{
    return GrabPointBehavior == EGrabPointBehavior::DualHanded;
}

UPrimitiveComponent* AVRGrabbableActor::GetGrabCollisionComponent()
{
    return ActorMesh;
}

UPrimitiveComponent* AVRGrabbableActor::GetPhysicsComponent()
{
    return ActorMesh;
}

float AVRGrabbableActor::GetObjectWeight() const
{
    return ActorMesh->GetMass();
}

bool AVRGrabbableActor::HasMainGrabSocket(bool bIsLeftHand) const
{
    if (!GrabPointMain || !ShouldUseMainGrabPoint())
        return false;

    return true;
}

bool AVRGrabbableActor::HasSecondaryGrabSocket(bool bIsLeftHand) const
{
    if (!GrabPointSecond || !ShouldUseSecondaryGrabPoint())
        return false;

	return true;
}

FVector AVRGrabbableActor::GetMainGrabPointLocation_Implementation() const
{
    return GetMainGrabPointTransform().GetLocation();
}

FVector AVRGrabbableActor::GetSecondaryGrabPointLocation_Implementation() const
{
    return GetSecondaryGrabPointTransform().GetLocation();
}

FTransform AVRGrabbableActor::GetMainGrabPointTransform_Implementation() const
{
    return GrabPointMain ? GrabPointMain->GetComponentTransform() : GetActorTransform();
}

FTransform AVRGrabbableActor::GetSecondaryGrabPointTransform_Implementation() const
{
    return GrabPointSecond ? GrabPointSecond->GetComponentTransform() : GetActorTransform();
}

FTransform AVRGrabbableActor::GetMainGrabSocketTransform_Implementation(bool bIsLeftHand) const
{
    if (!GrabPointMain)
        return GetActorTransform();

    return GrabPointMain->GetComponentTransform();
}

FTransform AVRGrabbableActor::GetSecondaryGrabSocketTransform_Implementation(bool bIsLeftHand) const
{
    if (!GrabPointSecond)
        return GetActorTransform();

    return GrabPointSecond->GetComponentTransform();
}

void AVRGrabbableActor::OnUnifiedGrab_Implementation(USkeletalMeshComponent* HandMesh, bool bIsLeftHand, bool bIsSnapping, EGrabPointType GrabPointType)
{
    UE_LOG(LogTemp, Display, TEXT("%s: Unified grab - Hand: %s, Snapping: %s, GrabPoint: %d, Current State: %d"),
        *GetName(), *HandMesh->GetOwner()->GetName(), bIsSnapping ? TEXT("YES") : TEXT("NO"),
        (int32)GrabPointType, (int32)CurrentGrabState);

    // Validate grab attempt against current object state
    if (!CanAcceptGrab(bIsSnapping, GrabPointType, HandMesh))
    {
        UE_LOG(LogTemp, Warning, TEXT("Grab rejected - conflicts with current state"));
        return;
    }

    // Resolve conflicts with existing grabs (force releases if necessary)
    HandleGrabConflicts(HandMesh, bIsSnapping, GrabPointType);

    // Handle physics-based free grab
    if (!bIsSnapping)
    {
        // Track hand in free grabbing collection
        if (!FreeGrabbingHands.Contains(HandMesh))
        {
            FreeGrabbingHands.Add(HandMesh);
            UE_LOG(LogTemp, Display, TEXT("%s: Added hand to free grabbing list (total: %d)"), *GetName(), FreeGrabbingHands.Num());
        }

        // Log proximity to grab points for debugging
        UE_LOG(LogTemp, Display, TEXT("%s: Free grab near %s grab point"), *GetName(),
            GrabPointType == EGrabPointType::Main ? TEXT("MAIN") : TEXT("SECONDARY"));

        // Update state if transitioning from ungrabbed
        if (CurrentGrabState == EGrabState::NotGrabbed)
        {
            CurrentGrabState = EGrabState::FreeGrab;
            UE_LOG(LogTemp, Display, TEXT("%s: Set to FreeGrab state"), *GetName());
        }

        bIsHeld = true;
        return;
    }

    // Validate grab point component for snap operations
    UStaticMeshComponent* GrabPointToAlign = nullptr;
    if (GrabPointType == EGrabPointType::Main)
    {
        GrabPointToAlign = this->GrabPointMain;
    }
    else if (GrabPointType == EGrabPointType::Secondary)
    {
        GrabPointToAlign = this->GrabPointSecond;
    }

    if (!GrabPointToAlign)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnUnifiedGrab: Snap failed, no valid GrabPointComponent for the given type."));
        return;
    }

    // Get hand reference for socket names
    AVRHand* Hand = Cast<AVRHand>(HandMesh->GetOwner());
    if (!Hand)
    {
        UE_LOG(LogTemp, Error, TEXT("OnUnifiedGrab: Could not get hand reference"));
        return;
    }

    // Handle secondary hand attachment to already-attached object
    if (ActorMesh->GetAttachParent() != nullptr)
    {
        UE_LOG(LogTemp, Display, TEXT("%s: Second hand snap grab detected"), *GetName());

        // Create physics constraint for secondary hand
        if (GrabPointType == EGrabPointType::Secondary && IsSecondaryGrabPointAvailable())
        {
            bSecondaryGrabPointOccupied = true;
            SecondaryGrabPointHand = HandMesh;
            SecondaryGripTransform = HandMesh->GetComponentTransform();

            CreateSecondHandConstraint(HandMesh, GrabPointToAlign, bIsLeftHand);

            UE_LOG(LogTemp, Display, TEXT("%s: Second-hand snap to secondary point complete"), *GetName());
        }
        else if (GrabPointType == EGrabPointType::Main && IsMainGrabPointAvailable())
        {
            bMainGrabPointOccupied = true;
            MainGrabPointHand = HandMesh;
            InitialGripTransform = HandMesh->GetComponentTransform();

            CreateSecondHandConstraint(HandMesh, GrabPointToAlign, bIsLeftHand);

            UE_LOG(LogTemp, Display, TEXT("%s: Second-hand snap to main point complete"), *GetName());
        }

        CurrentGrabState = EGrabState::SnapGrab;
        bIsHeld = true;
        return;
    }

    // Execute primary hand snap attachment

    // Update grab point occupancy tracking
    if (GrabPointType == EGrabPointType::Main)
    {
        bMainGrabPointOccupied = true;
        MainGrabPointHand = HandMesh;
    }
    else
    {
        bSecondaryGrabPointOccupied = true;
        SecondaryGrabPointHand = HandMesh;
    }

    // Calculate socket-based attachment transforms
    FName HandSocketName = Hand->GetHandGripSocketName();

    FTransform GrabPointWorldTransform;
    if (GrabPointType == EGrabPointType::Main)
    {
        GrabPointWorldTransform = GetMainGrabPointTransform();
    }
    else
    {
        GrabPointWorldTransform = GetSecondaryGrabPointTransform();
    }

    const FTransform ActorMeshWorldTransform = ActorMesh->GetComponentTransform();

    // Calculate relative positioning for socket alignment
    const FTransform Offset = GrabPointWorldTransform.GetRelativeTransform(ActorMeshWorldTransform);
    const FTransform TargetRelativeTransform = Offset.Inverse();

    // Perform attachment with physics state management
    bWasSimulatingPhysics = ActorMesh->IsSimulatingPhysics();
    if (bWasSimulatingPhysics)
    {
        ActorMesh->SetSimulatePhysics(false);
    }

    // Store original scale and update the target transform
    FVector OriginalScale = ActorMesh->GetRelativeScale3D();

    // Update the target transform to preserve original scale
    FTransform UpdatedTargetTransform = TargetRelativeTransform;
    UpdatedTargetTransform.SetScale3D(OriginalScale);

    // Attach to hand socket with precise positioning
    ActorMesh->AttachToComponent(HandMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);

    // Set the complete transform with preserved scale
    ActorMesh->SetRelativeTransform(UpdatedTargetTransform);

    // Store reference transform for dual-hand operations
    InitialGripTransform = HandMesh->GetComponentTransform();

    // Update object state
    CurrentGrabState = EGrabState::SnapGrab;
    bIsHeld = true;

    UE_LOG(LogTemp, Display, TEXT("%s: First-Hand Snap Grab complete."), *GetName());
}

void AVRGrabbableActor::ShowCanBeGrabbed_Implementation(bool bIsGrabbable)
{
    GetGrabCollisionComponent()->SetRenderCustomDepth(bIsGrabbable);
}

void AVRGrabbableActor::ForceRelease()
{
    ReleaseFromHand(MainGrabPointHand);
    ReleaseFromHand(SecondaryGrabPointHand);

    // Release all free grabbing hands
    TArray<USkeletalMeshComponent*> HandsToRelease = FreeGrabbingHands;
    for (USkeletalMeshComponent* Hand : HandsToRelease)
    {
        ReleaseFromHand(Hand);
    }
}

void AVRGrabbableActor::ReleaseFromHand(USkeletalMeshComponent* HandMesh)
{
    if (HandMesh)
    {
        if (AVRHand* Hand = Cast<AVRHand>(HandMesh->GetOwner()))
        {
            Hand->ReleaseObject();
        }
    }
}

void AVRGrabbableActor::PrepareForDestroy()
{
    UE_LOG(LogTemp, Warning, TEXT("Preparing %s for destruction"), *GetName());

    // Only force release if object is being held
    if (bIsHeld)
    {
        ForceRelease();
    }

    // Disable physics simulation before disabling collision
    if (ActorMesh)
    {
        ActorMesh->SetSimulatePhysics(false);
        ActorMesh->SetEnableGravity(false);

        ActorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ActorMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    }

    // Clears any remaining references
    bIsHeld = false;
    MainGrabPointHand = nullptr;
    SecondaryGrabPointHand = nullptr;
    FreeGrabbingHands.Empty();
    CurrentGrabState = EGrabState::NotGrabbed;

    // Clean up all second hand constraints
    for (auto& ConstraintPair : SecondHandConstraints)
    {
        if (ConstraintPair.Value)
        {
            ConstraintPair.Value->BreakConstraint();
            ConstraintPair.Value->DestroyComponent();
        }
    }
    SecondHandConstraints.Empty();

    // Disable tick
    SetActorTickEnabled(false);
}

void AVRGrabbableActor::SafeDestroy()
{
    // Prepare for destruction
    PrepareForDestroy();

    // Delay just to ensure constraint cleanup is complete 
    FTimerHandle DestroyTimer;
    GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [this]()
        {
            if (IsValid(this))
            {
                UE_LOG(LogTemp, Warning, TEXT("Safe destroying %s"), *GetName());
                Destroy();
            }
        }, 0.05f, false);
}