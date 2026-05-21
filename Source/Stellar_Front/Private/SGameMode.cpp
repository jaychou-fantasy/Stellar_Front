// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SGameMode.h"
#include "SHUD.h"
#include "SCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASGameMode::ASGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/BP_Player"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ASHUD::StaticClass();
}
