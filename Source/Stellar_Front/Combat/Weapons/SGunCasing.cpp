// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Weapons/SGunCasing.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASGunCasing::ASGunCasing()
{
	Casing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing"));
	SetRootComponent(Casing);
	Casing->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	// Casing->SetGenerateOverlapEvents(true);
	Casing->OnComponentHit.AddDynamic(this,&ASGunCasing::OnActorHit);

	
	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->UpdatedComponent = Casing;// along with casing component
	ProjectileMovementComp->InitialSpeed = 500.0f;
	ProjectileMovementComp->ProjectileGravityScale = 2.0f;
	ProjectileMovementComp->bShouldBounce = true;
	ProjectileMovementComp->Bounciness = 0.5f;
	
	RotatingMovementComp = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComp"));
	
	SetLifeSpan(2.0f);

}

void ASGunCasing::OnActorHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bHasHit)
	{
		UGameplayStatics::PlaySoundAtLocation(this,CasingSound,GetActorLocation(),0.5f);
		bHasHit = true;
		RotatingMovementComp->RotationRate = FRotator::ZeroRotator;
	}
}


void ASGunCasing::BeginPlay()
{
	Super::BeginPlay();
	
	RotateRate = FMath::RandRange(-1800.0f,1800.0f);
	RotatingMovementComp->RotationRate = FRotator(RotateRate,RotateRate,RotateRate);
	
	
}

void ASGunCasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
