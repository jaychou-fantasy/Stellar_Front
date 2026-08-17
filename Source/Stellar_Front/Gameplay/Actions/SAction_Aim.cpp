// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Actions/SAction_Aim.h"
#include "Gameplay/Actions/SActionComponent.h"
#include "Character/SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"




void USAction_Aim::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);

	if (ASCharacter* Character = Cast<ASCharacter>(Instigator))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = AimSpeed;
		TargetFov = Aim_Fov;

		GetWorld()->GetTimerManager().SetTimer(
			FovTimerHandle,
			this,
			&USAction_Aim::UpdateFov,
			GetWorld()->GetDeltaSeconds(),
			true
		);
	}
}

void USAction_Aim::StopAction_Implementation(AActor* Instigator)
{
	Super::StopAction_Implementation(Instigator);

	if (ASCharacter* Character = Cast<ASCharacter>(Instigator))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetWalkSpeed();
		TargetFov = Character->GetDefaultFov();

		GetWorld()->GetTimerManager().SetTimer(
			FovTimerHandle,
			this,
			&USAction_Aim::UpdateFov,
			GetWorld()->GetDeltaSeconds(),
			true
		);
	}
}

void USAction_Aim::UpdateFov()
{
	ASCharacter* Character = Cast<ASCharacter>(GetOwningComponent()->GetOwner());
	if (!Character)
	{
		GetWorld()->GetTimerManager().ClearTimer(FovTimerHandle);
		return;
	}

	UCameraComponent* CameraComp = Character->GetCamera();
	const float NewFov = FMath::FInterpTo(
		CameraComp->FieldOfView,
		TargetFov,
		GetWorld()->GetDeltaSeconds(),
		8.0f
	);
	CameraComp->SetFieldOfView(NewFov);

	if (FMath::IsNearlyEqual(NewFov, TargetFov, 0.1f))
	{
		CameraComp->SetFieldOfView(TargetFov);
		GetWorld()->GetTimerManager().ClearTimer(FovTimerHandle);
	}
}
