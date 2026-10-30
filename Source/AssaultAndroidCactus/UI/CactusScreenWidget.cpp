#include "UI/CactusScreenWidget.h"
#include "Game/CactusGameMode.h"
#include "Engine/World.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
void ApplyEnergyMaterial(UMaterialInstanceDynamic* Material, const FCactusEnergyAppearance& Style)
{
    if (!Material) return;
        Material->SetScalarParameterValue(TEXT("CoreFill"), Style.CoreFill);
        Material->SetVectorParameterValue(TEXT("EnergyColor"), Style.Color);
        Material->SetVectorParameterValue(TEXT("Accent"), Style.Accent);
        Material->SetScalarParameterValue(TEXT("Brightness"), Style.Brightness);
        Material->SetScalarParameterValue(TEXT("GlowStrength"), Style.GlowStrength);
        Material->SetScalarParameterValue(TEXT("RingSpeed"), Style.RingSpeed);
        Material->SetScalarParameterValue(TEXT("FlowSpeed"), Style.FlowSpeed);
        Material->SetScalarParameterValue(TEXT("ParticleSpeed"), Style.ParticleSpeed);
        Material->SetScalarParameterValue(TEXT("BubbleSpeed"), Style.BubbleSpeed);
        Material->SetScalarParameterValue(TEXT("RingCount"), Style.RingCount);
        Material->SetScalarParameterValue(TEXT("ParticleCount"), Style.ParticleCount);
        Material->SetScalarParameterValue(TEXT("BubbleCount"), Style.BubbleCount);
        Material->SetScalarParameterValue(TEXT("RingWidth"), Style.RingWidth);
}
}

UCactusScreenWidget::UCactusScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SelectionOrbStyle.CoreFill = 0;
}

void UCactusScreenWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyAppearance();
}

void UCactusScreenWidget::ApplyAppearance()
{
    CoreMaterial = EnergyCore ? EnergyCore->GetDynamicMaterial() : nullptr;
    OrbMaterial = SelectionOrb ? SelectionOrb->GetDynamicMaterial() : nullptr;
    BackgroundMaterial = Background ? Background->GetDynamicMaterial() : nullptr;
    ApplyEnergyMaterial(CoreMaterial, EnergyCoreStyle);
    ApplyEnergyMaterial(OrbMaterial, SelectionOrbStyle);
    if (UMaterialInstanceDynamic* Material = BackgroundMaterial)
    {
        Material->SetVectorParameterValue(TEXT("BaseColor"), BackgroundStyle.BaseColor);
        Material->SetVectorParameterValue(TEXT("AuraColor"), BackgroundStyle.AuraColor);
        Material->SetVectorParameterValue(TEXT("GridColor"), BackgroundStyle.GridColor);
        Material->SetVectorParameterValue(TEXT("SweepColor"), BackgroundStyle.SweepColor);
        Material->SetVectorParameterValue(TEXT("RingColor"), BackgroundStyle.RingColor);
        Material->SetVectorParameterValue(TEXT("GridDensity"), FLinearColor(BackgroundStyle.GridDensity.X, BackgroundStyle.GridDensity.Y, 0, 0));
        Material->SetVectorParameterValue(TEXT("AuraCenter"), FLinearColor(BackgroundStyle.AuraCenter.X, BackgroundStyle.AuraCenter.Y, 0, 0));
        Material->SetScalarParameterValue(TEXT("AuraFalloff"), BackgroundStyle.AuraFalloff);
        Material->SetScalarParameterValue(TEXT("SweepSpeed"), BackgroundStyle.SweepSpeed);
        Material->SetScalarParameterValue(TEXT("RingSpeed"), BackgroundStyle.RingSpeed);
        Material->SetScalarParameterValue(TEXT("Brightness"), BackgroundStyle.Brightness);
    }
    Buttons.Reset();
    ButtonMaterials.Reset();
    PressPulses.Reset();
    for (UButton* Button : { StartButton.Get(), RetryButton.Get(), TitleButton.Get(), QuitButton.Get() })
    {
        if (!Button) continue;
        Buttons.Add(Button);
        PressPulses.Add(0);
        UMaterialInstanceDynamic* Material = SelectionMaterial ? UMaterialInstanceDynamic::Create(SelectionMaterial, this) : nullptr;
        ButtonMaterials.Add(Material);
        if (Material)
        {
        Material->SetVectorParameterValue(TEXT("BaseColor"), ButtonStyle.BaseColor);
        Material->SetVectorParameterValue(TEXT("FocusFillColor"), ButtonStyle.FocusFillColor);
        Material->SetVectorParameterValue(TEXT("EnergyColor"), ButtonStyle.EnergyColor);
        Material->SetVectorParameterValue(TEXT("Accent"), ButtonStyle.Accent);
        Material->SetVectorParameterValue(TEXT("FlashColor"), ButtonStyle.FlashColor);
        Material->SetScalarParameterValue(TEXT("Brightness"), ButtonStyle.Brightness);
        Material->SetScalarParameterValue(TEXT("SweepSpeed"), ButtonStyle.SweepSpeed);
        Material->SetScalarParameterValue(TEXT("SweepStrength"), ButtonStyle.SweepStrength);
        Material->SetScalarParameterValue(TEXT("BorderStrength"), ButtonStyle.BorderStrength);
        Material->SetScalarParameterValue(TEXT("BorderWidth"), ButtonStyle.BorderWidth);
        Material->SetScalarParameterValue(TEXT("FlashStrength"), ButtonStyle.FlashStrength);
            Material->SetScalarParameterValue(TEXT("Selected"), IsDesignTime() && Buttons.Num() == 1 ? 1 : 0);
            FButtonStyle Style = Button->GetStyle();
            for (FSlateBrush* Brush : { &Style.Normal, &Style.Hovered, &Style.Pressed })
            {
                Brush->SetResourceObject(Material);
                Brush->DrawAs = ESlateBrushDrawType::Image;
                Brush->TintColor = FSlateColor(FLinearColor::White);
            }
            Button->SetStyle(Style);
        }
    }
}

void UCactusScreenWidget::NativeDestruct()
{
    if (IsDesignTime()) { Super::NativeDestruct(); return; }
    // Designerに保存したTransformを復元し、再表示時の拡大・移動の積み重なりを防ぐ。
    SetRenderTransform(ScreenBaseTransform);
    SetRenderOpacity(ScreenBaseOpacity);
    if (EnergyCore) EnergyCore->SetRenderTransform(CoreBaseTransform);
    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
        if (ButtonBaseTransforms.IsValidIndex(Index)) Buttons[Index]->SetRenderTransform(ButtonBaseTransforms[Index]);
    if (SelectionOrb)
        if (UCanvasPanelSlot* OrbSlot = Cast<UCanvasPanelSlot>(SelectionOrb->Slot)) OrbSlot->SetPosition(OrbBasePosition);
    Super::NativeDestruct();
}

void UCactusScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    ApplyAppearance();
    ScreenBaseTransform = GetRenderTransform();
    ScreenBaseOpacity = GetRenderOpacity();
    CoreBaseTransform = EnergyCore ? EnergyCore->GetRenderTransform() : FWidgetTransform();
    ButtonBaseTransforms.Reset();
    for (UButton* Button : Buttons) ButtonBaseTransforms.Add(Button->GetRenderTransform());
    if (SelectionOrb)
        if (UCanvasPanelSlot* OrbSlot = Cast<UCanvasPanelSlot>(SelectionOrb->Slot)) OrbBasePosition = OrbSlot->GetPosition();
    SelectedIndex = INDEX_NONE;
    AnimationTime = ExitTime = 0;
    PendingAction = ECactusMenuAction::None;
    bActionDispatched = false;
    SetRenderOpacity(0);
    for (UButton* Button : Buttons)
    {
        Button->OnHovered.AddUniqueDynamic(this, &UCactusScreenWidget::ButtonHovered);
        Button->OnPressed.AddUniqueDynamic(this, &UCactusScreenWidget::ButtonPressed);
    }
    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
    {
        Buttons[Index]->SetNavigationRuleExplicit(EUINavigation::Up, Buttons[(Index + Buttons.Num() - 1) % Buttons.Num()]);
        Buttons[Index]->SetNavigationRuleExplicit(EUINavigation::Down, Buttons[(Index + 1) % Buttons.Num()]);
        Buttons[Index]->SetNavigationRuleExplicit(EUINavigation::Previous, Buttons[(Index + Buttons.Num() - 1) % Buttons.Num()]);
        Buttons[Index]->SetNavigationRuleExplicit(EUINavigation::Next, Buttons[(Index + 1) % Buttons.Num()]);
    }
    if (StartButton) StartButton->OnClicked.AddUniqueDynamic(this, &UCactusScreenWidget::StartPressed);
    if (RetryButton) RetryButton->OnClicked.AddUniqueDynamic(this, &UCactusScreenWidget::StartPressed);
    if (TitleButton) TitleButton->OnClicked.AddUniqueDynamic(this, &UCactusScreenWidget::TitlePressed);
    if (QuitButton) QuitButton->OnClicked.AddUniqueDynamic(this, &UCactusScreenWidget::QuitPressed);
}

void UCactusScreenWidget::FocusInitialButton()
{
    if (!Buttons.IsEmpty()) Buttons[0]->SetUserFocus(GetOwningPlayer());
}

void UCactusScreenWidget::ButtonHovered()
{
    if (PendingAction != ECactusMenuAction::None) return;
    for (UButton* Button : Buttons)
        if (Button->IsHovered()) { Button->SetUserFocus(GetOwningPlayer()); break; }
}

void UCactusScreenWidget::ButtonPressed()
{
    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
        if (Buttons[Index]->IsPressed()) PressPulses[Index] = 1;
}

void UCactusScreenWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (IsDesignTime()) return;
    AnimationTime += DeltaSeconds;
    const float Intro = IntroDuration <= 0 ? 1.0f : FMath::Clamp(AnimationTime / IntroDuration, 0.0f, 1.0f);
    const float Ease = 1.0f - FMath::Pow(1.0f - Intro, 3.0f);
    if (PendingAction != ECactusMenuAction::None) ExitTime += DeltaSeconds;
    const float Exit = PendingAction == ECactusMenuAction::None ? 0.0f
        : ExitDuration <= 0 ? 1.0f : FMath::Clamp(ExitTime / ExitDuration, 0.0f, 1.0f);
    SetRenderOpacity(ScreenBaseOpacity * Ease * (1.0f - Exit));
    SetRenderTranslation(ScreenBaseTransform.Translation + IntroOffset * (1.0f - Ease) + ExitOffset * Exit);
    if (EnergyCore) EnergyCore->SetRenderScale(CoreBaseTransform.Scale * FMath::Lerp(CoreIntroScale, 1.0f, Ease));
    for (UMaterialInstanceDynamic* Material : { CoreMaterial.Get(), OrbMaterial.Get(), BackgroundMaterial.Get() })
        if (Material) Material->SetScalarParameterValue(TEXT("UISeconds"), AnimationTime);
    int32 Current = INDEX_NONE;
    for (int32 Index = 0; Index < Buttons.Num(); ++Index)
        if (Buttons[Index]->HasUserFocus(GetOwningPlayer())) { Current = Index; break; }
    if (Current != SelectedIndex && Current != INDEX_NONE)
    {
        if (SelectedIndex != INDEX_NONE && SelectSound) UGameplayStatics::PlaySound2D(this, SelectSound, FMath::Max(SelectVolume, 0.0f));
        SelectedIndex = Current;
    }
    for (int32 Index = 0; Index < ButtonMaterials.Num(); ++Index)
        if (UMaterialInstanceDynamic* Material = ButtonMaterials[Index])
        {
            float Value = 0;
            Material->GetScalarParameterValue(TEXT("Selected"), Value);
            Value = FMath::FInterpTo(Value, Index == SelectedIndex ? 1.0f : 0.0f, DeltaSeconds, FocusInterpSpeed);
            Material->SetScalarParameterValue(TEXT("Selected"), Value);
            PressPulses[Index] = FMath::Max(0.0f, PressPulses[Index] - DeltaSeconds / FMath::Max(PressPulseDuration, 0.01f));
            Material->SetScalarParameterValue(TEXT("PressPulse"), PressPulses[Index]);
            Material->SetScalarParameterValue(TEXT("UISeconds"), AnimationTime);
            Buttons[Index]->SetRenderScale(ButtonBaseTransforms[Index].Scale * FMath::Lerp(1.0f, FMath::Clamp(FocusScale, 1.0f, 1.5f), Value));
            Buttons[Index]->SetRenderTranslation(ButtonBaseTransforms[Index].Translation + FocusOffset * Value
                + (ButtonIntroOffset + ButtonIndexOffset * Index) * (1.0f - Ease));
        }
    if (bOrbFollowsSelection && SelectionOrb && Buttons.IsValidIndex(SelectedIndex))
        if (UCanvasPanelSlot* OrbSlot = Cast<UCanvasPanelSlot>(SelectionOrb->Slot))
            if (const UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Buttons[SelectedIndex]->Slot))
            {
                FVector2D Position = OrbSlot->GetPosition();
                const float Target = ButtonSlot->GetPosition().Y + (ButtonSlot->GetSize().Y - OrbSlot->GetSize().Y) * 0.5f + OrbFollowOffsetY;
                Position.Y = FMath::FInterpTo(Position.Y, Target, DeltaSeconds, OrbFollowSpeed);
                OrbSlot->SetPosition(Position);
            }
    // UMG自身のTickで進めるため、結果画面のWorld Pause中も決定演出が動く。
    if (PendingAction != ECactusMenuAction::None && !bActionDispatched && Exit >= 1) ExecuteAction();
}

void UCactusScreenWidget::SetFinalScore(int32 Score)
{
    if (FinalScoreText) FinalScoreText->SetText(FText::FromString(FString::Printf(TEXT("%0*d"), FMath::Clamp(ScoreDigits, 1, 10), Score)));
}

void UCactusScreenWidget::PlayConfirmSound()
{
    if (ConfirmSound) UGameplayStatics::PlaySound2D(this, ConfirmSound, FMath::Max(ConfirmVolume, 0.0f));
}

void UCactusScreenWidget::StartPressed()
{
    BeginAction(ECactusMenuAction::Start);
}

void UCactusScreenWidget::TitlePressed()
{
    BeginAction(ECactusMenuAction::Title);
}

void UCactusScreenWidget::QuitPressed()
{
    BeginAction(ECactusMenuAction::Quit);
}

void UCactusScreenWidget::BeginAction(ECactusMenuAction Action)
{
    if (PendingAction != ECactusMenuAction::None) return;
    PendingAction = Action;
    ExitTime = 0;
    if (PressPulses.IsValidIndex(SelectedIndex)) PressPulses[SelectedIndex] = 1;
    PlayConfirmSound();
}

void UCactusScreenWidget::ExecuteAction()
{
    const ECactusMenuAction Action = PendingAction;
    // 画面を外す処理はGameModeへ渡す。一度だけ実行して次のTickから再発行しない。
    bActionDispatched = true;
    if (Action == ECactusMenuAction::Quit)
        UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
    else if (ACactusGameMode* Mode = GetWorld()->GetAuthGameMode<ACactusGameMode>())
    {
        if (Action == ECactusMenuAction::Start) Mode->StartGame();
        if (Action == ECactusMenuAction::Title) Mode->ReturnToTitle();
    }
}

FReply UCactusScreenWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (TitleButton && (KeyEvent.GetKey() == EKeys::Escape || KeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        TitlePressed();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, KeyEvent);
}
