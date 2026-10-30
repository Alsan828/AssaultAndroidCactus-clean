#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CactusPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UCactusWeaponComponent;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ECactusCameraMode : uint8
{
    TopDown,
    FirstPerson
};

// 移動はCharacterMovement、入力はEnhanced Inputに任せる。
UCLASS()
class ASSAULTANDROIDCACTUS_API ACactusPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ACactusPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void NotifyControllerChanged() override;
    virtual void PawnClientRestart() override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    bool IsDowned() const { return bDowned; }

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    bool IsInvulnerable() const;

    UFUNCTION(BlueprintCallable, Category = "Player|Weapon Switch")
    bool SwitchWeapon();

    UFUNCTION(BlueprintPure, Category = "Player|Weapon Switch")
    float GetWeaponSwitchCooldownRemaining() const;

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveInViewSpace(FVector2D Input);

    UFUNCTION(BlueprintCallable, Category = "Aim")
    void AimAtWorldPoint(FVector WorldPoint);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetCameraMode(ECactusCameraMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Camera")
    ECactusCameraMode GetCameraMode() const { return CameraMode; }

    UFUNCTION(BlueprintPure, Category = "Aim")
    FVector GetAimPoint() const { return AimPoint; }

    UFUNCTION(BlueprintPure, Category = "Aim")
    FVector GetAimDirection() const { return AimDirection; }

    UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
    UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
    UCactusWeaponComponent* GetWeapon() const { return Weapon; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Weapon Switch", meta = (ClampMin = "0", Units = "s"))
    float WeaponSwitchInvulnerability = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Weapon Switch", meta = (ClampMin = "0", Units = "s"))
    float WeaponSwitchCooldown = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "1.0"))
    float MaxHealth = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "0.0", Units = "s"))
    float HitInvulnerability = 0.75f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "0.1", Units = "s"))
    float RecoveryDuration = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (ClampMin = "0.0", Units = "s"))
    float RecoveryInvulnerability = 1.5f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
    float Health = 10.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health")
    bool bDowned = false;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> TopDownBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> TopDownCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<UCactusWeaponComponent> Weapon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> InputMapping;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> GamepadLookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SwitchViewAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ExitAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> FireAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SwitchWeaponAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.01"))
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "1.0", Units = "deg/s"))
    float GamepadLookSpeed = 180.0f;

    // 調整値はBPのClass Defaultsで設定し、生成時とPlay開始時に適用する。
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
    float MaxMoveSpeed = 420.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement", meta = (ClampMin = "0.0"))
    float MoveAcceleration = 1800.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Movement", meta = (ClampMin = "0.0"))
    float MoveBrakingDeceleration = 2000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|Top Down", meta = (ClampMin = "0.0", Units = "cm"))
    float TopDownArmLength = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|Top Down", meta = (ClampMin = "-89.0", ClampMax = "0.0", Units = "deg"))
    float TopDownCameraPitch = -60.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|Top Down", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
    float TopDownFieldOfView = 55.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|First Person", meta = (Units = "cm"))
    float FirstPersonEyeHeight = 64.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|First Person", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
    float FirstPersonFieldOfView = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Camera|First Person", meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float ViewPitchLimit = 80.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Aim", meta = (ClampMin = "1.0", Units = "cm"))
    float AimTraceDistance = 100000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Aim", meta = (ClampMin = "1.0", Units = "cm"))
    float GamepadAimDistance = 600.0f;

private:
    void RecoverFromDown();
    FTimerHandle RecoveryTimer;
    double InvulnerableUntil = 0;
    double NextWeaponSwitchTime = 0;

    void ApplyPlayerSettings();
    void ConfigureLocalInput();
    void ApplyCameraMode();
    void UpdateAim();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void LookWithGamepad(const FInputActionValue& Value);
    void SetGamepadAimActive(bool bActive);
    void SwitchView();
    void SwitchWeaponFromInput();
    void ExitGame();
    void StartFire();
    void StopFire();

    UPROPERTY(VisibleInstanceOnly, Category = "Camera")
    ECactusCameraMode CameraMode = ECactusCameraMode::TopDown;

    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> RegisteredInputSubsystem;
    bool bGamepadAimActive = false;
    FVector AimPoint = FVector::ZeroVector;
    FVector AimDirection = FVector::ForwardVector;
};
