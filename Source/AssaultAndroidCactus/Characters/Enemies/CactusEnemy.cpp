#include "Characters/Enemies/CactusEnemy.h"
#include "Characters/CactusPlayerCharacter.h"
#include "Game/Stages/CactusEnemySpawner.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Characters/Enemies/CactusEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Items/CactusItemPickup.h"
#include "Engine/World.h"

ACactusEnemy::ACactusEnemy()
{
    PrimaryActorTick.bCanEverTick = false;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = ACactusEnemyAIController::StaticClass();
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -96));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->DefaultLandMovementMode = MOVE_Walking;
    Movement->bRunPhysicsWithNoController = false;
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0, 540, 0);
    Movement->MaxWalkSpeed = 220.0f;
    Movement->MaxAcceleration = 1200.0f;
    Movement->BrakingDecelerationWalking = 1600.0f;
    // 移動アニメーションの加速度判定が働くよう、経路追従も入力・加速度で動かす。
    Movement->GetNavMovementProperties()->bUseAccelerationForPaths = true;

    ContactArea = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ContactArea"));
    ContactArea->SetupAttachment(GetRootComponent());
    // 本体の衝突は変えず、中心間120cmの停止位置まで接触判定を広げる。
    ContactArea->InitCapsuleSize(80, 100);
    ContactArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ContactArea->SetCollisionObjectType(ECC_WorldDynamic);
    ContactArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    ContactArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ContactArea->SetGenerateOverlapEvents(true);
    ContactArea->SetCanEverAffectNavigation(false);
    ContactArea->OnComponentBeginOverlap.AddDynamic(this, &ACactusEnemy::OnContactBegin);
    ContactArea->OnComponentEndOverlap.AddDynamic(this, &ACactusEnemy::OnContactEnd);
}

void ACactusEnemy::BeginPlay()
{
    Super::BeginPlay();
    Health = FMath::IsFinite(MaxHealth) ? FMath::Max(MaxHealth, 1.0f) : 10.0f;
    bDead = false;
    GetCharacterMovement()->Activate();
    GetCharacterMovement()->SetComponentTickEnabled(true);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (ContactPlayer.IsValid()) OnContactBegin(ContactArea, ContactPlayer.Get(), ContactPlayer->GetCapsuleComponent(), 0, false, FHitResult());
}

void ACactusEnemy::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(ContactTimer);
    ContactPlayer.Reset();
    Super::EndPlay(Reason);
}

ACactusEnemySpawner* ACactusEnemy::GetSpawnSource() const
{
    return SpawnSource.Get();
}

void ACactusEnemy::OnContactBegin(UPrimitiveComponent* Component, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    ACactusPlayerCharacter* Player = Cast<ACactusPlayerCharacter>(OtherActor);
    if (!Player || OtherComponent != Player->GetCapsuleComponent() || ContactDamage <= 0 || bDead) return;
    ContactPlayer = Player;
    if (!HasActorBegunPlay()) return;
    ApplyContactDamage();
    GetWorldTimerManager().SetTimer(ContactTimer, this, &ACactusEnemy::ApplyContactDamage,
        FMath::Max(ContactInterval, 0.1f), true);
}

void ACactusEnemy::OnContactEnd(UPrimitiveComponent* Component, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 BodyIndex)
{
    if (OtherActor != ContactPlayer.Get() || !ContactPlayer.IsValid() ||
        OtherComponent != ContactPlayer->GetCapsuleComponent()) return;
    ContactPlayer.Reset();
    GetWorldTimerManager().ClearTimer(ContactTimer);
}

void ACactusEnemy::ApplyContactDamage()
{
    ACactusPlayerCharacter* Player = ContactPlayer.Get();
    if (!IsValid(Player) || bDead || !ContactArea->IsOverlappingComponent(Player->GetCapsuleComponent()))
    {
        GetWorldTimerManager().ClearTimer(ContactTimer);
        ContactPlayer.Reset();
        return;
    }
    if (Player->IsDowned() || ContactDamage <= 0 || GetWorld()->GetTimeSeconds() < NextContactTime) return;
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusContact), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, Query) &&
        Hit.GetActor() != Player) return; // 薄い壁を挟んだ重なりではダメージを与えない。
    // 接触・離脱を繰り返しても攻撃間隔を飛ばさない。
    NextContactTime = GetWorld()->GetTimeSeconds() + FMath::Max(ContactInterval, 0.1f);
    UGameplayStatics::ApplyDamage(Player, ContactDamage, GetController(), this, nullptr);
}

float ACactusEnemy::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bDead || IsActorBeingDestroyed() || !CanBeDamaged() ||
        !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || Cast<ACactusEnemy>(DamageCauser)) return 0.0f;

    const float AppliedDamage = FMath::Min(DamageAmount, Health);
    Health -= AppliedDamage;
    bDead = Health <= 0.0f;
    // UE標準のダメージ通知からも更新後の体力を参照できる。
    Super::TakeDamage(AppliedDamage, DamageEvent, EventInstigator, DamageCauser);
    if (bDead && !IsActorBeingDestroyed())
    {
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        SetActorTickEnabled(false);
        SetCanBeDamaged(false);
        SetActorEnableCollision(false);
        // Destroyの波クリア通知より前に出す。リセット・Level終了のDestroyでは出さない。
        SpawnDeathDrop();
        Destroy();
    }
    return AppliedDamage;
}

void ACactusEnemy::SpawnDeathDrop()
{
    if (!HasAuthority() || !DropItemClass || DropItemClass->HasAnyClassFlags(CLASS_Abstract) ||
        !FMath::IsFinite(DropChance) || DropChance <= 0) return;
    if (DropChance < 1 && FMath::FRand() >= DropChance) return;
    const float Height = FMath::IsFinite(DropHeight) ? FMath::Max(DropHeight, 0.0f) : 30.0f;
    const FVector Position = GetActorLocation() + FVector(0, 0, Height - GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ACactusItemPickup>(DropItemClass, Position, FRotator::ZeroRotator, Parameters);
}
