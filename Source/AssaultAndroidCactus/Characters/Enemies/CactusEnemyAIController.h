#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CactusEnemyAIController.generated.h"

// NavMesh上の経路追従はUEへ任せ、一定間隔で追跡先だけを確認する。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    ACactusEnemyAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|AI", meta = (ClampMin = "0.05", Units = "s"))
    float ChaseUpdateInterval = 0.25f;

private:
    void UpdateChase();
    void ClearChase();

    TWeakObjectPtr<APawn> CurrentTarget;
    FTimerHandle ChaseTimer;
};
