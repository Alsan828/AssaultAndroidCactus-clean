#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CactusItemPickup.generated.h"

class ACactusPlayerCharacter;
class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

// 接触と一度だけの取得を共通化。効果は子クラスで定義する。
UCLASS(Abstract)
class ASSAULTANDROIDCACTUS_API ACactusItemPickup : public AActor
{
    GENERATED_BODY()

public:
    ACactusItemPickup();

protected:
    virtual void BeginPlay() override;
    virtual bool ApplyEffect(ACactusPlayerCharacter* Player) { return false; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    TObjectPtr<USphereComponent> PickupArea;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UStaticMeshComponent> Visual;

private:
    UFUNCTION()
    void OnPickupBegin(UPrimitiveComponent* Component, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

    void TryCollect(ACactusPlayerCharacter* Player);
    bool bCollected = false;
};
