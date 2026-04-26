#include "HuntGameMode.h"
#include "HuntBossEnemy.h"
#include "HuntClueActor.h"
#include "HuntEnemyBase.h"
#include "HuntExtractionPoint.h"
#include "HuntHUD.h"
#include "HuntPlayerCharacter.h"
#include "HuntPlayerController.h"
#include "HuntSpawner.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

AHuntGameMode::AHuntGameMode()
{
	DefaultPawnClass = AHuntPlayerCharacter::StaticClass();
	PlayerControllerClass = AHuntPlayerController::StaticClass();
	HUDClass = AHuntHUD::StaticClass();
}

void AHuntGameMode::StartPlay()
{
	Super::StartPlay();

	CacheLevelActors();

	if (bCreatePrototypeActorsIfMissing)
	{
		EnsurePrototypeActors();
	}

	SetPhase(EHuntGamePhase::MainMenu);

	if (AHuntPlayerCharacter* Player = Cast<AHuntPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->SetActorLocation(FVector(0.0f, 0.0f, 160.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(FRotator::ZeroRotator);
		if (AController* PlayerController = Player->GetController())
		{
			PlayerController->SetControlRotation(FRotator::ZeroRotator);
		}
		Player->SetInputEnabledByGame(false);
	}
}

void AHuntGameMode::StartRun()
{
	if (Phase != EHuntGamePhase::MainMenu)
	{
		return;
	}

	CollectedClues = 0;
	BanishRemaining = 0.0f;
	SetPhase(EHuntGamePhase::FindClues);

	if (AHuntPlayerCharacter* Player = Cast<AHuntPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->SetActorLocation(FVector(0.0f, 0.0f, 160.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Player->SetActorRotation(FRotator::ZeroRotator);
		if (AController* PlayerController = Player->GetController())
		{
			PlayerController->SetControlRotation(FRotator::ZeroRotator);
		}
		Player->SetInputEnabledByGame(true);
	}
}

void AHuntGameMode::RegisterClueCollected(AHuntClueActor* Clue)
{
	if (!Clue || Phase != EHuntGamePhase::FindClues)
	{
		return;
	}

	++CollectedClues;
	if (CollectedClues == 1)
	{
		SpawnTravelEnemies();
	}

	if (CollectedClues >= RequiredClues)
	{
		UnlockBoss();
	}
}

void AHuntGameMode::BossKilled(AHuntBossEnemy* KilledBoss)
{
	if (Phase == EHuntGamePhase::FightBoss || Phase == EHuntGamePhase::BossUnlocked)
	{
		BeginBanish();
	}
}

void AHuntGameMode::PlayerReachedExtraction(AHuntPlayerCharacter* Player)
{
	if (CanExtract() && Player && !Player->IsDead())
	{
		SetPhase(EHuntGamePhase::Victory);
		Player->SetInputEnabledByGame(false);
		GetWorldTimerManager().ClearTimer(BanishSpawnTimer);
	}
}

void AHuntGameMode::PlayerDied(AHuntPlayerCharacter* Player)
{
	SetPhase(EHuntGamePhase::Defeat);
	GetWorldTimerManager().ClearTimer(BanishTimer);
	GetWorldTimerManager().ClearTimer(BanishSpawnTimer);
}

FText AHuntGameMode::GetPhaseText() const
{
	switch (Phase)
	{
	case EHuntGamePhase::MainMenu:
		return FText::FromString(TEXT("Start Game"));
	case EHuntGamePhase::FindClues:
		return FText::FromString(TEXT("Find three glowing clues"));
	case EHuntGamePhase::BossUnlocked:
		return FText::FromString(TEXT("Boss lair unlocked"));
	case EHuntGamePhase::FightBoss:
		return FText::FromString(TEXT("Kill the boss"));
	case EHuntGamePhase::Banish:
		return FText::FromString(TEXT("Banish the boss"));
	case EHuntGamePhase::Extract:
		return FText::FromString(TEXT("Go to extraction"));
	case EHuntGamePhase::Victory:
		return FText::FromString(TEXT("Victory"));
	case EHuntGamePhase::Defeat:
		return FText::FromString(TEXT("Defeat"));
	default:
		return FText::GetEmpty();
	}
}

void AHuntGameMode::SetPhase(EHuntGamePhase NewPhase)
{
	Phase = NewPhase;

	if (ExtractionPoint)
	{
		ExtractionPoint->SetActiveExtraction(Phase == EHuntGamePhase::Extract);
	}
}

void AHuntGameMode::UnlockBoss()
{
	SetPhase(EHuntGamePhase::BossUnlocked);

	if (!Boss)
	{
		const FVector BossLocation(1700.0f, 0.0f, 120.0f);
		Boss = GetWorld()->SpawnActor<AHuntBossEnemy>(AHuntBossEnemy::StaticClass(), BossLocation, FRotator::ZeroRotator);
	}

	SetPhase(EHuntGamePhase::FightBoss);
}

void AHuntGameMode::BeginBanish()
{
	BanishRemaining = BanishDuration;
	SetPhase(EHuntGamePhase::Banish);

	GetWorldTimerManager().SetTimer(BanishTimer, this, &AHuntGameMode::TickBanish, 1.0f, true);
	GetWorldTimerManager().SetTimer(BanishSpawnTimer, this, &AHuntGameMode::SpawnTravelEnemies, BanishSpawnInterval, true);
}

void AHuntGameMode::TickBanish()
{
	BanishRemaining = FMath::Max(0.0f, BanishRemaining - 1.0f);
	if (BanishRemaining <= 0.0f)
	{
		FinishBanish();
	}
}

void AHuntGameMode::FinishBanish()
{
	GetWorldTimerManager().ClearTimer(BanishTimer);
	GetWorldTimerManager().ClearTimer(BanishSpawnTimer);
	SetPhase(EHuntGamePhase::Extract);
}

void AHuntGameMode::EnsurePrototypeActors()
{
	SpawnPrototypeWorld();

	if (Clues.Num() == 0)
	{
		const FVector ClueLocations[] =
		{
			FVector(550.0f, -450.0f, 90.0f),
			FVector(980.0f, 520.0f, 90.0f),
			FVector(-650.0f, 720.0f, 90.0f)
		};

		for (const FVector& Location : ClueLocations)
		{
			if (AHuntClueActor* Clue = GetWorld()->SpawnActor<AHuntClueActor>(AHuntClueActor::StaticClass(), Location, FRotator::ZeroRotator))
			{
				Clues.Add(Clue);
			}
		}
	}

	if (!ExtractionPoint)
	{
		ExtractionPoint = GetWorld()->SpawnActor<AHuntExtractionPoint>(AHuntExtractionPoint::StaticClass(), FVector(-1500.0f, -650.0f, 110.0f), FRotator::ZeroRotator);
	}

	if (Spawners.Num() == 0)
	{
		const FVector SpawnerLocations[] =
		{
			FVector(1200.0f, -900.0f, 100.0f),
			FVector(-900.0f, 1200.0f, 100.0f)
		};

		for (const FVector& Location : SpawnerLocations)
		{
			if (AHuntSpawner* Spawner = GetWorld()->SpawnActor<AHuntSpawner>(AHuntSpawner::StaticClass(), Location, FRotator::ZeroRotator))
			{
				Spawners.Add(Spawner);
			}
		}
	}
}

void AHuntGameMode::SpawnPrototypeWorld()
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		return;
	}

	UMaterialInterface* GridMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	UMaterialInterface* WhiteMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (ADirectionalLight* SunLight = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 1200.0f), FRotator(-35.0f, 20.0f, 0.0f)))
	{
		SunLight->GetLightComponent()->SetIntensity(25.0f);
		SunLight->SetActorLabel(TEXT("PrototypeSunLight"));
	}

	if (ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
	{
		SkyLight->GetLightComponent()->SetIntensity(8.0f);
		SkyLight->SetActorLabel(TEXT("PrototypeSkyLight"));
	}

	if (APointLight* CenterLight = GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(), FVector(0.0f, 0.0f, 500.0f), FRotator::ZeroRotator))
	{
		CenterLight->GetLightComponent()->SetIntensity(50000.0f);
		CenterLight->PointLightComponent->SetAttenuationRadius(8000.0f);
		CenterLight->SetActorLabel(TEXT("PrototypeCenterLight"));
	}

	const auto SpawnMeshActor = [this, CubeMesh](const FVector& Location, const FVector& Scale, UMaterialInterface* Material, const TCHAR* Label)
	{
		AStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator::ZeroRotator);
		if (MeshActor)
		{
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			if (Material)
			{
				MeshActor->GetStaticMeshComponent()->SetMaterial(0, Material);
			}
			MeshActor->SetActorScale3D(Scale);
			MeshActor->SetActorLabel(Label);
		}
	};

	SpawnMeshActor(FVector(0.0f, 0.0f, -55.0f), FVector(80.0f, 80.0f, 1.0f), GridMaterial ? GridMaterial : WhiteMaterial, TEXT("PrototypeVisibleGround"));
	SpawnMeshActor(FVector(900.0f, 0.0f, 260.0f), FVector(0.8f, 14.0f, 5.0f), WhiteMaterial, TEXT("PrototypeWhiteWallInFront"));
	SpawnMeshActor(FVector(350.0f, -450.0f, 140.0f), FVector(1.0f, 1.0f, 3.0f), WhiteMaterial, TEXT("PrototypeLeftMarker"));
	SpawnMeshActor(FVector(350.0f, 450.0f, 140.0f), FVector(1.0f, 1.0f, 3.0f), WhiteMaterial, TEXT("PrototypeRightMarker"));
	SpawnMeshActor(FVector(700.0f, 600.0f, 120.0f), FVector(3.0f, 2.0f, 2.5f), WhiteMaterial, TEXT("PrototypeHouseA"));
	SpawnMeshActor(FVector(-550.0f, 850.0f, 120.0f), FVector(2.5f, 2.5f, 2.0f), WhiteMaterial, TEXT("PrototypeHouseB"));
	SpawnMeshActor(FVector(1100.0f, -350.0f, 120.0f), FVector(2.0f, 3.0f, 2.0f), WhiteMaterial, TEXT("PrototypeHouseC"));

	if (ATextRenderActor* GuideText = GetWorld()->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), FVector(760.0f, 0.0f, 420.0f), FRotator(0.0f, 180.0f, 0.0f)))
	{
		GuideText->GetTextRender()->SetText(FText::FromString(TEXT("DEBUG MAP\nWASD Move  |  F Collect Clues  |  Hold E Dark Sight")));
		GuideText->GetTextRender()->SetTextRenderColor(FColor::White);
		GuideText->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
		GuideText->GetTextRender()->SetWorldSize(70.0f);
		GuideText->SetActorLabel(TEXT("PrototypeGuideText"));
	}
}

void AHuntGameMode::SpawnTravelEnemies()
{
	if (Spawners.Num() == 0)
	{
		return;
	}

	const int32 SpawnCount = Phase == EHuntGamePhase::Banish ? 2 : TravelEnemyCount;
	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		if (AHuntSpawner* Spawner = Spawners[Index % Spawners.Num()])
		{
			Spawner->SpawnEnemy();
		}
	}
}

void AHuntGameMode::CacheLevelActors()
{
	Clues.Reset();
	Spawners.Reset();
	Boss = nullptr;
	ExtractionPoint = nullptr;

	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		Clues.Add(*It);
	}

	for (TActorIterator<AHuntSpawner> It(GetWorld()); It; ++It)
	{
		Spawners.Add(*It);
	}

	for (TActorIterator<AHuntBossEnemy> It(GetWorld()); It; ++It)
	{
		Boss = *It;
		break;
	}

	for (TActorIterator<AHuntExtractionPoint> It(GetWorld()); It; ++It)
	{
		ExtractionPoint = *It;
		break;
	}
}
