// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction_Sprint.h"
#include "SActionComponent.h"
#include "SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void USAction_Sprint::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);

	if (ASCharacter* Character = Cast<ASCharacter>(Instigator))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void USAction_Sprint::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);

	if (ASCharacter* Character = Cast<ASCharacter>(Instigator))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetWalkSpeed();
	}
}
