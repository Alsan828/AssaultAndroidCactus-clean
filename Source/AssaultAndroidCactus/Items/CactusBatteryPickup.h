#pragma once

#include "CoreMinimal.h"
#include "Items/CactusItemPickup.h"
#include "CactusBatteryPickup.generated.h"

UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusBatteryPickup : public ACactusItemPickup
{
    GENERATED_BODY()
protected:
    virtual bool ApplyEffect(ACactusPlayerCharacter* Player) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Battery", meta = (ClampMin = "0.1"))
    float RestoreAmount = 25.0f;
};
