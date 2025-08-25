// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FingerData.generated.h"

USTRUCT(BlueprintType)
struct FFingerData
{
    GENERATED_BODY()
    
    // Finger curl values (0.0 = fully extended, 1.0 = fully curled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|Fingers")
    float Thumb = 0.0f;
    
    // Finger curl values (0.0 = fully extended, 1.0 = fully curled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|Fingers")
    float Index = 0.0f;
    
    // Finger curl values (0.0 = fully extended, 1.0 = fully curled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|Fingers")
    float Middle = 0.0f;
    
    // Finger curl values (0.0 = fully extended, 1.0 = fully curled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|Fingers")
    float Ring = 0.0f;
    
    // Finger curl values (0.0 = fully extended, 1.0 = fully curled)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand|Fingers")
    float Pinky = 0.0f;
    
};
