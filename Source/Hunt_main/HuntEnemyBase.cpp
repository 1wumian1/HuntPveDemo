#include "HuntEnemyBase.h"
#include "HuntPlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AHuntEnemyBase::AHuntEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
	// 让小怪在追玩家时会顺势转身朝向，配合移动动画看起来更自然
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	bUseControllerRotationYaw = false;

	// 默认骨骼网格：使用项目里 ZombiePackV1 自带的 SK_ZombieAA。蓝图里仍然可以覆盖。
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// UE Character 的 mesh 默认要往下偏移 capsule 半高，朝向旋转 -90 度才能正常站立
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -92.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> ZombieMeshAsset(TEXT("/Game/ZombiePackV1/ZombieAA/Mesh/SK_ZombieAA.SK_ZombieAA"));
		if (ZombieMeshAsset.Succeeded())
		{
			MeshComp->SetSkeletalMesh(ZombieMeshAsset.Object);
		}

		// 默认动画蓝图：用 ZombiePack 自带的 ThirdPerson_AnimBP_SNP，里面接好了 Idle/Walk/Run 状态机，
		// 角色一移动就会自动播放对应动作。
		static ConstructorHelpers::FClassFinder<UAnimInstance> ZombieAnimBPClass(TEXT("/Game/ZombiePackV1/DemoContent/Animations/ThirdPerson_AnimBP_SNP"));
		if (ZombieAnimBPClass.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(ZombieAnimBPClass.Class);
		}
	}

	// Default audio cues using the Sword_Fighting_SFX battle roar pack for hit reactions
	// and the GunSoundPack body-impact cues for melee swings. Override on a BP to taste.
	static ConstructorHelpers::FObjectFinder<USoundBase> EnemyHitCue(TEXT("/Game/Sword_Fighting_SFX/Cues/Battle_Roar_3_Cue.Battle_Roar_3_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> EnemyAttackCue(TEXT("/Game/GunSoundPack/BonusSounds/punch_slap_whack_hit_01_Cue.punch_slap_whack_hit_01_Cue"));

	if (EnemyHitCue.Succeeded()) { HitSound = EnemyHitCue.Object; }
	if (EnemyAttackCue.Succeeded()) { AttackSound = EnemyAttackCue.Object; }
}

void AHuntEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		OriginalMeshScale = MeshComp->GetRelativeScale3D();
	}
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

	if (!CanDetectPlayer(Target, Distance))
	{
		return;
	}

	if (Distance > AttackRange)
	{
		const FVector MoveDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
		AddMovementInput(MoveDirection, 1.0f);
		// bOrientRotationToMovement 已经会让角色朝向移动方向，这里不再硬扳 actor 旋转，
		// 否则会与 CharacterMovement 的平滑转身打架，导致动画卡顿/原地旋转
	}
	else if (GetWorld()->GetTimeSeconds() - LastAttackTime >= AttackInterval)
	{
		LastAttackTime = GetWorld()->GetTimeSeconds();

		// 攻击瞬间也面向玩家，让攻击动画/前冲表现更可信
		FVector FacingDir = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
		if (!FacingDir.IsNearlyZero())
		{
			SetActorRotation(FacingDir.Rotation());
		}

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
		PlayAttackAnimation();
	}
}

void AHuntEnemyBase::PlayAttackAnimation()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// 蓝图里如果配置了攻击 Montage，就优先用 Montage（视觉最好）
	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (AttackMontage && AnimInstance)
	{
		AnimInstance->Montage_Play(AttackMontage, AttackMontagePlayRate);
		return;
	}

	// 否则用 C++ 兜底表现：短暂向玩家方向前冲 + 网格放大一点点，呈现"扑击"的攻击节奏
	StartFallbackAttackImpulse();
}

void AHuntEnemyBase::StartFallbackAttackImpulse()
{
	if (FallbackAttackLungeDistance > 0.0f)
	{
		const FVector LungeImpulse = GetActorForwardVector() * (FallbackAttackLungeDistance / FMath::Max(FallbackAttackDuration, 0.05f));
		// 用 Launch 比直接 SetActorLocation 更安全，会被 CharacterMovement 接管不会穿墙
		LaunchCharacter(FVector(LungeImpulse.X, LungeImpulse.Y, 0.0f), true, true);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeScale3D(OriginalMeshScale * FallbackAttackMeshScalePunch);
	}

	GetWorldTimerManager().ClearTimer(FallbackAttackTimer);
	GetWorldTimerManager().SetTimer(FallbackAttackTimer, this, &AHuntEnemyBase::EndFallbackAttackImpulse, FallbackAttackDuration, false);
}

void AHuntEnemyBase::EndFallbackAttackImpulse()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeScale3D(OriginalMeshScale);
	}
}

AHuntPlayerCharacter* AHuntEnemyBase::FindTargetPlayer() const
{
	return Cast<AHuntPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

bool AHuntEnemyBase::CanDetectPlayer(AHuntPlayerCharacter* Target, float Distance)
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Distance <= DirectDetectionRange)
	{
		LastDetectedPlayerTime = CurrentTime;
		return true;
	}

	if (Target && Target->HasRecentFootstepNoise(CurrentTime))
	{
		const float DistanceToNoise = FVector::Dist2D(GetActorLocation(), Target->GetLastFootstepNoiseLocation());
		if (DistanceToNoise <= Target->GetLastFootstepNoiseRange())
		{
			LastDetectedPlayerTime = CurrentTime;
			return true;
		}
	}

	return CurrentTime - LastDetectedPlayerTime <= AlertMemoryDuration;
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
	GetWorldTimerManager().ClearTimer(FallbackAttackTimer);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeScale3D(OriginalMeshScale);
	}

	SetLifeSpan(DespawnDelay);
}
