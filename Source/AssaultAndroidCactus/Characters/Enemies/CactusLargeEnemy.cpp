#include "Characters/Enemies/CactusLargeEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ACactusLargeEnemy::ACactusLargeEnemy()
{
    MaxHealth = 30.0f;
    ContactDamage = 1.0f;
    StoppingDistance = 140.0f;
    bStopToShoot = false;
    ShootingRange = 700.0f;
    FireInterval = 1.5f;
    ProjectileSpeed = 700.0f;

    GetCapsuleComponent()->InitCapsuleSize(52.5f, 120.0f);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -120));
    GetMesh()->SetRelativeScale3D(FVector(1.25f));
    ContactArea->InitCapsuleSize(105.0f, 125.0f);
    GetCharacterMovement()->MaxWalkSpeed = 180.0f;
}
