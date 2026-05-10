#include "HuntPlayerController.h"
#include "HuntGameMode.h"
#include "InputCoreTypes.h"

void AHuntPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	FInputActionBinding& StartGameBinding = InputComponent->BindAction(TEXT("StartGame"), IE_Pressed, this, &AHuntPlayerController::StartGamePressed);
	StartGameBinding.bConsumeInput = false;
}

void AHuntPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		StartGamePressed();
	}
}

void AHuntPlayerController::StartGamePressed()
{
	if (AHuntGameMode* HuntGameMode = GetWorld()->GetAuthGameMode<AHuntGameMode>())
	{
		HuntGameMode->StartRun();
	}
}
