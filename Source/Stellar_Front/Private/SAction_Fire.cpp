// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Fire.h"
#include "SActionComponent.h"
#include "SCharacter.h"
#include "SGunBase.h"



bool USAction_Fire::CanStart_Implementation(AActor* Instigator)
{
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	if (Character)
	{
		ASGunBase* Gun = Character->GetEquippedGun();
		if (!Gun)
		{
			return false;
		}

		//check first time 
		if (Gun->MagHasAmmo() && Super::CanStart_Implementation(Instigator))
		{
			return true;
		}
		//play "Ka Ke" sound like no bullet in that gun
		Gun->PlayKakeSound();
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
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this,"FireDelay_Elapsed",Character);
		float FireRate = Gun->GetFireRate();
		
		FireDelay_Elapsed(Character);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_FireDelay,Delegate,FireRate,true);
		// Normally, inside a Character subclass, you can directly use GetTimerManager,
		// but outside of it, you need to use GetWorld
	}
}


void USAction_Fire::FireDelay_Elapsed(APawn* InstigatorPawn)
{
	ASCharacter* InstigatorCharacter = Cast<ASCharacter>(InstigatorPawn);
	ASGunBase* Gun = IsValid(InstigatorCharacter) ? InstigatorCharacter->GetEquippedGun() : nullptr;

	//check second time here. To prevent firing without Any Ammo during Shooting process
	if (!IsValid(Gun) || !Gun->MagHasAmmo())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
		StopAction(InstigatorPawn);
		return;
	}

	//check Aim at the FireDelay_Elapsed
	USActionComponent* OwningComp = this->GetOwningComponent();
	FGameplayTag AimTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Aim"));
	bool bIsNowAiming = OwningComp->ActiveGameplaytags.HasTag(AimTag);

	Gun->WeaponFire(InstigatorPawn,bIsNowAiming);
}


void USAction_Fire::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_FireDelay);
}
