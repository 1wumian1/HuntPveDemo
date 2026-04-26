#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntExtractionPoint.generated.h"

class AHuntPlayerCharacter;
class UBoxComponent;
class UPointLightComponent;
class USoundBase;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntExtractionPoint : public AActor
{
	GENERATED_BODY()

public:
	AHuntExtractionPoint();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Hunt|Extraction")
	void TryExtract(AHuntPlayerCharacter* Player);

	void SetActiveExtraction(bool bActive);

protected:
	UFUNCTION()
	void OnExtractionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ExtractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> BeaconLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunt|Extraction")
	TObjectPtr<USoundBase> ExtractSound;

	bool bActive = false;
};
