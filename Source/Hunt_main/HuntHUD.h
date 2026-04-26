#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HuntHUD.generated.h"

UCLASS()
class HUNT_MAIN_API AHuntHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawBar(const FVector2D& Position, const FVector2D& Size, float Percent, const FLinearColor& FillColor, const FString& Label);
};
