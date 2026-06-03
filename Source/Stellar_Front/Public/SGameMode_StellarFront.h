// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SGameState.h"
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
	
protected:
	
	
	// UPROPERTY()
	// bool bReady = false;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "GameMode")
	int32 MaxPlayerPerTeam = 20;	
	
	
	void AssignTeam(APlayerState* PlayerState);
	
	void SetPhase(EGamePhase NewPhase);
	
};
