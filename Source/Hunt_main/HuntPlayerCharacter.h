#pragma once

#include "CoreMinimal.h"
#include "Hunt_mainCharacter.h"
#include "HuntTypes.h"
#include "HuntPlayerCharacter.generated.h"

class AHuntWeaponBase;
class UCameraShakeBase;
class USoundBase;
class USpotLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHuntHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

UCLASS(Blueprintable)
class HUNT_MAIN_API AHuntPlayerCharacter : public AHunt_mainCharacter
{
	GENERATED_BODY()

public:
	AHuntPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Hunt|Player")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Player")
	void AddExplosiveCharge(int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Hunt|Player")
	void SetInputEnabledByGame(bool bEnabled);

	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return bDead; }
	bool IsDarkSightActive() const { return bDarkSightActive; }
	float GetDamageFlashAlpha() const { return DamageFlashAlpha; }
	int32 GetHealCharges() const { return HealCharges; }
	int32 GetExplosiveCharges() const { return ExplosiveCharges; }
	AHuntWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UPROPERTY(BlueprintAssignable, Category = "Hunt|Player")
	FHuntHealthChangedSignature OnHealthChanged;

protected:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void StartJump();
	void StopJump();
	void Fire();
	void Reload();
	void SwitchWeapon();
	void Interact();
	void UseHeal();
	void ThrowExplosive();
	void StartDarkSight();
	void StopDarkSight();

	void SpawnDefaultWeapons();
	void EquipWeapon(int32 NewIndex);
	void Die();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 1.0))
	float MaxHealth = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0))
	int32 HealCharges = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0))
	int32 ExplosiveCharges = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0.0))
	float HealAmount = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0.0))
	float InteractRange = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0.0))
	float ExplosiveRange = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0.0))
	float ExplosiveDamage = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> HurtSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> HealSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> MudFootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> WoodFootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> HurtCameraShake;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> DebugHeadLight;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AHuntWeaponBase>> Weapons;

	UPROPERTY(Transient)
	TObjectPtr<AHuntWeaponBase> CurrentWeapon;

	float CurrentHealth = 0.0f;
	int32 CurrentWeaponIndex = INDEX_NONE;
	bool bDead = false;
	bool bAcceptsGameplayInput = false;
	bool bDarkSightActive = false;
	float DamageFlashAlpha = 0.0f;
	float DistanceSinceLastFootstep = 0.0f;
	FVector LastFootstepLocation = FVector::ZeroVector;
};
