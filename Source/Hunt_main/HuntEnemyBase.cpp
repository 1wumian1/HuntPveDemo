#include "HuntEnemyBase.h"
#include "HuntPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AHuntEnemyBase::AHuntEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AHuntEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
}

void AHuntEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		return;
	}

	AHuntPlayerCharacter* Target = FindTargetPlayer();
	if (!Target || Target->IsDead())
	{
		return;
	}

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float Distance = ToTarget.Size2D();

	if (Distance > AttackRange)
	{
		const FVector MoveDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
		AddMovementInput(MoveDirection, 1.0f);
		SetActorRotation(MoveDirection.Rotation());
	}
	else if (GetWorld()->GetTimeSeconds() - LastAttackTime >= AttackInterval)
	{
		LastAttackTime = GetWorld()->GetTimeSeconds();
		AttackTarget();
	}
}

float AHuntEnemyBase::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || Damage <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return Damage;
}

void AHuntEnemyBase::AttackTarget()
{
	if (AHuntPlayerCharacter* Target = FindTargetPlayer())
	{
		UGameplayStatics::ApplyDamage(Target, MeleeDamage, GetController(), this, UDamageType::StaticClass());
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}
}

AHuntPlayerCharacter* AHuntEnemyBase::FindTargetPlayer() const
{
	return Cast<AHuntPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

void AHuntEnemyBase::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	SetLifeSpan(DespawnDelay);
}
