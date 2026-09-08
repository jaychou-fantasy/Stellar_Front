// Fill out your copyright notice in the Description page of Project Settings.

#include "World/Objectives/SControlNode.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/Match/SGameMode_StellarFront.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ASControlNode::ASControlNode()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	CaptureArea = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureArea"));
	SetRootComponent(CaptureArea);
	CaptureArea->InitSphereRadius(500.0f);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CaptureArea->SetCollisionObjectType(ECC_WorldDynamic);
	
	CaptureArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	CaptureArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CaptureArea->SetGenerateOverlapEvents(true);
	
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CaptureArea);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASControlNode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ASGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASGameState>() : nullptr;
		if (GameState)
		{
			CachedGameState = GameState;
			GameState->OnPhaseChanged.AddDynamic(this, &ASControlNode::HandlePhaseChanged);
			HandlePhaseChanged(GameState->GetPhase());
		}
	}
}

void ASControlNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ASGameState* GameState = CachedGameState.Get())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &ASControlNode::HandlePhaseChanged);
	}
	GetWorldTimerManager().ClearTimer(CaptureTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ASControlNode::HandlePhaseChanged(EGamePhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(CaptureTimerHandle);
	if (NewPhase != EGamePhase::OrbitalCombat || bCompleted)
	{
		ControllingTeam = ETeam::None;
		return;
	}

	GetWorldTimerManager().SetTimer(CaptureTimerHandle, this, &ASControlNode::UpdateCapture, CaptureTickInterval, true);
}

void ASControlNode::UpdateCapture()
{
	if (bCompleted)
	{
		return;
	}

	//get overlap actors
	TArray<AActor*> OverlappingActors;
	CaptureArea->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	bool bHasRedPlayer = false;
	bool bHasBluePlayer = false;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		APawn* Pawn = Cast<APawn>(OverlappingActor);
		ASPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<ASPlayerState>() : nullptr;
		if (!PlayerState || !PlayerState->IsAlive())
		{
			continue;
		}

		switch (PlayerState->GetTeam())
		{
		case ETeam::Red:
			bHasRedPlayer = true;
			break;
		case ETeam::Blue:
			bHasBluePlayer = true;
			break;
		default:
			break;
		}
	}

	if (bHasRedPlayer == bHasBluePlayer)
	{
		ControllingTeam = ETeam::None;
		return;
	}

	ControllingTeam = bHasRedPlayer ? ETeam::Red : ETeam::Blue;
	if (!bHasRedPlayer)
	{
		return;
	}

	CaptureProgress = FMath::Clamp(/// refrain the risk that the duration can be 0.  (0 can divide nothing)
		CaptureProgress + CaptureTickInterval / FMath::Max(CaptureDuration, KINDA_SMALL_NUMBER), 
		0.0f, 1.0f);
	if (CaptureProgress < 1.0f)
	{
		return;
	}

	ASGameMode_StellarFront* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASGameMode_StellarFront>() : nullptr;
	if (GameMode && GameMode->CompleteRedControlNode())
	{
		bCompleted = true;
		GetWorldTimerManager().ClearTimer(CaptureTimerHandle);
	}
}

void ASControlNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASControlNode, ControllingTeam);
	DOREPLIFETIME(ASControlNode, CaptureProgress);
}
