#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HuntEnemyBase.generated.h"

class AHuntPlayerCharacter;
class UAnimMontage;
class USoundBase;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AHuntEnemyBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	bool IsDead() const { return bDead; }
	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }

protected:
	virtual void Die();
	virtual void AttackTarget();
	virtual AHuntPlayerCharacter* FindTargetPlayer() const;
	bool CanDetectPlayer(AHuntPlayerCharacter* Target, float Distance);

	// 播放攻击动画/蒙太奇；若没绑定 AttackMontage 则退回到 C++ 兜底前冲表现
	void PlayAttackAnimation();

	// C++ 兜底：在没有任何攻击 Montage 的情况下，做一次轻量级前冲 + 抖动表现
	void StartFallbackAttackImpulse();
	void EndFallbackAttackImpulse();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 1.0))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float MeleeDamage = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float AttackRange = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.01))
	float AttackInterval = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float ChaseSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float DirectDetectionRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float AlertMemoryDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Enemy", meta = (ClampMin = 0.0))
	float DespawnDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Feedback")
	TObjectPtr<USoundBase> AttackSound;

	// 蓝图里可以配置一个攻击蒙太奇（例如僵尸抓挠/扑击）。如果留空，C++ 会做一次前冲抖动作为兜底反馈。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Animation", meta = (ClampMin = 0.1))
	float AttackMontagePlayRate = 1.0f;

	// 兜底攻击表现：向玩家方向短暂前冲的距离（cm）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Animation", meta = (ClampMin = 0.0))
	float FallbackAttackLungeDistance = 80.0f;

	// 兜底攻击表现：网格瞬间放大量，营造扑击的视觉冲击
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Animation", meta = (ClampMin = 1.0))
	float FallbackAttackMeshScalePunch = 1.15f;

	// 兜底攻击表现：恢复原状的时长
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Animation", meta = (ClampMin = 0.05))
	float FallbackAttackDuration = 0.25f;

	float CurrentHealth = 0.0f;
	float LastAttackTime = -1000.0f;
	float LastDetectedPlayerTime = -1000.0f;
	bool bDead = false;

	FVector OriginalMeshScale = FVector(1.0f);
	FTimerHandle FallbackAttackTimer;
};
