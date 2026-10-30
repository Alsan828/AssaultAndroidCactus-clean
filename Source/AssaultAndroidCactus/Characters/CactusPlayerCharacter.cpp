#include "Characters/CactusPlayerCharacter.h"
#include "Game/CactusGameMode.h"
#include "Weapons/CactusWeaponComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

namespace CactusMovement
{
constexpr float CapsuleRadius = 42.0f;
constexpr float CapsuleHalfHeight = 96.0f;
}

ACactusPlayerCharacter::ACactusPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(CactusMovement::CapsuleRadius, CactusMovement::CapsuleHalfHeight);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
    GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;

    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -CactusMovement::CapsuleHalfHeight));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    GetMesh()->bCastHiddenShadow = true;

    TopDownBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("TopDownBoom"));
    TopDownBoom->SetupAttachment(GetRootComponent());
    TopDownBoom->SetUsingAbsoluteRotation(true);
    // 壁の近くで俯瞰カメラが急に寄らないよう、追従距離を固定する。
    TopDownBoom->bDoCollisionTest = false;

    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(TopDownBoom, USpringArmComponent::SocketName);
    TopDownCamera->bUsePawnControlRotation = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetRootComponent());
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetAutoActivate(false);

    // BP_CactusCharacterのコンポーネント一覧にも表示される。
    Weapon = CreateDefaultSubobject<UCactusWeaponComponent>(TEXT("Weapon"));
    ApplyPlayerSettings();
}

void ACactusPlayerCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyPlayerSettings();
}

void ACactusPlayerCharacter::ApplyPlayerSettings()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = FMath::Max(MaxMoveSpeed, 0.0f);
    Movement->MaxAcceleration = FMath::Max(MoveAcceleration, 0.0f);
    Movement->BrakingDecelerationWalking = FMath::Max(MoveBrakingDeceleration, 0.0f);
    TopDownBoom->TargetArmLength = FMath::Max(TopDownArmLength, 0.0f);
    FRotator BoomRotation = TopDownBoom->GetRelativeRotation();
    BoomRotation.Pitch = FMath::Clamp(TopDownCameraPitch, -89.0f, 0.0f);
    TopDownBoom->SetRelativeRotation(BoomRotation);
    TopDownCamera->SetFieldOfView(FMath::Clamp(TopDownFieldOfView, 5.0f, 170.0f));
    FVector EyeLocation = FirstPersonCamera->GetRelativeLocation();
    EyeLocation.Z = FirstPersonEyeHeight;
    FirstPersonCamera->SetRelativeLocation(EyeLocation);
    FirstPersonCamera->SetFieldOfView(FMath::Clamp(FirstPersonFieldOfView, 5.0f, 170.0f));
}

void ACactusPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyPlayerSettings();
    AimDirection = GetActorForwardVector();
    Health = FMath::IsFinite(MaxHealth) ? FMath::Max(MaxHealth, 1.0f) : 10.0f;
    bDowned = false;
    InvulnerableUntil = 0;
    NextWeaponSwitchTime = 0;
    AimPoint = GetActorLocation() + AimDirection * AimTraceDistance;
    ConfigureLocalInput();
    ApplyCameraMode();
}

void ACactusPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    StopFire();
    GetWorldTimerManager().ClearTimer(RecoveryTimer);
    if (RegisteredInputSubsystem.IsValid() && InputMapping)
    {
        RegisteredInputSubsystem->RemoveMappingContext(InputMapping);
    }
    Super::EndPlay(Reason);
}

bool ACactusPlayerCharacter::IsInvulnerable() const
{
    return bDowned || (GetWorld() && GetWorld()->GetTimeSeconds() < InvulnerableUntil);
}

float ACactusPlayerCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!CanBeDamaged() || IsActorBeingDestroyed() || IsInvulnerable() ||
        !FMath::IsFinite(DamageAmount) || DamageAmount <= 0) return 0;
    const float Applied = FMath::Min(Health, DamageAmount);
    Health -= Applied;
    InvulnerableUntil = GetWorld()->GetTimeSeconds() + FMath::Max(HitInvulnerability, 0.0f);
    if (Health <= 0)
    {
        bDowned = true;
        if (ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>()) Mode->ConsumeDownedBattery();
        StopFire();
        ConsumeMovementInputVector();
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
        // 体力0はダウン。ActorやLevelを作り直さず、同じ場所で復帰する。
        GetWorldTimerManager().SetTimer(RecoveryTimer, this, &ACactusPlayerCharacter::RecoverFromDown,
            FMath::Max(RecoveryDuration, 0.1f), false);
    }
    Super::TakeDamage(Applied, DamageEvent, EventInstigator, DamageCauser);
    return Applied;
}

void ACactusPlayerCharacter::RecoverFromDown()
{
    bDowned = false;
    Health = FMath::IsFinite(MaxHealth) ? FMath::Max(MaxHealth, 1.0f) : 10.0f;
    InvulnerableUntil = GetWorld()->GetTimeSeconds() + FMath::Max(RecoveryInvulnerability, 0.0f);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ACactusPlayerCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();
    StopFire();
    bGamepadAimActive = false;
    ConfigureLocalInput();
    ApplyCameraMode();
}

void ACactusPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    bGamepadAimActive = false;
    ConfigureLocalInput();
    ApplyCameraMode();
}

void ACactusPlayerCharacter::ConfigureLocalInput()
{
    APlayerController* Player = Cast<APlayerController>(Controller);
    if (!Player || !Player->GetLocalPlayer() || !InputMapping)
    {
        return;
    }
    UEnhancedInputLocalPlayerSubsystem* Subsystem = Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (Subsystem)
    {
        // 他のUIなどが追加したMappingContextは消さない。
        Subsystem->RemoveMappingContext(InputMapping);
        Subsystem->AddMappingContext(InputMapping, 0);
        RegisteredInputSubsystem = Subsystem;
    }
}

void ACactusPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    if (MoveAction) Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACactusPlayerCharacter::Move);
    if (LookAction) Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACactusPlayerCharacter::Look);
    if (GamepadLookAction) Input->BindAction(GamepadLookAction, ETriggerEvent::Triggered, this, &ACactusPlayerCharacter::LookWithGamepad);
    if (SwitchViewAction) Input->BindAction(SwitchViewAction, ETriggerEvent::Started, this, &ACactusPlayerCharacter::SwitchView);
    if (SwitchWeaponAction) Input->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &ACactusPlayerCharacter::SwitchWeaponFromInput);
    if (ExitAction) Input->BindAction(ExitAction, ETriggerEvent::Started, this, &ACactusPlayerCharacter::ExitGame);
    if (FireAction)
    {
        Input->BindAction(FireAction, ETriggerEvent::Started, this, &ACactusPlayerCharacter::StartFire);
        Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ACactusPlayerCharacter::StopFire);
        Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &ACactusPlayerCharacter::StopFire);
    }
}

void ACactusPlayerCharacter::StartFire()
{
    if (bDowned) return;
    UpdateAim();
    Weapon->StartFiring();
}

void ACactusPlayerCharacter::StopFire()
{
    if (Weapon) Weapon->StopFiring();
}

float ACactusPlayerCharacter::GetWeaponSwitchCooldownRemaining() const
{
    return GetWorld() ? FMath::Max(float(NextWeaponSwitchTime-GetWorld()->GetTimeSeconds()),0.0f) : 0.0f;
}

void ACactusPlayerCharacter::SwitchWeaponFromInput()
{
    SwitchWeapon();
}

bool ACactusPlayerCharacter::SwitchWeapon()
{
    if (bDowned || !Weapon || !GetWorld() || GetWorld()->IsPaused() || GetWeaponSwitchCooldownRemaining() > 0) return false;
    const ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>();
    if (Mode && (Mode->IsStageFinished() || Mode->GetStageOutcome() != ECactusStageOutcome::InProgress)) return false;
    Weapon->ToggleAttackMode();
    const double Now = GetWorld()->GetTimeSeconds();
    const float Protection = FMath::IsFinite(WeaponSwitchInvulnerability) ? FMath::Max(WeaponSwitchInvulnerability,0.0f) : 1.0f;
    const float Cooldown = FMath::IsFinite(WeaponSwitchCooldown) ? FMath::Max(WeaponSwitchCooldown,0.0f) : 1.5f;
    // 既存の被弾・復帰保護を短くしない。連打で保護時間を延長させない。
    InvulnerableUntil = FMath::Max(InvulnerableUntil,Now+Protection);
    NextWeaponSwitchTime = Now+FMath::Max(Cooldown,Protection);
    return true;
}

void ACactusPlayerCharacter::Move(const FInputActionValue& Value)
{
    MoveInViewSpace(Value.Get<FVector2D>());
}

void ACactusPlayerCharacter::MoveInViewSpace(FVector2D Input)
{
    if (bDowned || !Controller || Input.ContainsNaN()) return;
    const float Yaw = CameraMode == ECactusCameraMode::FirstPerson
        ? Controller->GetControlRotation().Yaw : TopDownCamera->GetComponentRotation().Yaw;
    const FRotationMatrix Basis(FRotator(0.0f, Yaw, 0.0f));
    // 斜め入力の速度制限はCharacterMovementが行う。
    AddMovementInput(Basis.GetUnitAxis(EAxis::X), Input.Y);
    AddMovementInput(Basis.GetUnitAxis(EAxis::Y), Input.X);
}

void ACactusPlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D Input = Value.Get<FVector2D>();
    if (Input.ContainsNaN() || Input.IsNearlyZero()) return;
    SetGamepadAimActive(false);
    if (CameraMode != ECactusCameraMode::FirstPerson) return;
    AddControllerYawInput(Input.X * MouseSensitivity);
    AddControllerPitchInput(Input.Y * MouseSensitivity);
}

void ACactusPlayerCharacter::LookWithGamepad(const FInputActionValue& Value)
{
    const FVector2D Input = Value.Get<FVector2D>();
    if (Input.ContainsNaN() || Input.IsNearlyZero()) return;
    SetGamepadAimActive(true);
    // 右スティックは両視点で上下のみ反転し、左右は通常方向にする。
    FVector2D Stick = Input.GetClampedToMaxSize(1.0f);
    Stick.Y = -Stick.Y;
    if (CameraMode == ECactusCameraMode::FirstPerson)
    {
        // マウスの移動量と違い、スティックは毎秒の旋回速度として扱う。
        const float Step = FMath::Max(GamepadLookSpeed, 1.0f) * GetWorld()->GetDeltaSeconds();
        AddControllerYawInput(Stick.X * Step);
        AddControllerPitchInput(Stick.Y * Step);
    }
    else
    {
        const FRotationMatrix Basis(FRotator(0.0f, TopDownCamera->GetComponentRotation().Yaw, 0.0f));
        const FVector Direction = Basis.GetUnitAxis(EAxis::X) * Stick.Y + Basis.GetUnitAxis(EAxis::Y) * Stick.X;
        AimAtWorldPoint(GetActorLocation() + Direction.GetSafeNormal() * GamepadAimDistance);
    }
}

void ACactusPlayerCharacter::SetGamepadAimActive(bool bActive)
{
    if (bGamepadAimActive == bActive) return;
    bGamepadAimActive = bActive;
    if (APlayerController* Player = Cast<APlayerController>(Controller); Player && Player->GetLocalPlayer())
    {
        Player->bShowMouseCursor = CameraMode == ECactusCameraMode::TopDown && !bGamepadAimActive;
    }
}

void ACactusPlayerCharacter::SwitchView()
{
    SetCameraMode(CameraMode == ECactusCameraMode::TopDown
        ? ECactusCameraMode::FirstPerson : ECactusCameraMode::TopDown);
}

void ACactusPlayerCharacter::SetCameraMode(ECactusCameraMode NewMode)
{
    if (CameraMode == NewMode) return;
    CameraMode = NewMode;
    if (CameraMode == ECactusCameraMode::FirstPerson && Controller)
    {
        Controller->SetControlRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
    }
    // 位置、速度、移動Componentは切り替えず、視点だけを変更する。
    ApplyCameraMode();
}

void ACactusPlayerCharacter::ApplyCameraMode()
{
    const bool bFirstPerson = CameraMode == ECactusCameraMode::FirstPerson;
    TopDownCamera->SetActive(!bFirstPerson);
    FirstPersonCamera->SetActive(bFirstPerson);
    bUseControllerRotationYaw = bFirstPerson;
    GetMesh()->SetOwnerNoSee(bFirstPerson);

    APlayerController* Player = Cast<APlayerController>(Controller);
    if (!Player || !Player->GetLocalPlayer()) return;
    Player->bShowMouseCursor = !bFirstPerson && !bGamepadAimActive;
    Player->DefaultMouseCursor = EMouseCursor::Crosshairs;
    if (Player->PlayerCameraManager)
    {
        Player->PlayerCameraManager->ViewPitchMin = -FMath::Clamp(ViewPitchLimit, 1.0f, 89.0f);
        Player->PlayerCameraManager->ViewPitchMax = FMath::Clamp(ViewPitchLimit, 1.0f, 89.0f);
    }
    if (bFirstPerson)
    {
        Player->SetInputMode(FInputModeGameOnly().SetConsumeCaptureMouseDown(false));
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        Mode.SetHideCursorDuringCapture(false);
        Player->SetInputMode(Mode);
    }
    Player->SetViewTarget(this);
}

void ACactusPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateAim();
}

void ACactusPlayerCharacter::AimAtWorldPoint(FVector WorldPoint)
{
    if (WorldPoint.ContainsNaN()) return;
    FVector Direction = WorldPoint - GetActorLocation();
    Direction.Z = 0.0f;
    if (Direction.IsNearlyZero()) return;
    AimPoint = WorldPoint;
    AimDirection = Direction.GetSafeNormal();
    SetActorRotation(FRotator(0.0f, AimDirection.Rotation().Yaw, 0.0f));
}

void ACactusPlayerCharacter::UpdateAim()
{
    APlayerController* Player = Cast<APlayerController>(Controller);
    if (!Player) return;
    FVector Origin, Direction;
    const bool bFirstPerson = CameraMode == ECactusCameraMode::FirstPerson;
    if (bFirstPerson)
    {
        Origin = FirstPersonCamera->GetComponentLocation();
        Direction = Player->GetControlRotation().Vector();
    }
    else if (bGamepadAimActive)
    {
        // スティックを離しても照準を維持し、古いマウス位置へ戻さない。
        AimAtWorldPoint(GetActorLocation() + GetActorForwardVector() * GamepadAimDistance);
        return;
    }
    else if (!Player->GetLocalPlayer() || !Player->DeprojectMousePositionToWorld(Origin, Direction))
    {
        return;
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CactusAim), true, this);
    FHitResult Hit;
    const FVector End = Origin + Direction * AimTraceDistance;
    FVector Target = End;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Query))
    {
        Target = Hit.ImpactPoint;
    }
    else if (!bFirstPerson)
    {
        // 床の外を指しても、足元の平面へ投影して向きを求める。
        const FPlane Ground(GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), FVector::UpVector);
        const float FacingPlane = FVector::DotProduct(Direction, FVector::UpVector);
        if (FMath::IsNearlyZero(FacingPlane)) return;
        Target = FMath::RayPlaneIntersection(Origin, Direction, Ground);
    }
    if (bFirstPerson)
    {
        AimPoint = Target;
        AimDirection = Direction;
    }
    else
    {
        AimAtWorldPoint(Target);
    }
}

void ACactusPlayerCharacter::ExitGame()
{
    UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(Controller), EQuitPreference::Quit, false);
}
