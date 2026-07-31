// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGunBase.generated.h"

class UAnimMontage;
class UParticleSystem;
class USoundBase;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class ASProjectileBase;
class UMetaSoundSource;
class UNiagaraSystem;
enum class ESCharacterState : uint8;

USTRUCT(BlueprintType)
struct FWeaponFireAnimation
{
	GENERATED_BODY()

	/** First-person arms animation played when this weapon fires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* ArmMontage = nullptr;

	/** Weapon animation played when this weapon fires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* WeaponMontage = nullptr;
};

UCLASS(Abstract)
class STELLAR_FRONT_API ASGunBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ASGunBase();

	/** Returns the skeletal mesh that belongs to this weapon. */
	
	UFUNCTION(BlueprintCallable)
	bool HasAmmo() const;
	
	UFUNCTION(BlueprintCallable)
	int32 GetRestAmmo() const { return RestAmmo; };
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetGunMesh() const { return GunMeshComponent; }
	UStaticMeshComponent* GetBarrel() const { return Barrel; }
	
	UFUNCTION(BlueprintCallable,Category = "Weapon")
	FVector GetAimSocketLocation() const;
	
	float GetFireRate() const { return FireRate; };
	
	const FWeaponFireAnimation& GetFireAnimation(ESCharacterState State, bool bIsAiming) const;
	
	TSubclassOf<ASProjectileBase> GetProjectileClass() const { return ProjectileClass; }
	
	UMetaSoundSource* GetFireSound() const { return FireSound; }
	UNiagaraSystem* GetMuzzleFlash() const { return MuzzleFlash; }

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
	
	
	//fire properties
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Weapon")
	int32 Ammo;
	
	UPROPERTY(BlueprintReadOnly)
	int32 RestAmmo;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	TSubclassOf<ASProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	UMetaSoundSource* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	UNiagaraSystem* MuzzleFlash;
	
	//fire anim montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponFireAnimation IdleFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponFireAnimation SprintFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponFireAnimation AimFire;

};
