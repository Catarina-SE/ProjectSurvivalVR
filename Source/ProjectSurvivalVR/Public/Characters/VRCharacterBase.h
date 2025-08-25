// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SurvivalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SpotLightComponent.h"
#include "VRCharacterBase.generated.h"


class UCameraComponent;
class UInputAction;
class AVRHand;
class UNiagaraComponent;
class UNavAreaBase;

// Enum to manage the character's primary movement state.
UENUM(BlueprintType)
enum class EVRMovementState : uint8
{
    Locomotion,
    Climbing,
    Falling
};

UENUM(BlueprintType)
enum class ESnapTurnDegrees : uint8
{
    Deg15,
    Deg30,
    Deg45
};

UCLASS()
class PROJECTSURVIVALVR_API AVRCharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    AVRCharacterBase();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

#pragma region Components

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> VROrigin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> MouthCollider;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UNiagaraComponent> TeleportTraceNiagaraSystem;

    // Survival component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Survival")
    USurvivalComponent* SurvivalComponent;

#pragma endregion

#pragma region Hands
    // Hand class for spawning left hand
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Classes")
    TSubclassOf<AVRHand> LeftHandClass;

    // Hand class for spawning right hand
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Classes")
    TSubclassOf<AVRHand> RightHandClass;

    // Reference variable for the spawned left hand
    UPROPERTY(BlueprintReadOnly, Category = "VR|Hands")
    AVRHand* LeftHand;

    // Reference variable for the spawned right hand
    UPROPERTY(BlueprintReadOnly, Category = "VR|Hands")
    AVRHand* RightHand;

    void SpawnHands();
#pragma endregion

#pragma region InputActions
    void SetupInputBindings();

    UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
    UInputAction* VerticalMovementAction;

    UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
    UInputAction* HorizontalMovementAction;

    UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
    UInputAction* SprintAction;

    UPROPERTY(EditDefaultsOnly, Category = "VR|Input")
    UInputAction* TurnAction;

#pragma endregion   

#pragma region Movement and Speed

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement")
    float JoystickDeadzone = 0.2f;
    
    UPROPERTY(BlueprintReadOnly)
    float HorizontalMovement = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float VerticalMovement = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement")
    float NormalMoveSpeed = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement")
    float SprintMoveSpeed = 300.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement")
    bool bIsSprinting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Climbing")
    EVRMovementState CurrentMovementState = EVRMovementState::Locomotion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Climbing")
    TObjectPtr<AVRHand> ClimbingHand_Left = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Climbing")
    TObjectPtr<AVRHand> ClimbingHand_Right = nullptr;

    // Track which hand has priority for movement control
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Climbing")
    TObjectPtr<AVRHand> PrimaryClimbingHand = nullptr;

    // Store anchor positions for both hands
    FVector LeftHandClimbAnchor;
    FVector RightHandClimbAnchor;

    // Store the anchor for the primary climbing hand
    FVector PrimaryHandClimbAnchor;

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Climbing")
    float MinHeadClearance = 50.0f;

    // Physics state backup
    bool bOriginalSimulatePhysics;
    bool bOriginalEnableGravity;
    ECollisionEnabled::Type OriginalCollisionEnabled;

    // Helper functions
    bool IsCharacterStuckInGeometry() const;
    void FixHeightAfterClimbing();

protected:
    // Head collision prevention
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    bool bEnableHeadCollisionPrevention = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    float HeadCollisionRadius = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    float HeadRepulsionForce = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    float HeadCollisionDistance = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    float MaxRepulsionDistance = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Collision")
    float RepulsionSmoothingSpeed = 5.0f;

    FVector CurrentRepulsionForce = FVector::ZeroVector;

    void ProcessHeadCollisionPrevention(float DeltaTime);
    FVector CalculateHeadRepulsionForce() const;
    bool IsHeadNearWall(FVector& OutWallNormal, float& OutDistance) const;
    FCollisionQueryParams GetHeadCollisionQueryParams() const;

protected:
    // Head movement compensation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Compensation")
    bool bEnableHeadMovementCompensation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Compensation")
    float HeadWallDetectionDistance = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Compensation")
    float CompensationStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Head Compensation")
    float MaxCompensationSpeed = 100.0f;

    // Track head movement
    FVector LastHeadPosition = FVector::ZeroVector;
    FVector HeadVelocity = FVector::ZeroVector;
    bool bHasValidLastHeadPosition = false;

    void ProcessHeadMovementCompensation(float DeltaTime);
    FVector CalculateHeadWallPenetration() const;
    FVector GetCompensatedMovement(const FVector& HeadMovement, const FVector& WallPenetration) const;

protected:
    // Smart wall detection for height fixing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Climbing")
    float EdgeDetectionDistance = 30.0f;

    bool IsCharacterAtEdge() const;
    bool WouldCharacterGetPushedSideways() const;
	FVector GetEdgeAvoidanceDirection() const;

protected:

    // Get the current primary climbing hand
    UFUNCTION(BlueprintPure, Category = "VR|Climbing")
    AVRHand* GetPrimaryClimbingHand() const { return PrimaryClimbingHand; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Movement|Turn")
    bool bSnapTurnEnabled;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VR|Movement|Turn",
        meta = (
            EditCondition = "bSnapTurnEnabled",
            ClampMin = "15.0",
            ClampMax = "90.0",
            UIMin = "15.0",
            UIMax = "90.0"
            ))
    float SnapTurnDegrees = 30.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Turn",
        meta = (
            EditCondition = "bSnapTurnEnabled"
            ));
    bool bSnapTurnExecuted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement|Turn",
        meta = (
            EditCondition = "!bSnapTurnEnabled",
            ClampMin = "30.0",
            ClampMax = "180.0",
            UIMin = "30.0",
            UIMax = "180.0"
            ))
    float SmoothTurnRate = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|Classes")
    TSubclassOf<AActor> TeleportVisualizerClass;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Teleport")
    TObjectPtr<AActor> TeleportViasualizerReference;
   
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Teleport")
    bool bTeleportTraceActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Teleport")
    bool bValidTeleportTrace = false;

    UPROPERTY(EditDefaultsOnly, Category = "VR|Movement|Teleport")
    float TeleportLaunchSpeed = 650.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "VR|Movement|Teleport")
    float TeleportProjectileRadius = 3.6f;
    
    UPROPERTY(EditDefaultsOnly, Category = "VR|Movement|Teleport")
    float NavMeshCellHeight = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "VR|Movement|Teleport")
    FVector TeleportProjectPointToNavigationQueryExtend = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Teleport")
    FVector ProjectedTeleportLocation;

public:
    UFUNCTION(BlueprintCallable, Category = "VR|Movement|Sprint")
	bool IsSprinting() const { return bIsSprinting; }

	void StartClimbing(AVRHand* GrabbingHand);
	void StopClimbing(AVRHand* ReleasingHand);

#pragma endregion

#pragma region Movement Input Handlers

protected:
    void HorizontalMovementInput(const FInputActionValue& Value);
    void VerticalMovementInput(const FInputActionValue& Value);
    void ResetHorizontalMovement(const FInputActionValue& Value);
    void ResetVerticalMovement(const FInputActionValue& Value);
    void Sprint();
    void UpdateMovementSpeed();

    void ProcessLocomotionMovement();
    void ProcessClimbingMovement(float DeltaTime);

#pragma endregion

#pragma region Turn Input Handlers
    void HandleTurnAction(const FInputActionValue& Value);
    void CompleteTurnAction();
    void SnapTurn(bool RightTurn);
    void SmoothTurn(float TurnValue);
#pragma endregion

#pragma region Teleport handlers

protected:
    UFUNCTION(BlueprintCallable, Category = "VR|Movement|Teleport")
    void StartTeleport();
    UFUNCTION(BlueprintCallable, Category = "VR|Movement|Teleport")
    TArray<FVector> TeleportTrace(FVector StartPosition, FVector ForwardVector);
    UFUNCTION(BlueprintCallable, Category = "VR|Movement|Teleport")
    void TryTeleport();

    // NavMesh area validation for teleport
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement|Teleport")
    bool bValidateReachability = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Movement|Teleport")
    float MaxTeleportDistance = 1000.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement|Teleport")
    TSubclassOf<UNavAreaBase> CurrentPlayerNavArea;

public:

    UFUNCTION(BlueprintCallable, Category = "VR|Movement|Teleport")
    bool CanReachLocation(const FVector& TargetLocation);

#pragma endregion

#pragma region Mouth Overlap Handlers

private:

    UFUNCTION()
    void OnMouthBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnMouthEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    bool bIsOverlappingMouth;

public:
    bool IsOverlappingMouth() { return bIsOverlappingMouth; };

#pragma endregion

#pragma region Fall Detection

protected:
    // Fall detection settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|FallDetection")
    float FallDistanceThreshold = 500.0f; // Distance in cm before triggering death screen
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|FallDetection")
    bool bEnableFallDetection = true;
    
    // Widget class for death screen
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|FallDetection")
    TSubclassOf<UUserWidget> DeathScreenWidgetClass;
    
    // Internal state
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|FallDetection")
    float FallStartHeight = 0.0f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|FallDetection")
    bool bIsFalling = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|FallDetection")
    bool bFallDetectionTriggered = false;
    
    // Widget instance
    UPROPERTY(BlueprintReadOnly, Category = "VR|Death")
    UUserWidget* DeathScreenWidget = nullptr;
    
    // Fall detection functions
    void UpdateFallDetection();
    void TriggerDeathScreen();
    void ResetFallDetection();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "VR|Death")
    void OnDeath();

#pragma endregion

#pragma region Locomotion Settings

protected:
    // Locomotion method toggles
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Locomotion Settings")
    bool bJoystickMovementEnabled = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Locomotion Settings")
    bool bTeleportEnabled = true;
    
    // Turn method setting (true = smooth, false = snap)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Locomotion Settings")
    bool bSmoothTurnEnabled = false; // Default to snap turn
    
    // Snap turn degree setting using enum
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Locomotion Settings")
    ESnapTurnDegrees CurrentSnapTurnDegrees = ESnapTurnDegrees::Deg30;

    // Movement lock state (locks both joystick and teleport)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR|Movement Lock")
    bool bMovementLocked = false;

    // Headlight Components

    /** Spotlight component for headlight functionality */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Headlight", meta = (AllowPrivateAccess = "true"))
    class USpotLightComponent* HeadlightComponent;

#pragma region Stamina Settings

    // Stamina System
    UFUNCTION()
    void OnStaminaForcedStop();

#pragma endregion

public:

    // Headlight System

    /** Enables or disables the headlight */
    UFUNCTION(BlueprintCallable, Category = "Headlight")
    void SetHeadlightEnabled(bool bEnabled);

    /** Returns true if headlight is currently enabled */
    UFUNCTION(BlueprintPure, Category = "Headlight")
    bool IsHeadlightEnabled() const;

    // Locomotion toggle functions
    UFUNCTION(BlueprintCallable, Category = "VR|Locomotion Settings")
    bool ToggleJoystickMovement();
    
    UFUNCTION(BlueprintCallable, Category = "VR|Locomotion Settings")
    bool ToggleTeleportMovement();

    // Lock/Unlock all movement (joystick + teleport)
    UFUNCTION(BlueprintCallable, Category = "VR|Movement Lock")
    void LockMovement();
    
    UFUNCTION(BlueprintCallable, Category = "VR|Movement Lock")
    void UnlockMovement();
    
    // Check if movement is currently allowed (combines user settings + lock state)
    UFUNCTION(BlueprintPure, Category = "VR|Movement Lock")
    bool IsJoystickMovementAllowed() const;
    
    UFUNCTION(BlueprintPure, Category = "VR|Movement Lock")
    bool IsTeleportAllowed() const;
    
    // Check lock state
    UFUNCTION(BlueprintPure, Category = "VR|Movement Lock")
    bool IsMovementLocked() const { return bMovementLocked; }
    
    UFUNCTION(BlueprintCallable, Category = "VR|Locomotion Settings")
    bool ToggleSmoothTurn();
    
    UFUNCTION(BlueprintCallable, Category = "VR|Locomotion Settings")
    bool ToggleSnapTurn();
    
    UFUNCTION(BlueprintCallable, Category = "VR|Locomotion Settings")
    bool SetSnapTurnDegrees(ESnapTurnDegrees Degrees);
    
    // Getter functions for current states
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    bool IsJoystickMovementEnabled() const { return bJoystickMovementEnabled; }
    
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    bool IsTeleportEnabled() const { return bTeleportEnabled; }
    
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    bool IsSmoothTurnEnabled() const { return bSmoothTurnEnabled; }
    
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    bool IsSnapTurnEnabled() const { return !bSmoothTurnEnabled; }
    
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    ESnapTurnDegrees GetCurrentSnapTurnDegrees() const { return CurrentSnapTurnDegrees; }
    
    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    float GetSnapTurnDegreesAsFloat() const;

    UFUNCTION(BlueprintPure, Category = "VR|Locomotion Settings")
    ESnapTurnDegrees GetSnapTurnDegreesAsEnum() const;

private:
    // Internal function to convert enum to float value
    float ConvertSnapTurnEnumToFloat(ESnapTurnDegrees EnumValue) const;
    
    // Internal function to update the snap turn system
    void UpdateSnapTurnSettings();

#pragma endregion

};