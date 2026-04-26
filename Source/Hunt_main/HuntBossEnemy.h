#pragma once

#include "CoreMinimal.h"
#include "HuntEnemyBase.h"
#include "HuntBossEnemy.generated.h"

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntBossEnemy : public AHuntEnemyBase
{
	GENERATED_BODY()

public:
	AHuntBossEnemy();

protected:
	virtual void Die() override;
};
