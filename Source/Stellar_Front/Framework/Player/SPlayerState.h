// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPlayerState.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	None,
	Red,
	Blue
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, ETeam, NewTeam);

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	None,
	Assault,
	Engineer
};

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API ASPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ASPlayerState();

	UPROPERTY(BlueprintAssignable, Category = "Player|Team")
	FOnTeamChanged OnTeamChanged;

	// -- Getters --
	UFUNCTION(BlueprintPure)
	ETeam GetTeam() const { return Team; }

	EPlayerRole GetRole() const { return PlayerRole; }
	
	void SetReady() { if (HasAuthority()) bReady = true; }
	bool GetReady() const { return bReady; }
	
	
	
	// -- Setters (server-only, HasAuthority guarded) --
	void SetTeam(ETeam NewTeam);
	void SetRole(EPlayerRole NewRole)      { if (HasAuthority()) PlayerRole = NewRole; }
	void SetCarryingKey(bool bCarry)       { if (HasAuthority()) bIsCarryingKey = bCarry; }
	void SetIsAlive(bool bAlive)           { if (HasAuthority()) bIsAlive = bAlive; }
	void MarkEvacuated()                   { if (HasAuthority()) bHasEvacuated = true; }

	// -- Stats helpers (server-only) --
	void AddKills()  { if (HasAuthority()) Kills++; }
	void AddDeaths() { if (HasAuthority()) Deaths++; }
	//void AddAssists() { if (HasAuthority()) Assists++; } 


	//Getter
	bool IsAlive() const { return bIsAlive; }
	int32 GetKills() const { return Kills; }
	int32 GetDeaths() const { return Deaths; } 

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Team, BlueprintReadOnly)
	ETeam Team = ETeam::None;

	UFUNCTION()
	void OnRep_Team();
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	EPlayerRole PlayerRole = EPlayerRole::None;
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	bool bReady = false;
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	int32 Kills = 0;
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	int32 Deaths = 0;
	
	/*UPROPERTY(Replicated,BlueprintReadOnly)
	int32 Assists;*/
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	bool bIsCarryingKey = false;
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	bool bHasEvacuated = false;

	UPROPERTY(Replicated,BlueprintReadOnly)
	bool bIsAlive = true;
};
