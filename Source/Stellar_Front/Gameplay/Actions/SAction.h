// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SAction.generated.h"

class USActionComponent;
/**
 * 
 */


USTRUCT()
struct FActionRepData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	bool bIsRunning;
	
	UPROPERTY()
	AActor* Instigator;
};


//that means you can directly create BP subclass,rather than create c++ subclass then Bp it.(but i choose the latter one,the former one is more suitable for short time debug)
UCLASS(Blueprintable)
class STELLAR_FRONT_API USAction : public UObject
{
	GENERATED_BODY()

public:
	//init belonged ActionComponent
	void Initialize(USActionComponent* NewActionComp);
	
	UPROPERTY(EditDefaultsOnly,Category = "Action")
	bool bAutoStart;
	
	UPROPERTY(EditDefaultsOnly,Category = "Action")
	FName ActionName;
	

	UFUNCTION(BlueprintCallable,Category = "Action")
	USActionComponent* GetOwningComponent() const;
	
	UFUNCTION(BlueprintCallable,Category = "Action")
	bool IsRunning() const ;
	
	UFUNCTION(BlueprintNativeEvent,Category = "Action")
	bool CanStart(AActor* Instigator);
	
	UFUNCTION(BlueprintNativeEvent,Category = "Action")
	void StartAction(AActor* Instigator);
	
	UFUNCTION(BlueprintNativeEvent,Category = "Action")
	void StopAction(AActor* Instigator);
	
	UWorld* GetWorld() const override;
	
	// Only UObject requires this; components derived from UActorComponent don't need it (but they do use SetIsReplicatedByDefault)
	// This function returns false by default (networking disabled)
	// Changing it to true enables networking
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
	
	
protected:
	// Components that absolutely need to participate in replication or Blueprint processes must be marked as UPROPERTY
	UPROPERTY(Replicated)
	USActionComponent* ActionComp;
	
	UPROPERTY(ReplicatedUsing="OnRep_RepData")
	FActionRepData RepData;
	
	UFUNCTION()
	void OnRep_RepData();
	
	UPROPERTY(Replicated)
	float TimeStarted;
	
	/* Tags added to owning actor when activated, removed when action stops */
	UPROPERTY(EditDefaultsOnly,Category = "Tags")
	FGameplayTagContainer GrantTags;
	
	/* Action can only start if OwningActor has none of these Tags applied */
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer BlockedTags;
};
