// Fill out your copyright notice in the Description page of Project Settings.


#include "SPlayerController.h"
#include "SPlayerState.h"

void ASPlayerController::ServerSetReady_Implementation()
{
	ASPlayerState* PS = GetPlayerState<ASPlayerState>();                                                            
	if (PS) 
	{
		PS->SetReady();    
	}
}