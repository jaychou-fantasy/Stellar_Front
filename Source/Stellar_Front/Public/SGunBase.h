// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGunBase.generated.h"

class ASGunCasing;
class UAnimMontage;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;
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

USTRUCT(BlueprintType)
struct FOnHitFlashSound
{
	GENERATED_BODY()

	/** Material applied to the shared bullet-hole actor for this physical surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Feedback")
	UMaterialInterface* DecalMaterial = nullptr;

	/** Spawn scale for the shared bullet-hole actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Feedback")
	FVector DecalScale = FVector(0.4f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Feedback")
	UNiagaraSystem* OnHitFlash = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Feedback")
	USoundBase* OnHitSound = nullptr;
};


UCLASS(Abstract)
class STELLAR_FRONT_API ASGunBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ASGunBase();
	
	void PlayKakeSound();
	
	UFUNCTION(BlueprintCallable)
	void WeaponFire(APawn* InstigatorPawn, bool bIsAiming);
	
	UFUNCTION()
	void SpawnCasing();
	
	UFUNCTION(BlueprintCallable)
	void WeaponReload(APawn* InstigatorPawn);

	/** Returns 1*/
	UFUNCTION(BlueprintCallable)
	bool MagHasAmmo() const;
	
	UFUNCTION(BlueprintCallable)
	bool TotalHasAmmo() const;
	
	UFUNCTION(BlueprintCallable)
	int32 GetRestMagAmmo() const { return MagRestAmmo; };
	
	UFUNCTION(BlueprintCallable)
	int32 GetMagSize() const { return MagSize; };
	
	
	/** Returns 2*/
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
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Icon")
	UTexture2D* WeaponIcon;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Icon")
	FText WeaponName;
	
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
	
	//casing actor
	UPROPERTY(EditAnywhere, Category = "Fire")
	TSubclassOf<ASGunCasing> CasingClass;

	/** Shared actor blueprint containing the DecalComponent, normally BP_BulletDecal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit Feedback")
	TSubclassOf<AActor> DecalActor;
	
	//Ammo
	UPROPERTY(EditDefaultsOnly,Category = "Fire")
	USoundBase* KaKeSound;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Fire")
	int32 TotalAmmo;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Fire")
	int32 MagSize;
	
	UPROPERTY(BlueprintReadOnly)
	int32 MagRestAmmo;
	
	UFUNCTION(BlueprintCallable)//used for Magazines of different volumn
	void UpdateMagSize(int32 NewAmmoNumber);
	
	UFUNCTION()
	void ConsumeMagAmmo();
	
	UFUNCTION()
	void ConsumeTotalAmmo(int32 Delta);
	
	UFUNCTION(BlueprintCallable)
	void ReloadAmmo();
	
	
	//Fire
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	float FireRate;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	float VerticalRecoil;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	float HorizontalRecoil;
	
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	TSubclassOf<ASProjectileBase> ProjectileClass;

	UPROPERTY(VisibleAnywhere, Category = "Fire")
	FName GunMuzzleName;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	UMetaSoundSource* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	USoundBase* ReloadSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	UNiagaraSystem* MuzzleFlash;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback")
	FOnHitFlashSound DirtOnHit;

	/** Used for both SurfaceType_Default and the Concrete physical material. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback")
	FOnHitFlashSound DefaultConcreteOnHit;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback")
	FOnHitFlashSound GlassOnHit;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback")
	FOnHitFlashSound EnemyOnHit;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Feedback")
	FOnHitFlashSound WoodOnHit;

	/** Spawns the shared bullet-hole actor, then applies this surface's material to its DecalComponent. */
	void SpawnImpactDecal(const FHitResult& Hit, APawn* InstigatorPawn, UMaterialInterface* DecalMaterial, const FVector& DecalScale);
	
	//fire anim montage
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FWeaponFireAnimation IdleFire;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FWeaponFireAnimation SprintFire;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FWeaponFireAnimation AimFire;
	
	
	//reload anim montage
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Animation")
	UAnimMontage* WeaponReloadAnim = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Animation")
	UAnimMontage* ArmReloadAnim = nullptr;
};
