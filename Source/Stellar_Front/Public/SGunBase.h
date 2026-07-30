// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGunBase.generated.h"

UCLASS(Abstract)
class STELLAR_FRONT_API ASGunBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ASGunBase();

	/** Returns the skeletal mesh that belongs to this weapon. */
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetGunMesh() const { return GunMeshComponent; }
	
	UFUNCTION(BlueprintCallable,Category = "Weapon")
	FVector GetAimSocketLocation();

protected:
	virtual void BeginPlay() override;
	
	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* GunMeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* Barrel;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* Stock;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* Magazine;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* Scope;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMeshComponent* Sight;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category = "Weapon")
	int32 Ammo;
	
	UPROPERTY(BlueprintReadOnly,Category = "Weapon")
	FVector AimSocketLocation;

public:	
	virtual void Tick(float DeltaTime) override;

};
