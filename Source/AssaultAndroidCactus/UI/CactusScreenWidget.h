#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CactusScreenWidget.generated.h"

class UButton;
class UTextBlock;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USoundBase;
class UImage;

USTRUCT(BlueprintType)
struct FCactusEnergyAppearance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", ClampMax = "1", ToolTip = "0は中央透明、1は球全体を表示。"))
    float CoreFill = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Color = FLinearColor(0.02f, 0.65f, 1.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Accent = FLinearColor(1.0f, 0.48f, 0.18f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float Brightness = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float GlowStrength = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3", ToolTip = "0で停止。負数で逆回転。"))
    float RingSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float FlowSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float ParticleSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float BubbleSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", ClampMax = "8"))
    int32 RingCount = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", ClampMax = "32"))
    int32 ParticleCount = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", ClampMax = "20"))
    int32 BubbleCount = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.1", UIMax = "3"))
    float RingWidth = 1.0f;
};

USTRUCT(BlueprintType)
struct FCactusButtonAppearance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor BaseColor = FLinearColor(0.008f, 0.021f, 0.032f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor FocusFillColor = FLinearColor(0.012f, 0.065f, 0.085f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor EnergyColor = FLinearColor(0.02f, 0.75f, 1.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor Accent = FLinearColor(0.95f, 0.72f, 0.10f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor FlashColor = FLinearColor(0.4f, 0.8f, 1.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float Brightness = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float SweepSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float SweepStrength = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float BorderStrength = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.1", UIMax = "3"))
    float BorderWidth = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float FlashStrength = 1.0f;
};

USTRUCT(BlueprintType)
struct FCactusBackgroundAppearance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor BaseColor = FLinearColor(0.003f, 0.008f, 0.017f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor AuraColor = FLinearColor(0.002f, 0.032f, 0.050f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor GridColor = FLinearColor(0.003f, 0.010f, 0.014f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor SweepColor = FLinearColor(0, 0.008f, 0.013f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor RingColor = FLinearColor(0, 0.013f, 0.020f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1"))
    FVector2D GridDensity = FVector2D(64, 36);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FVector2D AuraCenter = FVector2D(0.24f, 0.53f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.1", UIMax = "20"))
    float AuraFalloff = 7.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float SweepSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (UIMin = "-3", UIMax = "3"))
    float RingSpeed = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", UIMax = "3"))
    float Brightness = 1.0f;
};
enum class ECactusMenuAction : uint8 { None, Start, Title, Quit };

// WBP_Title / WBP_Resultの共通操作。配置・色・文言はUMG Designerで編集する。
UCLASS(Abstract)
class ASSAULTANDROIDCACTUS_API UCactusScreenWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UCactusScreenWidget(const FObjectInitializer& ObjectInitializer);
    void FocusInitialButton();
    void SetFinalScore(int32 Score);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> StartButton;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> RetryButton;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> TitleButton;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> QuitButton;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> FinalScoreText;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> EnergyCore;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> SelectionOrb;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Background;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0", Units = "s"))
    float IntroDuration = 0.65f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "1.0", ClampMax = "1.5"))
    float FocusScale = 1.04f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0", Units = "s"))
    float ExitDuration = 0.18f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    FVector2D IntroOffset = FVector2D(0, 28);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    FVector2D ExitOffset = FVector2D(0, -12);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0", UIMax = "2"))
    float CoreIntroScale = 0.80f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    FVector2D FocusOffset = FVector2D(9, 0);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    FVector2D ButtonIntroOffset = FVector2D(0, 18);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    FVector2D ButtonIndexOffset = FVector2D(0, 8);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0", ToolTip = "0は即時切替。"))
    float FocusInterpSpeed = 14.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    bool bOrbFollowsSelection = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0", ToolTip = "0は即時移動。"))
    float OrbFollowSpeed = 15.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion")
    float OrbFollowOffsetY = 0.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Motion", meta = (ClampMin = "0.01", Units = "s"))
    float PressPulseDuration = 0.25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance")
    FCactusEnergyAppearance EnergyCoreStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance")
    FCactusEnergyAppearance SelectionOrbStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance")
    FCactusButtonAppearance ButtonStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance")
    FCactusBackgroundAppearance BackgroundStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Sound", meta = (ClampMin = "0", UIMax = "1"))
    float SelectVolume = 0.35f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Sound", meta = (ClampMin = "0", UIMax = "1"))
    float ConfirmVolume = 0.40f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance", meta = (ClampMin = "1", ClampMax = "10"))
    int32 ScoreDigits = 8;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Appearance")
    TObjectPtr<UMaterialInterface> SelectionMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Sound")
    TObjectPtr<USoundBase> SelectSound;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu|Sound")
    TObjectPtr<USoundBase> ConfirmSound;

private:
    UFUNCTION() void StartPressed();
    UFUNCTION() void TitlePressed();
    UFUNCTION() void QuitPressed();
    UFUNCTION() void ButtonHovered();
    UFUNCTION() void ButtonPressed();
    void PlayConfirmSound();
    void BeginAction(ECactusMenuAction Action);
    void ExecuteAction();
    void ApplyAppearance();

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> Buttons;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> ButtonMaterials;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> OrbMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BackgroundMaterial;
    FWidgetTransform ScreenBaseTransform;
    float ScreenBaseOpacity = 1;
    FWidgetTransform CoreBaseTransform;
    TArray<FWidgetTransform> ButtonBaseTransforms;
    FVector2D OrbBasePosition;
    TArray<float> PressPulses;
    float AnimationTime = 0;
    float ExitTime = 0;
    bool bActionDispatched = false;
    ECactusMenuAction PendingAction = ECactusMenuAction::None;
    int32 SelectedIndex = INDEX_NONE;
};
