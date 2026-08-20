// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Match/SGameMode_StellarFront.h"
#include "EngineUtils.h"
#include "Framework/Match/SGameState.h"
#include "Framework/Player/SPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Pawn.h"


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
	
	//Try to Spawn Player case: encroach blocking geometry ; case: not encroaching
	UWorld* World = GetWorld();
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;//first to create default pawn object to try to "fit"
	if (!World || !PawnToFit)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	
	//two Arrays ,two cases
	TArray<APlayerStart*> UnoccupiedTeamStarts;
	TArray<APlayerStart*> AdjustableTeamStarts;
	
	
	//Iterator is an ptr to an APlayerStart,*It to dereference
	for (TActorIterator<APlayerStart> It(World) ; It ; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (It->PlayerStartTag != RequiredTag)
		{
			continue;
		}
		FVector StartLocation = PlayerStart->GetActorLocation();
		const FRotator StartRotation = PlayerStart->GetActorRotation();
		if (!World->EncroachingBlockingGeometry(PawnToFit,StartLocation,StartRotation))
		{
			//case not encroach
			UnoccupiedTeamStarts.Add(PlayerStart);
		}
		//try to find a nearby location to spawn without collision---case2
		else if (World->FindTeleportSpot(PawnToFit,StartLocation,StartRotation))
		{
			AdjustableTeamStarts.Add(PlayerStart);
		}
	}
	//start to spawn
	if (!UnoccupiedTeamStarts.IsEmpty())
	{
		//return when it's not empty
		return UnoccupiedTeamStarts[FMath::RandRange(0,UnoccupiedTeamStarts.Num()-1)];
	}
	if (!AdjustableTeamStarts.IsEmpty())
	{
		//return when it's not empty
		return AdjustableTeamStarts[FMath::RandRange(0,AdjustableTeamStarts.Num()-1)];
	}
	//other case that can't spawn
	UE_LOG(LogGameMode,Warning,TEXT("No usable PlayerStart for Team Tag: %s"),*RequiredTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
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
	UE_LOG(LogGameMode,Log,TEXT("Match entered InProgress"));
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

void ASGameMode_StellarFront::HandlePlayerDeath(AActor* Instigator, APawn* VictimPawn)
{
	//Update Victim's PlayerState
	ASPlayerState* VictimPlayerState = VictimPawn ? VictimPawn->GetPlayerState<ASPlayerState>() : nullptr;
	if (!VictimPlayerState)
	{
		UE_LOG(LogGameMode,Warning,TEXT("HandlePlayerDeath: Victim %s has no ASPlayerState"),*GetNameSafe(VictimPawn));
		return;
	}
	VictimPlayerState->SetIsAlive(false);
	VictimPlayerState->AddDeaths();
	UE_LOG(LogGameMode,Log,TEXT("Death recorded: Victim=%s Deaths=%d Alive=%s"),*GetNameSafe(VictimPlayerState),VictimPlayerState->GetDeaths(),VictimPlayerState->IsAlive() ? TEXT("true") : TEXT("false"));

	//Update Killer's PlayerState
	ASPlayerState* KillerPlayerState = nullptr;
	if (APawn* InstigatorPawn = Cast<APawn>(Instigator))
	{
		KillerPlayerState = InstigatorPawn->GetPlayerState<ASPlayerState>();
	}
	// A Controller is not a Pawn, so this fallback is checked only when the Pawn cast fails.
	// because UE always pass Controller when face damage cause
	else if (AController* InstigatorController = Cast<AController>(Instigator))
	{
		KillerPlayerState = InstigatorController->GetPlayerState<ASPlayerState>();
	}
	
	//Kills++
		//111： Player kill: a valid killer PlayerState belongs to a different player.
	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->AddKills();
		UE_LOG(LogGameMode, Log, TEXT("Player kill recorded: Killer=%s Kills=%d Victim=%s"), *GetNameSafe(KillerPlayerState), KillerPlayerState->GetKills(), *GetNameSafe(VictimPlayerState));
	}
		//222:  Self death: the killer and victim resolve to the same PlayerState.
	else if (KillerPlayerState == VictimPlayerState)
	{
		UE_LOG(LogGameMode, Log, TEXT("Self death recorded: Player=%s Deaths=%d"), *GetNameSafe(VictimPlayerState), VictimPlayerState->GetDeaths());
	}
		//333:  Non-player kill: the source is null or has no associated ASPlayerState.
	else
	{
		UE_LOG(LogGameMode, Log, TEXT("Non-player kill recorded: Source=%s SourceClass=%s Victim=%s Deaths=%d"), *GetNameSafe(Instigator), *GetNameSafe(Instigator ? Instigator->GetClass() : nullptr), *GetNameSafe(VictimPlayerState), VictimPlayerState->GetDeaths());
	}
	
	//Respawn
	ASPlayerController* VictimPlayerController = VictimPawn->GetController<ASPlayerController>();
	if (!VictimPlayerController)
	{
		UE_LOG(LogGameMode,Warning,TEXT("Cannot schedule respawn: Victim=%s has no Controller"),*GetNameSafe(VictimPawn));
		VictimPawn->Destroy();
		return;
	}
	//detach pawn from controller
	VictimPawn->DetachFromControllerPendingDestroy();
	VictimPawn->Destroy();
	
	FTimerDelegate RespawnTimerDelegate;
	RespawnTimerDelegate.BindUObject(this,&ASGameMode_StellarFront::RespawnPlayer,TWeakObjectPtr<ASPlayerController>(VictimPlayerController));
	
	TWeakObjectPtr<ASPlayerController>ControllerIndex(VictimPlayerController);
	FTimerHandle& RespawnTimerHandle = PendingRespawnTimers.FindOrAdd(ControllerIndex);	
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle,RespawnTimerDelegate,RespawnDelay,false);
}

void ASGameMode_StellarFront::CancelRespawn(ASPlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}
	const TWeakObjectPtr<ASPlayerController> ControllerIndex(PlayerController);//create a weak ptr that points to "a PlayerController"
	//(don't force to possess an Controller ptr,when Player Log out, this ptr to Controller won't be destroyed)
	FTimerHandle* TimerHandle = PendingRespawnTimers.Find(ControllerIndex);
	//Find Function return a ptr to TimerHandle(TimerHandle*)
	//And if you wanna use it as "Handle" itself, then you'd dereference it --- *(Handle*)
	if (!TimerHandle)
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(*TimerHandle);
	PendingRespawnTimers.Remove(ControllerIndex);
	UE_LOG(LogGameMode,Log,TEXT("Respawn cancelled: Controller=%s"),*GetNameSafe(PlayerController));
}

void ASGameMode_StellarFront::RespawnPlayer(TWeakObjectPtr<ASPlayerController> PlayerController)
{
	if (!PlayerController.IsValid())
	{
		return;
	}
	ASPlayerController* Controller = PlayerController.Get(); //transform the weak reference ptr to normal ptr
	ASPlayerState* PlayerState = Controller->GetPlayerState<ASPlayerState>();
	if (!PlayerState || PlayerState->IsAlive())
	{
		return;
	}
	//Check is Dead----Done
	
	//Respawn logic -> Set Alive
	PlayerState->SetIsAlive(true);
	RestartPlayer(Controller);//try to find a APawn to spawn
	
	//if Respawn failed, re-set the "Alives state"
	if (!Controller->GetPawn())
	{
		PlayerState->SetIsAlive(false);
		UE_LOG(LogGameMode,Warning,TEXT("Respawn failed: Controller=%s"),*GetNameSafe(Controller));
		return;
	}
	UE_LOG(LogGameMode,Log,TEXT("Respawn succeeded: PlayerState=%s Pawn=%s Team=%d Alive=%s"),*GetNameSafe(PlayerState),*GetNameSafe(Controller->GetPawn()),static_cast<int32>(PlayerState->GetTeam()),PlayerState->IsAlive() ? TEXT("true") : TEXT("false"));
}


bool ASGameMode_StellarFront::ReadyToEndMatch_Implementation()
{
	return Super::ReadyToEndMatch_Implementation();
}

void ASGameMode_StellarFront::Logout(AController* Exiting)
{
	//Cancel Respawn
	if (ASPlayerController* PlayerController = Cast<ASPlayerController>(Exiting))
	{
		CancelRespawn(PlayerController);
	}
	
	Super::Logout(Exiting);
}

void ASGameMode_StellarFront::HandleMatchHasEnded()
{
	const int32 CancelledCount = PendingRespawnTimers.Num();
	for (TPair<TWeakObjectPtr<ASPlayerController>, FTimerHandle> Pair : PendingRespawnTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
	}
	//clear Timer     all the data should be cleared when The Match Has been Ended
	PendingRespawnTimers.Empty();
	UE_LOG(LogGameMode,Log,TEXT("Match ended: cancelled %d pending respawn timers"), CancelledCount);
	
	Super::HandleMatchHasEnded();
}

void ASGameMode_StellarFront::StartMatch()
{
	Super::StartMatch();
}

void ASGameMode_StellarFront::EndMatch()
{
	Super::EndMatch();
}
