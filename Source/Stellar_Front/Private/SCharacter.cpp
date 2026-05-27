// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "SActionComponent.h"
#include "SAttributeComponent.h"
#include "SProjectileBase.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"


ASCharacter::ASCharacter()
{
	// Create a CameraComponent
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	CameraComponent->SetRelativeLocation(FVector(0, 0, BaseEyeHeight)); // Position the camera
	CameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1PComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	Mesh1PComponent->SetupAttachment(CameraComponent);
	Mesh1PComponent->CastShadow = false;
	Mesh1PComponent->SetRelativeRotation(FRotator(2.0f, -15.0f, 5.0f));
	Mesh1PComponent->SetRelativeLocation(FVector(0, 0, -160.0f));

	// Create a gun mesh component
	GunMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	GunMeshComponent->CastShadow = false;
	GunMeshComponent->SetupAttachment(Mesh1PComponent, "GripPoint");
	
	ActionComp = CreateDefaultSubobject<USActionComponent>("ActionComp");
	AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");
	
}


void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(Input_Move, ETriggerEvent::Triggered, this, &ASCharacter::MoveInput);
	EnhancedInputComponent->BindAction(Input_Look, ETriggerEvent::Triggered, this, &ASCharacter::LookInput);

	// Jump exists in the base class, we dont need our own function
	EnhancedInputComponent->BindAction(Input_Jump, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(Input_Fire, ETriggerEvent::Triggered, this, &ASCharacter::Fire);

	const APlayerController* PC = GetController<APlayerController>();
	const ULocalPlayer* LP = PC->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	// Add mappings for our game, more complex games may have multiple Contexts that are added/removed at runtime
	Subsystem->AddMappingContext(DefaultInputMapping, 0);
}

FVector ASCharacter::GetPawnViewLocation() const
{
	return CameraComponent->GetComponentLocation();
}


void ASCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (IsLocallyControlled())
	{
		/* Play landed camera anim */
		APlayerController* PC = Cast<APlayerController>(GetController());
		PC->PlayerCameraManager->StartCameraShake(LandedCameraShake);

		//UGameplayStatics::PlaySound2D(this, LandedSound);
	}
}

void ASCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// Don't play when this code is running for another player's character (multiplayer)
	if (IsLocallyControlled())
	{
		/* Play jump camera anim */
		APlayerController* PC = Cast<APlayerController>(GetController());
		PC->PlayerCameraManager->StartCameraShake(JumpCameraShake);

		//UGameplayStatics::PlaySound2D(this, JumpedSound);
	}
}


void ASCharacter::Fire()
{
	ActionComp->StartActionByName(this,"Fire");
}

void ASCharacter::MoveInput(const FInputActionValue& InputValue)
{
	// Combined input from forward/back (X) and left/right (Y)
	FVector2d MoveValue = InputValue.Get<FVector2d>();

	// add movement in that direction
	AddMovementInput(GetActorForwardVector(), MoveValue.X);

	// add movement in that direction
	AddMovementInput(GetActorRightVector(), MoveValue.Y);
}

void ASCharacter::LookInput(const FInputActionValue& InputValue)
{
	// Combined input from look up/down (X) and left/right (Y)
	FVector2d LookValue = InputValue.Get<FVector2d>();

	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}
