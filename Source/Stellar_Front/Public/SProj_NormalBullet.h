// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SProjectileBase.h"
#include "SProj_NormalBullet.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API ASProj_NormalBullet : public ASProjectileBase
{
	GENERATED_BODY()

public:
	ASProj_NormalBullet();
	
protected:
	
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
