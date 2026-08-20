// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Framework/Match/SGameState.h"
#include "Framework/Player/SPlayerController.h"
#include "Framework/Player/SPlayerState.h"
#include "SGameMode_StellarFront.generated.h"



/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API ASGameMode_StellarFront : public AGameMode
{
	GENERATED_BODY()
	
public:
	ASGameMode_StellarFront();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	virtual bool ReadyToStartMatch_Implementation() override;
	
	virtual void HandleMatchHasStarted() override;
	
	virtual bool ReadyToEndMatch_Implementation() override;
	
	virtual void HandleMatchHasEnded() override;
	
	virtual void Logout(AController* Exiting) override;

	virtual void StartMatch() override;
	virtual void EndMatch() override;
	
	void StartDeployment();
	void EndDeployment();

public:
	void HandlePlayerDeath(AActor* Instigator,APawn* VictimPawn);
	
	
	//@fixme: set to protected if done
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GameMode")
	int32 MaxPlayerPerTeam = 20;
protected:
	//you can find corresponding TimerHandle when called "CancelRespawn" in "Log out || Handle Match Ended"
	TMap<TWeakObjectPtr<ASPlayerController>,FTimerHandle> PendingRespawnTimers;
	
	void CancelRespawn(ASPlayerController* PlayerController);
	
	// UPROPERTY()
	// bool bReady = false;
	void RespawnPlayer(TWeakObjectPtr<ASPlayerController> PlayerController);
	
	UPROPERTY(EditDefaultsOnly,Category = "GameMode")
	float RespawnDelay = 2.0f;
	
	void AssignTeam(ASPlayerState* PlayerState);
	
	int32 CountPlayersInTeam(ETeam TargetTeam,const APlayerState* PlayerToIgnore = nullptr) const;
	
	void SetPhase(EGamePhase NewPhase);
	
};
