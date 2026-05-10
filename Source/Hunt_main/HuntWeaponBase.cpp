#include "HuntWeaponBase.h"
#include "HuntPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHuntWeaponBase::AHuntWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Pre-load default sound assets from the GunSoundPack so the weapon ships with audio without
	// any Blueprint configuration. You can still override these via the EditAnywhere properties
	// in a Blueprint subclass (or in the placed actor) if you want different cues.
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunShot01(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_shot_01_Cue.gun_shotgun_shot_01_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunShot02(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_shot_02_Cue.gun_shotgun_shot_02_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunShot03(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_shot_03_Cue.gun_shotgun_shot_03_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunShot04(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_shot_04_Cue.gun_shotgun_shot_04_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunDryFire(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_dry_fire_01_Cue.gun_shotgun_dry_fire_01_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ShotgunReload(TEXT("/Game/GunSoundPack/Guns/gun_shotgun_load_bullet_01_Cue.gun_shotgun_load_bullet_01_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> BodyImpact(TEXT("/Game/GunSoundPack/BonusSounds/punch_general_body_impact_01_Cue.punch_general_body_impact_01_Cue"));

	if (ShotgunShot01.Succeeded()) { FireSounds.Add(ShotgunShot01.Object); }
	if (ShotgunShot02.Succeeded()) { FireSounds.Add(ShotgunShot02.Object); }
	if (ShotgunShot03.Succeeded()) { FireSounds.Add(ShotgunShot03.Object); }
	if (ShotgunShot04.Succeeded()) { FireSounds.Add(ShotgunShot04.Object); }
	if (ShotgunDryFire.Succeeded()) { EmptySound = ShotgunDryFire.Object; }
	if (ShotgunReload.Succeeded()) { ReloadSound = ShotgunReload.Object; }
	if (BodyImpact.Succeeded()) { HitSound = BodyImpact.Object; }
}

void AHuntWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	AmmoInMagazine = MagazineSize;
}

void AHuntWeaponBase::ConfigureAs(EHuntWeaponKind InKind)
{
	WeaponKind = InKind;

	if (WeaponKind == EHuntWeaponKind::Katana)
	{
		WeaponName = TEXT("Katana");
		Damage = 95.0f;
		FireInterval = 0.7f;
		Range = 260.0f;
		SpreadDegrees = 0.0f;
		PelletCount = 1;
		MeleeRadius = 125.0f;
		MagazineSize = 0;
		ReserveAmmo = 0;
		ReloadDuration = 0.0f;
	}
	else
	{
		WeaponName = TEXT("Shotgun");
		Damage = 24.0f;
		FireInterval = 0.95f;
		Range = 6500.0f;
		SpreadDegrees = 6.5f;
		PelletCount = 9;
		MeleeRadius = 0.0f;
		MagazineSize = 2;
		// 后备弹药拉满到 1000，相当于演示期间几乎不会打光
		ReserveAmmo = 1000;
		ReloadDuration = 2.0f;
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
	return World && OwningPlayer && !bReloading && (IsMeleeWeapon() || AmmoInMagazine > 0) && World->GetTimeSeconds() - LastFireTime >= FireInterval;
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

	if (!IsMeleeWeapon() && AmmoInMagazine <= 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EmptySound, OwningPlayer->GetActorLocation());
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
	if (!IsMeleeWeapon())
	{
		--AmmoInMagazine;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector Forward = Camera->GetForwardVector();

	if (IsMeleeWeapon())
	{
		ApplyMeleeHit(Start, Start + Forward * Range);
	}
	else
	{
		const int32 ShotsToFire = FMath::Max(1, PelletCount);
		int32 DebugHitCount = 0;

		// 之前是细线 LineTrace + Pawn fallback，但只要先撞上墙/草丛/小石头，就根本不会再走 fallback，
		// 而且每颗弹丸只有一根细线，瞄准一两度偏差就完全擦不到怪。
		// 现在改成"球体扫描"：每颗弹丸像一颗小弹珠一样划过空间，命中体积大约是 16cm 半径，
		// 同时分别用 Visibility 和 Pawn 通道扫一遍，取距离更近的那个作为最终命中——
		// 这样既不会穿墙打人（墙近时取墙），也能命中那些不阻挡 Visibility 通道的角色 capsule。
		const float TraceRadius = PelletTraceRadius;
		const FCollisionShape SweepShape = FCollisionShape::MakeSphere(TraceRadius);

		for (int32 ShotIndex = 0; ShotIndex < ShotsToFire; ++ShotIndex)
		{
			const FVector ShotDirection = FMath::VRandCone(Forward, FMath::DegreesToRadians(SpreadDegrees));
			const FVector End = Start + ShotDirection * Range;

			FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponTrace), true, OwningPlayer);
			Params.AddIgnoredActor(this);
			Params.bReturnPhysicalMaterial = false;

			FHitResult VisibilityHit;
			FHitResult PawnHit;
			const bool bVisibilityHit = GetWorld()->SweepSingleByChannel(VisibilityHit, Start, End, FQuat::Identity, ECC_Visibility, SweepShape, Params);
			const bool bPawnHit = GetWorld()->SweepSingleByChannel(PawnHit, Start, End, FQuat::Identity, ECC_Pawn, SweepShape, Params);

			FHitResult Hit;
			if (bVisibilityHit && bPawnHit)
			{
				Hit = (PawnHit.Distance < VisibilityHit.Distance) ? PawnHit : VisibilityHit;
			}
			else if (bPawnHit)
			{
				Hit = PawnHit;
			}
			else if (bVisibilityHit)
			{
				Hit = VisibilityHit;
			}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			const FVector DebugEnd = Hit.bBlockingHit ? Hit.ImpactPoint : End;
			DrawDebugLine(GetWorld(), Start, DebugEnd, Hit.bBlockingHit ? FColor::Green : FColor::Red, false, 1.5f, 0, 0.5f);
			if (Hit.bBlockingHit)
			{
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 12.0f, FColor::Yellow, false, 1.5f);
			}
#endif

			if (Hit.bBlockingHit)
			{
				++DebugHitCount;
				ApplyHit(Hit, ShotDirection);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[Hunt] Shotgun fire: %d/%d pellets connected, ammo left=%d"), DebugHitCount, ShotsToFire, AmmoInMagazine);
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

	const float DamageDealt = HitActor->TakeDamage(Damage, DamageEvent, OwningPlayer ? OwningPlayer->GetController() : nullptr, this);
	UE_LOG(LogTemp, Log, TEXT("[Hunt] Shotgun pellet hit %s -> dealt %.1f damage (requested %.1f)"), *HitActor->GetName(), DamageDealt, Damage);
	UGameplayStatics::PlaySoundAtLocation(this, HitSound, Hit.ImpactPoint);
}

void AHuntWeaponBase::ApplyMeleeHit(const FVector& Start, const FVector& End)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HuntWeaponMeleeTrace), true, OwningPlayer);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(MeleeRadius), Params);

	TSet<AActor*> DamagedActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == OwningPlayer || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);
		UGameplayStatics::ApplyDamage(HitActor, Damage, OwningPlayer ? OwningPlayer->GetController() : nullptr, this, UDamageType::StaticClass());
		const FVector SoundLocation = Hit.ImpactPoint.IsNearlyZero() ? HitActor->GetActorLocation() : FVector(Hit.ImpactPoint);
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, SoundLocation);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugSphere(GetWorld(), End, MeleeRadius, 16, FColor::Red, false, 0.08f);
#endif
}

bool AHuntWeaponBase::IsMeleeWeapon() const
{
	return WeaponKind == EHuntWeaponKind::Katana;
}

bool AHuntWeaponBase::UsesAmmo() const
{
	return !IsMeleeWeapon();
}

void AHuntWeaponBase::StartReload()
{
	if (IsMeleeWeapon() || bReloading || AmmoInMagazine >= MagazineSize || ReserveAmmo <= 0)
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

	if (OwningPlayer)
	{
		OwningPlayer->NotifyWeaponAmmoChanged(this);
	}
}
