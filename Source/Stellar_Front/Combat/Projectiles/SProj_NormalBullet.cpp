// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Projectiles/SProj_NormalBullet.h"
#include "Components/SphereComponent.h"
#include "Gameplay/Attributes/SAttributeComponent.h"
#include "Gameplay/FunctionLibrary/SGameplayFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

ASProj_NormalBullet::ASProj_NormalBullet()
{
	SphereComp->SetSphereRadius(1.0f);
	SphereComp->OnComponentHit.RemoveDynamic(this, &ASProj_NormalBullet::OnActorHit);
	SphereComp->OnComponentHit.AddDynamic(this, &ASProj_NormalBullet::OnProjectileHit);
	
	InitialLifeSpan = 10.0f;
	DamageAmount = 20.0f;
}

void ASProj_NormalBullet::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//we set this "Instigator" in "SpawnActor<>()" s
	if (OtherActor && OtherActor != GetInstigator())
	{
		USGameplayFunctionLibrary::ApplyDamageAndImpulse(GetInstigator(), OtherActor, DamageAmount, Hit);
	}

	ASProjectileBase::OnActorHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
