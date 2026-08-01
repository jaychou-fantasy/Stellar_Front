// Fill out your copyright notice in the Description page of Project Settings.


#include "SGunBase.h"
#include "SCharacter.h"
#include "SGunCasing.h"

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

void ASGunBase::WeaponFire(APawn* InstigatorPawn)
{
	ASCharacter* Character = Cast<ASCharacter>(InstigatorPawn);	
	if (Character)
	{
		//use recoil
		Character->AddControllerPitchInput(VerticalRecoil);
		Character->AddControllerYawInput(FMath::RandRange(-HorizontalRecoil,HorizontalRecoil));
		
		ConsumeAmmo();
	}
	
}

void ASGunBase::SpawnCasing()
{
	FActorSpawnParameters SpawnParams;
	FTransform Transform = GunMeshComponent->GetSocketTransform(TEXT("Casing"));
	
	GetWorld()->SpawnActor<ASGunCasing>(CasingClass,Transform,SpawnParams);
}

bool ASGunBase::HasAmmo() const
{
	return RestAmmo > 0;
}

void ASGunBase::UpdateAmmo(int32 NewAmmoNumber)
{
	Ammo = NewAmmoNumber;
}

void ASGunBase::ConsumeAmmo()
{
	RestAmmo--;
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
