// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRActor.h"
#include "Interfaces/Interactable.h"
#include "VRGrabbableActor.generated.h"

class UBoxComponent;
class UPhysicsConstraintComponent;

UENUM(BlueprintType)
enum class EGrabType : uint8 {
	Climb,	// Special climbing behavior
	None	// Default behavior = hybrid free/snap (no enum needed)
};

UENUM(BlueprintType)
enum class EGrabPointType : uint8 {
	None,
	Main,
	Secondary
};

// Controls which grab points are available
UENUM(BlueprintType)
enum class EGrabPointBehavior : uint8 {
	None,           // No grab points, free grab only
	MainOnly,       // Only main grab point available
	DualHanded      // Both main and secondary grab points available
};

// Track what type of grab is currently active
UENUM(BlueprintType)
enum class EGrabState : uint8 {
	NotGrabbed,     // No hands grabbing
	FreeGrab,       // Free grab active (grab points available for snapping)
	SnapGrab        // Snap grab active (free grabs locked out)
};

UCLASS()
class PROJECTSURVIVALVR_API AVRGrabbableActor : public AVRActor, public IInteractable
{
GENERATED_BODY()

public:
	AVRGrabbableActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#pragma region IInteractable

	virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation,
		bool bIsLeftHand = false, ECollisionChannel HandChannel = ECC_Pawn) override;
	virtual void OnRelease(USkeletalMeshComponent* InComponent) override;

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|Interaction")
	void ShowCanBeGrabbed(bool bIsGrabbable) override;

	UFUNCTION(BlueprintPure, Category = "VR|Components")
	virtual UPrimitiveComponent* GetGrabCollisionComponent() override;

	UFUNCTION(BlueprintPure, Category = "VR|Components")
	virtual UPrimitiveComponent* GetPhysicsComponent() override;

	UFUNCTION(BlueprintPure, Category = "VR|Weight")
	virtual float GetObjectWeight() const override;

#pragma endregion

#pragma region Components
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<UStaticMeshComponent> GrabPointMain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<UStaticMeshComponent> GrabPointSecond;

#pragma endregion

#pragma region Setup Properties

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Setup|Physics")
    float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Setup|Physics")
    bool bUseCustomWeight = false;

	// Enables physics simulation on BeginPlay/Construction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Setup|Physics")
	bool bStartSimulatePhysics = true;

	// Enables physics simulation when grabbed (can only be false if bStartSimulatePhysics is false)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Setup|Physics", 
		meta = (EditCondition = "!bStartSimulatePhysics", EditConditionHides))
	bool bSimulatePhysicsOnGrab = true;

	// Only needed for special objects like climbing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Setup")
	EGrabType GrabType = EGrabType::None;

	// Controls which grab points are available for this object
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Setup")
	EGrabPointBehavior GrabPointBehavior = EGrabPointBehavior::None;

#pragma endregion

#pragma region State Variables

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Info")
	bool bIsHeld;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Info")
	bool bWasSimulatingPhysics;

	// Track current grab state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|GrabState")
	EGrabState CurrentGrabState = EGrabState::NotGrabbed;

	// Track grab point occupancy
	UPROPERTY(BlueprintReadOnly, Category = "VR|GrabPoints")
	bool bMainGrabPointOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "VR|GrabPoints")
	bool bSecondaryGrabPointOccupied = false;

#pragma endregion

#pragma region Hand Tracking

	// Track which hand is using which grab point
	UPROPERTY(BlueprintReadOnly, Category = "VR|Info")
	USkeletalMeshComponent* MainGrabPointHand = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "VR|Info")
	USkeletalMeshComponent* SecondaryGrabPointHand = nullptr;

	// Track hands that are free grabbing (not at grab points)
	UPROPERTY(BlueprintReadOnly, Category = "VR|Info")
	TArray<USkeletalMeshComponent*> FreeGrabbingHands;

#pragma endregion

#pragma region Physics System

	// Physics constraints for second hand grabs
	UPROPERTY(BlueprintReadOnly, Category = "VR|Physics")
	TMap<USkeletalMeshComponent*, UPhysicsConstraintComponent*> SecondHandConstraints;

	// Store transforms for two-handed interaction
	FTransform InitialGripTransform;
	FTransform SecondaryGripTransform;
	FTransform InitialHandsRelativeTransform;

#pragma endregion

#pragma region Internal Functions

	// Update the object rotation when two hands are holding
	void UpdateTwoHandedRotation();

	void SetupGrabPointOccupancy(EGrabPointType GrabPointType, USkeletalMeshComponent* HandMesh);
	FTransform GetGrabSocketTransformForType(EGrabPointType GrabPointType, bool bIsLeftHand) const;

	// Grab conflict resolution functions
	bool CanAcceptGrab(bool bIncomingIsSnap, EGrabPointType IncomingGrabPoint, USkeletalMeshComponent* IncomingHand) const;
	void HandleGrabConflicts(USkeletalMeshComponent* IncomingHand, bool bIncomingIsSnap, EGrabPointType IncomingGrabPoint);
	void ForceReleaseHand(USkeletalMeshComponent* HandToRelease);
	void ForceReleaseAllFreeGrabs();
	EGrabPointType GetHandGrabPointType(USkeletalMeshComponent* Hand) const;

	// Second hand constraint creation
	void CreateSecondHandConstraint(USkeletalMeshComponent* HandMesh, UStaticMeshComponent* GrabPointComponent, bool bIsLeftHand);

	// Physics setup
	void SetupPhysics();

#pragma endregion

public:

#pragma region Getters

	// Get the current grab type for this object (for special objects only)
    UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	EGrabType GetGrabType() const { return GrabType; }

	// Get the grab point behavior
	UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	EGrabPointBehavior GetGrabPointBehavior() const { return GrabPointBehavior; }

	// Get current grab state
	UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	EGrabState GetCurrentGrabState() const { return CurrentGrabState; }

	// Getter for ActorMesh (since it's protected)
	UFUNCTION(BlueprintPure, Category = "VR|Components")
	UStaticMeshComponent* GetActorMesh() const { return ActorMesh; }

	// Helper functions to determine if grab points should be used
	UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	bool ShouldUseGrabPoints() const;
	
	UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	bool ShouldUseMainGrabPoint() const;
	
	UFUNCTION(BlueprintPure, Category = "VR|Interaction")
	bool ShouldUseSecondaryGrabPoint() const;

	// Returns if the item is being held
	UFUNCTION(BlueprintPure, Category =  "VR|Interaction")
	bool IsBeingHeld() const { return bIsHeld; }

#pragma endregion

#pragma region Grab Point Queries

	// Check if grab points have sockets
	UFUNCTION(BlueprintPure, Category = "VR|GrabPoints")
	bool HasMainGrabSocket(bool bIsLeftHand) const;

	UFUNCTION(BlueprintPure, Category = "VR|GrabPoints")
	bool HasSecondaryGrabSocket(bool bIsLeftHand) const;

	// Get the main grab point location
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FVector GetMainGrabPointLocation() const;

	// Get the second grab point location
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FVector GetSecondaryGrabPointLocation() const;

	// Get the main grab point transform
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FTransform GetMainGrabPointTransform() const;

	// Get the secondary grab point transform
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FTransform GetSecondaryGrabPointTransform() const;

	// Get grab point socket transforms
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FTransform GetMainGrabSocketTransform(bool bIsLeftHand) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|GrabPoints")
	FTransform GetSecondaryGrabSocketTransform(bool bIsLeftHand) const;

	// Check grab point availability
	UFUNCTION(BlueprintPure, Category = "VR|GrabPoints")
	bool IsMainGrabPointAvailable() const { return !bMainGrabPointOccupied; }

	UFUNCTION(BlueprintPure, Category = "VR|GrabPoints")
	bool IsSecondaryGrabPointAvailable() const { return !bSecondaryGrabPointOccupied; }

#pragma endregion

#pragma region Grab System

	// Unified grab function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VR|Interaction")
	void OnUnifiedGrab(USkeletalMeshComponent* HandMesh, bool bIsLeftHand, bool bIsSnapping, EGrabPointType GrabPointType);

#pragma endregion

#pragma region Cleanup

protected:
	void ForceRelease();
	void ReleaseFromHand(USkeletalMeshComponent* HandMesh);
	virtual void PrepareForDestroy();
	void SafeDestroy();

#pragma endregion

};