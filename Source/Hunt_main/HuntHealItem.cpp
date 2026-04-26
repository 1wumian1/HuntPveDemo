#include "HuntHealItem.h"
#include "HuntPlayerCharacter.h"
#include "Components/SphereComponent.h"

AHuntHealItem::AHuntHealItem()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	RootComponent = PickupSphere;
	PickupSphere->InitSphereRadius(65.0f);
	PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
}

void AHuntHealItem::BeginPlay()
{
	Super::BeginPlay();

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AHuntHealItem::OnPickupOverlap);
}

void AHuntHealItem::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AHuntPlayerCharacter* Player = Cast<AHuntPlayerCharacter>(OtherActor))
	{
		Player->Heal(HealAmount);
		Destroy();
	}
}
