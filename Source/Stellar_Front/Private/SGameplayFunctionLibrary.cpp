// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameplayFunctionLibrary.h"
#include "SAttributeComponent.h"

bool USGameplayFunctionLibrary::ApplyDamage(AActor* DamageCauser, AActor* TargetActor, float DamageAmount)
{
	USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributeComp(TargetActor);
	if (AttributeComp)
	{
		return AttributeComp->ApplyHealthchange(DamageCauser,-DamageAmount);
	}
	return false;
}


bool USGameplayFunctionLibrary::ApplyDamageAndImpulse(AActor* DamageCauser, AActor* TargetActor, float DamageAmount,const FHitResult& HitResult)
{
	//first call "Apply Damage" for Health Change
	if (ApplyDamage(DamageCauser,TargetActor,DamageAmount))
	{
		UPrimitiveComponent* HitComp = HitResult.GetComponent();
		if (HitComp && HitComp->IsSimulatingPhysics(HitResult.BoneName))
		{
			FVector HitDirection  = HitResult.TraceEnd - HitResult.TraceStart;
			HitDirection.Normalize();//return a Unit Vector.(eg.vertical and horizontal)
			HitComp->AddImpulseAtLocation(HitDirection*300000.0f,HitResult.ImpactPoint,HitResult.BoneName);
			UE_LOG(LogTemp, Warning, TEXT("Fire impulse!"))
		}
		//apply damage::your first duty
		//apply impulse::incidental duty
		return true;
	}
	return false;
}
