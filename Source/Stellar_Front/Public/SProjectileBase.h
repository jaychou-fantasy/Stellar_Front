// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SProjectileBase.generated.h"


class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;

class UAudioComponent;
class USoundCue;
class UCameraShakeBase;


UCLASS(ABSTRACT)
class STELLAR_FRONT_API ASProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:
	ASProjectileBase();

protected:
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Damage")
	float DamageAmount;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Gravity")
	float ProjectileGravity = 0.0f;
	
	/** Sphere collision component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Components")
	USphereComponent* SphereComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovementComp;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	UParticleSystemComponent* EffectComp;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	UAudioComponent* AudioComp;
	
	
	//impact visual effects,sound,shake
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* ImpactVFX;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	USoundCue* ImpactSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects|Shake")
	TSubclassOf<UCameraShakeBase> ImpactShake;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects|Shake")
	float ImpactShakeInnerRadius;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects|Shake")
	float ImpactShakeOuterRadius;

	
	
	/** called when projectile hits something */
	UFUNCTION()
	void OnActorHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	void Explode();
	
	virtual void PostInitializeComponents() override;
	
public:
	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return SphereComp; }

	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovementComp; }
};
