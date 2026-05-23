// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SProjectileBase.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASProjectileBase::ASProjectileBase()
{
	// Use a sphere as a simple collision representation
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetCollisionProfileName("Projectile");
	/*************///Assign on "OnComponentHit"
	SphereComp->OnComponentHit.AddDynamic(this, &ASProjectileBase::OnActorHit);	// set up a notification for when this component hits something blocking
	RootComponent = SphereComp;
	
	// Players can't walk on it
	SphereComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	SphereComp->CanCharacterStepUpOn = ECB_No;

	EffectComp = CreateDefaultSubobject<UParticleSystemComponent>("EffectComp");
	EffectComp->SetupAttachment(RootComponent);
	
	AudioComp = CreateDefaultSubobject<UAudioComponent>("AudioComp");
	AudioComp->SetupAttachment(RootComponent);
	
	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	
	//updateComponent--->the comp truly be controlled to move
	ProjectileMovementComp->UpdatedComponent = SphereComp;
	ProjectileMovementComp->InitialSpeed = 3000.f;
	//ProjectileMovementComp->MaxSpeed = 3000.f;
	
	//ProjectileMovementComp->bShouldBounce = true;
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	
	//i want it to be override in different guns and bullets
	ProjectileMovementComp->ProjectileGravityScale = ProjectileGravity;
	
	
	
	ImpactShakeInnerRadius = 0.0f;
	ImpactShakeOuterRadius = 1500.0f;
}

void ASProjectileBase::OnActorHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Explode();
}

void ASProjectileBase::Explode_Implementation()
{
	//if (!this->IsPendingKillPending())
	if (ImpactVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactVFX, GetActorLocation(), FRotator::ZeroRotator, FVector(5.0f));
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this,ImpactSound,GetActorLocation());
	}
	if (ImpactShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this,ImpactShake,GetActorLocation(),ImpactShakeInnerRadius,ImpactShakeOuterRadius);
	}
	
	Destroy();
	
}


void ASProjectileBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
