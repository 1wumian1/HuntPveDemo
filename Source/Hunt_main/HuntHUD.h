#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HuntHUD.generated.h"

class AHuntPlayerCharacter;
class AHuntGameMode;
class UFont;

UCLASS()
class HUNT_MAIN_API AHuntHUD : public AHUD
{
	GENERATED_BODY()

public:
	AHuntHUD();

	virtual void DrawHUD() override;

protected:
	void DrawBar(const FVector2D& Position, const FVector2D& Size, float Percent, const FLinearColor& FillColor, const FString& Label);
	void DrawDarkSightClues(AHuntPlayerCharacter* Player, const FVector2D& Screen);
	void DrawDarkSightExtraction(AHuntPlayerCharacter* Player, const FVector2D& Screen, AHuntGameMode* HuntGameMode);
	void DrawDarkSightBoss(AHuntPlayerCharacter* Player, const FVector2D& Screen, AHuntGameMode* HuntGameMode);
	void DrawScreenBox(const FVector2D& Center, float Size, const FLinearColor& Color);
	void DrawMainMenu(const FVector2D& Screen);

	void DrawTextBlock(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, UFont* FontOverride = nullptr);
	void DrawCenteredText(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* FontOverride = nullptr);

	// Optional font overrides for the main menu / HUD text. Default fonts shipped with the
	// engine (Roboto) only support Latin glyphs - if you want Chinese characters to render
	// you can import Microsoft YaHei / Noto Sans CJK / SimHei into the project, then point
	// these properties at the imported UFont assets via a Blueprint subclass of HuntHUD.
	UPROPERTY(EditDefaultsOnly, Category = "Hunt|HUD|Fonts")
	TObjectPtr<UFont> TitleFont;

	UPROPERTY(EditDefaultsOnly, Category = "Hunt|HUD|Fonts")
	TObjectPtr<UFont> BodyFont;

	UPROPERTY(EditDefaultsOnly, Category = "Hunt|HUD|Fonts")
	TObjectPtr<UFont> SmallFont;
};
