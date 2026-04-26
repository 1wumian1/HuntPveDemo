#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HuntEnemyBase.generated.h"

class AHuntPlayerCharacter;
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
	float DespawnDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Feedback")
	TObjectPtr<USoundBase> AttackSound;

	float CurrentHealth = 0.0f;
	float LastAttackTime = -1000.0f;
	bool bDead = false;
};
