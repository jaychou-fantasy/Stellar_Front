// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SAction.h"
#include "SAction_Aim.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API USAction_Aim : public USAction
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AimSpeed = 300.0f;
	
	UPROPERTY(EditDefaultsOnly,Category = "Aim")
	float Aim_Fov = 75.0f;

	float TargetFov = 0.0f;

	FTimerHandle FovTimerHandle;
	void UpdateFov();

public:
	void StartAction_Implementation(AActor* Instigator) override;
	void StopAction_Implementation(AActor* Instigator) override;
};
