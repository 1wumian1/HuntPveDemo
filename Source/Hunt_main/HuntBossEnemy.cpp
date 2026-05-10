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

	// Boss 攻击表现：兜底前冲距离更长、缩放打击感更强（如果蓝图里挂了 Montage 会优先用 Montage）
	FallbackAttackLungeDistance = 130.0f;
	FallbackAttackMeshScalePunch = 1.18f;
	FallbackAttackDuration = 0.32f;

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
