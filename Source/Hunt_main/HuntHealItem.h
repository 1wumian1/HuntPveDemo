#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntHealItem.generated.h"

class USphereComponent;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntHealItem : public AActor
{
	GENERATED_BODY()

public:
	AHuntHealItem();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Pickup", meta = (ClampMin = 0.0))
	float HealAmount = 50.0f;
};
