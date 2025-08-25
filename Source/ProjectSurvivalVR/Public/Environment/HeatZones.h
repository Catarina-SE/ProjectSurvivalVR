// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Components/SphereComponent.h>
#include <NiagaraComponent.h>
#include <Components/BoxComponent.h>
#include "HeatZones.generated.h"




UCLASS()
class PROJECTSURVIVALVR_API AHeatZones : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AHeatZones();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
	
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> HeatZoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USphereComponent> HeatZoneBoxCollider;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> ShelteredBoxCollider;



public:
	// Heat Zone Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone")
	float IntenseHeatRecoveryRate = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone")
    float ShelteredHeatRecoveryRate = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone | Settings")
	bool bUseHeatZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat Zone | Settings")
	bool bUseShelteredZone = true;

	// Overlap Event Handlers
    UFUNCTION()
    void OnHeatZoneOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnHeatZoneOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void OnShelteredZoneOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnShelteredZoneOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
