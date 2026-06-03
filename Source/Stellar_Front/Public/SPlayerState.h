// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPlayerState.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	Red,
	Blue
};

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
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

	// -- Getters --
	ETeam GetTeam() const { return Team; }
	EPlayerRole GetRole() const { return Role; }

	// -- Setters (server-only, HasAuthority guarded) --
	void SetTeam(ETeam NewTeam)           { if (HasAuthority()) Team = NewTeam; }
	void SetRole(EPlayerRole NewRole)      { if (HasAuthority()) Role = NewRole; }
	void SetCarryingKey(bool bCarry)       { if (HasAuthority()) bIsCarryingKey = bCarry; }
	void SetIsAlive(bool bAlive)           { if (HasAuthority()) bIsAlive = bAlive; }
	void MarkEvacuated()                   { if (HasAuthority()) bHasEvacuated = true; }

	// -- Stats helpers (server-only) --
	void AddKills()  { if (HasAuthority()) Kills++; }
	void AddDeaths() { if (HasAuthority()) Deaths++; }
	//void AddAssists() { if (HasAuthority()) Assists++; } 


protected:
	UPROPERTY(Replicated,BlueprintReadOnly)
	ETeam Team;
	
	UPROPERTY(Replicated,BlueprintReadOnly)
	EPlayerRole Role = EPlayerRole::Assault;
	
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
