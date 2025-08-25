#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VRGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelTransition, FString, LevelName);

UCLASS()
class PROJECTSURVIVALVR_API UVRGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UVRGameInstance();

protected:
    virtual void Init() override;

#pragma region Level Management

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Levels")
    FString MenuLevelName = "MenuLevel";

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Levels")
    FString GameLevelName = "MainLevel";

    FTimerHandle LevelTransitionTimer;

#pragma endregion

public:

#pragma region Level Transitions

    // Return to game from menu
    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void ReturnToGame();

    // Go to menu
    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void GoToMenu();

    // Generic level transition with delay
    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void TransitionToLevel(const FString& LevelName, float DelaySeconds = 0.5f);

    // Immediate level transition
    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void TransitionToLevelImmediate(const FString& LevelName);

#pragma endregion

#pragma region Getters/Setters

    UFUNCTION(BlueprintPure, Category = "Level Management")
    FString GetMenuLevelName() const { return MenuLevelName; }

    UFUNCTION(BlueprintPure, Category = "Level Management")
    FString GetGameLevelName() const { return GameLevelName; }

    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void SetMenuLevelName(const FString& NewMenuLevel) { MenuLevelName = NewMenuLevel; }

    UFUNCTION(BlueprintCallable, Category = "Level Management")
    void SetGameLevelName(const FString& NewGameLevel) { GameLevelName = NewGameLevel; }

#pragma endregion

#pragma region Events

    UPROPERTY(BlueprintAssignable, Category = "Level Management")
    FOnLevelTransition OnLevelTransition;

    UFUNCTION(BlueprintImplementableEvent, Category = "Level Management")
    void OnReturnToGameStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Level Management")
    void OnGoToMenuStarted();

#pragma endregion

protected:
    void ExecuteLevelTransition(const FString& LevelName);
};