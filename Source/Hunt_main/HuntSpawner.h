#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntSpawner.generated.h"

class AHuntEnemyBase;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntSpawner : public AActor
{
	GENERATED_BODY()

public:
	AHuntSpawner();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Spawner")
	AHuntEnemyBase* SpawnEnemy();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Spawner")
	int32 SpawnEnemies(int32 Count);

	void SetMaxAlive(int32 InMaxAlive) { MaxAliveEnemies = InMaxAlive; }

protected:
	void CleanupDeadReferences();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Spawner")
	TSubclassOf<AHuntEnemyBase> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Spawner", meta = (ClampMin = 0.0))
	float SpawnRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Spawner", meta = (ClampMin = 1))
	int32 MaxAliveEnemies = 8;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AHuntEnemyBase>> AliveEnemies;
};
