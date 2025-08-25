#include "Environment/HeatZones.h"
#include "Components/SurvivalComponent.h"


AHeatZones::AHeatZones()
{
    
    SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
    RootComponent = SceneRoot;

    HeatZoneBoxCollider = CreateDefaultSubobject<USphereComponent>("HeatZoneSphereCollider");
    HeatZoneBoxCollider->SetupAttachment(RootComponent);

    ShelteredBoxCollider = CreateDefaultSubobject<UBoxComponent>("ShelteredSphereCollider");
    ShelteredBoxCollider->SetupAttachment(RootComponent);

    HeatZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>("HeatZoneMesh");
    HeatZoneMesh->SetupAttachment(RootComponent);

    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("NiagaraComponent");
	NiagaraComponent->SetupAttachment(HeatZoneMesh);

}

void AHeatZones::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    
    if (HeatZoneBoxCollider)
    {
        HeatZoneBoxCollider->SetHiddenInGame(true);
        HeatZoneBoxCollider->SetVisibility(bUseHeatZone, true); 
    }

    
    if (ShelteredBoxCollider)
    {
        ShelteredBoxCollider->SetHiddenInGame(true);
        ShelteredBoxCollider->SetVisibility(bUseShelteredZone);
    }
}


void AHeatZones::BeginPlay()
{
	Super::BeginPlay();

    
    if (bUseHeatZone)
    {
        HeatZoneBoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        HeatZoneBoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AHeatZones::OnHeatZoneOverlapBegin);
        HeatZoneBoxCollider->OnComponentEndOverlap.AddDynamic(this, &AHeatZones::OnHeatZoneOverlapEnd);
    }
    else
    {
        HeatZoneBoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

   
    if (bUseShelteredZone)
    {
        ShelteredBoxCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        ShelteredBoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AHeatZones::OnShelteredZoneOverlapBegin);
        ShelteredBoxCollider->OnComponentEndOverlap.AddDynamic(this, &AHeatZones::OnShelteredZoneOverlapEnd);
    }
    else
    {
        ShelteredBoxCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

}

void AHeatZones::OnHeatZoneOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    USurvivalComponent* SurvivalComp = Cast<USurvivalComponent>(OtherActor->FindComponentByClass<USurvivalComponent>());
    if (SurvivalComp)
    {
        SurvivalComp->SetIsInIntenseHeatZone(true);
        SurvivalComp->SetIntenseHeatRecoveryRate(IntenseHeatRecoveryRate);

        UE_LOG(LogTemp, Log, TEXT("Entered Heat Zone: %s"), *OtherActor->GetName());
    }
}

void AHeatZones::OnHeatZoneOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
    if (SurvivalComp)
    {
        SurvivalComp->SetIsInIntenseHeatZone(false);

        UE_LOG(LogTemp, Log, TEXT("Exited Heat Zone: %s"), *OtherActor->GetName());
    }
}

void AHeatZones::OnShelteredZoneOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
    if (SurvivalComp)
    {
        // Set heat zone state to sheltered
        SurvivalComp->SetIsInShelteredZone(true);
        SurvivalComp->SetShelteredHeatRecoveryRate(ShelteredHeatRecoveryRate);

        UE_LOG(LogTemp, Log, TEXT("Entered Sheltered Zone: %s"), *OtherActor->GetName());
    }
}

void AHeatZones::OnShelteredZoneOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    USurvivalComponent* SurvivalComp = OtherActor->FindComponentByClass<USurvivalComponent>();
    if (SurvivalComp)
    {
        // Exit sheltered zone
        SurvivalComp->SetIsInShelteredZone(false);

        UE_LOG(LogTemp, Log, TEXT("Exited Sheltered Zone: %s"), *OtherActor->GetName());
    }
}

