#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntClueActor.generated.h"

class AHuntPlayerCharacter;
class UPointLightComponent;
class USphereComponent;
class USoundBase;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntClueActor : public AActor
{
	GENERATED_BODY()

public:
	AHuntClueActor();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Clue")
	void Collect(AHuntPlayerCharacter* Player);

	bool IsCollected() const { return bCollected; }
	void SetDarkSightHighlighted(bool bHighlighted);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> GlowLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Clue")
	TObjectPtr<USoundBase> CollectSound;

	bool bCollected = false;
};
