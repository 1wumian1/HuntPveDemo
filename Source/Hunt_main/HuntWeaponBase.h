#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HuntTypes.h"
#include "HuntWeaponBase.generated.h"

class AHuntPlayerCharacter;
class UAudioComponent;
class USoundBase;
class UCameraShakeBase;

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AHuntWeaponBase();

	void ConfigureAs(EHuntWeaponKind InKind);
	void SetOwningPlayer(AHuntPlayerCharacter* InOwner);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Weapon")
	bool TryFire();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Weapon")
	void StartReload();

	UFUNCTION(BlueprintCallable, Category = "Hunt|Weapon")
	bool CanFire() const;

	FName GetWeaponName() const { return WeaponName; }
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetMagazineSize() const { return MagazineSize; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }
	bool IsReloading() const { return bReloading; }
	EHuntWeaponKind GetWeaponKind() const { return WeaponKind; }

protected:
	virtual void BeginPlay() override;

	void FinishReload();
	void ApplyHit(const FHitResult& Hit, const FVector& ShotDirection);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EHuntWeaponKind WeaponKind = EHuntWeaponKind::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponName = TEXT("Pistol");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.0))
	float Damage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.01))
	float FireInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.0))
	float Range = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.0))
	float SpreadDegrees = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	int32 MagazineSize = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0))
	int32 ReserveAmmo = 36;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = 0.0))
	float ReloadDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TArray<TObjectPtr<USoundBase>> FireSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> EmptySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	UPROPERTY(Transient)
	TObjectPtr<AHuntPlayerCharacter> OwningPlayer;

	int32 AmmoInMagazine = 0;
	float LastFireTime = -1000.0f;
	bool bReloading = false;
	FTimerHandle ReloadTimer;
};
