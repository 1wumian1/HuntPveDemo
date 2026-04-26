#pragma once

#include "CoreMinimal.h"
#include "Hunt_mainPlayerController.h"
#include "HuntPlayerController.generated.h"

UCLASS()
class HUNT_MAIN_API AHuntPlayerController : public AHunt_mainPlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

protected:
	void StartGamePressed();
};
