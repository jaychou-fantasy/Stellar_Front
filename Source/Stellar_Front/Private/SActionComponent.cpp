// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionComponent.h"

#include "SAction.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
USActionComponent::USActionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void USActionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//server only
	if (GetOwner()->HasAuthority())
	{
		for (TSubclassOf<USAction> ActionClass : DefaultActionsClasses)
		{
			AddActionFromClass(GetOwner(),ActionClass);
		}
	}
}

void USActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//Stop All
	//why do the copy:  because we call "Stop Action"--this triggered "Remove Action" from Actions.
	//but you can't remove action from Actions while you are iterating it;
	TArray<USAction*> ActionsCpy = Actions;
	for (USAction* Action : ActionsCpy)
	{
		if (Action && Action->IsRunning())
		{
			Action->StopAction(GetOwner());
		}
	}
	
	//complete your job, then "EndPlay"
	Super::EndPlay(EndPlayReason);
}

void USActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//just for debugging
	for (USAction* Action : Actions)
	{
		FColor TextColor = Action->IsRunning() ? FColor::Blue : FColor::White;
		FString ActionMsg = FString::Printf(TEXT("[%s] Action: %s : IsRunning: %s"),*GetNameSafe(GetOwner()),*Action->ActionName.ToString(),Action->IsRunning() ? TEXT("true") : TEXT("false"));
		//@fixme:LogOnScreen
		//LogOnScreen(this, ActionMsg, TextColor, 0.0f);
	}
}

void USActionComponent::ServerStartActionByName_Implementation(AActor* Instigator, FName ActionName)
{
	StartActionByName(Instigator,ActionName);
}

void USActionComponent::ServerStopActionByName_Implementation(AActor* Instigator, FName ActionName)
{
	StopActionByName(Instigator,ActionName);
}


void USActionComponent::AddActionFromClass(AActor* Instigator, TSubclassOf<USAction> ActionClass)
{
	if (!ensure(ActionClass))
	{
		return;
	}
	
	//skip for client (double insurance)
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client attempting to AddAction. [Class: %s]"), *GetNameSafe(ActionClass));
		return;
	}
	/* The Outer can be thought of as a container. When we create a new action here with ActionComponent as the Outer,
	 if the ActionComponent (the Outer) is destroyed, all actions within it will also be destroyed.
	 Here, the action's Outer is set to ActionComponent.
	 This means action->GetOuter() returns ActionComponent, and action->GetOuter()->GetOuter() returns the SCharacter.*/
	USAction* NewAction = NewObject<USAction>(GetOuter(),ActionClass);
	NewAction->Initialize(this);//bind to belonged ActionComponent
	
	if (ensure(NewAction))
	{
		Actions.Add(NewAction);
		UE_LOG(LogTemp, Log, TEXT("Add action named:%s"), *GetNameSafe(NewAction));
		
		// Next, we design a way to automatically add actions without needing to specify them in DefaultActions,
		//auto add (depending on bAutoStart)
		if (NewAction->bAutoStart && NewAction->CanStart(Instigator))
		{
			NewAction->StartAction(Instigator);
		}
	}
}

void USActionComponent::RemoveAction(USAction* ActionToRemove)
{
	if (!ensure(ActionToRemove) && !ActionToRemove->IsRunning())
	{
		return;//that means this action didn't need to be removed
	}
	Actions.Remove(ActionToRemove);
}

USAction* USActionComponent::GetAction(TSubclassOf<USAction> ActionClass) const
{
	for(USAction* Action : Actions)
	{
		//SubclassToCheck->IsA(SomeParentClass)：is a function to check if SubclassToCheck is a subclass of the SomeParentClass
		if (Action && Action->IsA(ActionClass))
		{
			return Action;
		}
	}
	return nullptr;
}

//just give a logic. And Called In "ServerStartActionByName"
bool USActionComponent::StartActionByName(AActor* Instigator, FName ActionName)
{
	for (USAction* Action :Actions)
	{
		if (Action && Action->ActionName == ActionName)
		{
			UE_LOG(LogTemp, Warning, TEXT("Checking Action: %s"), *Action->ActionName.ToString());
			if (!Action->CanStart(Instigator))
			{
				FString FailingMsg = FString::Printf(TEXT("Failed to run: %s"), *ActionName.ToString());
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FailingMsg);
				// -1 means create a new message every time
				continue;
			}
			//If on client,Call by Sevrer_StartActionByName
			//If on Server,just run its logic
			if (!GetOwner()->HasAuthority())
			{
				ServerStartActionByName(Instigator,ActionName);
			}
			Action->StartAction(Instigator);
			return true;
			/* @fixme: If actions performed on the server cannot replicate to the client,
			 we need to replicate the actions container and the bIsRunning state.
			 First replicate the actions, then replicate StartAction.*/
		}
	}
	return false;
}

bool USActionComponent::StopActionByName(AActor* Instigator, FName ActionName)
{
	for (USAction* Action :Actions)
	{
		if (Action && Action->ActionName == ActionName)
		{
			if (Action->IsRunning())
			{
				if (!GetOwner()->HasAuthority())
				{
					ServerStopActionByName(Instigator,ActionName);
				}
				Action->StopAction(Instigator);
				return true;
			}
		}
	}
	return false;
}



// Here, Channel refers to the replication channel between a UObject subclass on the server and its counterpart on the client.
// Bunch -> network data packet
// RepFlags -> replication configuration flags
bool USActionComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,FReplicationFlags* RepFlags)
{
	// Call super first, which returns false initially. Only becomes true if any changes occur later, and the " | " operator will return true.
	bool WroteSomething =  Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (USAction* Action : Actions)
	{
		// Serializes the state of a UObject into the network data packet and sends it to the corresponding client (or receiver)
		WroteSomething |= Channel->ReplicateSubobject(Action,*Bunch,*RepFlags);
		// Logical OR: if either side is true, the result is true.
		/* If Channel successfully replicates a change in any Action (from server to client), it returns true.
		 This means that as long as at least one Action successfully syncs, the function returns true (and that Action is replicated).*/
	}
	return WroteSomething;
}
// It's fine that the return value isn't used elsewhere because Channel->ReplicateSubobject(Action, *Bunch, *RepFlags) already handles replicating the Action
// (synchronizing server changes to the client).
// Client-to-server replication is handled via ServerStartActionByName.


//"USAction" itself Replicate the "State of Action"
//"USActionComponent" Replicate the "Actions" themself
void USActionComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(USActionComponent,Actions);
}