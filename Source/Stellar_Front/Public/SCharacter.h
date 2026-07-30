// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// Included for struct FInputActionInstance (Enhanced Input)
#include "InputAction.h"
#include "SGunBase.h"
#include "SCharacter.generated.h"

class UInputMappingContext;
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class ASProjectileBase;
class USoundBase;
class UAnimSequence;
class UParticleSystem;
class USAttributeComponent;
class USActionComponent;
class USInteractionComponent;


UENUM(BlueprintType)
enum class ESCharacterState : uint8
{
	Idle,
	Walk,
	Sprint
};

UCLASS()
class ASCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere,Category = "FOV")
	float Default_Fov = 105.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 600.0f;

	UPROPERTY(BlueprintReadOnly,Category  = "Movement")
	float MoveX = 0.0f;
	UPROPERTY(BlueprintReadOnly,Category  = "Movement")
	float MoveY = 0.0f;

	UPROPERTY(EditAnywhere,Category = "Movement")
	float NormalMouseSensitivity = 1.0;
	
	UPROPERTY(EditAnywhere,Category = "Movement")
	float AimMouseSensitivity = 0.4;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	ESCharacterState CharacterState = ESCharacterState::Idle;

	// -- Enhanced Input -- //

	/* Holds collection of currently active and available InputActions */
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* DefaultInputMapping;

	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Move;

	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Look;

	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Jump;

	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Fire;
	
	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Interact;

	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Aim;
	
	UPROPERTY(EditDefaultsOnly, Category= "Input")
	UInputAction* Input_Sprint;
	
	
	/** Pawn mesh: 1st person view  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMeshComponent* ArmComponent;


	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	USAttributeComponent* AttributeComp;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	USActionComponent* ActionComp;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components")
	USInteractionComponent* InteractionComp;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> LandedCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> JumpCameraShake;

	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	TSubclassOf<ASGunBase> GunClass;

	/** The weapon spawned and currently equipped by this character. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Weapons")
	TObjectPtr<ASGunBase> EquippedGun;
public:
	ASCharacter();

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnJumped_Implementation() override;


protected:
	void SpawnWeapon();
	
	/** Fires a projectile. */
	void Fire();

	void StartAim();
	void StopAim();

	void StartSprint();
	void StopSprint();

	void MoveInput(const FInputActionValue& InputValue);
	void StopMove();
	void UpdateCharacterState();

	void LookInput(const FInputActionValue& InputValue);

	void PrimaryInteract(const FInputActionValue& InputValue);

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	//virtual void Tick(float DeltaSeconds) override;

	bool bWantsToSprint = false;

public:


	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetArm() const { return ArmComponent; }
	UCameraComponent* GetCamera() const { return CameraComponent; }
	
	/** Returns the weapon spawned for this character. */
	UFUNCTION(BlueprintPure, Category = "Weapons")
	ASGunBase* GetEquippedGun() const { return EquippedGun; }

	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return CameraComponent; }

	float GetWalkSpeed() const { return WalkSpeed; }
	float GetMoveX() const {return MoveX;}
	float GetMoveY() const {return MoveY;}
	float GetDefaultFov() const {return Default_Fov;}
	
	UFUNCTION(BlueprintCallable)
	ESCharacterState GetCharacterState() const { return CharacterState; }

	virtual FVector GetPawnViewLocation() const override;
	
	void PostInitializeComponents() override;
	
	void BeginPlay() override;
};
