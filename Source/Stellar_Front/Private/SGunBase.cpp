// Fill out your copyright notice in the Description page of Project Settings.


#include "SGunBase.h"

// Sets default values
ASGunBase::ASGunBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
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

FVector ASGunBase::GetAimSocketLocation() 
{
	AimSocketLocation =  GunMeshComponent->GetSocketLocation(TEXT("AimSocket"));
	return AimSocketLocation;
}

// Called when the game starts or when spawned
void ASGunBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASGunBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
