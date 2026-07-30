// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SAction.h"
#include "SAction_Sprint.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API USAction_Sprint : public USAction
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 900.0f;

public:
	void StartAction_Implementation(AActor* Instigator) override;
	void StopAction_Implementation(AActor* Instigator) override;
};
