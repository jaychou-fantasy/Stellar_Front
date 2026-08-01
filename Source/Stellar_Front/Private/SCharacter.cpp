// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "SActionComponent.h"
#include "SAttributeComponent.h"
#include "SGunBase.h"
#include "SInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


ASCharacter::ASCharacter()
{
	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
 	ArmComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
 	ArmComponent->SetupAttachment(GetCapsuleComponent());
 	ArmComponent->CastShadow = false;
 	ArmComponent->SetRelativeRotation(FRotator(2.0f, -15.0f, 5.0f));
 	ArmComponent->SetRelativeLocation(FVector(0, 0, -160.0f));
	
	// Create a CameraComponent
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraComponent->SetupAttachment(GetArm(),TEXT("Head"));
	//CameraComponent->SetRelativeLocation(FVector(0, 0, BaseEyeHeight)); // Position the camera
	CameraComponent->bUsePawnControlRotation = true;
	CameraComponent->SetFieldOfView(Default_Fov);
	
	ActionComp = CreateDefaultSubobject<USActionComponent>("ActionComp");
	AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");
	InteractionComp = CreateDefaultSubobject<USInteractionComponent>("InteractionComp");

	PrimaryActorTick.bCanEverTick = true;
}

void ASCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Arm has Head socket: %d"), ArmComponent->DoesSocketExist(TEXT("Head")));
	UE_LOG(LogTemp, Warning, TEXT("Camera parent socket: %s"), *CameraComponent->GetAttachSocketName().ToString());
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	SpawnWeapon();
}

float ASCharacter::UpdateSensitivity()
{
	FGameplayTag AimTag = FGameplayTag::RequestGameplayTag("Status.Aim");
	if (ActionComp->ActiveGameplaytags.HasTag(AimTag))
	{
		return AimMouseSensitivity;
	}
	else
	{
		return NormalMouseSensitivity;
	}
}


FVector ASCharacter::GetPawnViewLocation() const
{
	return CameraComponent->GetComponentLocation();
}


void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(Input_Move, ETriggerEvent::Triggered, this, &ASCharacter::MoveInput);
	EnhancedInputComponent->BindAction(Input_Move, ETriggerEvent::Completed, this, &ASCharacter::StopMove);
	EnhancedInputComponent->BindAction(Input_Look, ETriggerEvent::Triggered, this, &ASCharacter::LookInput);

	// Jump exists in the base class, we don't need our own function
	EnhancedInputComponent->BindAction(Input_Jump, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	
	EnhancedInputComponent->BindAction(Input_Fire, ETriggerEvent::Started, this, &ASCharacter::StartFire);
	EnhancedInputComponent->BindAction(Input_Fire, ETriggerEvent::Completed, this, &ASCharacter::StopFire);

	EnhancedInputComponent->BindAction(Input_Aim, ETriggerEvent::Started, this, &ASCharacter::StartAim);
	EnhancedInputComponent->BindAction(Input_Aim, ETriggerEvent::Completed, this, &ASCharacter::StopAim);

	EnhancedInputComponent->BindAction(Input_Sprint,ETriggerEvent::Started,this,&ASCharacter::StartSprint);
	EnhancedInputComponent->BindAction(Input_Sprint,ETriggerEvent::Completed,this,&ASCharacter::StopSprint);


	EnhancedInputComponent->BindAction(Input_Interact,ETriggerEvent::Triggered,this,&ASCharacter::PrimaryInteract);

	const APlayerController* PC = GetController<APlayerController>();
	const ULocalPlayer* LP = PC->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	// Add mappings for our game, more complex games may have multiple Contexts that are added/removed at runtime
	Subsystem->AddMappingContext(DefaultInputMapping, 0);
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

void ASCharacter::SpawnWeapon()
{
	if (IsValid(EquippedGun))
	{
		return;
	}

	if (!ensureMsgf(GunClass, TEXT("GunClass is not assigned on %s"), *GetName()))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASGunBase* SpawnedGun = GetWorld()->SpawnActor<ASGunBase>(
		GunClass,
		FTransform::Identity,
		SpawnParams
	);

	if (IsValid(SpawnedGun))
	{
		EquippedGun = SpawnedGun;
		EquippedGun->AttachToComponent(ArmComponent,FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
}


void ASCharacter::StartFire()
{
	ActionComp->StartActionByName(this,"Fire");
}

void ASCharacter::StopFire()
{
	ActionComp->StopActionByName(this,"Fire");
}


void ASCharacter::StartAim()
{
	ActionComp->StartActionByName(this,"Aim");
}

void ASCharacter::StopAim()
{
	ActionComp->StopActionByName(this,"Aim");
}

void ASCharacter::StartSprint()
{
	bWantsToSprint = true;
	ActionComp->StartActionByName(this,"Sprint");
	UpdateCharacterState();
}

void ASCharacter::StopSprint()
{
	bWantsToSprint = false;
	ActionComp->StopActionByName(this,"Sprint");
	UpdateCharacterState();
}



void ASCharacter::MoveInput(const FInputActionValue& InputValue)
{
	// Combined input from forward/back (X) and left/right (Y)
	FVector2d MoveValue = InputValue.Get<FVector2d>();

	// add movement in that direction
	MoveX = MoveValue.X;
	MoveY = MoveValue.Y;
	UpdateCharacterState();
	AddMovementInput(GetActorForwardVector(),MoveValue.X);

	// add movement in that direction
	AddMovementInput(GetActorRightVector(), MoveValue.Y);
}

void ASCharacter::StopMove()
{
	MoveX = 0.0f;
	MoveY = 0.0f;
	UpdateCharacterState();
}

void ASCharacter::UpdateCharacterState()
{
	const bool bHasMoveInput = !FMath::IsNearlyZero(MoveX) || !FMath::IsNearlyZero(MoveY);

	if (bHasMoveInput && bWantsToSprint)
	{
		CharacterState = ESCharacterState::Sprint;
	}
	else if (bHasMoveInput)
	{
		CharacterState = ESCharacterState::Walk;
	}
	else
	{
		CharacterState = ESCharacterState::Idle;
	}
}

void ASCharacter::LookInput(const FInputActionValue& InputValue)
{
	// Combined input from look up/down (X) and left/right (Y)
	FVector2d LookValue = InputValue.Get<FVector2d>();

	float MouseSens = UpdateSensitivity();
	AddControllerYawInput(LookValue.X * MouseSens);
	AddControllerPitchInput(LookValue.Y * MouseSens);
}

void ASCharacter::PrimaryInteract(const FInputActionValue& InputValue)
{
	InteractionComp->PrimaryInteract();
}
