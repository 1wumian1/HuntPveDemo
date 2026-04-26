#include "HuntWeaponBase.h"
#include "HuntPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AHuntWeaponBase::AHuntWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AHuntWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	AmmoInMagazine = MagazineSize;
}

void AHuntWeaponBase::ConfigureAs(EHuntWeaponKind InKind)
{
	WeaponKind = InKind;

	if (WeaponKind == EHuntWeaponKind::Rifle)
	{
		WeaponName = TEXT("Rifle");
		Damage = 75.0f;
		FireInterval = 0.85f;
		Range = 18000.0f;
		SpreadDegrees = 0.25f;
		MagazineSize = 5;
		ReserveAmmo = 25;
		ReloadDuration = 1.9f;
	}
	else
	{
		WeaponName = TEXT("Pistol");
		Damage = 35.0f;
		FireInterval = 0.35f;
		Range = 12000.0f;
		SpreadDegrees = 0.75f;
		MagazineSize = 6;
		ReserveAmmo = 36;
		ReloadDuration = 1.25f;
	}

	AmmoInMagazine = MagazineSize;
}

void AHuntWeaponBase::SetOwningPlayer(AHuntPlayerCharacter* InOwner)
{
	OwningPlayer = InOwner;
	SetOwner(InOwner);
	SetInstigator(InOwner);
}

bool AHuntWeaponBase::CanFire() const
{
	const UWorld* World = GetWorld();
	return World && OwningPlayer && !bReloading && AmmoInMagazine > 0 && World->GetTimeSeconds() - LastFireTime >= FireInterval;
}

bool AHuntWeaponBase::TryFire()
{
	if (!OwningPlayer)
	{
		return false;
	}

	if (bReloading)
	{
		return false;
	}

	if (AmmoInMagazine <= 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EmptySound, OwningPlayer->GetActorLocation());
		StartReload();
		return false;
	}

	if (!CanFire())
	{
		return false;
	}

	UCameraComponent* Camera = OwningPlayer->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return false;
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
	--AmmoInMagazine;

	const FVector Start = Camera->GetComponentLocation();
	const FVector Forward = Camera->GetForwardVector();
	const FVector ShotDirection = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadDegrees));
	const FVector End = Start + ShotDirection * Range;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponTrace), true, OwningPlayer);
	GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (Hit.bBlockingHit)
	{
		ApplyHit(Hit, ShotDirection);
	}

	if (FireSounds.Num() > 0)
	{
		const int32 SoundIndex = FMath::RandRange(0, FireSounds.Num() - 1);
		UGameplayStatics::PlaySoundAtLocation(this, FireSounds[SoundIndex], OwningPlayer->GetActorLocation(), FMath::RandRange(0.92f, 1.08f));
	}

	if (APlayerController* PC = Cast<APlayerController>(OwningPlayer->GetController()))
	{
		if (FireCameraShake)
		{
			PC->ClientStartCameraShake(FireCameraShake);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugLine(GetWorld(), Start, Hit.bBlockingHit ? Hit.ImpactPoint : End, FColor::Orange, false, 0.08f, 0, 1.0f);
#endif

	if (AmmoInMagazine <= 0)
	{
		StartReload();
	}

	return true;
}

void AHuntWeaponBase::ApplyHit(const FHitResult& Hit, const FVector& ShotDirection)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		return;
	}

	FPointDamageEvent DamageEvent;
	DamageEvent.Damage = Damage;
	DamageEvent.HitInfo = Hit;
	DamageEvent.ShotDirection = ShotDirection;
	DamageEvent.DamageTypeClass = UDamageType::StaticClass();

	HitActor->TakeDamage(Damage, DamageEvent, OwningPlayer ? OwningPlayer->GetController() : nullptr, this);
	UGameplayStatics::PlaySoundAtLocation(this, HitSound, Hit.ImpactPoint);
}

void AHuntWeaponBase::StartReload()
{
	if (bReloading || AmmoInMagazine >= MagazineSize || ReserveAmmo <= 0)
	{
		return;
	}

	bReloading = true;
	UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, OwningPlayer ? OwningPlayer->GetActorLocation() : GetActorLocation());
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AHuntWeaponBase::FinishReload, ReloadDuration, false);
}

void AHuntWeaponBase::FinishReload()
{
	const int32 NeededAmmo = MagazineSize - AmmoInMagazine;
	const int32 LoadedAmmo = FMath::Min(NeededAmmo, ReserveAmmo);

	AmmoInMagazine += LoadedAmmo;
	ReserveAmmo -= LoadedAmmo;
	bReloading = false;
}
