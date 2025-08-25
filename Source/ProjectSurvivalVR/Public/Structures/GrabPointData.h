// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrabPointData.generated.h"

USTRUCT(BlueprintType)
struct FGrabPointData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|GrabData")
	FName SocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR|GrabData")
	TSoftObjectPtr<UAnimationAsset> GrabAnimation;
};
