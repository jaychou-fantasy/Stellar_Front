// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/SAnimInstance.h"
#include "Combat/Weapons/SGunBase.h"
#include "Character/SCharacter.h"
#include "GameFramework/Character.h"


USAnimInstance::USAnimInstance()
{
	MaxSwayRotation = 8.0f;
	SwaySpeed = 3.0f;
}

void USAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// Gun = Cast<ASGunBase>(GetOwningActor());
	// if (Gun)
	// {
	// 	Character = Cast<ASCharacter>(Gun->GetOwner());
	// }
	
}

void USAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// ASCharacter* Pawn = Cast<ASCharacter>(TryGetPawnOwner());
	// if (Character)
	// {
	// 	MoveXX = Character->GetMoveX();
	// 	MoveYY = Character->GetMoveY();
	// }
	
	// The weapon sway itself is applied to a specific bone in the AnimationBlueprint
	CalcWeaponSway(DeltaSeconds);
}



void USAnimInstance::CalcWeaponSway(float DeltaTime)
{
	const APawn* MyPawn = Cast<APawn>(GetOwningActor());
	if (MyPawn == nullptr)
	{
		// Can be nullptr while in editor
		return;
	}

	// Sway Rotation
	FRotator CharControlRotation = MyPawn->GetControlRotation();
	FRotator RawDelta = SwayOldRotation - CharControlRotation;
	RawDelta.Normalize();
	SwayDeltaRotation = FMath::RInterpTo(SwayDeltaRotation, RawDelta, DeltaTime, SwaySpeed);
	SwayOldRotation = CharControlRotation;
	// Invert the roll ( counter movement ) and clamp it.
	// todo: expose all these params
	SwayDeltaRotation.Roll = FMath::Clamp(SwayDeltaRotation.Yaw * -1.35f, MaxSwayRotation * -1.0f, MaxSwayRotation);

	// Sway Translation
	FVector InputDirection = MyPawn->GetActorTransform().InverseTransformVectorNoScale(MyPawn->GetLastMovementInputVector());
	InputDirection.Normalize();

	// todo: expose all these params
	InputDirection = InputDirection * FVector(-0.50f, -0.55, 1.0f) * 6.0f;
	SwayDeltaTranslation = FMath::Lerp(SwayDeltaTranslation, InputDirection, DeltaTime * SwaySpeed);
}
