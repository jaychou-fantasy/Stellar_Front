// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Fire.h"
//#include "GameFramework/Character.h"
#include "SActionComponent.h"
#include "SCharacter.h"
#include "SGunBase.h"
#include "SProjectileBase.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MetasoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "SGunBase.h"

USAction_Fire::USAction_Fire()
{
	GunMuzzleName = "Muzzle";
}

bool USAction_Fire::CanStart_Implementation(AActor* Instigator)
{
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	if (Character)
	{
		ASGunBase* Gun = Character->GetEquippedGun();
		if (Gun && Gun->HasAmmo() &&Super::CanStart_Implementation(Instigator))
		{
			return true;
		}
	}
	return false;
}

void USAction_Fire::StartAction_Implementation(AActor* Instigator)
{
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	if (!Character)
	{
		return;	
	}

	ASGunBase* Gun = Character->GetEquippedGun();
	USkeletalMeshComponent* GunMesh = IsValid(Gun) ? Gun->GetGunMesh() : nullptr;
	if (!IsValid(GunMesh))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot fire: no valid equipped gun mesh on %s"), *Character->GetName());
		return;
	}

	// Only mark the action as running after all required weapon references exist.
	Super::StartAction_Implementation(Instigator);
	
	//the above sentence are same time triggered no matter you are server/client,and we just dont want the client accidentally create an extra projectile
	//so we just need to limit the timer ------ only run in Server
	if (Instigator->HasAuthority())
	{
		USActionComponent* OwningComp = this->GetOwningComponent();
		FGameplayTag AimTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Aim"));
		bool bIsAiming = OwningComp->ActiveGameplaytags.HasTag(AimTag);
		
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this,"FireDelay_Elapsed",Character,bIsAiming);
		float FireRate = Gun->GetFireRate();
		
		FireDelay_Elapsed(Character,bIsAiming);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_FireDelay,Delegate,FireRate,true);
		// Normally, inside a Character subclass, you can directly use GetTimerManager, 
		// but outside of it, you need to use GetWorld
	}
}


void USAction_Fire::FireDelay_Elapsed(ASCharacter* InstigatorCharacter,bool bIsAiming)
{
	if (!IsValid(InstigatorCharacter))
	{
		return;
	}

	ASGunBase* Gun = InstigatorCharacter->GetEquippedGun();
	USkeletalMeshComponent* GunMesh = IsValid(Gun) ? Gun->GetGunMesh() : nullptr;
	USkeletalMeshComponent* Mesh1P = InstigatorCharacter->GetArm();

	// Get the animation object for the arms mesh
	//aim fire
	//idle fire
	//sprint | walk fire
	//Play Arm|Weapon Animation && Play Muzzle FX
	UAnimInstance* ArmAnim = Mesh1P->GetAnimInstance();
	UAnimInstance* GunAnim = GunMesh->GetAnimInstance();
	const FWeaponFireAnimation& FireAnimation = Gun->GetFireAnimation(InstigatorCharacter->GetCharacterState(),bIsAiming);
	
	if (ArmAnim && GunAnim)
	{
		UAnimMontage* ArmMontage = FireAnimation.ArmMontage;
		UAnimMontage* WeaponMontage = FireAnimation.WeaponMontage;
		
		ArmAnim->Montage_Play(ArmMontage);
		GunAnim->Montage_Play(WeaponMontage);
	}
	//spawn emitter
	if (UNiagaraSystem* MuzzleFlash = Gun->GetMuzzleFlash())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash,Gun->GetBarrel(),TEXT("Muzzle"),FVector::ZeroVector,FRotator::ZeroRotator,EAttachLocation::KeepRelativeOffset,true);
	}
	//play meta sound
	if (UMetaSoundSource* FireSound = Gun->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this,FireSound);
		//UGameplayStatics::PlaySoundAtLocation(this, FireSound, InstigatorCharacter->GetActorLocation());//they are in the same location,no need to get gun location
	}

	
	// try and fire a projectile
	const TSubclassOf<ASProjectileBase> ProjectileClass = Gun->GetProjectileClass();
	if (ensureAlways(ProjectileClass))
	{
		FVector MuzzleLocation = GunMesh->GetSocketLocation(GunMuzzleName);
		FRotator MuzzleRotation = InstigatorCharacter->GetControlRotation();
		
		//Ray Check
		
		FHitResult Hit;
		FVector TraceStart = InstigatorCharacter->GetPawnViewLocation();//override as "Camera Comp's ViewLocations"
		FVector TraceEnd = TraceStart + MuzzleRotation.Vector() * 5000;//@fixme:this "5000" can transform into a "variable"
		
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

		FCollisionShape CollisionShape;
		CollisionShape.SetSphere(ProjectileClass->GetDefaultObject<ASProjectileBase>()->GetSphereRadius());//GetDefaultObject----> Cast "UClass*" to "ASprojectileBase*"
		
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(InstigatorCharacter);
		// Query conditions (filter rules) -- primarily used to specify which objects to ignore during collision detection, whether to use complex collision, whether to return physical materials, etc.
		/*
		Common uses:
			AddIgnoredActor(): Ignores a specific Actor, excluding it from collision
			Set bTraceComplex and other toggles
			Set debug information
			It does NOT determine which object types participate in the detection �� it only tells the engine "how to query and who to ignore"
		*/
		
		
		if (GetWorld()->SweepSingleByObjectType(Hit,TraceStart,TraceEnd,FQuat::Identity,ObjectQueryParams,CollisionShape,CollisionQueryParams))
		{
			TraceEnd = Hit.ImpactPoint;
		}
		
		FRotator ProjRotation;
		// Fall back since we failed to find any blocking hit
		// Settle for a slightly less accurate direction as a fallback(tui er qiu qi ci)
		ProjRotation = FRotationMatrix::MakeFromX(TraceEnd - MuzzleLocation).Rotator();
		/*Takes your provided XAxis as the local forward direction of the object
		  Automatically calculates a suitable Y and Z axis (ensures an orthonormal basis)
		  Finally generates an FRotationMatrix (rotation matrix)*/
		
		//Begin Spawn  Projectile at the Muzzle
		FTransform SpawnMT = FTransform(ProjRotation , MuzzleLocation);//combine Location w/ Rotation
		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;// can be nullptr if colliding with wall or sth.
		ActorSpawnParams.Instigator = InstigatorCharacter;

		GetWorld()->SpawnActor<ASProjectileBase>(ProjectileClass, SpawnMT, ActorSpawnParams);
	}
	
	//StopAction(InstigatorCharacter);
}


void USAction_Fire::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
	
}
