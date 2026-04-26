#include "HuntPlayerCharacter.h"
#include "HuntClueActor.h"
#include "HuntExtractionPoint.h"
#include "HuntGameMode.h"
#include "HuntWeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AHuntPlayerCharacter::AHuntPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// The template camera expects art sockets that the C++ prototype does not have yet.
	// Put the camera on the capsule so the debug level is visible without imported meshes.
	if (UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		Camera->SetupAttachment(GetCapsuleComponent());
		Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->bUsePawnControlRotation = true;
	}

	DebugHeadLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Debug Head Light"));
	DebugHeadLight->SetupAttachment(GetFirstPersonCameraComponent());
	DebugHeadLight->SetRelativeLocation(FVector::ZeroVector);
	DebugHeadLight->SetRelativeRotation(FRotator::ZeroRotator);
	DebugHeadLight->SetIntensity(80000.0f);
	DebugHeadLight->SetAttenuationRadius(5000.0f);
	DebugHeadLight->SetInnerConeAngle(22.0f);
	DebugHeadLight->SetOuterConeAngle(48.0f);
}

void AHuntPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	SpawnDefaultWeapons();
	SetInputEnabledByGame(false);
	LastFootstepLocation = GetActorLocation();
}

void AHuntPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DamageFlashAlpha > 0.0f)
	{
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - DeltaSeconds * 1.8f);
	}

	if (bAcceptsGameplayInput && !bDead && GetVelocity().Size2D() > 150.0f && GetCharacterMovement()->IsMovingOnGround())
	{
		DistanceSinceLastFootstep += FVector::Dist2D(GetActorLocation(), LastFootstepLocation);
		LastFootstepLocation = GetActorLocation();

		if (DistanceSinceLastFootstep >= 220.0f)
		{
			DistanceSinceLastFootstep = 0.0f;
			USoundBase* FootstepSound = GetActorLocation().X > 0.0f ? WoodFootstepSound : MudFootstepSound;
			UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation(), FMath::RandRange(0.9f, 1.1f));
		}
	}
	else
	{
		LastFootstepLocation = GetActorLocation();
	}
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
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AHuntPlayerCharacter::Interact);
	PlayerInputComponent->BindAction(TEXT("UseHeal"), IE_Pressed, this, &AHuntPlayerCharacter::UseHeal);
	PlayerInputComponent->BindAction(TEXT("ThrowExplosive"), IE_Pressed, this, &AHuntPlayerCharacter::ThrowExplosive);
	PlayerInputComponent->BindAction(TEXT("DarkSight"), IE_Pressed, this, &AHuntPlayerCharacter::StartDarkSight);
	PlayerInputComponent->BindAction(TEXT("DarkSight"), IE_Released, this, &AHuntPlayerCharacter::StopDarkSight);
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

void AHuntPlayerCharacter::AddExplosiveCharge(int32 Amount)
{
	ExplosiveCharges = FMath::Max(0, ExplosiveCharges + Amount);
}

void AHuntPlayerCharacter::SetInputEnabledByGame(bool bEnabled)
{
	bAcceptsGameplayInput = bEnabled;
	GetCharacterMovement()->MaxWalkSpeed = bEnabled ? 600.0f : 0.0f;
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
	if (bAcceptsGameplayInput && !bDead && CurrentWeapon)
	{
		CurrentWeapon->TryFire();
	}
}

void AHuntPlayerCharacter::Reload()
{
	if (bAcceptsGameplayInput && !bDead && CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

void AHuntPlayerCharacter::SwitchWeapon()
{
	if (bAcceptsGameplayInput && !bDead && Weapons.Num() > 1)
	{
		EquipWeapon((CurrentWeaponIndex + 1) % Weapons.Num());
	}
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
	if (bAcceptsGameplayInput && HealCharges > 0 && CurrentHealth < MaxHealth && !bDead)
	{
		--HealCharges;
		Heal(HealAmount);
	}
}

void AHuntPlayerCharacter::ThrowExplosive()
{
	if (!bAcceptsGameplayInput || ExplosiveCharges <= 0 || bDead)
	{
		return;
	}

	--ExplosiveCharges;
	const FVector Origin = GetActorLocation() + GetActorForwardVector() * 300.0f;

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);
	UGameplayStatics::ApplyRadialDamage(this, ExplosiveDamage, Origin, ExplosiveRange, UDamageType::StaticClass(), IgnoredActors, this, GetController(), true);
	UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Origin);
}

void AHuntPlayerCharacter::StartDarkSight()
{
	bDarkSightActive = true;

	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		It->SetDarkSightHighlighted(true);
	}
}

void AHuntPlayerCharacter::StopDarkSight()
{
	bDarkSightActive = false;

	for (TActorIterator<AHuntClueActor> It(GetWorld()); It; ++It)
	{
		It->SetDarkSightHighlighted(false);
	}
}

void AHuntPlayerCharacter::SpawnDefaultWeapons()
{
	if (!GetWorld())
	{
		return;
	}

	for (const EHuntWeaponKind Kind : { EHuntWeaponKind::Pistol, EHuntWeaponKind::Rifle })
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AHuntWeaponBase* Weapon = GetWorld()->SpawnActor<AHuntWeaponBase>(AHuntWeaponBase::StaticClass(), GetActorTransform(), Params);
		if (Weapon)
		{
			Weapon->ConfigureAs(Kind);
			Weapon->SetOwningPlayer(this);
			Weapon->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			Weapons.Add(Weapon);
		}
	}

	EquipWeapon(0);
}

void AHuntPlayerCharacter::EquipWeapon(int32 NewIndex)
{
	if (!Weapons.IsValidIndex(NewIndex))
	{
		return;
	}

	CurrentWeaponIndex = NewIndex;
	CurrentWeapon = Weapons[CurrentWeaponIndex];
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
