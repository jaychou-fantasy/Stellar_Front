// Fill out your copyright notice in the Description page of Project Settings.


#include "SProj_NormalBullet.h"
#include "Components/SphereComponent.h"
#include "SAttributeComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ASProj_NormalBullet::ASProj_NormalBullet()
{
	SphereComp->SetSphereRadius(20.0f);
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ASProj_NormalBullet::OnActorOverlap);
	
	InitialLifeSpan = 10.0f;
	DamageAmount = 20.0f;
}

void ASProj_NormalBullet::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}
