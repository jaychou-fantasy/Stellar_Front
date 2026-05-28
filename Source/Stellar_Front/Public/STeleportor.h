// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SGameplayInterface.h"
#include "GameFramework/Actor.h"
#include "STeleportor.generated.h"

class UStaticMeshComponent;
class UArrowComponent;

UCLASS()
class STELLAR_FRONT_API ASTeleportor : public AActor , public ISGameplayInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASTeleportor();

	// UPROPERTY(EditAnywhere)
	// FVector2D TargetLocation;
	
	void Interact_Implementation(APawn* InstigatorPawn) override;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category  = "Teleport")
	ASTeleportor* TargetTeleportor;

	UArrowComponent* GetArrow() const { return ArrowComp ;}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;
	
	UPROPERTY(VisibleAnywhere)
	UArrowComponent* ArrowComp;
	
	//now we stipulate can only teleport between these two location
	/*@fixme: at our planet,we can only teleport to the space station
	when at space station, we can teleport to space elevator & our planet & rival's planet
	*/
	
	//@fixme: set it to Replicated
	UPROPERTY(BlueprintReadOnly)
	bool bTeleportedTo = false;
	
	UPROPERTY(EditAnywhere,Category = "Teleport")
	FName TeleportName;
	
	UPROPERTY(EditDefaultsOnly,Category = "Teleport")
	float TeleportDelay;
	
	UFUNCTION()
	void TeleportInstigator(APawn* InstigatorPawn);

};
