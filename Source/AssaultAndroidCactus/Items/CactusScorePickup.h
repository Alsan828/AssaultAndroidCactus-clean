#pragma once

#include "CoreMinimal.h"
#include "Items/CactusItemPickup.h"
#include "CactusScorePickup.generated.h"

UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusScorePickup : public ACactusItemPickup
{
    GENERATED_BODY()

protected:
    virtual bool ApplyEffect(ACactusPlayerCharacter* Player) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Score", meta = (ClampMin = "1"))
    int32 ScoreValue = 100;
};
