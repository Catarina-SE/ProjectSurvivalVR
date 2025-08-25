// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SurvivalComponent.h"
#include "BaseVRHUD.generated.h"


UCLASS()
class PROJECTSURVIVALVR_API UBaseVRHUD : public UUserWidget
{
	GENERATED_BODY()

private:

	USurvivalComponent* SurvivalComponent;

protected:
	// Called when the widget is constructed
	virtual void NativeConstruct() override;

	float Hunger;

	float Thirst;

	float Temperature;

	FTimerHandle UpdateUITimer;

	void RefreshUI();

	// Returns the Hunger Percentage
	UFUNCTION(BlueprintPure, Category = "Survival")
	float GetHungerPercentage();

	// Returns the Thirst Percentage
	UFUNCTION(BlueprintPure, Category = "Survival")
	float GetThirstPercentage();

	// Returns the Temperature Text
	UFUNCTION(BlueprintPure)
	FString GetTemperatureText() const;
	
public:

	// Stamina UI Property
	UPROPERTY(BlueprintReadOnly, Category = "Survival UI")
	float Stamina = 100.0f;

	// Add this function declaration
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Survival UI")
	float GetStaminaPercentage();

};