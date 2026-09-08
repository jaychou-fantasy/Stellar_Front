// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SGameState.generated.h"


UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	None,
	WarmingUp,
	PreDeploy,
	OrbitalCombat,
	SearchKey,
	UpLoad,
	Evacuation
};

//after define EGamePhase
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged,EGamePhase,NewPhase);


/**
 * 
 */

UCLASS()
class STELLAR_FRONT_API ASGameState : public AGameState
{
	GENERATED_BODY()

public:
	//this function only called in GameMode->SetPhase
	void SetCurrentPhase(EGamePhase NewPhase);
	void SetRedControlNodes(int32 NewValue);
	int32 GetRedControlNodes() const { return RedControlNodes; }
	
	UPROPERTY(BlueprintAssignable)
	FOnPhaseChanged OnPhaseChanged;
	
	UFUNCTION(BlueprintPure)
	EGamePhase GetPhase() const
	{
		return CurrentPhase;
	}
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	EGamePhase CurrentPhase = EGamePhase::None;
	
	UFUNCTION()
	void OnRep_Phase();
	
	UPROPERTY(Replicated, BlueprintReadOnly)
    int32 RedControlNodes = 0;                                                                                            
    UPROPERTY(Replicated, BlueprintReadOnly)                                             
    int32 BlueControlNodes = 0;     
	
    // ===== key =====                                                                                               
    UPROPERTY(ReplicatedUsing = OnRep_KeyStatus, BlueprintReadOnly)                   
    bool bKeyFound = false;                                                                     
	
    UPROPERTY(Replicated, BlueprintReadOnly)
    APlayerState* KeyHolder = nullptr;    
	
    UPROPERTY(Replicated, BlueprintReadOnly)                                           
    FVector UploadZoneLocation = FVector::ZeroVector;   
	
    UPROPERTY(Replicated, BlueprintReadOnly)
    float UploadProgress = 0.0f; 
	
    UFUNCTION()
    void OnRep_KeyStatus();
	
    // ===== evacuate =====               
    UPROPERTY(Replicated, BlueprintReadOnly)                
    FVector EvacShipLocation = FVector::ZeroVector;
	
    UPROPERTY(Replicated, BlueprintReadOnly)                                                             
    float EvacTimeRemaining = 0.0f;     
	
    // ===== score =====                                             
    UPROPERTY(Replicated, BlueprintReadOnly)                                                            
    int32 RedScore = 0;                                                                                                 
    UPROPERTY(Replicated, BlueprintReadOnly)                                                             
    int32 BlueScore = 0;        

};
