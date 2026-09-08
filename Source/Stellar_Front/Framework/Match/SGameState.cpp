// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Match/SGameState.h"
#include "Net/UnrealNetwork.h"

//run on server
void ASGameState::SetCurrentPhase(EGamePhase NewPhase)
{
	if (HasAuthority())
	{
		CurrentPhase = NewPhase;
		OnPhaseChanged.Broadcast(CurrentPhase);
	}
}

void ASGameState::SetRedControlNodes(int32 NewValue)
{
	if (HasAuthority())
	{
		RedControlNodes = NewValue;
	}
}

//run on remote client
void ASGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(CurrentPhase);
}

void ASGameState::OnRep_KeyStatus()
{
}

void ASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const                       
{                                                                                                                     
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);                                                              
                                                                                                                        
	DOREPLIFETIME(ASGameState, CurrentPhase);         
	DOREPLIFETIME(ASGameState, RedControlNodes);                                                                      
	DOREPLIFETIME(ASGameState, BlueControlNodes);                                                                     
	DOREPLIFETIME(ASGameState, bKeyFound);                                                                            
	DOREPLIFETIME(ASGameState, KeyHolder);                                                                            
	DOREPLIFETIME(ASGameState, UploadZoneLocation);                                                                   
	DOREPLIFETIME(ASGameState, UploadProgress);                                                                       
	DOREPLIFETIME(ASGameState, EvacShipLocation);                                                                     
	DOREPLIFETIME(ASGameState, EvacTimeRemaining);                                                                    
	DOREPLIFETIME(ASGameState, RedScore);                                                                             
	DOREPLIFETIME(ASGameState, BlueScore);         

}       
