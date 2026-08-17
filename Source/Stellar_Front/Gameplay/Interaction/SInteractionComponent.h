// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SInteractionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STELLAR_FRONT_API USInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	void PrimaryInteract();
	
	USInteractionComponent();

protected:
	UFUNCTION(Server,Reliable)
	void ServerInteract(AActor* InFocus);
	
	void FindBestInteractable();
	
	UPROPERTY()
	AActor* FocusedActor;
	
	// The next three variables are for more flexible tweaking of interaction range + the collision type of interactable objects
	UPROPERTY(EditDefaultsOnly, Category = "Trace")
	float TraceDistance;
	
	UPROPERTY(EditDefaultsOnly, Category = "Trace")
	float TraceRadius;
	
	// This is an enum value::the collision channel that you wanna query about
	// ECC_Visibility, ECC_Pawn, ECC_WorldDynamic are examples of its enum values, hence we use TEnumByte<>
	//ECollisionChannel is enum;  CollisionChannel is a variable,has multiple possibility but can only be one  concrete value
	UPROPERTY(EditDefaultsOnly, Category = "Trace")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	
	
	
	
	// Called when the game starts
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
