// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "SActionComponent.generated.h"

class USAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionStateChanged,USActionComponent*,OwningComp,USAction*,Action);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STELLAR_FRONT_API USActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USActionComponent();
	
	UPROPERTY(BlueprintAssignable)
	FOnActionStateChanged OnActionStarted;
	
	UPROPERTY(BlueprintAssignable)
	FOnActionStateChanged OnActionStopped;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Tags")
	FGameplayTagContainer ActiveGameplaytags;
	
	
	// add Action in "Actions" but don't start it
	// means you are "granted" this action
	UFUNCTION(BlueprintCallable,Category = "Actions")
	void AddActionFromClass(AActor* Instigator,TSubclassOf<USAction> ActionClass);

	UFUNCTION(BlueprintCallable,Category = "Actions")
	void RemoveAction(USAction* ActionToRemove);
	
	UFUNCTION(BlueprintCallable,Category = "Actions")
	USAction* GetAction(TSubclassOf<USAction> ActionClass) const;
	
	//means you are "using" this action
	//call "Start Action",add "GrantTags" to  "ActiveGameplayTags"(why set "byname" version here: to check whether satisfy the requirement of start action)
	UFUNCTION(BlueprintCallable,Category = "Actions")
	bool StartActionByName(AActor* Instigator, FName ActionName);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	bool StopActionByName(AActor* Instigator, FName ActionName);

	
	// To replicate a UObject's subobject, you must set replication on the component it's attached to
	bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	
protected:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//accomplish in beginplay 
	//Grant Abilities at GameStart
	UPROPERTY(EditAnywhere,Category = "Actions")
	TArray<TSubclassOf<USAction>> DefaultActionsClasses;
	
	UPROPERTY(BlueprintReadOnly,Replicated)
	TArray<USAction*> Actions;
	
	UFUNCTION(Server,Reliable)
	void ServerStartActionByName(AActor* Instigator,FName ActionName);
	
	UFUNCTION(Server,Reliable)
	void ServerStopActionByName(AActor* Instigator,FName ActionName);
	
	
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
