// Fill out your copyright notice in the Description page of Project Settings.


#include "SProj_MagicBullet.h"

#include "Components/SphereComponent.h"

ASProj_MagicBullet::ASProj_MagicBullet()
{
	SphereComp->SetSphereRadius(20.0f);
	SphereComp->OnComponentBeginOverlap.AddDynamic(this,&ASProj_MagicBullet::OnActorOverlap);
	
	InitialLifeSpan = 10.0f;
}

void ASProj_MagicBullet::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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

		Explode();
	}
}
