#include "Characters/Enemies/CactusEnemyAIController.h"
#include "Characters/Enemies/CactusEnemy.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ACactusEnemyAIController::ACactusEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACactusEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    const float Interval = FMath::Max(ChaseUpdateInterval, 0.05f);
    // 複数の敵が同じフレームに判断を更新しないよう、初回を分散する。
    GetWorldTimerManager().SetTimer(ChaseTimer, this, &ACactusEnemyAIController::UpdateChase,
        Interval, true, FMath::FRandRange(0.01f, Interval));
}

void ACactusEnemyAIController::ClearChase()
{
    GetWorldTimerManager().ClearTimer(ChaseTimer);
    StopMovement();
    CurrentTarget.Reset();
}

void ACactusEnemyAIController::OnUnPossess()
{
    ClearChase();
    Super::OnUnPossess();
}

void ACactusEnemyAIController::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearChase();
    Super::EndPlay(Reason);
}

void ACactusEnemyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    // 加速度で追従しても、停止距離に到達した後は惰性で滑らせない。
    if (Result.IsSuccess())
        if (ACactusEnemy* Enemy = Cast<ACactusEnemy>(GetPawn()))
            Enemy->GetCharacterMovement()->StopMovementImmediately();
    Super::OnMoveCompleted(RequestID, Result);
}

void ACactusEnemyAIController::UpdateChase()
{
    ACactusEnemy* Enemy = Cast<ACactusEnemy>(GetPawn());
    const APlayerController* Player = GetWorld()->GetFirstPlayerController();
    APawn* Target = Player ? Player->GetPawn() : nullptr;
    if (!IsValid(Enemy) || Enemy->IsDead() || !IsValid(Target) ||
        Target == Enemy || Target->IsActorBeingDestroyed() ||
        (Cast<ACactusPlayerCharacter>(Target) && CastChecked<ACactusPlayerCharacter>(Target)->IsDowned()))
    {
        StopMovement();
        CurrentTarget.Reset();
        return;
    }

    if (Enemy->TryAttackPlayer(Cast<ACactusPlayerCharacter>(Target)))
    {
        StopMovement();
        CurrentTarget = Target;
        return;
    }

    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Navigation || !Navigation->GetNavDataForProps(GetNavAgentPropertiesRef(), GetNavAgentLocation()))
    {
        StopMovement(); // ナビデータがない場所で直進へ切り替えることはしない。
        CurrentTarget.Reset();
        return;
    }

    const float Radius = FMath::Max(Enemy->GetStoppingDistance(), 0.0f);
    if (FVector::DistSquared2D(Enemy->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Radius))
    {
        StopMovement();
        CurrentTarget = Target;
        return;
    }
    if (CurrentTarget.Get() == Target && GetMoveStatus() != EPathFollowingStatus::Idle)
        return; // 有効な経路はUEが追従・更新する。毎回経路を作り直さない。

    CurrentTarget = Target;
    FAIMoveRequest Request(Target);
    Request.SetUsePathfinding(true);
    Request.SetAllowPartialPath(false);
    Request.SetProjectGoalLocation(true);
    Request.SetAcceptanceRadius(Radius);
    // 既存の「中心間120cm」を維持し、カプセル半径を加算しない。
    Request.SetReachTestIncludesAgentRadius(false);
    Request.SetReachTestIncludesGoalRadius(false);
    MoveTo(Request);
}
