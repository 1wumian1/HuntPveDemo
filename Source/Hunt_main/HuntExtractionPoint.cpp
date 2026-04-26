#include "HuntExtractionPoint.h"
#include "HuntGameMode.h"
#include "HuntPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

AHuntExtractionPoint::AHuntExtractionPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	ExtractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionVolume"));
	RootComponent = ExtractionVolume;
	ExtractionVolume->SetBoxExtent(FVector(250.0f, 250.0f, 160.0f));
	ExtractionVolume->SetCollisionProfileName(TEXT("Trigger"));

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(RootComponent);
	BeaconLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.2f));
	BeaconLight->SetIntensity(0.0f);
	BeaconLight->SetAttenuationRadius(850.0f);
}

void AHuntExtractionPoint::BeginPlay()
{
	Super::BeginPlay();

	ExtractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AHuntExtractionPoint::OnExtractionOverlap);
	SetActiveExtraction(false);
}

void AHuntExtractionPoint::TryExtract(AHuntPlayerCharacter* Player)
{
	if (!Player || !bActive)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, ExtractSound, GetActorLocation());
	if (AHuntGameMode* HuntGameMode = GetWorld()->GetAuthGameMode<AHuntGameMode>())
	{
		HuntGameMode->PlayerReachedExtraction(Player);
	}
}

void AHuntExtractionPoint::SetActiveExtraction(bool bInActive)
{
	bActive = bInActive;
	BeaconLight->SetIntensity(bActive ? 9000.0f : 500.0f);
}

void AHuntExtractionPoint::OnExtractionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryExtract(Cast<AHuntPlayerCharacter>(OtherActor));
}
