// Fill out your copyright notice in the Description page of Project Settings.


#include "SGunBase.h"
#include "SCharacter.h"

// Sets default values
ASGunBase::ASGunBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create a gun mesh component
	GunMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	GunMeshComponent->CastShadow = false;
	//GunMeshComponent->SetupAttachment(ArmComponent, TEXT("GripPoint"));
	
	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(GunMeshComponent, TEXT("Barrel"));
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Stock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stock"));
	Stock->SetupAttachment(GunMeshComponent, TEXT("Stock"));
	Stock->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Magazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine"));
	Magazine->SetupAttachment(GunMeshComponent, TEXT("Mag 1"));
	Magazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Scope = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scope"));
	Scope->SetupAttachment(GunMeshComponent, TEXT("Lens"));
	Scope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Sight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight"));
	Sight->SetupAttachment(GunMeshComponent, TEXT("Sight"));
	Sight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ASGunBase::HasAmmo() const
{
	return RestAmmo > 0;
}

FVector ASGunBase::GetAimSocketLocation() const
{
	return GunMeshComponent->GetSocketLocation(TEXT("AimSocket"));
}

const FWeaponFireAnimation& ASGunBase::GetFireAnimation(ESCharacterState State, bool bIsAiming) const
{
	if (bIsAiming)
	{
		return AimFire;
	}
	switch (State)
	{
		case ESCharacterState::Walk:   //use the same fire anim montage as SPRINT
		case ESCharacterState::Sprint:
			return SprintFire;
		case ESCharacterState::Idle:
		default:
			return IdleFire;
	}
}

void ASGunBase::BeginPlay()
{
	Super::BeginPlay();
	RestAmmo = Ammo;
	
}
