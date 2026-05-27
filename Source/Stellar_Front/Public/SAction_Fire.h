// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SAction.h"
#include "SAction_Fire.generated.h"

class UParticleSystem;
class ASProjectileBase;

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API USAction_Fire : public USAction
{
	GENERATED_BODY()
public:
	USAction_Fire();
	
	virtual void StartAction_Implementation(AActor* Instigator) override;
	
protected:
	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category="Fire")
	TSubclassOf<ASProjectileBase> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditDefaultsOnly, Category="Fire")
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	UAnimSequence* FireAnimation;//----just an mp4 file  || while AnimMontage is actually a mp4_control_system 
	//UAnimMontage* FireAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	UParticleSystem* MuzzleFlash;
	
	UPROPERTY(EditAnywhere,Category = "Fire")
	float FireAnimDelay;
	
	UPROPERTY(VisibleAnywhere,Category = "Fire")
	FName GunMuzzleName;
	
	UPROPERTY(VisibleAnywhere,Category = "Fire")
	FName ArmSlotName;
	
	UFUNCTION()
	void FireDelay_Elapsed(ASCharacter* InstigatorCharacter);
};
