// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SAction.h"
#include "SAction_Fire.generated.h"

class APawn;

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API USAction_Fire : public USAction
{
	GENERATED_BODY()
public:
	
	virtual bool CanStart_Implementation(AActor* Instigator) override;
	virtual void StartAction_Implementation(AActor* Instigator) override;
	virtual void StopAction_Implementation(AActor* Instigator) override;
	
protected:
	FTimerHandle TimerHandle_FireDelay;
	UFUNCTION()
	void FireDelay_Elapsed(APawn* InstigatorPawn, bool bIsAiming);
};
