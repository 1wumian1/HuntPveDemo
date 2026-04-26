#include "HuntClueActor.h"
#include "HuntGameMode.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AHuntClueActor::AHuntClueActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	RootComponent = InteractionSphere;
	InteractionSphere->InitSphereRadius(75.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(RootComponent);
	GlowLight->SetLightColor(FLinearColor(0.1f, 0.8f, 1.0f));
	GlowLight->SetIntensity(5500.0f);
	GlowLight->SetAttenuationRadius(450.0f);
}

void AHuntClueActor::Collect(AHuntPlayerCharacter* Player)
{
	if (bCollected)
	{
		return;
	}

	bCollected = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	UGameplayStatics::PlaySoundAtLocation(this, CollectSound, GetActorLocation());

	if (AHuntGameMode* HuntGameMode = GetWorld()->GetAuthGameMode<AHuntGameMode>())
	{
		HuntGameMode->RegisterClueCollected(this);
	}
}

void AHuntClueActor::SetDarkSightHighlighted(bool bHighlighted)
{
	if (!bCollected && GlowLight)
	{
		GlowLight->SetIntensity(bHighlighted ? 12000.0f : 5500.0f);
	}
}
