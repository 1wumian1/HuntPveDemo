#include "HuntBossEnemy.h"
#include "HuntGameMode.h"

AHuntBossEnemy::AHuntBossEnemy()
{
	MaxHealth = 650.0f;
	MeleeDamage = 35.0f;
	AttackRange = 170.0f;
	AttackInterval = 1.0f;
	ChaseSpeed = 430.0f;
	DespawnDelay = 4.0f;
	SetActorScale3D(FVector(1.45f));
}

void AHuntBossEnemy::Die()
{
	if (bDead)
	{
		return;
	}

	Super::Die();

	if (AHuntGameMode* HuntGameMode = GetWorld()->GetAuthGameMode<AHuntGameMode>())
	{
		HuntGameMode->BossKilled(this);
	}
}
