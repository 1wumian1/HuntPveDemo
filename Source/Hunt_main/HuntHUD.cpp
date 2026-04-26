#include "HuntHUD.h"
#include "HuntGameMode.h"
#include "HuntPlayerCharacter.h"
#include "HuntWeaponBase.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void AHuntHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	AHuntGameMode* HuntGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AHuntGameMode>() : nullptr;
	AHuntPlayerCharacter* Player = Cast<AHuntPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	const FVector2D Screen(Canvas->SizeX, Canvas->SizeY);

	if (HuntGameMode && HuntGameMode->GetPhase() == EHuntGamePhase::MainMenu)
	{
		DrawText(TEXT("Hunt PVE Demo"), FLinearColor::White, Screen.X * 0.5f - 150.0f, Screen.Y * 0.28f, nullptr, 2.0f);
		DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f), Screen.X * 0.5f - 130.0f, Screen.Y * 0.5f - 35.0f, 260.0f, 70.0f);
		DrawText(TEXT("Start Game"), FLinearColor::Yellow, Screen.X * 0.5f - 82.0f, Screen.Y * 0.5f - 16.0f, nullptr, 1.55f);
		DrawText(TEXT("Press Enter or Left Mouse"), FLinearColor::Gray, Screen.X * 0.5f - 120.0f, Screen.Y * 0.5f + 55.0f, nullptr, 0.85f);
		return;
	}

	if (Player)
	{
		DrawBar(FVector2D(40.0f, Screen.Y - 90.0f), FVector2D(280.0f, 22.0f), Player->GetHealth() / Player->GetMaxHealth(), FLinearColor::Red, TEXT("HP"));

		if (AHuntWeaponBase* Weapon = Player->GetCurrentWeapon())
		{
			const FString WeaponLine = FString::Printf(TEXT("%s  %d/%d  Reserve:%d%s"), *Weapon->GetWeaponName().ToString(), Weapon->GetAmmoInMagazine(), Weapon->GetMagazineSize(), Weapon->GetReserveAmmo(), Weapon->IsReloading() ? TEXT(" Reloading") : TEXT(""));
			DrawText(WeaponLine, FLinearColor::White, 40.0f, Screen.Y - 55.0f, nullptr, 1.0f);
		}

		const FString ItemLine = FString::Printf(TEXT("Heal[H]: %d   Dynamite[G]: %d   Dark Sight[E]: %s"), Player->GetHealCharges(), Player->GetExplosiveCharges(), Player->IsDarkSightActive() ? TEXT("ON") : TEXT("OFF"));
		DrawText(ItemLine, FLinearColor::White, 40.0f, Screen.Y - 32.0f, nullptr, 0.85f);

		if (Player->GetDamageFlashAlpha() > 0.0f)
		{
			DrawRect(FLinearColor(1.0f, 0.0f, 0.0f, 0.22f * Player->GetDamageFlashAlpha()), 0.0f, 0.0f, Screen.X, Screen.Y);
		}

		if (Player->IsDarkSightActive())
		{
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), 0.0f, 0.0f, Screen.X, Screen.Y);
			DrawText(TEXT("DARK SIGHT"), FLinearColor(0.1f, 0.8f, 1.0f), Screen.X - 170.0f, Screen.Y - 65.0f, nullptr, 1.1f);
		}
	}

	if (HuntGameMode)
	{
		DrawText(HuntGameMode->GetPhaseText().ToString(), FLinearColor::Yellow, 40.0f, 40.0f, nullptr, 1.2f);

		const FString ClueLine = FString::Printf(TEXT("Clues: %d/%d"), HuntGameMode->GetCollectedClues(), HuntGameMode->GetRequiredClues());
		DrawText(ClueLine, FLinearColor::White, 40.0f, 70.0f, nullptr, 1.0f);

		if (HuntGameMode->GetPhase() == EHuntGamePhase::Banish)
		{
			const FString BanishLine = FString::Printf(TEXT("Banish: %.0fs"), HuntGameMode->GetBanishRemaining());
			DrawText(BanishLine, FLinearColor(0.5f, 0.8f, 1.0f, 1.0f), 40.0f, 100.0f, nullptr, 1.0f);
		}
		else if (HuntGameMode->GetPhase() == EHuntGamePhase::Victory)
		{
			DrawText(TEXT("EXTRACTION SUCCESS"), FLinearColor::Green, Screen.X * 0.5f - 160.0f, Screen.Y * 0.45f, nullptr, 1.8f);
		}
		else if (HuntGameMode->GetPhase() == EHuntGamePhase::Defeat)
		{
			DrawText(TEXT("YOU DIED"), FLinearColor::Red, Screen.X * 0.5f - 85.0f, Screen.Y * 0.45f, nullptr, 1.8f);
		}
	}
}

void AHuntHUD::DrawBar(const FVector2D& Position, const FVector2D& Size, float Percent, const FLinearColor& FillColor, const FString& Label)
{
	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
	DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.85f), Position.X, Position.Y, Size.X, Size.Y);
	DrawRect(FillColor, Position.X + 2.0f, Position.Y + 2.0f, (Size.X - 4.0f) * ClampedPercent, Size.Y - 4.0f);
	DrawText(Label, FLinearColor::White, Position.X + 6.0f, Position.Y - 1.0f, nullptr, 0.85f);
}
