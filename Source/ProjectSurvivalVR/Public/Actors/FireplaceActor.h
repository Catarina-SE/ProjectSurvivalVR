// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "FireplaceActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFireComplete);

class AWoodLog;

UCLASS()
class PROJECTSURVIVALVR_API AFireplaceActor : public AActor
{
	GENERATED_BODY()

public:
	AFireplaceActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#pragma region Components

	// Main fireplace mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FireplaceMesh;

	// Wood log mesh components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WoodLog1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WoodLog2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WoodLog3;

	// Fireplace detection sphere 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* FireplaceDetectionSphere;

	// Heat zone sphere (for intense heat when fireplace complete)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* HeatZoneSphere;

	// Sheltered zone box (for shelter from cold)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* ShelteredZoneBox;

	// Fire effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* FireEffects;

#pragma endregion

#pragma region Settings

	// === FIREPLACE SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireplace Settings")
	bool bEnableFireplace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fireplace Settings", meta = (EditCondition = "bEnableFireplace"))
	int32 RequiredLogsCount = 3;

	// === HEAT ZONE SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone Settings")
	bool bEnableHeatZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone Settings", meta = (EditCondition = "bEnableHeatZone"))
	float IntenseHeatRecoveryRate = 0.2f;

	// === SHELTERED ZONE SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sheltered Zone Settings")
	bool bEnableShelteredZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sheltered Zone Settings", meta = (EditCondition = "bEnableShelteredZone"))
	float ShelteredHeatRecoveryRate = 0.05f;

	// === MATERIALS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta = (EditCondition = "bEnableFireplace"))
	UMaterialInterface* GhostMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials", meta = (EditCondition = "bEnableFireplace"))
	UMaterialInterface* WoodMaterial = nullptr;

#pragma endregion

private:

#pragma region Fireplace State

	// Array-based log management
	UPROPERTY(VisibleAnywhere, Category = "Fireplace State")
	TArray<bool> LogSlotStates;

	UPROPERTY()
	TArray<UStaticMeshComponent*> LogMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Fireplace State")
	int32 PlacedLogsCount = 0;

	// Current ghost effect tracking
	UPROPERTY()
	AWoodLog* CurrentDetectedLog = nullptr;

	int32 CurrentGhostSlot = -1;

	// State flags
	bool bFireplaceComplete = false;
	bool bHeatZoneActive = false;

#pragma endregion

#pragma region Fireplace Functions

	UFUNCTION()
	void InitializeLogArrays();

	// Fireplace overlap events
	UFUNCTION()
	void OnFireplaceDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnFireplaceDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Fireplace logic
	void ShowGhostEffect(AWoodLog* WoodLog);
	void HideGhostEffect();
	void PlaceLog();
	void OnFireplaceComplete();

	// Fireplace utilities
	int32 GetNextAvailableSlot() const;
	UStaticMeshComponent* GetLogMeshByIndex(int32 Index) const;
	void SetLogVisibilityAndMaterial(int32 LogIndex, bool bVisible, UMaterialInterface* Material);
	bool IsFireplaceComplete() const;

#pragma endregion

#pragma region Heat Zone Functions

	// Heat zone overlap events
	UFUNCTION()
	void OnHeatZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHeatZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Sheltered zone overlap events  
	UFUNCTION()
	void OnShelteredZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnShelteredZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Heat zone management
	void ActivateHeatZone();
	void DeactivateHeatZone();

#pragma endregion

public:

	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnFireComplete OnFireComplete;

#pragma region Public Interface

	// Check states
	UFUNCTION(BlueprintCallable, Category = "Fireplace")
	bool CanLightFire() const { return bEnableFireplace && IsFireplaceComplete(); }

	UFUNCTION(BlueprintCallable, Category = "Fireplace")
	bool IsHeatZoneActive() const { return bHeatZoneActive; }

	UFUNCTION(BlueprintCallable, Category = "Fireplace")
	bool IsFireplaceEnabled() const { return bEnableFireplace; }

#pragma endregion
};