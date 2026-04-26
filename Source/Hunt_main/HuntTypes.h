#pragma once

#include "CoreMinimal.h"
#include "HuntTypes.generated.h"

UENUM(BlueprintType)
enum class EHuntGamePhase : uint8
{
	MainMenu UMETA(DisplayName = "Main Menu"),
	FindClues UMETA(DisplayName = "Find Clues"),
	BossUnlocked UMETA(DisplayName = "Boss Unlocked"),
	FightBoss UMETA(DisplayName = "Fight Boss"),
	Banish UMETA(DisplayName = "Banish"),
	Extract UMETA(DisplayName = "Extract"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

UENUM(BlueprintType)
enum class EHuntWeaponKind : uint8
{
	Pistol UMETA(DisplayName = "Pistol"),
	Rifle UMETA(DisplayName = "Rifle")
};
