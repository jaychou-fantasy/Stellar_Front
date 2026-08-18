// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Match/SGameMode_StellarFront.h"
#include "EngineUtils.h"
#include "Framework/Match/SGameState.h"
#include "Framework/Player/SPlayerState.h"
#include "GameFramework/PlayerStart.h"


ASGameMode_StellarFront::ASGameMode_StellarFront()
{
	/* set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/BP_Player"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;*/

	// use our custom HUD class
	//HUDClass = ASHUD::StaticClass();
	
	
	//true when the game need not start imediately
	bDelayedStart = true;
}


//when client login in the game
void ASGameMode_StellarFront::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (!NewPlayer)
	{
		return;
	}
	
	ASPlayerState* PS = NewPlayer->GetPlayerState<ASPlayerState>();
	if (PS)
	{
		AssignTeam(PS);
	}
	
	//@fixme:check if match the condition to start game
}

AActor* ASGameMode_StellarFront::ChoosePlayerStart_Implementation(AController* Player)
{
	const ASPlayerState* PlayerState = Player ? Player->GetPlayerState<ASPlayerState>() : nullptr;
	if (!PlayerState)
	{
		//find a proper and random player_start to spawn character
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	
	FName RequiredTag;
	
	switch (PlayerState->GetTeam())
	{
	case ETeam::Red:
		RequiredTag = TEXT("Red");
		break;
	case ETeam::Blue:
		RequiredTag = TEXT("Blue");
		break;
	default:
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	
	TArray<APlayerStart*> TeamStarts;
	
	for (TActorIterator<APlayerStart> It(GetWorld()) ; It ; ++It)
	{
		if (It->PlayerStartTag == RequiredTag)
		{
			TeamStarts.Add(*It);
		}
		if (TeamStarts.IsEmpty())
		{
			UE_LOG(LogGameMode,Warning,TEXT("No PlayerStart for Team Tag : %s"), *RequiredTag.ToString());
			return Super::ChoosePlayerStart_Implementation(Player);
		}
	}
	//return when it's not empty
	return TeamStarts[FMath::RandRange(0,TeamStarts.Num()-1)];
}

int32 ASGameMode_StellarFront::CountPlayersInTeam(ETeam TargetTeam, const APlayerState* PlayerToIgnore) const
{
	const ASGameState* StellarGameState = GetGameState<ASGameState>();
	if (!StellarGameState)
	{
		return 0;
	}
	
	int32 Cnt = 0;
	for (APlayerState* PS :StellarGameState->PlayerArray)
	{
		//don't count himself (as he has not in team yet)
		if (PS == PlayerToIgnore)
		{
			continue;
		}
		
		const ASPlayerState* SPlayerState = Cast<ASPlayerState>(PS);
		if (SPlayerState && SPlayerState->GetTeam() == TargetTeam)
		{
			Cnt++;
		}
	}
	
	return Cnt;
}


//@fixme:: we can parse RedCnt&BlueCnt as a global Variable,and we don't need to calculate when every player login in (or maybe 20 is rather small,don't bother to change)
void ASGameMode_StellarFront::AssignTeam(ASPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}
	
	const int32 RedCnt = CountPlayersInTeam(ETeam::Red , PlayerState);
	const int32 BlueCnt = CountPlayersInTeam(ETeam::Blue , PlayerState);
	
	const ETeam AssignedTeam = (RedCnt <= BlueCnt) ? ETeam::Red : ETeam::Blue;
	PlayerState->SetTeam(AssignedTeam);
}


bool ASGameMode_StellarFront::ReadyToStartMatch_Implementation()
{
	if (GetMatchState() != MatchState::WaitingToStart)
	{
		return false;
	}
	return CountPlayersInTeam(ETeam::Red) >= 1 && CountPlayersInTeam(ETeam::Blue ) >= 1;
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


void ASGameMode_StellarFront::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	StartDeployment();
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

void ASGameMode_StellarFront::StartMatch()
{
	Super::StartMatch();
}

void ASGameMode_StellarFront::EndMatch()
{
	Super::EndMatch();
}

