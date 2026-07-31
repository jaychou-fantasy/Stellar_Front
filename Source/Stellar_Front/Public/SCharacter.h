// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UInputMappingContext;
class UInputComponent;
class UInputAction;
class USkeletalMeshComponent;
class UCameraComponent;
class ASGunBase;
class USoundBase;
class UAnimSequence;
class UParticleSystem;
class USAttributeComponent;
class USActionComponent;
class USInteractionComponent;
struct FInputActionValue;


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
	float NormalMouseSensitivity = 0.7f;
	
	UPROPERTY(EditAnywhere,Category = "Movement")
	float AimMouseSensitivity = 0.4f;
	
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

	void PostInitializeComponents() override;
	
	void BeginPlay() override;
	
protected:
	void SpawnWeapon();
	
	/** Fires a projectile. */
	void StartFire();
	void StopFire();

	void StartAim();
	void StopAim();

	void StartSprint();
	void StopSprint();

	void MoveInput(const FInputActionValue& InputValue);
	void StopMove();
	void UpdateCharacterState();
	float UpdateSensitivity();

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
	
};
