// Fill out your copyright notice in the Description page of Project Settings.


#include "SGunBase.h"

// Sets default values
ASGunBase::ASGunBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

