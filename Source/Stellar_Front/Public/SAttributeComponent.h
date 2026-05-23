// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged,AActor* ,InstigatorActor,USAttributeComponent*,OwningComp,float ,NewHealth,float,Delta);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STELLAR_FRONT_API USAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USAttributeComponent();
	
	//static function for other Component use
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	static USAttributeComponent* GetAttributeComp(AActor* FromActor);

	UFUNCTION(BlueprintCallable,Category = "Attributes")
	static bool IsActorAlive(AActor* CheckActor);
	
protected:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Replicated,Category = "Attributes")
	float Health;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Replicated,Category = "Attributes")
	float HealthMax;
	
	//multicast FOnHealthChanged
	UFUNCTION(NetMulticast,Reliable)//@fixme:note as unreliable as you move the "state" out of SCharacter
	void MulticastHealthChanged(AActor* Instigator,float NewHealth,float Delta);
	
public:
	UPROPERTY(BlueprintAssignable,Category = "Attributes")
	FOnHealthChanged OnHealthChanged;
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	bool ApplyHealthchange(AActor* InstigatorActor,float Delta);
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	bool IsAlive() const;
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	bool IsFullHealth() const;
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	float GetHealthMax() const;
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	float GetHealth() const;
	
	UFUNCTION(BlueprintCallable,Category = "Attributes")
	bool Kill(AActor* Instigator);
};
