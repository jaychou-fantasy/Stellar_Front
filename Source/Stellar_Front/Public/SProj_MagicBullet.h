// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SProjectileBase.h"
#include "SProj_MagicBullet.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API ASProj_MagicBullet : public ASProjectileBase
{
	GENERATED_BODY()
public:
	ASProj_MagicBullet();
	
protected:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
