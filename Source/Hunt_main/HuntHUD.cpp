#include "HuntHUD.h"
#include "HuntBossEnemy.h"
#include "HuntClueActor.h"
#include "HuntExtractionPoint.h"
#include "HuntGameMode.h"
#include "HuntPlayerCharacter.h"
#include "HuntWeaponBase.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AHuntHUD::AHuntHUD()
{
	// Engine default fonts only cover Latin script. Override these in a Blueprint subclass
	// (e.g. BP_HuntHUD) and point them at an imported CJK-capable font (Microsoft YaHei,
	// Noto Sans CJK, SimHei, etc.) if you want the Chinese text in the main menu to render.
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (RobotoFont.Succeeded())
	{
		TitleFont = RobotoFont.Object;
		BodyFont = RobotoFont.Object;
		SmallFont = RobotoFont.Object;
	}
}

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
		DrawMainMenu(Screen);
		return;
	}

	if (Player)
	{
		// Crosshair: only visible while the player is actually aiming (right mouse held).
		if (Player->IsAimingShotgun())
		{
			const FVector2D Centre(Screen.X * 0.5f, Screen.Y * 0.5f);
			const FLinearColor CrosshairColor(1.0f, 0.95f, 0.4f, 1.0f);
			const float DotSize = 3.0f;
			DrawRect(CrosshairColor, Centre.X - DotSize * 0.5f, Centre.Y - DotSize * 0.5f, DotSize, DotSize);

			const float TickInner = 4.0f;
			const float TickLen = 4.0f;
			DrawLine(Centre.X - TickInner - TickLen, Centre.Y, Centre.X - TickInner, Centre.Y, CrosshairColor);
			DrawLine(Centre.X + TickInner, Centre.Y, Centre.X + TickInner + TickLen, Centre.Y, CrosshairColor);
			DrawLine(Centre.X, Centre.Y - TickInner - TickLen, Centre.X, Centre.Y - TickInner, CrosshairColor);
			DrawLine(Centre.X, Centre.Y + TickInner, Centre.X, Centre.Y + TickInner + TickLen, CrosshairColor);
		}

		DrawBar(FVector2D(40.0f, Screen.Y - 90.0f), FVector2D(280.0f, 22.0f), Player->GetHealth() / Player->GetMaxHealth(), FLinearColor::Red, TEXT("HP"));

		if (AHuntWeaponBase* Weapon = Player->GetCurrentWeapon())
		{
			const FString WeaponLine = Weapon->UsesAmmo()
				? FString::Printf(TEXT("%s  %d/%d  Reserve:%d%s"), *Weapon->GetWeaponName().ToString(), Weapon->GetAmmoInMagazine(), Weapon->GetMagazineSize(), Weapon->GetReserveAmmo(), Weapon->IsReloading() ? TEXT(" Reloading") : TEXT(""))
				: FString::Printf(TEXT("%s  Melee"), *Weapon->GetWeaponName().ToString());
			DrawText(WeaponLine, FLinearColor::White, 40.0f, Screen.Y - 55.0f, nullptr, 1.0f);
		}

		const TCHAR* MoveState = Player->IsSprinting() ? TEXT("Sprint") : (Player->IsHuntCrouched() ? TEXT("Crouch") : TEXT("Walk"));
		const bool bHealAvailable = Player->GetHealth() < Player->GetMaxHealth();
		const FString ItemLine = FString::Printf(TEXT("Heal[H]: %s   Dark Sight[E]: %s   Move: %s"),
			bHealAvailable ? TEXT("Ready") : TEXT("Full"),
			Player->IsDarkSightActive() ? TEXT("ON") : TEXT("OFF"),
			MoveState);
		DrawText(ItemLine, FLinearColor::White, 40.0f, Screen.Y - 32.0f, nullptr, 0.85f);

		if (Player->GetDamageFlashAlpha() > 0.0f)
		{
			DrawRect(FLinearColor(1.0f, 0.0f, 0.0f, 0.22f * Player->GetDamageFlashAlpha()), 0.0f, 0.0f, Screen.X, Screen.Y);
		}

		if (Player->IsDarkSightActive())
		{
			DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.18f), 0.0f, 0.0f, Screen.X, Screen.Y);
			DrawText(TEXT("DARK SIGHT"), FLinearColor(0.1f, 0.8f, 1.0f), Screen.X - 170.0f, Screen.Y - 65.0f, nullptr, 1.1f);
			DrawText(TEXT("Clues=blue  Boss=red  Extraction=green"), FLinearColor(0.4f, 0.85f, 1.0f), Screen.X - 360.0f, Screen.Y - 40.0f, nullptr, 0.8f);
			DrawDarkSightClues(Player, Screen);
			DrawDarkSightBoss(Player, Screen, HuntGameMode);
			DrawDarkSightExtraction(Player, Screen, HuntGameMode);
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
		else if (HuntGameMode->GetPhase() == EHuntGamePhase::Extract)
		{
			DrawText(TEXT("Banish complete - hold E to find extraction"), FLinearColor(0.4f, 1.0f, 0.6f, 1.0f), 40.0f, 100.0f, nullptr, 1.0f);
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

void AHuntHUD::DrawMainMenu(const FVector2D& Screen)
{
	// Vignette: dark semi-transparent backdrop on top of the rendered world.
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f), 0.0f, 0.0f, Screen.X, Screen.Y);

	// Top accent strip.
	const float StripHeight = 6.0f;
	DrawRect(FLinearColor(0.78f, 0.18f, 0.12f, 1.0f), 0.0f, Screen.Y * 0.18f - StripHeight, Screen.X, StripHeight);

	// ===== Title block =====
	const float TitleY = Screen.Y * 0.10f;
	DrawCenteredText(TEXT("HUNT  PVE  DEMO"), Screen.X * 0.5f, TitleY, FLinearColor(1.0f, 0.92f, 0.55f, 1.0f), 3.4f, TitleFont);
	DrawCenteredText(TEXT("亨特 PVE 演示  -  暗夜赏金猎人"), Screen.X * 0.5f, TitleY + 60.0f, FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), 1.2f, BodyFont);

	// ===== Two-column layout =====
	const float ContentTop = Screen.Y * 0.27f;
	const float PanelWidth = FMath::Min(440.0f, (Screen.X - 240.0f) * 0.5f);
	const float PanelHeight = Screen.Y * 0.52f;
	const float Gap = 60.0f;
	const float TotalWidth = PanelWidth * 2.0f + Gap;
	const float LeftX = (Screen.X - TotalWidth) * 0.5f;
	const float RightX = LeftX + PanelWidth + Gap;

	auto DrawPanel = [this](float X, float Y, float W, float H, const FLinearColor& AccentColor)
	{
		DrawRect(FLinearColor(0.05f, 0.05f, 0.06f, 0.85f), X, Y, W, H);
		DrawRect(AccentColor, X, Y, W, 4.0f);
		DrawRect(AccentColor, X, Y + H - 4.0f, W, 4.0f);
		DrawLine(X, Y, X, Y + H, AccentColor);
		DrawLine(X + W, Y, X + W, Y + H, AccentColor);
	};

	// Gameplay panel (left).
	DrawPanel(LeftX, ContentTop, PanelWidth, PanelHeight, FLinearColor(0.78f, 0.18f, 0.12f, 0.9f));
	DrawTextBlock(TEXT("GAMEPLAY  /  玩法目标"), LeftX + 24.0f, ContentTop + 18.0f, FLinearColor(1.0f, 0.85f, 0.4f, 1.0f), 1.25f, TitleFont);

	const FLinearColor GameplayColor(0.92f, 0.92f, 0.92f, 1.0f);
	const TCHAR* GameplayLines[] =
	{
		TEXT("1.  探索地图，寻找 3 个发光线索"),
		TEXT("    Explore and locate 3 glowing clues"),
		TEXT(""),
		TEXT("2.  收集线索后会刷出小怪，并解锁 BOSS"),
		TEXT("    Clues unlock the boss; expect ambushes"),
		TEXT(""),
		TEXT("3.  击杀 BOSS 后开始放逐 (5s)"),
		TEXT("    Kill the boss to start the 5s banish"),
		TEXT(""),
		TEXT("4.  放逐期间持续刷怪，固守阵地"),
		TEXT("    Defend during the banish phase"),
		TEXT(""),
		TEXT("5.  放逐完成后按住 E 暗视，绿色光柱"),
		TEXT("    标出撤离点 - 抵达即胜利"),
		TEXT("    Use Dark Sight (E) to find extraction"),
	};

	float LineY = ContentTop + 60.0f;
	for (const TCHAR* Line : GameplayLines)
	{
		DrawTextBlock(Line, LeftX + 28.0f, LineY, GameplayColor, 0.95f, BodyFont);
		LineY += 22.0f;
	}

	// Controls panel (right).
	DrawPanel(RightX, ContentTop, PanelWidth, PanelHeight, FLinearColor(0.18f, 0.62f, 0.92f, 0.9f));
	DrawTextBlock(TEXT("CONTROLS  /  按键操作"), RightX + 24.0f, ContentTop + 18.0f, FLinearColor(0.5f, 0.85f, 1.0f, 1.0f), 1.25f, TitleFont);

	struct FKeyHint
	{
		const TCHAR* Key;
		const TCHAR* Desc;
	};

	const FKeyHint Hints[] =
	{
		{TEXT("WASD"),       TEXT("Move  /  移动")},
		{TEXT("Mouse"),      TEXT("Look  /  视角")},
		{TEXT("L-Shift"),    TEXT("Sprint  /  奔跑")},
		{TEXT("L-Ctrl"),     TEXT("Crouch  /  蹲下")},
		{TEXT("Space"),      TEXT("Jump  /  跳跃")},
		{TEXT("LMB"),        TEXT("Fire  /  开火")},
		{TEXT("RMB"),        TEXT("Aim  /  瞄准")},
		{TEXT("R"),          TEXT("Reload  /  装弹")},
		{TEXT("F"),          TEXT("Interact  /  互动")},
		{TEXT("E (hold)"),   TEXT("Dark Sight  /  暗视")},
		{TEXT("H"),          TEXT("Heal  /  治疗 (回满)")},
	};

	LineY = ContentTop + 60.0f;
	for (const FKeyHint& Hint : Hints)
	{
		const float KeyBoxW = 92.0f;
		const float KeyBoxH = 22.0f;
		DrawRect(FLinearColor(0.13f, 0.13f, 0.14f, 0.95f), RightX + 24.0f, LineY, KeyBoxW, KeyBoxH);
		DrawLine(RightX + 24.0f, LineY, RightX + 24.0f + KeyBoxW, LineY, FLinearColor(0.5f, 0.85f, 1.0f, 1.0f));
		DrawLine(RightX + 24.0f, LineY + KeyBoxH, RightX + 24.0f + KeyBoxW, LineY + KeyBoxH, FLinearColor(0.5f, 0.85f, 1.0f, 1.0f));
		DrawTextBlock(Hint.Key, RightX + 30.0f, LineY + 2.0f, FLinearColor(0.6f, 0.95f, 1.0f, 1.0f), 0.95f, SmallFont);
		DrawTextBlock(Hint.Desc, RightX + 24.0f + KeyBoxW + 14.0f, LineY + 2.0f, FLinearColor(0.92f, 0.92f, 0.92f, 1.0f), 0.95f, BodyFont);
		LineY += KeyBoxH + 6.0f;
	}

	// ===== Start button =====
	const float ButtonW = 320.0f;
	const float ButtonH = 64.0f;
	const float ButtonX = (Screen.X - ButtonW) * 0.5f;
	const float ButtonY = Screen.Y * 0.86f;

	const float Pulse = 0.45f + 0.45f * FMath::Sin(GetWorld() ? GetWorld()->GetTimeSeconds() * 3.0f : 0.0f);
	const FLinearColor ButtonFill(0.78f, 0.18f, 0.12f, 0.9f + Pulse * 0.1f);
	DrawRect(ButtonFill, ButtonX, ButtonY, ButtonW, ButtonH);
	DrawRect(FLinearColor(1.0f, 0.95f, 0.55f, Pulse), ButtonX, ButtonY, ButtonW, 3.0f);
	DrawRect(FLinearColor(1.0f, 0.95f, 0.55f, Pulse), ButtonX, ButtonY + ButtonH - 3.0f, ButtonW, 3.0f);

	DrawCenteredText(TEXT("START GAME  /  开始游戏"), Screen.X * 0.5f, ButtonY + 16.0f, FLinearColor(1.0f, 0.95f, 0.55f, 1.0f), 1.45f, TitleFont);
	DrawCenteredText(TEXT("Press Enter or Left Mouse  /  按 回车 或 鼠标左键"), Screen.X * 0.5f, ButtonY + ButtonH + 14.0f, FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), 0.95f, SmallFont);

	// Bottom credit / version line.
	DrawCenteredText(TEXT("Hunt: Showdown PVE prototype  |  All assets are placeholders"), Screen.X * 0.5f, Screen.Y - 28.0f, FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), 0.8f, SmallFont);
}

void AHuntHUD::DrawTextBlock(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, UFont* FontOverride)
{
	DrawText(Text, Color, X, Y, FontOverride, Scale);
}

void AHuntHUD::DrawCenteredText(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* FontOverride)
{
	UFont* FontToUse = FontOverride ? FontOverride : GEngine->GetSmallFont();
	float TextW = 0.0f;
	if (FontToUse)
	{
		// UFont returns dimensions as int32, convert to float for centering math.
		int32 IntHeight = 0;
		int32 IntWidth = 0;
		FontToUse->GetStringHeightAndWidth(Text, IntHeight, IntWidth);
		TextW = static_cast<float>(IntWidth) * Scale;
	}
	else
	{
		TextW = Text.Len() * 8.0f * Scale;
	}
	DrawText(Text, Color, CenterX - TextW * 0.5f, Y, FontOverride, Scale);
}

void AHuntHUD::DrawBar(const FVector2D& Position, const FVector2D& Size, float Percent, const FLinearColor& FillColor, const FString& Label)
{
	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
	DrawRect(FLinearColor(0.03f, 0.03f, 0.03f, 0.85f), Position.X, Position.Y, Size.X, Size.Y);
	DrawRect(FillColor, Position.X + 2.0f, Position.Y + 2.0f, (Size.X - 4.0f) * ClampedPercent, Size.Y - 4.0f);
	DrawText(Label, FLinearColor::White, Position.X + 6.0f, Position.Y - 1.0f, nullptr, 0.85f);
}

void AHuntHUD::DrawDarkSightClues(AHuntPlayerCharacter* Player, const FVector2D& Screen)
{
	if (!Player || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!PlayerController)
	{
		return;
	}

	int32 VisibleClueCount = 0;
	const float RevealRange = Player->GetDarkSightRevealRange();
	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		AHuntClueActor* Clue = *It;
		if (!Clue || Clue->IsCollected())
		{
			continue;
		}

		const float Distance = FVector::Dist(Player->GetActorLocation(), Clue->GetActorLocation());
		if (Distance > RevealRange)
		{
			continue;
		}

		FVector2D ScreenLocation;
		const bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(Clue->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f), ScreenLocation, true);
		if (!bOnScreen)
		{
			continue;
		}

		++VisibleClueCount;
		const float Alpha = FMath::Clamp(1.0f - Distance / RevealRange, 0.35f, 1.0f);
		const FLinearColor ClueColor(0.0f, 0.45f, 1.0f, Alpha);
		const float MarkerSize = FMath::Lerp(44.0f, 72.0f, Alpha);

		DrawScreenBox(ScreenLocation, MarkerSize, ClueColor);
		DrawLine(Screen.X * 0.5f, Screen.Y * 0.5f, ScreenLocation.X, ScreenLocation.Y, FLinearColor(0.0f, 0.35f, 1.0f, 0.22f));

		const FString DistanceText = FString::Printf(TEXT("CLUE %.0fm"), Distance / 100.0f);
		DrawText(DistanceText, FLinearColor(0.2f, 0.85f, 1.0f, 1.0f), ScreenLocation.X - 34.0f, ScreenLocation.Y + MarkerSize * 0.5f + 4.0f, nullptr, 0.85f);
	}

	const FString StatusText = VisibleClueCount > 0
		? FString::Printf(TEXT("Dark Sight: %d clue signal(s)"), VisibleClueCount)
		: TEXT("Dark Sight: no clue signal nearby");
	DrawText(StatusText, FLinearColor(0.2f, 0.85f, 1.0f, 1.0f), Screen.X * 0.5f - 120.0f, Screen.Y * 0.72f, nullptr, 1.0f);
}

void AHuntHUD::DrawDarkSightBoss(AHuntPlayerCharacter* Player, const FVector2D& Screen, AHuntGameMode* HuntGameMode)
{
	// 只有当 Boss 已经被生成（线索集齐之后），且还没死时才高亮。
	// 死亡进入 Banish 阶段后，Boss 已经"消失"，玩家不再需要追踪它。
	if (!Player || !HuntGameMode || !GetWorld())
	{
		return;
	}

	const EHuntGamePhase Phase = HuntGameMode->GetPhase();
	if (Phase != EHuntGamePhase::BossUnlocked && Phase != EHuntGamePhase::FightBoss)
	{
		return;
	}

	AHuntBossEnemy* Boss = HuntGameMode->GetBoss();
	if (!Boss || Boss->IsDead())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!PlayerController)
	{
		return;
	}

	// 红色：饱和度高一点，与蓝色（线索）和绿色（撤离）形成清晰对比
	const FLinearColor BossColor(1.0f, 0.18f, 0.18f, 1.0f);

	FVector2D ScreenLocation;
	const bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(Boss->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f), ScreenLocation, true);
	const float Distance = FVector::Dist(Player->GetActorLocation(), Boss->GetActorLocation());

	if (bOnScreen)
	{
		const float MarkerSize = 110.0f;
		DrawScreenBox(ScreenLocation, MarkerSize, BossColor);

		// 红色脉动光圈，让 Boss 标记从远处也能一眼看到
		const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f);
		DrawScreenBox(ScreenLocation, MarkerSize * 0.55f, FLinearColor(1.0f, 0.35f, 0.35f, 0.4f + 0.45f * Pulse));

		// 从屏幕中心拉一条红线指向 Boss，跟撤离点保持一致的视觉语言
		DrawLine(Screen.X * 0.5f, Screen.Y * 0.5f, ScreenLocation.X, ScreenLocation.Y, FLinearColor(1.0f, 0.25f, 0.25f, 0.35f));

		const FString DistanceText = FString::Printf(TEXT("BOSS  %.0fm"), Distance / 100.0f);
		DrawText(DistanceText, BossColor, ScreenLocation.X - 36.0f, ScreenLocation.Y + MarkerSize * 0.5f + 6.0f, nullptr, 1.0f);

		DrawText(TEXT("Boss signal locked - approach to engage"), BossColor, Screen.X * 0.5f - 200.0f, Screen.Y * 0.68f, nullptr, 1.0f);
	}
	else
	{
		// Boss 不在视野里时，提示玩家转向
		const FString OffscreenText = FString::Printf(TEXT("Boss out of view - %.0fm away, look around"), Distance / 100.0f);
		DrawText(OffscreenText, BossColor, Screen.X * 0.5f - 220.0f, Screen.Y * 0.68f, nullptr, 1.0f);
	}
}

void AHuntHUD::DrawDarkSightExtraction(AHuntPlayerCharacter* Player, const FVector2D& Screen, AHuntGameMode* HuntGameMode)
{
	// Only reveal the extraction once the boss has been banished and we are in the extract phase.
	if (!Player || !HuntGameMode || HuntGameMode->GetPhase() != EHuntGamePhase::Extract || !GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
	if (!PlayerController)
	{
		return;
	}

	const FLinearColor ExtractColor(0.25f, 1.0f, 0.45f, 1.0f);
	int32 VisibleCount = 0;

	// Extraction is revealed at full alpha across the entire map (no distance falloff) - the
	// banish has cleared the path so the player should always know where to run.
	for (TActorIterator<AHuntExtractionPoint> It(GetWorld()); It; ++It)
	{
		AHuntExtractionPoint* Extract = *It;
		if (!Extract)
		{
			continue;
		}

		FVector2D ScreenLocation;
		const bool bOnScreen = PlayerController->ProjectWorldLocationToScreen(Extract->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f), ScreenLocation, true);
		if (!bOnScreen)
		{
			continue;
		}

		++VisibleCount;
		const float Distance = FVector::Dist(Player->GetActorLocation(), Extract->GetActorLocation());
		const float MarkerSize = 96.0f;

		DrawScreenBox(ScreenLocation, MarkerSize, ExtractColor);
		// Pulsing inner ring so the extraction marker stands out from the clue boxes.
		const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f);
		DrawScreenBox(ScreenLocation, MarkerSize * 0.55f, FLinearColor(0.4f, 1.0f, 0.6f, 0.4f + 0.4f * Pulse));
		DrawLine(Screen.X * 0.5f, Screen.Y * 0.5f, ScreenLocation.X, ScreenLocation.Y, FLinearColor(0.25f, 1.0f, 0.45f, 0.35f));

		const FString DistanceText = FString::Printf(TEXT("EXTRACTION  %.0fm"), Distance / 100.0f);
		DrawText(DistanceText, ExtractColor, ScreenLocation.X - 60.0f, ScreenLocation.Y + MarkerSize * 0.5f + 6.0f, nullptr, 1.0f);
	}

	if (VisibleCount > 0)
	{
		DrawText(TEXT("Extraction beacon located - reach it to escape"), ExtractColor, Screen.X * 0.5f - 200.0f, Screen.Y * 0.76f, nullptr, 1.05f);
	}
	else
	{
		DrawText(TEXT("Extraction beacon active - turn around to find it"), ExtractColor, Screen.X * 0.5f - 220.0f, Screen.Y * 0.76f, nullptr, 1.0f);
	}
}

void AHuntHUD::DrawScreenBox(const FVector2D& Center, float Size, const FLinearColor& Color)
{
	const float HalfSize = Size * 0.5f;
	const float Left = Center.X - HalfSize;
	const float Right = Center.X + HalfSize;
	const float Top = Center.Y - HalfSize;
	const float Bottom = Center.Y + HalfSize;

	DrawLine(Left, Top, Right, Top, Color);
	DrawLine(Right, Top, Right, Bottom, Color);
	DrawLine(Right, Bottom, Left, Bottom, Color);
	DrawLine(Left, Bottom, Left, Top, Color);
	DrawLine(Center.X - 10.0f, Center.Y, Center.X + 10.0f, Center.Y, Color);
	DrawLine(Center.X, Center.Y - 10.0f, Center.X, Center.Y + 10.0f, Color);
}
