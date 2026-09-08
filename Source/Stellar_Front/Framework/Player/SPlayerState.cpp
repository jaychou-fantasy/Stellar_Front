// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Player/SPlayerState.h"
#include "Net/UnrealNetwork.h"

ASPlayerState::ASPlayerState()
{
	SetReplicates(true);
}

void ASPlayerState::SetTeam(ETeam NewTeam)
{
	if (!HasAuthority() || Team == NewTeam)
	{
		return;
	}

	Team = NewTeam;
	OnTeamChanged.Broadcast(Team);
}

void ASPlayerState::OnRep_Team()
{
	OnTeamChanged.Broadcast(Team);
}


void ASPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASPlayerState,Team);
	DOREPLIFETIME(ASPlayerState,PlayerRole);
	DOREPLIFETIME(ASPlayerState,Kills);
	DOREPLIFETIME(ASPlayerState,Deaths);
	//DOREPLIFETIME(ASPlayerState,Assists);
	DOREPLIFETIME(ASPlayerState,bIsCarryingKey);
	DOREPLIFETIME(ASPlayerState,bHasEvacuated);
	DOREPLIFETIME(ASPlayerState,bIsAlive);
	DOREPLIFETIME(ASPlayerState, bReady);                                                                            

	
}
