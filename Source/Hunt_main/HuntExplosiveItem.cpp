#include "HuntExplosiveItem.h"
#include "HuntPlayerCharacter.h"
#include "Components/SphereComponent.h"

AHuntExplosiveItem::AHuntExplosiveItem()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	RootComponent = PickupSphere;
	PickupSphere->InitSphereRadius(65.0f);
	PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
}

void AHuntExplosiveItem::BeginPlay()
{
	Super::BeginPlay();

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AHuntExplosiveItem::OnPickupOverlap);
}

void AHuntExplosiveItem::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AHuntPlayerCharacter* Player = Cast<AHuntPlayerCharacter>(OtherActor))
	{
		Player->AddExplosiveCharge(Charges);
		Destroy();
	}
}
