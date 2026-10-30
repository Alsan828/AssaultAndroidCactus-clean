#pragma once
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CactusAimHUD.generated.h"

// 第1段階の照準と操作表示。ゲーム状態の判定は持たない。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusAimHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
