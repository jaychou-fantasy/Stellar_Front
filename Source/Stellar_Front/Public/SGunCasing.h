// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGunCasing.generated.h"

class URotatingMovementComponent;
class UProjectileMovementComponent;

UCLASS()
class STELLAR_FRONT_API ASGunCasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ASGunCasing();
	UFUNCTION()
	
	void OnActorHit(UPrimitiveComponent* HitComponent ,AActor* OtherActor ,UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Mesh")
	UStaticMeshComponent* Casing;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Mesh")
	UProjectileMovementComponent* ProjectileMovementComp;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Mesh")
	URotatingMovementComponent* RotatingMovementComp;
	
	UPROPERTY(EditDefaultsOnly,Category = "Casing")
	USoundBase* CasingSound;
	
	float RotateRate;
	bool bHasHit = false;

public:	
	virtual void Tick(float DeltaTime) override;

};
