// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Actions/SAction_Reload.h"

#include "Character/SCharacter.h"
#include "Combat/Weapons/SGunBase.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

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
	if (!Gun)
	{
		StopAction(Instigator);
		return;
	}
	UAnimInstance* GunAnim = Gun->GetGunMesh()->GetAnimInstance();
	UAnimMontage* WeaponReloadMontage = Gun->GetWeaponReloadAnim();
	if (!WeaponReloadMontage || !GunAnim)
	{
		StopAction(Instigator);
		return;
	}

	FOnMontageEnded ReloadEndedDelegate;
	//bind OnReloadAnimEnded(from this--inherented from UObject) to this DELEGATE
	ReloadEndedDelegate.BindUObject(this,&USAction_Reload::OnReloadAnimEnded);

	Gun->WeaponReload(Character);

	//Set Delegate must after Montage play，‘cause this time the corresponding Montage Instance are created
	if(GunAnim->Montage_IsActive(WeaponReloadMontage))
	{
		//Then Bind this DELEGATE to this MONTAGE
		GunAnim->Montage_SetEndDelegate(ReloadEndedDelegate,WeaponReloadMontage);
	}
	else
	{
		StopAction(Instigator);
	}
}

void USAction_Reload::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);
}

void USAction_Reload::OnReloadAnimEnded(UAnimMontage* ReloadMontage, bool bIsInterrupted)
{
	if (IsRunning())
	{
		StopAction(RepData.Instigator);
	}
}
