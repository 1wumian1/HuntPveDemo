#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HuntTypes.h"
#include "HuntGameMode.generated.h"

class AHuntBossEnemy;
class AHuntClueActor;
class AHuntEnemyBase;
class AHuntExtractionPoint;
class AHuntPlayerCharacter;
class AHuntSpawner;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHuntGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Hunt|Flow")
	void StartRun();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Flow")
	void RegisterClueCollected(AHuntClueActor* Clue);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Flow")
	void BossKilled(AHuntBossEnemy* Boss);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Flow")
	void PlayerReachedExtraction(AHuntPlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Flow")
	void PlayerDied(AHuntPlayerCharacter* Player);

	EHuntGamePhase GetPhase() const { return Phase; }
	int32 GetCollectedClues() const { return CollectedClues; }
	int32 GetRequiredClues() const { return RequiredClues; }
	float GetBanishRemaining() const { return BanishRemaining; }
	bool CanExtract() const { return Phase == EHuntGamePhase::Extract; }
	FText GetPhaseText() const;

	// 当前 Boss 实例（线索收集完成后才会被生成）。HUD 需要用它来画暗视红色高亮。
	AHuntBossEnemy* GetBoss() const { return Boss; }

protected:
	void SetPhase(EHuntGamePhase NewPhase);
	void UnlockBoss();
	void BeginBanish();
	void TickBanish();
	void FinishBanish();
	void EnsurePrototypeActors();
	void SpawnPrototypeWorld();
	void SpawnTravelEnemies();
	void CacheLevelActors();
	void MovePlayerToStart(AHuntPlayerCharacter* Player) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Flow", meta = (ClampMin = 1))
	int32 RequiredClues = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Flow", meta = (ClampMin = 1.0))
	float BanishDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Flow", meta = (ClampMin = 0.1))
	float BanishSpawnInterval = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Flow", meta = (ClampMin = 0))
	int32 TravelEnemyCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Boss")
	TSubclassOf<AHuntBossEnemy> BossClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Boss")
	FVector BossSpawnLocation = FVector(1700.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Boss")
	FRotator BossSpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Boss")
	FName BossSpawnTag = TEXT("BossSpawn");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Flow")
	bool bCreatePrototypeActorsIfMissing = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AHuntClueActor>> Clues;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AHuntSpawner>> Spawners;

	UPROPERTY(Transient)
	TObjectPtr<AHuntBossEnemy> Boss;

	UPROPERTY(Transient)
	TObjectPtr<AHuntExtractionPoint> ExtractionPoint;

	EHuntGamePhase Phase = EHuntGamePhase::MainMenu;
	int32 CollectedClues = 0;
	float BanishRemaining = 0.0f;
	FTimerHandle BanishTimer;
	FTimerHandle BanishSpawnTimer;
};
