#include "HuntSpawner.h"
#include "HuntEnemyBase.h"
#include "Kismet/KismetMathLibrary.h"

AHuntSpawner::AHuntSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	EnemyClass = AHuntEnemyBase::StaticClass();
}

AHuntEnemyBase* AHuntSpawner::SpawnEnemy()
{
	CleanupDeadReferences();

	if (!GetWorld() || AliveEnemies.Num() >= MaxAliveEnemies)
	{
		return nullptr;
	}

	const FVector Offset = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(100.0f, SpawnRadius);
	const FVector SpawnLocation = GetActorLocation() + FVector(Offset.X, Offset.Y, 90.0f);
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* SpawnClass = EnemyClass ? EnemyClass.Get() : AHuntEnemyBase::StaticClass();
	AHuntEnemyBase* Enemy = GetWorld()->SpawnActor<AHuntEnemyBase>(SpawnClass, SpawnLocation, SpawnRotation, Params);
	if (Enemy)
	{
		AliveEnemies.Add(Enemy);
	}

	return Enemy;
}

int32 AHuntSpawner::SpawnEnemies(int32 Count)
{
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (SpawnEnemy())
		{
			++Spawned;
		}
	}
	return Spawned;
}

void AHuntSpawner::CleanupDeadReferences()
{
	AliveEnemies.RemoveAll([](const TObjectPtr<AHuntEnemyBase>& Enemy)
	{
		return !Enemy || Enemy->IsDead();
	});
}
