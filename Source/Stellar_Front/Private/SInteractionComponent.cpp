// Fill out your copyright notice in the Description page of Project Settings.


#include "SInteractionComponent.h"

#include "SGameplayInterface.h"

static TAutoConsoleVariable<bool> CVarDrawInteraction(TEXT("su.InteractionDrawDebug"),false,TEXT("Enable Debug Lines for Interaction Component"));

USInteractionComponent::USInteractionComponent()
{
	//since we throw a ray to detect whether there's a object to be interacted in front of us,we need this to be true
	PrimaryComponentTick.bCanEverTick = true;
	
	TraceDistance = 500.0f;
	TraceRadius = 30.0f;
	CollisionChannel = ECC_WorldDynamic;
	
}
// Called when the game starts
void USInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}
void USInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	APawn* MyPawn = Cast<APawn>(GetOwner());
	/* For example, on the client, there are interaction components for you and other players.
	 On your client, all these interaction components would run Interact logic continuously.
	 This check ensures only the locally controlled player runs this function for interaction.
	 IsLocallyControlled allows us to execute this only for the character I am locally controlling.
	 However, this also means that on the client, Player 1's "Press E" widget will also be displayed for Player 2, Player 3, etc.*/
	if (MyPawn->IsLocallyControlled())
	{
		FindBestInteractable();
	}
}

//use ray to find
void USInteractionComponent::FindBestInteractable()
{
	bool bDebugDraw = CVarDrawInteraction.GetValueOnGameThread();
	
	AActor* MyActor = GetOwner();
	
	TArray<FHitResult> Hits;
	
	FVector EyeLocation;
	FRotator EyeRotation;
	MyActor->GetActorEyesViewPoint(EyeLocation,EyeRotation);
	FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * TraceDistance);
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
	
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(TraceRadius);
	
	bool bBlockingHit = GetWorld()->SweepMultiByObjectType(Hits,EyeLocation,TraceEnd,FQuat::Identity,ObjectQueryParams,CollisionShape);
	//FQuat::Identity  means "Empty Rotation";
	
	//Empty ref before filling it
	FocusedActor = nullptr;
	
	FColor LineColor = bBlockingHit? FColor::Green : FColor::Red;
	for (FHitResult Hit :Hits)
	{
		if (bDebugDraw)
		{
			DrawDebugSphere(GetWorld(), Hit.ImpactPoint, TraceRadius, 32, LineColor, false, 2.0f);
			// 32 is the number of segments for the sphere
		}
		
		AActor* HitActor = Hit.GetActor();
		if (HitActor)
		{
			if (HitActor->Implements<USGameplayInterface>())
			{
				//Populate FocusActor
				FocusedActor = HitActor;
				break;//only detect the first Actor we hit
			}
		}
	}
	if (FocusedActor)
	{
		//@fixme : create Interaction Prompt Widget
	}
}

void USInteractionComponent::PrimaryInteract()
{
	ServerInteract(FocusedActor);
}

void USInteractionComponent::ServerInteract_Implementation(AActor* Infocus)
{
	if (Infocus == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "No Focus Actor To Interact!");
		return;
	}
	
	APawn* MyPawn = Cast<APawn>(GetOwner());
	ISGameplayInterface::Execute_Interact(Infocus,MyPawn);
}