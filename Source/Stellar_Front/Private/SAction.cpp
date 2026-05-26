// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction.h"
#include "SActionComponent.h"
#include "Net/UnrealNetwork.h"

void USAction::Initialize(USActionComponent* NewActionComp)
{
	ActionComp = NewActionComp;
}

USActionComponent* USAction::GetOwningComponent() const
{
	return ActionComp;
}

bool USAction::IsRunning() const
{
	return RepData.bIsRunning;
}

bool USAction::CanStart_Implementation(AActor* Instigator)
{
	USActionComponent* AC = GetOwningComponent();
	if (IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("Action already running"));
		return false;
	}
	if (AC->ActiveGameplaytags.HasAny(BlockedTags))
	{
		return false;
	}
	return true;
}


// This implements the basic start and end notifications in UASAction. 
//use Tag--demonstrate "the State of Action".
void USAction::StartAction_Implementation(AActor* Instigator)
{
	UE_LOG(LogTemp, Log, TEXT("Running: %s"), *GetNameSafe(this));

	USActionComponent* AC = GetOwningComponent();
	AC->ActiveGameplaytags.AppendTags(GrantTags);
	
	RepData.bIsRunning = true;
	RepData.Instigator = Instigator;
	
	//server get Time_Start
	//@fixme: to accomplish ACTION UI effect
	TimeStarted = GetWorld()->GetTimeSeconds();
	
	GetOwningComponent()->OnActionStarted.Broadcast(AC,this);
}

void USAction::StopAction_Implementation(AActor* Instigator)
{	
	UE_LOG(LogTemp, Log, TEXT("Stopped: %s"), *GetNameSafe(this));
	
	USActionComponent* AC = GetOwningComponent();
	
	RepData.bIsRunning = false;
	RepData.Instigator = Instigator;
	
	GetOwningComponent()->OnActionStopped.Broadcast(AC,this);
}	

// Only AActor, UActorComponent, and UWorldSubsystem have native awareness of the world
//this function ----- let USAction know "World"
UWorld* USAction::GetWorld() const
{
	// Outer is set when creating the action via NewObject<T> (the "this" at that time becomes the Outer, which is of type ActionComponent)
	AActor* Actor = Cast<AActor>(GetOuter());
	if (Actor)
	{
		return Actor->GetWorld();
	}
	return nullptr;
}

void USAction::OnRep_RepData()
{
	//because we replicate the "RepData"(not the state of action itself)
	//so we have to use this function to update "Action State"
	if (RepData.bIsRunning)
	{
		StartAction(RepData.Instigator);
	}
	else
	{
		StopAction(RepData.Instigator);
	}
}

void USAction::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USAction,RepData);
	DOREPLIFETIME(USAction,ActionComp);
}