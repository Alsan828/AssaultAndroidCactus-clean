#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemies/CactusRangedEnemy.h"
#include "CactusLargeEnemy.generated.h"

// 追跡しながら射撃し、近距離では接触攻撃を行う体力の高い敵。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusLargeEnemy : public ACactusRangedEnemy
{
    GENERATED_BODY()
public:
    ACactusLargeEnemy();
};
