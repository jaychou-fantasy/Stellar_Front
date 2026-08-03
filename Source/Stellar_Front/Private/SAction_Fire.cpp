// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Fire.h"
#include "SActionComponent.h"
#include "SCharacter.h"
#include "SGunBase.h"

USAction_Fire::USAction_Fire()
{
}

bool USAction_Fire::CanStart_Implementation(AActor* Instigator)
{
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	if (Character)
	{
		ASGunBase* Gun = Character->GetEquippedGun();
		if (Gun && Gun->HasAmmo() && Super::CanStart_Implementation(Instigator))
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


void USAction_Fire::FireDelay_Elapsed(APawn* InstigatorPawn, bool bIsAiming)
{
	ASCharacter* InstigatorCharacter = Cast<ASCharacter>(InstigatorPawn);
	if (!IsValid(InstigatorCharacter))
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
		return;
	}

	ASGunBase* Gun = InstigatorCharacter->GetEquippedGun();
	if (!IsValid(Gun) || !Gun->HasAmmo())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
		return;
	}

	Gun->WeaponFire(InstigatorPawn,bIsAiming);
}


void USAction_Fire::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
}
