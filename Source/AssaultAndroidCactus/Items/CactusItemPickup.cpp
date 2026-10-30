#include "Items/CactusItemPickup.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

ACactusItemPickup::ACactusItemPickup()
{
    PrimaryActorTick.bCanEverTick = false;
    PickupArea = CreateDefaultSubobject<USphereComponent>(TEXT("PickupArea"));
    SetRootComponent(PickupArea);
    PickupArea->InitSphereRadius(60.0f);
    PickupArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupArea->SetCollisionObjectType(ECC_WorldDynamic);
    PickupArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    PickupArea->SetGenerateOverlapEvents(true);
    PickupArea->SetCanEverAffectNavigation(false);
    PickupArea->OnComponentBeginOverlap.AddDynamic(this, &ACactusItemPickup::OnPickupBegin);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(PickupArea);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetGenerateOverlapEvents(false);
    Visual->SetCanEverAffectNavigation(false);
}

void ACactusItemPickup::BeginPlay()
{
    Super::BeginPlay();
    // 出現時から重なっている場合も、通常の接触と同じ処理へ渡す。
    TArray<UPrimitiveComponent*> Overlaps;
    PickupArea->GetOverlappingComponents(Overlaps);
    for (UPrimitiveComponent* Component : Overlaps)
        if (ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(Component->GetOwner()))
            if (Component == Player->GetCapsuleComponent()) TryCollect(Player);
}

void ACactusItemPickup::OnPickupBegin(UPrimitiveComponent* Component, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(OtherActor);
    if (Player && OtherComponent == Player->GetCapsuleComponent()) TryCollect(Player);
}

void ACactusItemPickup::TryCollect(ACactusPlayerCharacter* Player)
{
    if (!HasAuthority() || !HasActorBegunPlay() || bCollected || IsActorBeingDestroyed() ||
        !IsValid(Player) || Player->IsDowned() || Player->IsActorBeingDestroyed()) return;

    bCollected = true; // 効果の適用中に再び接触通知が来ても、二重取得しない。
    if (!ApplyEffect(Player))
    {
        bCollected = false;
        return; // 加算先がない等の失敗時は道具を残す。
    }
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    Destroy();
}
