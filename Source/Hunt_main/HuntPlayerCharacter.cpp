#include "HuntPlayerCharacter.h"
#include "HuntClueActor.h"
#include "HuntExtractionPoint.h"
#include "HuntGameMode.h"
#include "HuntWeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void PlayVisualAnimation(USkeletalMeshComponent* Mesh, UAnimSequence* Animation, float PlayRate, bool bStopExisting, bool bLoop = false)
	{
		if (!Mesh || !Animation)
		{
			return;
		}

		// Resource-pack assets do not need a project AnimBP here; single-node playback is more reliable for this visual wrapper.
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(Animation, bLoop);
		if (UAnimSingleNodeInstance* SingleNode = Mesh->GetSingleNodeInstance())
		{
			SingleNode->SetPlayRate(PlayRate);
		}
	}
}

AHuntPlayerCharacter::AHuntPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// The template camera expects art sockets that the C++ prototype does not have yet.
	// Put the camera on the capsule so the debug level is visible without imported meshes.
	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		Camera->SetupAttachment(GetCapsuleComponent());
		Camera->SetRelativeLocation(FVector(0.0f, 0.0f, StandingCameraHeight));
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->bUsePawnControlRotation = true;
	}

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(48.0f);

	DebugHeadLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Debug Head Light"));
	DebugHeadLight->SetupAttachment(GetFirstPersonCameraComponent());
	DebugHeadLight->SetRelativeLocation(FVector::ZeroVector);
	DebugHeadLight->SetRelativeRotation(FRotator::ZeroRotator);
	DebugHeadLight->SetIntensity(0.0f);
	DebugHeadLight->SetAttenuationRadius(5000.0f);
	DebugHeadLight->SetInnerConeAngle(22.0f);
	DebugHeadLight->SetOuterConeAngle(48.0f);

	FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Animated DB First Person Arms"));
	// Attach the FP arms to the camera so they follow Pitch (look up/down) AND Yaw (look left/right),
	// not just yaw via the capsule. Otherwise the arms only swing horizontally.
	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		FirstPersonArmsMesh->SetupAttachment(Camera);
	}
	else
	{
		FirstPersonArmsMesh->SetupAttachment(GetCapsuleComponent());
	}
	FirstPersonArmsMesh->SetRelativeLocation(FirstPersonArmsLocation);
	FirstPersonArmsMesh->SetRelativeRotation(FirstPersonArmsRotation);
	FirstPersonArmsMesh->SetRelativeScale3D(FirstPersonArmsScale);
	FirstPersonArmsMesh->SetOnlyOwnerSee(true);
	FirstPersonArmsMesh->SetCastShadow(false);
	FirstPersonArmsMesh->bCastDynamicShadow = false;
	FirstPersonArmsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArmsMesh->SetHiddenInGame(false);

	ShotgunVisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Animated DB Shotgun"));
	// Attach directly to the resource pack's weapon_r socket so the BP viewport preview matches PIE exactly.
	// This lets you tweak the gun pose with the gizmo in the BP viewport and see the final result without entering PIE.
	ShotgunVisualMesh->SetupAttachment(FirstPersonArmsMesh, TEXT("weapon_r"));
	ShotgunVisualMesh->SetRelativeLocation(FVector::ZeroVector);
	ShotgunVisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	ShotgunVisualMesh->SetRelativeScale3D(FVector(1.0f));
	ShotgunVisualMesh->SetOnlyOwnerSee(true);
	ShotgunVisualMesh->SetCastShadow(false);
	ShotgunVisualMesh->bCastDynamicShadow = false;
	ShotgunVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ShotgunMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Shotgun Muzzle"));
	ShotgunMuzzle->SetupAttachment(ShotgunVisualMesh);
	ShotgunMuzzle->SetRelativeLocation(FVector(175.7f, 0.0f, -120.0f));
	ShotgunMuzzle->SetRelativeRotation(FRotator::ZeroRotator);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ArmsMeshAsset(TEXT("/Game/Animated_DBShotgun/skeletalmeshes/mannequin/sk_mannequin_mesh_fps.sk_mannequin_mesh_fps"));
	if (ArmsMeshAsset.Succeeded())
	{
		FirstPersonArmsMesh->SetSkeletalMesh(ArmsMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ShotgunMeshAsset(TEXT("/Game/Animated_DBShotgun/skeletalmeshes/dbshotgun/SK_dbshotgun_mesh.SK_dbshotgun_mesh"));
	if (ShotgunMeshAsset.Succeeded())
	{
		ShotgunVisualMesh->SetSkeletalMesh(ShotgunMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterShoot01(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_shoot_01.ANIM_DB_shotgun_shoot_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterShoot02(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_shoot_02.ANIM_DB_shotgun_shoot_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterShoot03(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_shoot_03.ANIM_DB_shotgun_shoot_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponShoot01(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_shoot_01.ANIM_dbshotgun_shoot_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponShoot02(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_shoot_02.ANIM_dbshotgun_shoot_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponShoot03(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_shoot_03.ANIM_dbshotgun_shoot_03"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterReload(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_reload_02.ANIM_DB_shotgun_reload_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponReload(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_reload_02.ANIM_dbshotgun_reload_02"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterEmpty(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_empty.ANIM_DB_shotgun_empty"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponEmpty(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_empty.ANIM_dbshotgun_empty"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterEquip(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_take_up.ANIM_DB_shotgun_take_up"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponEquip(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_take_up.ANIM_dbshotgun_take_up"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterIdle(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_idle.ANIM_DB_shotgun_idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> WeaponIdle(TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_idle.ANIM_dbshotgun_idle"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterWalk(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_walk.ANIM_DB_shotgun_walk"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterRun(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_run.ANIM_DB_shotgun_run"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterSprint(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_sprint.ANIM_DB_shotgun_sprint"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CharacterJump(TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_jump.ANIM_DB_shotgun_jump"));

	if (CharacterShoot01.Succeeded() && WeaponShoot01.Succeeded())
	{
		ShotgunCharacterFireAnims.Add(CharacterShoot01.Object);
		ShotgunWeaponFireAnims.Add(WeaponShoot01.Object);
	}
	if (CharacterShoot02.Succeeded() && WeaponShoot02.Succeeded())
	{
		ShotgunCharacterFireAnims.Add(CharacterShoot02.Object);
		ShotgunWeaponFireAnims.Add(WeaponShoot02.Object);
	}
	if (CharacterShoot03.Succeeded() && WeaponShoot03.Succeeded())
	{
		ShotgunCharacterFireAnims.Add(CharacterShoot03.Object);
		ShotgunWeaponFireAnims.Add(WeaponShoot03.Object);
	}
	if (CharacterReload.Succeeded())
	{
		ShotgunCharacterReloadAnim = CharacterReload.Object;
	}
	if (WeaponReload.Succeeded())
	{
		ShotgunWeaponReloadAnim = WeaponReload.Object;
	}
	if (CharacterEmpty.Succeeded())
	{
		ShotgunCharacterEmptyAnim = CharacterEmpty.Object;
	}
	if (WeaponEmpty.Succeeded())
	{
		ShotgunWeaponEmptyAnim = WeaponEmpty.Object;
	}
	if (CharacterEquip.Succeeded())
	{
		ShotgunCharacterEquipAnim = CharacterEquip.Object;
	}
	if (WeaponEquip.Succeeded())
	{
		ShotgunWeaponEquipAnim = WeaponEquip.Object;
	}
	if (CharacterIdle.Succeeded())
	{
		ShotgunCharacterIdleAnim = CharacterIdle.Object;
	}
	if (WeaponIdle.Succeeded())
	{
		ShotgunWeaponIdleAnim = WeaponIdle.Object;
	}
	if (CharacterWalk.Succeeded())
	{
		ShotgunCharacterWalkAnim = CharacterWalk.Object;
	}
	if (CharacterRun.Succeeded())
	{
		ShotgunCharacterRunAnim = CharacterRun.Object;
	}
	if (CharacterSprint.Succeeded())
	{
		ShotgunCharacterSprintAnim = CharacterSprint.Object;
	}
	if (CharacterJump.Succeeded())
	{
		ShotgunCharacterJumpAnim = CharacterJump.Object;
	}

	// Default audio cues from the included sound packs. Override on the BP/instance if you want
	// different sounds. We pick neutral cues that work for the prototype demo.
	static ConstructorHelpers::FObjectFinder<USoundBase> DirtFootstep(TEXT("/Game/Footstep_Sounds_Pro/Cues/Dirt_footsteps/Dirt_footstep_1_Cue.Dirt_footstep_1_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> WoodFootstep(TEXT("/Game/Footstep_Sounds_Pro/Cues/Wood_footsteps/Wood_footstep_1_Cue.Wood_footstep_1_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HurtCue(TEXT("/Game/GunSoundPack/BonusSounds/punch_general_body_impact_03_Cue.punch_general_body_impact_03_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> HealCue(TEXT("/Game/GunSoundPack/BonusSounds/foley_soldier_gear_equipment_movement_grab_item_01_Cue.foley_soldier_gear_equipment_movement_grab_item_01_Cue"));

	if (DirtFootstep.Succeeded()) { MudFootstepSound = DirtFootstep.Object; }
	if (WoodFootstep.Succeeded()) { WoodFootstepSound = WoodFootstep.Object; }
	if (HurtCue.Succeeded()) { HurtSound = HurtCue.Object; }
	if (HealCue.Succeeded()) { HealSound = HealCue.Object; }
}

void AHuntPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	ResetShotgunVisualSetup();
	SpawnDefaultWeapons();
	SetInputEnabledByGame(false);
	LastFootstepLocation = GetActorLocation();
	LastFootstepNoiseLocation = GetActorLocation();
}

void AHuntPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DamageFlashAlpha > 0.0f)
	{
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - DeltaSeconds * 1.8f);
	}

	// 玩家速度整体降到了原来的 1/3（蹲伏只有 87 cm/s），所以判断"是否在移动"的阈值也要相应缩小，
	// 否则蹲行时永远低于阈值，永远不会触发脚步声。
	if (bAcceptsGameplayInput && !bDead && GetVelocity().Size2D() > 50.0f && GetCharacterMovement()->IsMovingOnGround())
	{
		DistanceSinceLastFootstep += FVector::Dist2D(GetActorLocation(), LastFootstepLocation);
		LastFootstepLocation = GetActorLocation();

		if (DistanceSinceLastFootstep >= GetCurrentFootstepDistance())
		{
			DistanceSinceLastFootstep = 0.0f;
			USoundBase* FootstepSound = GetActorLocation().X > 0.0f ? WoodFootstepSound : MudFootstepSound;
			UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation(), GetCurrentFootstepVolume() * FMath::RandRange(0.9f, 1.1f));
			ReportFootstepNoise();
		}
	}
	else
	{
		LastFootstepLocation = GetActorLocation();
	}

	if (bDarkSightActive)
	{
		UpdateDarkSightClues();
	}

	UpdateShotgunLocomotionAnimation();
}

void AHuntPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AHuntPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AHuntPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AHuntPlayerCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AHuntPlayerCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AHuntPlayerCharacter::StartJump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AHuntPlayerCharacter::StopJump);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AHuntPlayerCharacter::Fire);
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AHuntPlayerCharacter::Reload);
	PlayerInputComponent->BindAction(TEXT("SwitchWeapon"), IE_Pressed, this, &AHuntPlayerCharacter::SwitchWeapon);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AHuntPlayerCharacter::StartAim);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AHuntPlayerCharacter::StopAim);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AHuntPlayerCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("UseHeal"), IE_Pressed, this, &AHuntPlayerCharacter::UseHeal);
	PlayerInputComponent->BindAction(TEXT("DarkSight"), IE_Pressed, this, &AHuntPlayerCharacter::StartDarkSight);
	PlayerInputComponent->BindAction(TEXT("DarkSight"), IE_Released, this, &AHuntPlayerCharacter::StopDarkSight);
	PlayerInputComponent->BindAction(TEXT("ToggleCrouch"), IE_Pressed, this, &AHuntPlayerCharacter::ToggleCrouchState);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AHuntPlayerCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AHuntPlayerCharacter::StopSprint);
}

float AHuntPlayerCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || Damage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	DamageFlashAlpha = 1.0f;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	UGameplayStatics::PlaySoundAtLocation(this, HurtSound, GetActorLocation());
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (HurtCameraShake)
		{
			PC->ClientStartCameraShake(HurtCameraShake);
		}
	}

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return Damage;
}

void AHuntPlayerCharacter::Heal(float Amount)
{
	if (bDead)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	UGameplayStatics::PlaySoundAtLocation(this, HealSound, GetActorLocation());
}

void AHuntPlayerCharacter::SetInputEnabledByGame(bool bEnabled)
{
	bAcceptsGameplayInput = bEnabled;
	if (!bEnabled)
	{
		bSprintHeld = false;
		bSprinting = false;
		StopDarkSight();
	}
	UpdateMovementState();
}

void AHuntPlayerCharacter::MoveForward(float Value)
{
	if (bAcceptsGameplayInput && !bDead && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AHuntPlayerCharacter::MoveRight(float Value)
{
	if (bAcceptsGameplayInput && !bDead && !FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AHuntPlayerCharacter::Turn(float Value)
{
	if (!bDead)
	{
		AddControllerYawInput(Value);
	}
}

void AHuntPlayerCharacter::LookUp(float Value)
{
	if (!bDead)
	{
		AddControllerPitchInput(Value);
	}
}

void AHuntPlayerCharacter::StartJump()
{
	if (bAcceptsGameplayInput && !bDead)
	{
		Jump();
	}
}

void AHuntPlayerCharacter::StopJump()
{
	StopJumping();
}

void AHuntPlayerCharacter::Fire()
{
	UE_LOG(LogTemp, Log, TEXT("Hunt Fire input: accepts=%d dark=%d dead=%d weapon=%s ammo=%d"), bAcceptsGameplayInput, bDarkSightActive, bDead, CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("None"), CurrentWeapon ? CurrentWeapon->GetAmmoInMagazine() : -1);

	if (bAcceptsGameplayInput && !bDarkSightActive && !bDead && CurrentWeapon)
	{
		const bool bDryFireAttempt = CurrentWeapon->UsesAmmo()
			&& !CurrentWeapon->IsReloading()
			&& CurrentWeapon->GetAmmoInMagazine() <= 0;

		if (CurrentWeapon->TryFire())
		{
			if (CurrentWeapon->GetWeaponKind() == EHuntWeaponKind::Shotgun)
			{
				// The resource pack's shoot animation already plays the recoil; do NOT overlay a kick offset
				// on the arms mesh or the player will see a double "twitch" on top of the animation.
				PlayRandomShotgunFireAnimation();
			}
			OnHuntWeaponFired(CurrentWeapon->GetWeaponKind());
			OnHuntWeaponAmmoChanged(CurrentWeapon->GetWeaponKind(), CurrentWeapon->GetAmmoInMagazine(), CurrentWeapon->GetMagazineSize());
		}
		else if (bDryFireAttempt)
		{
			if (CurrentWeapon->GetWeaponKind() == EHuntWeaponKind::Shotgun)
			{
				PlayShotgunOneShotAnimation(ShotgunCharacterEmptyAnim, ShotgunWeaponEmptyAnim);
			}
			OnHuntWeaponDryFired(CurrentWeapon->GetWeaponKind());
		}
	}
}

void AHuntPlayerCharacter::Reload()
{
	UE_LOG(LogTemp, Log, TEXT("Hunt Reload input: accepts=%d dark=%d dead=%d weapon=%s ammo=%d"), bAcceptsGameplayInput, bDarkSightActive, bDead, CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("None"), CurrentWeapon ? CurrentWeapon->GetAmmoInMagazine() : -1);

	if (bAcceptsGameplayInput && !bDarkSightActive && !bDead && CurrentWeapon)
	{
		const bool bWillReload = CurrentWeapon->UsesAmmo()
			&& !CurrentWeapon->IsReloading()
			&& CurrentWeapon->GetAmmoInMagazine() < CurrentWeapon->GetMagazineSize()
			&& CurrentWeapon->GetReserveAmmo() > 0;
		CurrentWeapon->StartReload();
		if (bWillReload)
		{
			if (CurrentWeapon->GetWeaponKind() == EHuntWeaponKind::Shotgun)
			{
				PlayShotgunOneShotAnimation(ShotgunCharacterReloadAnim, ShotgunWeaponReloadAnim);
			}
			OnHuntWeaponReloadStarted(CurrentWeapon->GetWeaponKind());
		}
	}
}

void AHuntPlayerCharacter::SwitchWeapon()
{
	// The demo now keeps a single primary shotgun, so weapon switching is intentionally disabled.
}

void AHuntPlayerCharacter::StartAim()
{
	if (bAcceptsGameplayInput && !bDarkSightActive && !bDead)
	{
		ApplyShotgunAimState(true);
	}
}

void AHuntPlayerCharacter::StopAim()
{
	ApplyShotgunAimState(false);
}

void AHuntPlayerCharacter::Interact()
{
	if (!bAcceptsGameplayInput || bDead)
	{
		return;
	}

	UCameraComponent* Camera = GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractRange;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntInteractTrace), false, this);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (AHuntClueActor* Clue = Cast<AHuntClueActor>(Hit.GetActor()))
	{
		Clue->Collect(this);
	}
	else if (AHuntExtractionPoint* Extraction = Cast<AHuntExtractionPoint>(Hit.GetActor()))
	{
		Extraction->TryExtract(this);
	}
}

void AHuntPlayerCharacter::UseHeal()
{
	// 新规则：未满血时随时可以治疗（无次数限制、无冲刺限制），按一次直接回到满血
	if (bAcceptsGameplayInput && !bDead && CurrentHealth < MaxHealth)
	{
		Heal(MaxHealth);
	}
}

void AHuntPlayerCharacter::StartDarkSight()
{
	if (!bAcceptsGameplayInput || bDead)
	{
		return;
	}

	bDarkSightActive = true;
	bSprintHeld = false;
	bSprinting = false;
	UpdateMovementState();
	ApplyDarkSightVisuals(true);
	UpdateDarkSightClues();
}

void AHuntPlayerCharacter::StopDarkSight()
{
	if (!bDarkSightActive && !bDarkSightVisualsApplied)
	{
		return;
	}

	bDarkSightActive = false;

	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		It->SetDarkSightHighlighted(false);
	}

	ApplyDarkSightVisuals(false);
}

void AHuntPlayerCharacter::UpdateDarkSightClues()
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		const bool bNearby = !It->IsCollected() && FVector::DistSquared(GetActorLocation(), It->GetActorLocation()) <= FMath::Square(DarkSightRevealRange);
		It->SetDarkSightHighlighted(bNearby);
	}
}

void AHuntPlayerCharacter::ApplyDarkSightVisuals(bool bEnabled)
{
	UCameraComponent* Camera = GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return;
	}

	if (bEnabled)
	{
		if (!bDarkSightVisualsApplied)
		{
			SavedPostProcessBlendWeight = Camera->PostProcessBlendWeight;
		}

		bDarkSightVisualsApplied = true;
		Camera->PostProcessBlendWeight = 1.0f;
		Camera->PostProcessSettings.bOverride_ColorSaturation = true;
		Camera->PostProcessSettings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
		Camera->PostProcessSettings.VignetteIntensity = 0.75f;
	}
	else
	{
		Camera->PostProcessSettings.bOverride_ColorSaturation = false;
		Camera->PostProcessSettings.bOverride_VignetteIntensity = false;
		Camera->PostProcessBlendWeight = SavedPostProcessBlendWeight;
		bDarkSightVisualsApplied = false;
	}
}

void AHuntPlayerCharacter::ToggleCrouchState()
{
	if (!bAcceptsGameplayInput || bDead || bSprinting)
	{
		return;
	}

	bCrouchState = !bCrouchState;
	UpdateMovementState();
}

void AHuntPlayerCharacter::StartSprint()
{
	if (!bAcceptsGameplayInput || bDarkSightActive || bDead)
	{
		return;
	}

	bSprintHeld = true;
	bCrouchState = false;
	ApplyShotgunAimState(false);
	UpdateMovementState();
}

void AHuntPlayerCharacter::StopSprint()
{
	bSprintHeld = false;
	UpdateMovementState();
}

void AHuntPlayerCharacter::UpdateMovementState()
{
	bSprinting = bAcceptsGameplayInput && bSprintHeld && !bDarkSightActive && !bCrouchState && !bDead;

	if (bCrouchState)
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}

	GetCharacterMovement()->MaxWalkSpeed = bAcceptsGameplayInput ? (bSprinting ? SprintSpeed : (bCrouchState ? CrouchSpeed : WalkSpeed)) : 0.0f;

	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		Camera->SetRelativeLocation(FVector(0.0f, 0.0f, bCrouchState ? CrouchedCameraHeight : StandingCameraHeight));
	}
}

float AHuntPlayerCharacter::GetCurrentFootstepDistance() const
{
	if (bSprinting)
	{
		return SprintFootstepDistance;
	}

	return bCrouchState ? CrouchFootstepDistance : WalkFootstepDistance;
}

float AHuntPlayerCharacter::GetCurrentFootstepVolume() const
{
	if (bSprinting)
	{
		return 1.35f;
	}

	return bCrouchState ? 0.35f : 1.0f;
}

float AHuntPlayerCharacter::GetCurrentFootstepNoiseRange() const
{
	if (bSprinting)
	{
		return SprintNoiseRange;
	}

	return bCrouchState ? CrouchNoiseRange : WalkNoiseRange;
}

void AHuntPlayerCharacter::ReportFootstepNoise()
{
	LastFootstepNoiseLocation = GetActorLocation();
	LastFootstepNoiseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1000.0f;
	LastFootstepNoiseRange = GetCurrentFootstepNoiseRange();
}

bool AHuntPlayerCharacter::HasRecentFootstepNoise(float CurrentTime) const
{
	return CurrentTime - LastFootstepNoiseTime <= FootstepNoiseLifetime;
}

void AHuntPlayerCharacter::SpawnDefaultWeapons()
{
	if (!GetWorld())
	{
		return;
	}

	for (const EHuntWeaponKind Kind : { EHuntWeaponKind::Shotgun })
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const TSubclassOf<AHuntWeaponBase> WeaponClass = GetWeaponClassForKind(Kind);
		UClass* ClassToSpawn = WeaponClass ? *WeaponClass : AHuntWeaponBase::StaticClass();
		AHuntWeaponBase* Weapon = GetWorld()->SpawnActor<AHuntWeaponBase>(ClassToSpawn, GetActorTransform(), Params);
		if (Weapon)
		{
			Weapon->ConfigureAs(Kind);
			Weapon->SetOwningPlayer(this);
			if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
			{
				Weapon->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);
				// 武器相对于第一人称摄像机的偏移：前方、右侧、略低于视线
				Weapon->SetActorRelativeLocation(WeaponViewOffset);
				Weapon->SetActorRelativeRotation(WeaponViewRotation);
			}
			else
			{
				Weapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			}
			Weapon->SetActorHiddenInGame(true);
			Weapon->SetActorEnableCollision(false);
			Weapons.Add(Weapon);
		}
	}

	EquipWeapon(0);
}

TSubclassOf<AHuntWeaponBase> AHuntPlayerCharacter::GetWeaponClassForKind(EHuntWeaponKind Kind) const
{
	switch (Kind)
	{
	case EHuntWeaponKind::Shotgun:
		return ShotgunWeaponClass;
	case EHuntWeaponKind::Katana:
		return KatanaWeaponClass;
	default:
		return AHuntWeaponBase::StaticClass();
	}
}

void AHuntPlayerCharacter::EquipWeapon(int32 NewIndex)
{
	if (!Weapons.IsValidIndex(NewIndex))
	{
		return;
	}

	CurrentWeaponIndex = NewIndex;
	CurrentWeapon = Weapons[CurrentWeaponIndex];

	for (int32 WeaponIndex = 0; WeaponIndex < Weapons.Num(); ++WeaponIndex)
	{
		if (AHuntWeaponBase* Weapon = Weapons[WeaponIndex])
		{
			const bool bIsEquipped = WeaponIndex == CurrentWeaponIndex;
			Weapon->SetActorHiddenInGame(!bIsEquipped);
			Weapon->SetActorEnableCollision(false);
		}
	}

	OnHuntWeaponEquipped(CurrentWeapon->GetWeaponKind());
	if (CurrentWeapon->GetWeaponKind() == EHuntWeaponKind::Shotgun)
	{
		PlayShotgunOneShotAnimation(ShotgunCharacterEquipAnim, ShotgunWeaponEquipAnim);
	}
	OnHuntWeaponAmmoChanged(CurrentWeapon->GetWeaponKind(), CurrentWeapon->GetAmmoInMagazine(), CurrentWeapon->GetMagazineSize());
}

void AHuntPlayerCharacter::NotifyWeaponAmmoChanged(AHuntWeaponBase* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	OnHuntWeaponAmmoChanged(Weapon->GetWeaponKind(), Weapon->GetAmmoInMagazine(), Weapon->GetMagazineSize());
}

void AHuntPlayerCharacter::PlayShotgunVisualAnimation(UAnimSequence* CharacterAnim, UAnimSequence* WeaponAnim, float PlayRate, bool bStopExisting, bool bLoop)
{
	PlayVisualAnimation(FirstPersonArmsMesh, CharacterAnim, PlayRate, bStopExisting, bLoop);
	PlayVisualAnimation(ShotgunVisualMesh, WeaponAnim, PlayRate, bStopExisting, bLoop);

	GetWorldTimerManager().ClearTimer(ShotgunIdleReturnTimer);
	if (!bLoop)
	{
		// Schedule a return-to-idle so the hands keep holding the gun instead of freezing on the last frame.
		float Duration = 0.0f;
		if (CharacterAnim)
		{
			Duration = FMath::Max(Duration, CharacterAnim->GetPlayLength());
		}
		if (WeaponAnim)
		{
			Duration = FMath::Max(Duration, WeaponAnim->GetPlayLength());
		}
		if (PlayRate > KINDA_SMALL_NUMBER)
		{
			Duration /= PlayRate;
		}
		if (Duration > 0.05f)
		{
			GetWorldTimerManager().SetTimer(ShotgunIdleReturnTimer, this, &AHuntPlayerCharacter::OnShotgunOneShotFinished, Duration, false);
		}
	}
}

void AHuntPlayerCharacter::PlayShotgunOneShotAnimation(UAnimSequence* CharacterAnim, UAnimSequence* WeaponAnim, float PlayRate)
{
	bPlayingOneShotShotgunAnim = true;
	// Force the next locomotion update to re-apply, since we just clobbered the looping animation.
	CurrentShotgunLocomotion = EHuntShotgunLocomotion::Idle;
	PlayShotgunVisualAnimation(CharacterAnim, WeaponAnim, PlayRate, true, false);
}

void AHuntPlayerCharacter::OnShotgunOneShotFinished()
{
	bPlayingOneShotShotgunAnim = false;
	UpdateShotgunLocomotionAnimation(true);
}

void AHuntPlayerCharacter::PlayShotgunIdleAnimation()
{
	if (ShotgunCharacterIdleAnim || ShotgunWeaponIdleAnim)
	{
		PlayShotgunVisualAnimation(ShotgunCharacterIdleAnim, ShotgunWeaponIdleAnim, 1.0f, true, true);
	}
	else if (ShotgunCharacterEquipAnim || ShotgunWeaponEquipAnim)
	{
		// Fallback: hold the take-up pose so hands stay on the weapon instead of returning to T-pose.
		PlayShotgunVisualAnimation(ShotgunCharacterEquipAnim, ShotgunWeaponEquipAnim, 0.0001f, true, true);
	}
}

void AHuntPlayerCharacter::PlayRandomShotgunFireAnimation()
{
	if (ShotgunCharacterFireAnims.Num() == 0 || ShotgunWeaponFireAnims.Num() == 0)
	{
		UAnimSequence* CharacterShoot = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Animated_DBShotgun/anims/dbman/ANIM_DB_shotgun_shoot_01.ANIM_DB_shotgun_shoot_01"));
		UAnimSequence* WeaponShoot = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Animated_DBShotgun/anims/dbshotgun/ANIM_dbshotgun_shoot_01.ANIM_dbshotgun_shoot_01"));
		PlayShotgunOneShotAnimation(CharacterShoot, WeaponShoot);
		return;
	}

	const int32 AnimIndex = FMath::RandRange(0, FMath::Min(ShotgunCharacterFireAnims.Num(), ShotgunWeaponFireAnims.Num()) - 1);
	PlayShotgunOneShotAnimation(ShotgunCharacterFireAnims[AnimIndex], ShotgunWeaponFireAnims[AnimIndex]);
}

EHuntShotgunLocomotion AHuntPlayerCharacter::ResolveDesiredLocomotion() const
{
	if (bDead || !bAcceptsGameplayInput)
	{
		return EHuntShotgunLocomotion::Idle;
	}

	const UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (MovementComp && MovementComp->IsFalling())
	{
		return EHuntShotgunLocomotion::Jump;
	}

	const float Speed2D = GetVelocity().Size2D();
	if (Speed2D < WalkLocomotionSpeed)
	{
		return EHuntShotgunLocomotion::Idle;
	}

	if (bSprinting)
	{
		return EHuntShotgunLocomotion::Sprint;
	}

	if (Speed2D >= RunLocomotionSpeed)
	{
		return EHuntShotgunLocomotion::Run;
	}

	return EHuntShotgunLocomotion::Walk;
}

UAnimSequence* AHuntPlayerCharacter::GetCharacterLocomotionAnim(EHuntShotgunLocomotion State) const
{
	switch (State)
	{
	case EHuntShotgunLocomotion::Walk:
		return ShotgunCharacterWalkAnim;
	case EHuntShotgunLocomotion::Run:
		// Fall back to walk if no run anim is hooked up.
		return ShotgunCharacterRunAnim ? ShotgunCharacterRunAnim.Get() : ShotgunCharacterWalkAnim.Get();
	case EHuntShotgunLocomotion::Sprint:
		return ShotgunCharacterSprintAnim ? ShotgunCharacterSprintAnim.Get()
			: (ShotgunCharacterRunAnim ? ShotgunCharacterRunAnim.Get() : ShotgunCharacterWalkAnim.Get());
	case EHuntShotgunLocomotion::Jump:
		return ShotgunCharacterJumpAnim;
	case EHuntShotgunLocomotion::Idle:
	default:
		return ShotgunCharacterIdleAnim;
	}
}

void AHuntPlayerCharacter::UpdateShotgunLocomotionAnimation(bool bForce)
{
	// Don't fight an in-flight one-shot animation (fire / reload / equip / dry-fire).
	// Once that finishes (OnShotgunOneShotFinished), we'll be called again with bForce = true.
	if (bPlayingOneShotShotgunAnim)
	{
		return;
	}

	if (!FirstPersonArmsMesh)
	{
		return;
	}

	const EHuntShotgunLocomotion Desired = ResolveDesiredLocomotion();
	if (!bForce && Desired == CurrentShotgunLocomotion)
	{
		return;
	}

	UAnimSequence* CharacterAnim = GetCharacterLocomotionAnim(Desired);
	if (!CharacterAnim)
	{
		// No matching clip -> at least keep an idle pose so we never pop back to T-pose.
		CharacterAnim = ShotgunCharacterIdleAnim;
	}

	// The resource pack only ships an idle for the gun mesh, so loop the same idle on the weapon side
	// regardless of which locomotion clip is playing on the arms. We loop the character clip too so
	// jump/walk poses persist while the player is still in that movement state.
	PlayShotgunVisualAnimation(CharacterAnim, ShotgunWeaponIdleAnim, 1.0f, true, true);

	CurrentShotgunLocomotion = Desired;
}

void AHuntPlayerCharacter::ResetShotgunVisualSetup()
{
	if (FirstPersonArmsMesh && FirstPersonArmsMesh->GetSkeletalMeshAsset())
	{
		// Re-attach to the camera so the arms follow look-up / look-down (pitch). Attaching to the
		// capsule only inherits yaw, which is exactly the bug where the arms stay level while the
		// camera tilts up or down.
		USceneComponent* AttachTarget = GetFirstPersonCameraComponent();
		if (!AttachTarget)
		{
			AttachTarget = GetCapsuleComponent();
		}
		FirstPersonArmsMesh->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform);
		FirstPersonArmsMesh->SetRelativeLocation(FirstPersonArmsLocation);
		FirstPersonArmsMesh->SetRelativeRotation(FirstPersonArmsRotation);
		FirstPersonArmsMesh->SetRelativeScale3D(FirstPersonArmsScale);
		FirstPersonArmsMesh->SetHiddenInGame(false);
		FirstPersonArmsMesh->SetVisibility(true, true);
		FirstPersonArmsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FirstPersonArmsMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

		// Hide bones that would block the first-person camera (head/neck/spine on the mannequin).
		if (FirstPersonArmsMesh->GetNumBones() > 0)
		{
			for (const FName BoneName : { FName(TEXT("head")), FName(TEXT("neck_01")), FName(TEXT("neck")) })
			{
				if (FirstPersonArmsMesh->GetBoneIndex(BoneName) != INDEX_NONE)
				{
					FirstPersonArmsMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
				}
			}
		}
	}

	if (ShotgunVisualMesh && FirstPersonArmsMesh && FirstPersonArmsMesh->GetSkeletalMeshAsset())
	{
		// Make sure runtime attachment matches the BP-time SetupAttachment to weapon_r, but DO NOT
		// overwrite the relative transform that the artist tuned in the BP viewport.
		FName ResolvedAttach = NAME_None;
		const TArray<FName> Candidates = {
			ShotgunAttachSocket,
			FName(TEXT("weapon_r")), FName(TEXT("weapon_socket")), FName(TEXT("WeaponSocket")), FName(TEXT("GripPoint")),
			FName(TEXT("hand_r")), FName(TEXT("RightHand")), FName(TEXT("Hand_R")), FName(TEXT("hand_R"))
		};
		for (const FName Candidate : Candidates)
		{
			if (Candidate != NAME_None && FirstPersonArmsMesh->DoesSocketExist(Candidate))
			{
				ResolvedAttach = Candidate;
				break;
			}
		}

		UE_LOG(LogTemp, Verbose, TEXT("[Hunt] Shotgun attach target: %s"), *ResolvedAttach.ToString());

		if (ResolvedAttach != NAME_None)
		{
			// KeepRelativeTransform: preserves whatever transform the BP designer set on the gun mesh component.
			ShotgunVisualMesh->AttachToComponent(FirstPersonArmsMesh, FAttachmentTransformRules::KeepRelativeTransform, ResolvedAttach);
		}
		ShotgunVisualMesh->SetHiddenInGame(false);
		ShotgunVisualMesh->SetVisibility(true, true);
		ShotgunVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShotgunVisualMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	}

	ApplyShotgunAimState(false);

	// Drive the locomotion state machine once so the arms start out in the correct pose
	// (idle by default, but also handles cases where the player is already moving).
	bPlayingOneShotShotgunAnim = false;
	UpdateShotgunLocomotionAnimation(true);
}

void AHuntPlayerCharacter::ApplyShotgunAimState(bool bNewAiming)
{
	bAimingShotgun = bNewAiming;
	GetWorldTimerManager().ClearTimer(ShotgunFireKickTimer);

	// Simplified aim: just narrow the FOV. The crosshair is drawn by the HUD so the player can see
	// exactly where the centre of the screen is pointing.
	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		Camera->SetFieldOfView(bAimingShotgun ? AimCameraFov : DefaultCameraFov);
	}
}

void AHuntPlayerCharacter::ApplyShotgunFireKick()
{
	if (!FirstPersonArmsMesh)
	{
		return;
	}

	// Push the arms back/up briefly; the gun follows because it is parented to a hand bone.
	const FVector BaseLocation = bAimingShotgun ? FirstPersonArmsAimLocation : FirstPersonArmsLocation;
	const FRotator BaseRotation = bAimingShotgun ? FirstPersonArmsAimRotation : FirstPersonArmsRotation;
	FirstPersonArmsMesh->SetRelativeLocation(BaseLocation + ShotgunFireKickLocationOffset);
	FirstPersonArmsMesh->SetRelativeRotation(BaseRotation + ShotgunFireKickRotationOffset);
	GetWorldTimerManager().SetTimer(ShotgunFireKickTimer, this, &AHuntPlayerCharacter::ResetShotgunFireKick, 0.14f, false);
}

void AHuntPlayerCharacter::ResetShotgunFireKick()
{
	ApplyShotgunAimState(bAimingShotgun);
}

void AHuntPlayerCharacter::OnHuntWeaponFired_Implementation(EHuntWeaponKind WeaponKind)
{
	// Blueprint extension point. Core shotgun visuals are played directly in C++ so BP overrides cannot suppress them.
}

void AHuntPlayerCharacter::OnHuntWeaponDryFired_Implementation(EHuntWeaponKind WeaponKind)
{
	// Blueprint extension point. Core shotgun visuals are played directly in C++ so BP overrides cannot suppress them.
}

void AHuntPlayerCharacter::OnHuntWeaponReloadStarted_Implementation(EHuntWeaponKind WeaponKind)
{
	// Blueprint extension point. Core shotgun visuals are played directly in C++ so BP overrides cannot suppress them.
}

void AHuntPlayerCharacter::OnHuntWeaponAmmoChanged_Implementation(EHuntWeaponKind WeaponKind, int32 AmmoInMagazine, int32 MagazineSize)
{
	// Blueprint children can use this to show/hide visible shells on the shotgun.
}

void AHuntPlayerCharacter::OnHuntWeaponEquipped_Implementation(EHuntWeaponKind WeaponKind)
{
	// Blueprint extension point. Core shotgun visuals are played directly in C++ so BP overrides cannot suppress them.
}

void AHuntPlayerCharacter::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	SetInputEnabledByGame(false);

	if (AHuntGameMode* HuntGameMode = GetWorld()->GetAuthGameMode<AHuntGameMode>())
	{
		HuntGameMode->PlayerDied(this);
	}
}
