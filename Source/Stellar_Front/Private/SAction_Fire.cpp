// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Fire.h"
//#include "GameFramework/Character.h"
#include "SCharacter.h"
#include "SProjectileBase.h"
#include "Kismet/GameplayStatics.h"

USAction_Fire::USAction_Fire()
{
	GunMuzzleName = "Muzzle";
	ArmSlotName = "Arms";
}

void USAction_Fire::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
	
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	if (!Character)
	{
		return;	
	}
	USkeletalMeshComponent* Mesh1P = Character->GetMesh1P();
	USkeletalMeshComponent* GunMesh = Character->GetGunMesh();

	// Get the animation object for the arms mesh
	//Play Arm Animation && Play Muzzle FX
	UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, ArmSlotName, 0.0f);
		//AnimInstance->Montage_Play(FireAnimation);
	}
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlash, GunMesh,GunMuzzleName);
	UGameplayStatics::PlaySoundAtLocation(this, FireSound, Instigator->GetActorLocation());// actuall "GunMeshCompinent Location" but we just simplify it

	
	//the above sentence are same time triggered no matter you are server/client,and we just dont want the client accidentally create an extra projectile
	//so we just need to limit the timer ------ only run in Server
	if (Instigator->HasAuthority())
	{
		FTimerHandle TimerHandle_FireDelay;
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this,"FireDelay_Elapsed",Character);
		
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_FireDelay,Delegate,FireAnimDelay,false);
		// Normally, inside a Character subclass, you can directly use GetTimerManager, 
		// but outside of it, you need to use GetWorld
	}
}


void USAction_Fire::FireDelay_Elapsed(ASCharacter* InstigatorCharacter)
{
	// try and fire a projectile
	if (ensureAlways(ProjectileClass))
	{
		FVector MuzzleLocation = InstigatorCharacter->GetGunMesh()->GetSocketLocation(GunMuzzleName);
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
	
	StopAction(InstigatorCharacter);
}



