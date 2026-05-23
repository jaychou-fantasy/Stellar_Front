// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SGameplayFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API USGameplayFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable,Category = "Gameplay")
	static bool ApplyDamage(AActor* DamageCauser,AActor* TargetActor,float DamageAmount);
	
	UFUNCTION(BlueprintCallable,Category = "Gameplay")
	static bool ApplyDamageAndImpulse(AActor* DamageCauser,AActor* TargetActor,float DamageAmount,const FHitResult& HitResult);
	// If using & (reference) in Blueprint, it becomes an output pin, but adding const turns it back into an input pin while still retaining the reference behavior
	//so for the huge structure,you'd better use const+&  to avoid big waste of copy and keep it unchangable at same time
};
