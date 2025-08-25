// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Structures/FingerData.h"
#include "TimerManager.h"
#include "Actors/VRGrabbableActor.h"
#include "VRHand.generated.h"

class UMotionControllerComponent;
class UWidgetInteractionComponent;
class USphereComponent;
class UInputAction;
enum class ETriggerEvent : uint8;
struct FInputActionValue;
class USplineComponent;
class UPhysicsConstraintComponent;
class AVRClimbableActor;

USTRUCT(BlueprintType)
struct FGrabPointInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsAvailable = false;

    UPROPERTY(BlueprintReadOnly)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    EGrabPointType Type = EGrabPointType::None;

    UPROPERTY(BlueprintReadOnly)
    float Distance = FLT_MAX;

    UPROPERTY(BlueprintReadOnly)
    FTransform SocketTransform = FTransform::Identity;
};

UCLASS()
class PROJECTSURVIVALVR_API AVRHand : public AActor
{
	GENERATED_BODY()

public:
	AVRHand();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static TArray<AVRHand*> VRHands;

#pragma region InputActions

protected:
	UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
	UInputAction* GrabPressed;
	
	UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
	UInputAction* GrabReleased;

	UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
	UInputAction* OpenMenu;

protected:
	void SetupInputBindings();

#pragma endregion

#pragma region Components

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<UMotionControllerComponent> MotionController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<UWidgetInteractionComponent> WidgetInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USkeletalMeshComponent> HandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USphereComponent> GrabSphere;

	// Spline component defining thumb finger collision path
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USplineComponent> Spline_Thumb;

	// Spline component defining index finger collision path
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USplineComponent> Spline_Index;
	
	// Spline component defining middle finger collision path
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USplineComponent> Spline_Middle;
	
	// Spline component defining ring finger collision path
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USplineComponent> Spline_Ring;
	
	// Spline component defining pinky finger collision path
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<USplineComponent> Spline_Pinky;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Components")
	TObjectPtr<UStaticMeshComponent> GrabPointIndicator;

public:
	// Gets the motion controller component driving this hand
	UFUNCTION(BlueprintPure, Category = "VR|Components", meta = (ToolTip = "Returns the motion controller component that tracks this hand's position"))
	UMotionControllerComponent* GetMotionController() const { return MotionController; }

	// Gets the hand origin point used for distance calculations
	UFUNCTION(BlueprintPure, Category = "VR|Components", meta = (ToolTip = "Returns the static mesh component used as reference point for grab calculations"))
	UStaticMeshComponent* GetHandOriginPoint() const { return HandOriginPoint; }

#pragma endregion

#pragma region Physics System

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Physics")
	TObjectPtr<UPhysicsConstraintComponent> HandPhysicsConstraint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Physics")
	TObjectPtr<UStaticMeshComponent> HandOriginPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Physics")
	TObjectPtr<UPhysicsConstraintComponent> GrabConstraint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Interaction")
	float GrabReleaseThreshold = 30.0f;

	FTimerHandle DistanceCheckTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Interaction")
	float DistanceCheckInterval = 0.1f;

	// Root bone name for hand physics simulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Physics")
	FName RootBoneName;

	// Physics blend weight for hand mesh simulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Physics")
	float PhysicsBlendWeight = 0.15f;

	FTransform FrozenHandTransform;

protected:
	void CheckHandControllerDistance();
	void SetupGrabConstraint(UPrimitiveComponent* ObjectPhysicsComponent, bool bIsSnapping, EGrabPointType GrabPointType);

#pragma endregion

#pragma region Proximity Detection

protected:
	// Distance in cm within which hand will snap to grab points
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "VR|Interaction")
	float SnapRange = 15.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VR|Interaction")
	EGrabPointType CurrentTargetGrabPoint = EGrabPointType::None;

protected:
	FGrabPointInfo GetClosestAvailableGrabPoint(AVRGrabbableActor* Object);

#pragma endregion

#pragma region Hand Data

protected:
	// Which VR controller this hand represents (Left or Right)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Hand")
	EControllerHand HandType;

public:
	// Gets which controller hand this represents (Left or Right)
	UFUNCTION(BlueprintPure, Category = "VR|Hand", meta = (ToolTip = "Returns whether this is the left or right hand controller"))
	EControllerHand GetHandType() const { return HandType; }

#pragma endregion

#pragma region Procedural Fingers

protected:
	// Number of collision segments per finger for curl detection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Hand|ProceduralFingers")
	int FingerSteps = 4;

	// Whether to mirror finger animations for this hand
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Hand")
	bool bMirrorAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Hand|ProceduralFingers")
	TArray<FVector> FingerCache_Thumb;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Hand|ProceduralFingers")
	TArray<FVector> FingerCache_Index;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Hand|ProceduralFingers")
	TArray<FVector> FingerCache_Middle;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Hand|ProceduralFingers")
	TArray<FVector> FingerCache_Ring;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "VR|Hand|ProceduralFingers")
	TArray<FVector> FingerCache_Pinky;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Hand|ProceduralFingers")
	FFingerData FingerData;

protected:
	void SetupFingerAnimationData();
	TArray<FVector> GetFingerSteps(USplineComponent* FingerSpline);
	float TraceFingerSegment(TArray<FVector> FingerCacheArray);

public:
	// Returns current finger curl data for hand animation
	UFUNCTION(BlueprintPure, Category = "VR|Hand|ProceduralFingers", meta = (ToolTip = "Gets the current finger data containing curl values for all fingers"))
	FFingerData GetFingerData();

	// Calculates finger curl values based on collision with grabbed objects
	UFUNCTION(BlueprintCallable, Category = "VR|Hand|ProceduralFingers", meta = (ToolTip = "Updates finger data by tracing collision with currently grabbed object"))
	void TraceFingerData();

#pragma endregion

#pragma region Grab System

protected:
	TScriptInterface<IInteractable> GrabbedActor;
	bool bIsGrabbing = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "VR|Interaction")
	TArray<TScriptInterface<IInteractable>> OverlappingInteractables;

	UPROPERTY(BlueprintReadOnly, Category = "VR|Interaction")
	TScriptInterface<IInteractable> HoveredInteractable;

	UPROPERTY(BlueprintReadOnly)
	UPrimitiveComponent* GrabbedPrimitiveComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|GrabPoints")
	FName SocketName;

protected:
	UFUNCTION()
	void OnGrabSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
								bool bFromSweep, const FHitResult& SweepResult);
    
	UFUNCTION()
	void OnGrabSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateHoveredGrabbable();

	UFUNCTION(BlueprintCallable)
	void GrabObject();

public:
	// Releases the currently grabbed object
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "Releases any object currently being held by this hand"))
	void ReleaseObject();

	// Checks if this hand is currently grabbing an object
	UFUNCTION(BlueprintPure, meta = (ToolTip = "Returns true if this hand is currently holding an object"))
	bool IsGrabbing();

	// Checks if this hand is currently climbing
	UFUNCTION(BlueprintPure, Category = "VR|Climbing", meta = (ToolTip = "Returns true if this hand is attached to a climbable surface"))
	bool IsClimbing() const;

	// Gets the currently grabbed actor interface
	UFUNCTION(BlueprintPure, Category = "VR|Interaction", meta = (ToolTip = "Returns the interactable object currently being grabbed, if any"))
	TScriptInterface<IInteractable> GetGrabbedActor() const;

	// Called when hand starts hovering over a grabbable object
	UFUNCTION(BlueprintImplementableEvent, Category = "VR|Interaction", meta = (ToolTip = "Event triggered when hand starts hovering over an interactable object"))
	void OnGrab();

	UFUNCTION(BlueprintImplementableEvent, Category = "VR|Interaction")
	void OnGrabGrabbable(AVRGrabbableActor* GrabbableActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "VR|Interaction")
	void OnGrabClimbable(AVRClimbableActor* ClimbableActor);
	
	// Called when hand stops hovering over a grabbable object
	UFUNCTION(BlueprintImplementableEvent, Category = "VR|Interaction", meta = (ToolTip = "Event triggered when hand releases an interactable object"))
	void OnRelease();

	// Called when hover state changes to show visual feedback
	UFUNCTION(BlueprintNativeEvent, Category = "VR|Interaction", meta = (ToolTip = "Called when hand begins hovering over an interactable object"))
	void OnHoverChanged();

	// Called when hover state clears to hide visual feedback
	UFUNCTION(BlueprintNativeEvent, Category = "VR|Interaction", meta = (ToolTip = "Called when hand stops hovering over an interactable object"))
	void OnHoverCleared();

	UFUNCTION(BlueprintPure, Category = "VR|Hand")
    FName GetHandGripSocketName() const;
    
    UFUNCTION(BlueprintPure, Category = "VR|Hand")
    FName GetHandBoneName() const;

#pragma endregion

#pragma region Menu System

protected:
	// Opens or closes the hand menu interface
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "VR|Menu", meta = (ToolTip = "Toggles the hand menu on or off"))
	void ToggleMenu();

#pragma endregion
};