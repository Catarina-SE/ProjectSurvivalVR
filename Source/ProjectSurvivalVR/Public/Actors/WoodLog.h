// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/VRGrabbableActor.h"
#include "WoodLog.generated.h"


UCLASS()
class PROJECTSURVIVALVR_API AWoodLog : public AVRGrabbableActor
{
	GENERATED_BODY()

public:
	AWoodLog();

protected:
	virtual void BeginPlay() override;

public:
	void NotifyPlaced();
};
