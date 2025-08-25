#include "Core/VRGameInstance.h"
#include "Kismet/GameplayStatics.h"

UVRGameInstance::UVRGameInstance()
{
}

void UVRGameInstance::Init()
{
    Super::Init();
    UE_LOG(LogTemp, Log, TEXT("VRGameInstance initialized"));
}

void UVRGameInstance::ReturnToGame()
{
    UE_LOG(LogTemp, Log, TEXT("Returning to game"));
    OnReturnToGameStarted();
    TransitionToLevel(GameLevelName);
}

void UVRGameInstance::GoToMenu()
{
    UE_LOG(LogTemp, Log, TEXT("Going to menu"));
    OnGoToMenuStarted();
    TransitionToLevel(MenuLevelName);
}

void UVRGameInstance::TransitionToLevel(const FString& LevelName, float DelaySeconds)
{
    OnLevelTransition.Broadcast(LevelName);

    GetWorld()->GetTimerManager().SetTimer(
        LevelTransitionTimer,
        [this, LevelName]() { ExecuteLevelTransition(LevelName); },
        DelaySeconds,
        false
    );
}

void UVRGameInstance::TransitionToLevelImmediate(const FString& LevelName)
{
    OnLevelTransition.Broadcast(LevelName);
    ExecuteLevelTransition(LevelName);
}

void UVRGameInstance::ExecuteLevelTransition(const FString& LevelName)
{
    UE_LOG(LogTemp, Warning, TEXT("Transitioning to level: %s"), *LevelName);
    UGameplayStatics::OpenLevel(this, FName(*LevelName));
}