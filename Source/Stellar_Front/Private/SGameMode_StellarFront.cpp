// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode_StellarFront.h"
#include "SCharacter.h"
#include "SGameState.h"
#include "SPlayerState.h"

ASGameMode_StellarFront::ASGameMode_StellarFront()
{
	/* set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/BP_Player"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;*/

	// use our custom HUD class
	//HUDClass = ASHUD::StaticClass();
}


//when client login in the game
void ASGameMode_StellarFront::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	ASPlayerState* PS = NewPlayer->GetPlayerState<ASPlayerState>();
	if (PS)
	{
		AssignTeam(PS);
	}
	
	//@fixme:check if match the condition to start game
}


//@fixme:: we can parse RedCnt&BlueCnt as a global Variable,and we don't need to calculate when every player login in (or maybe 20 is rather small,don't bother to change)
void ASGameMode_StellarFront::AssignTeam(APlayerState* PlayerState)
{
	int32 RedCnt = 0;
	int32 BLueCnt = 0;
	for (APlayerState* State : GameState->PlayerArray)
	{
		ASPlayerState* SPS = Cast<ASPlayerState>(State);
		if (!SPS)
		{
			continue;
		}
		if (SPS->GetTeam() == ETeam::Red)
		{
			RedCnt++;
		}
		else
		{
			BLueCnt++;
		}
	}
	
	ASPlayerState* MyPS = Cast<ASPlayerState>(PlayerState);
	MyPS->SetTeam((RedCnt <= BLueCnt) ? ETeam::Red : ETeam::Blue);
}

void ASGameMode_StellarFront::SetPhase(EGamePhase NewPhase)
{
	ASGameState* GameState = GetGameState<ASGameState>();
	if (!GameState)
	{
		return;
	}
	GameState->SetCurrentPhase(NewPhase);
	/*
	 set the logic when switch phase
	 case Pre: .....
	 */
	
	
}

bool ASGameMode_StellarFront::ReadyToStartMatch_Implementation()
{
	return Super::ReadyToStartMatch_Implementation();
}

void ASGameMode_StellarFront::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

bool ASGameMode_StellarFront::ReadyToEndMatch_Implementation()
{
	return Super::ReadyToEndMatch_Implementation();
}

void ASGameMode_StellarFront::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

void ASGameMode_StellarFront::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}

void ASGameMode_StellarFront::StartDeployment()
{
	SetPhase(EGamePhase::PreDeploy);
	FTimerHandle DeployTimer;
	ASGameState* GameState = GetGameState<ASGameState>();  
	float DeployTime = GameState->DeployTimeRemaining;
	
	GetWorld()->GetTimerManager().SetTimer(DeployTimer,this,&ASGameMode_StellarFront::EndDeployment,DeployTime);
}

void ASGameMode_StellarFront::EndDeployment()
{
	SetPhase(EGamePhase::OrbitalCombat);
}


