// Fill out your copyright notice in the Description page of Project Settings.


#include "SPlayerState.h"
#include "Net/UnrealNetwork.h"

ASPlayerState::ASPlayerState()
{
	SetReplicates(true);
}


void ASPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) __const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASPlayerState,Team);
	DOREPLIFETIME(ASPlayerState,Role);
	DOREPLIFETIME(ASPlayerState,Kills);
	DOREPLIFETIME(ASPlayerState,Deaths);
	//DOREPLIFETIME(ASPlayerState,Assists);
	DOREPLIFETIME(ASPlayerState,bIsCarryingKey);
	DOREPLIFETIME(ASPlayerState,bHasEvacuated);
	DOREPLIFETIME(ASPlayerState,bIsAlive);
	
}