// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Projectiles/SProj_MagicBullet.h"

#include "Components/SphereComponent.h"

ASProj_MagicBullet::ASProj_MagicBullet()
{
	SphereComp->SetSphereRadius(1.0f);
	SphereComp->OnComponentHit.RemoveDynamic(this, &ASProj_MagicBullet::OnActorHit);
	SphereComp->OnComponentHit.AddDynamic(this, &ASProj_MagicBullet::OnProjectileHit);
	
	InitialLifeSpan = 10.0f;
}

void ASProj_MagicBullet::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics object
	if ((OtherActor) && (OtherActor != this) && (OtherComp) && OtherComp->IsSimulatingPhysics())
	{
		float RandomIntensity = FMath::RandRange(200.0f, 500.0f);

		OtherComp->AddImpulseAtLocation(GetVelocity() * RandomIntensity, GetActorLocation());

		FVector Scale = OtherComp->GetComponentScale();
		Scale *= 0.8f;

		if (Scale.GetMin() < 0.5f)
		{
			OtherActor->Destroy();
		}
		else
		{
			OtherComp->SetWorldScale3D(Scale);
		}

		UMaterialInstanceDynamic* MatInst = OtherComp->CreateDynamicMaterialInstance(0);
		if (MatInst)
		{
			MatInst->SetVectorParameterValue("Color", FLinearColor::MakeRandomColor());
		}

	}

	ASProjectileBase::OnActorHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
