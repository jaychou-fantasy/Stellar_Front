// Fill out your copyright notice in the Description page of Project Settings.


#include "STeleportor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"



// Sets default values
ASTeleportor::ASTeleportor()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>("MeshComp");
	RootComponent = MeshComp;
	
	ArrowComp = CreateDefaultSubobject<UArrowComponent>("ArrowComp");
	ArrowComp->SetupAttachment(RootComponent);
	
	TeleportDelay = 1.0f;
}

// Called when the game starts or when spawned
void ASTeleportor::BeginPlay()
{
	Super::BeginPlay();
	
}


void ASTeleportor::Interact_Implementation(APawn* InstigatorPawn)
{
	//@fixme: show Option UI
	FTimerHandle TimerHandle_DelatedTeleport;//"this" is the Object that your Function in.
	FTimerDelegate Delegate;
	Delegate.BindUFunction(this,"TeleportInstigator",InstigatorPawn);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_DelatedTeleport,Delegate,TeleportDelay,false);
	
}

void ASTeleportor::TeleportInstigator(APawn* InstigatorPawn)
{
	if (ensure(InstigatorPawn))
	{
		FVector Destination = TargetTeleportor->GetActorLocation() + ArrowComp->GetForwardVector() * 100.0f;
		FRotator Rotation = (-TargetTeleportor->GetArrow()->GetForwardVector()).Rotation();
		//face to Teleportor Device;---you can change it.
		
		InstigatorPawn->TeleportTo(Destination,Rotation,false,true);
		UE_LOG(LogTemp, Warning, TEXT("TeleportInstigator Called!"));
		// bNOCheck = true means it doesn't check for capsule collision with the ground or other overlaps -- teleports immediately
		/*@fixme: add UI-->Teleporting successfully
		 use TeleportName  &  bTeleported  for insurance */
	}
}



