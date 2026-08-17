// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Match/SGameState.h"
#include "Net/UnrealNetwork.h"

void ASGameState::SetCurrentPhase(EGamePhase NewPhase)
{
	if (HasAuthority())
	{
		CurrentPhase = NewPhase;
	}
}

void ASGameState::OnRep_Phase()
{
}

void ASGameState::OnRep_KeyStatus()
{
}

void ASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const                       
{                                                                                                                     
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);                                                              
                                                                                                                        
	DOREPLIFETIME(ASGameState, CurrentPhase);         
	DOREPLIFETIME(ASGameState,DeployTimeRemaining);
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
