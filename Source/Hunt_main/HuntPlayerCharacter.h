#pragma once

#include "CoreMinimal.h"
#include "Hunt_mainCharacter.h"
#include "HuntTypes.h"
#include "Templates/SubclassOf.h"
#include "HuntPlayerCharacter.generated.h"

class AHuntWeaponBase;
class UCameraShakeBase;
class UAnimSequence;
class USceneComponent;
class USoundBase;
class USkeletalMeshComponent;
class USpotLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHuntHealthChangedSignature, float, CurrentHealth, float, MaxHealth);

UENUM(BlueprintType)
enum class EHuntShotgunLocomotion : uint8
{
	Idle,
	Walk,
	Run,
	Sprint,
	Jump,
};

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
	void SetInputEnabledByGame(bool bEnabled);

	UFUNCTION(BlueprintNativeEvent, Category = "Hunt|Visuals")
	void OnHuntWeaponFired(EHuntWeaponKind WeaponKind);

	UFUNCTION(BlueprintNativeEvent, Category = "Hunt|Visuals")
	void OnHuntWeaponDryFired(EHuntWeaponKind WeaponKind);

	UFUNCTION(BlueprintNativeEvent, Category = "Hunt|Visuals")
	void OnHuntWeaponReloadStarted(EHuntWeaponKind WeaponKind);

	UFUNCTION(BlueprintNativeEvent, Category = "Hunt|Visuals")
	void OnHuntWeaponAmmoChanged(EHuntWeaponKind WeaponKind, int32 AmmoInMagazine, int32 MagazineSize);

	UFUNCTION(BlueprintNativeEvent, Category = "Hunt|Visuals")
	void OnHuntWeaponEquipped(EHuntWeaponKind WeaponKind);

	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return bDead; }
	bool IsDarkSightActive() const { return bDarkSightActive; }
	bool IsHuntCrouched() const { return bCrouchState; }
	bool IsSprinting() const { return bSprinting; }
	bool IsAimingShotgun() const { return bAimingShotgun; }
	bool HasRecentFootstepNoise(float CurrentTime) const;
	const FVector& GetLastFootstepNoiseLocation() const { return LastFootstepNoiseLocation; }
	float GetLastFootstepNoiseRange() const { return LastFootstepNoiseRange; }
	float GetDamageFlashAlpha() const { return DamageFlashAlpha; }
	float GetDarkSightRevealRange() const { return DarkSightRevealRange; }
	AHuntWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	void NotifyWeaponAmmoChanged(AHuntWeaponBase* Weapon);

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
	void StartAim();
	void StopAim();
	void Interact();
	void UseHeal();
	void StartDarkSight();
	void StopDarkSight();
	void ToggleCrouchState();
	void StartSprint();
	void StopSprint();

	void SpawnDefaultWeapons();
	TSubclassOf<AHuntWeaponBase> GetWeaponClassForKind(EHuntWeaponKind Kind) const;
	void EquipWeapon(int32 NewIndex);
	void Die();
	void UpdateMovementState();
	void UpdateDarkSightClues();
	void ApplyDarkSightVisuals(bool bEnabled);
	float GetCurrentFootstepDistance() const;
	float GetCurrentFootstepVolume() const;
	float GetCurrentFootstepNoiseRange() const;
	void ReportFootstepNoise();
	void PlayShotgunVisualAnimation(UAnimSequence* CharacterAnim, UAnimSequence* WeaponAnim, float PlayRate = 1.0f, bool bStopExisting = true, bool bLoop = false);
	void PlayShotgunOneShotAnimation(UAnimSequence* CharacterAnim, UAnimSequence* WeaponAnim, float PlayRate = 1.0f);
	void PlayRandomShotgunFireAnimation();
	void PlayShotgunIdleAnimation();
	void ResetShotgunVisualSetup();
	void ApplyShotgunAimState(bool bNewAiming);
	void ApplyShotgunFireKick();
	void ResetShotgunFireKick();
	void UpdateShotgunLocomotionAnimation(bool bForce = false);
	EHuntShotgunLocomotion ResolveDesiredLocomotion() const;
	UAnimSequence* GetCharacterLocomotionAnim(EHuntShotgunLocomotion State) const;
	void OnShotgunOneShotFinished();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 1.0))
	float MaxHealth = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0.0))
	float InteractRange = 250.0f;

	// 走路速度（原 600，整体放慢到原来的 1/3 以匹配 Hunt 的氛围节奏）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0))
	float WalkSpeed = 200.0f;

	// 蹲伏速度（原 260，整体放慢到原来的 1/3）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0))
	float CrouchSpeed = 87.0f;

	// 冲刺速度（原 1200，整体放慢到原来的 1/3）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0))
	float SprintSpeed = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0))
	float StandingCameraHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0.0))
	float CrouchedCameraHeight = 50.0f;

	// 脚步触发的位移阈值（cm）。整体放大到原来的 2 倍，意味着脚步音效频率减半，
	// 配合放慢后的移动速度听起来不会过于密集。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float WalkFootstepDistance = 440.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float CrouchFootstepDistance = 680.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float SprintFootstepDistance = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float WalkNoiseRange = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float CrouchNoiseRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float SprintNoiseRange = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stealth", meta = (ClampMin = 0.0))
	float FootstepNoiseLifetime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dark Sight", meta = (ClampMin = 0.0))
	float DarkSightRevealRange = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> HurtSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> HealSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> MudFootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> WoodFootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> HurtCameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	TSubclassOf<AHuntWeaponBase> ShotgunWeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	TSubclassOf<AHuntWeaponBase> KatanaWeaponClass;

	// 武器相对于第一人称摄像机的位置偏移，可在蓝图里微调
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	FVector WeaponViewOffset = FVector(30.f, 15.f, -20.f);

	// 武器相对于第一人称摄像机的旋转偏移
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	FRotator WeaponViewRotation = FRotator::ZeroRotator;

	// Socket / bone name on the arms mesh that the shotgun should attach to.
	// `weapon_r` is the socket that ships with the Animated_DBShotgun mannequin.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FName ShotgunAttachSocket = FName(TEXT("weapon_r"));

	// Relative transform of the shotgun against the chosen hand socket / bone.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector ShotgunHandRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FRotator ShotgunHandRelativeRotation = FRotator(0.0f, 90.0f, 90.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector ShotgunFireKickLocationOffset = FVector(-7.0f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FRotator ShotgunFireKickRotationOffset = FRotator(-12.0f, 0.0f, 0.0f);

	// The arms mesh is attached to the first-person camera so that it follows pitch (look up/down).
	// Default offsets here are expressed RELATIVE TO THE CAMERA (the camera sits at StandingCameraHeight
	// above the capsule center), which is roughly -168 along Z to put the mannequin's feet below the player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector FirstPersonArmsLocation = FVector(0.0f, 0.0f, -168.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FRotator FirstPersonArmsRotation = FRotator(0.0f, -90.0f, 0.0f);
 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector FirstPersonArmsScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	float DefaultCameraFov = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	float AimCameraFov = 68.0f;

	// Push the first-person camera forward toward the gun while aiming so the screen looks down the barrel.
	// Mirrors the resource pack's BP_weapon_base "zoom_cam" trick.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector CameraAimOffset = FVector(28.0f, 0.0f, -3.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> DebugHeadLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Shotgun Visuals")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Shotgun Visuals")
	TObjectPtr<USkeletalMeshComponent> ShotgunVisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Shotgun Visuals")
	TObjectPtr<USceneComponent> ShotgunMuzzle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TArray<TObjectPtr<UAnimSequence>> ShotgunCharacterFireAnims;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TArray<TObjectPtr<UAnimSequence>> ShotgunWeaponFireAnims;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunCharacterReloadAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunWeaponReloadAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunCharacterEmptyAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunWeaponEmptyAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunCharacterEquipAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunWeaponEquipAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunCharacterIdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	TObjectPtr<UAnimSequence> ShotgunWeaponIdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion")
	TObjectPtr<UAnimSequence> ShotgunCharacterWalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion")
	TObjectPtr<UAnimSequence> ShotgunCharacterRunAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion")
	TObjectPtr<UAnimSequence> ShotgunCharacterSprintAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion")
	TObjectPtr<UAnimSequence> ShotgunCharacterJumpAnim;

	// Speed (cm/s) above which the player starts the walk locomotion (instead of idle).
	// 也按 1/3 整体缩放（原 50），保证缓慢挪动也能切到走路动画。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion", meta = (ClampMin = 0.0))
	float WalkLocomotionSpeed = 17.0f;

	// Speed (cm/s) above which we use the run animation.
	// 同步缩放到 1/3（原 700），让冲刺速度（400）能稳定触发 Run 动画。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals|Locomotion", meta = (ClampMin = 0.0))
	float RunLocomotionSpeed = 233.0f;

	// ADS pose for the arms mesh: lift the whole mannequin up so hand_r reaches eye level and the gun barrel
	// becomes horizontal at the centre of the screen, like the resource pack's reference screenshot.
	// Expressed relative to the camera (see FirstPersonArmsLocation comment above).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FVector FirstPersonArmsAimLocation = FVector(15.0f, -8.0f, -135.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shotgun Visuals")
	FRotator FirstPersonArmsAimRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AHuntWeaponBase>> Weapons;

	UPROPERTY(Transient)
	TObjectPtr<AHuntWeaponBase> CurrentWeapon;

	float CurrentHealth = 0.0f;
	int32 CurrentWeaponIndex = INDEX_NONE;
	bool bDead = false;
	bool bAcceptsGameplayInput = false;
	bool bCrouchState = false;
	bool bSprintHeld = false;
	bool bSprinting = false;
	bool bDarkSightActive = false;
	bool bDarkSightVisualsApplied = false;
	bool bAimingShotgun = false;
	bool bPlayingOneShotShotgunAnim = false;
	EHuntShotgunLocomotion CurrentShotgunLocomotion = EHuntShotgunLocomotion::Idle;
	FTimerHandle ShotgunFireKickTimer;
	FTimerHandle ShotgunIdleReturnTimer;
	float DamageFlashAlpha = 0.0f;
	float SavedPostProcessBlendWeight = 0.0f;
	float DistanceSinceLastFootstep = 0.0f;
	FVector LastFootstepLocation = FVector::ZeroVector;
	FVector LastFootstepNoiseLocation = FVector::ZeroVector;
	float LastFootstepNoiseTime = -1000.0f;
	float LastFootstepNoiseRange = 0.0f;
};
