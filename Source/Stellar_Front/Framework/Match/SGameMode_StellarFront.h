// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Framework/Match/SGameState.h"
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
	
	virtual bool ReadyToStartMatch_Implementation() override;
	
	virtual void HandleMatchHasStarted() override;
	
	virtual bool ReadyToEndMatch_Implementation() override;
	
	virtual void HandleMatchHasEnded() override;
	
	virtual void Logout(AController* Exiting) override;

	virtual void StartMatch() override;
	virtual void EndMatch() override;
	
	void StartDeployment();
	void EndDeployment();
	
	//@fixme: set to protected if done
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "GameMode")
	int32 MaxPlayerPerTeam = 20;
protected:
	
	
	// UPROPERTY()
	// bool bReady = false;
	
		
	
	int32 RedCnt = 0;
	int32 BlueCnt = 0;
	
	void AssignTeam(APlayerState* PlayerState);
	
	void SetPhase(EGamePhase NewPhase);
	
};
