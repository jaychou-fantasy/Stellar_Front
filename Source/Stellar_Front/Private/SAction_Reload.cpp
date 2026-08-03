// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Reload.h"

#include "SCharacter.h"
#include "SGunBase.h"

bool USAction_Reload::CanStart_Implementation(AActor* Instigator)
{
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	ASGunBase* Gun = Character->GetEquippedGun();
	if (Gun->TotalHasAmmo() && (Gun->GetRestMagAmmo() < Gun->GetMagSize()) && Super::CanStart_Implementation(Instigator))
	{
		return true;
	}
	return false;	
}


void USAction_Reload::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);
	
	ASCharacter* Character = Cast<ASCharacter>(Instigator);
	ASGunBase* Gun = Character->GetEquippedGun();
	if (Gun)
	{
		Gun->WeaponReload(Character);
	}
	StopAction(Instigator);
}

void USAction_Reload::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
}
