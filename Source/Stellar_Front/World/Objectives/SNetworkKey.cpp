#include "World/Objectives/SNetworkKey.h"

#include "Components/StaticMeshComponent.h"
#include "Framework/Player/SPlayerState.h"
#include "Net/UnrealNetwork.h"

ASNetworkKey::ASNetworkKey()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
}

void ASNetworkKey::BeginPlay()
{
	Super::BeginPlay();

	ApplyActiveState();

	if (!HasAuthority())
	{
		return;
	}

	ASGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASGameState>() : nullptr;
	if (GameState)
	{
		//cache gamestate
		CachedGameState = GameState;
		GameState->OnPhaseChanged.AddDynamic(this, &ASNetworkKey::HandlePhaseChanged);
		HandlePhaseChanged(GameState->GetPhase());
	}
}

void ASNetworkKey::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ASGameState* GameState = CachedGameState.Get())
	{
		GameState->OnPhaseChanged.RemoveDynamic(this, &ASNetworkKey::HandlePhaseChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void ASNetworkKey::HandlePhaseChanged(EGamePhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bShouldBeActive = NewPhase == EGamePhase::SearchKey;
	if (bIsActive == bShouldBeActive)
	{
		return;
	}

	bIsActive = bShouldBeActive;
	ApplyActiveState();//run in server
	//force a net update to faster call "OnRep
	ForceNetUpdate();
}

void ASNetworkKey::OnRep_IsActive()
{
	//run in client 
	ApplyActiveState();
}

void ASNetworkKey::ApplyActiveState()
{
	MeshComp->SetHiddenInGame(!bIsActive, true);
	MeshComp->SetCollisionEnabled(bIsActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}


void ASNetworkKey::Interact_Implementation(APawn* InstigatorPawn)
{
	//all happen in server
	if (!HasAuthority() || !bIsActive || !IsValid(InstigatorPawn))
	{
		return; 
	}
	
	ASGameState* GameState = CachedGameState.Get();//same reason for not using "const" as below
	if (!GameState || GameState->GetPhase() != EGamePhase::SearchKey)
	{
		return;
	}
	
	//@fixme :: now time , only Team Red can hold the key
	ASPlayerState* PlayerState = InstigatorPawn->GetPlayerState<ASPlayerState>();//we will chanege the bIsCarryingKey state ,so don't use "const"
	if (!PlayerState || PlayerState->GetTeam() != ETeam::Red || !PlayerState->IsAlive())
	{
		return;
	}
	
	//true logic of calling function (set key holder & bKeyFound)
	PlayerState->SetCarryingKey(true);
	GameState->SetKeyHolder(PlayerState);
	UE_LOG(LogTemp, Log, TEXT("[DropKey] Key picked up: Key=%s Holder=%s"), *GetNameSafe(this), *GetNameSafe(PlayerState));
	
	bIsActive = false;//has been already gotten,set hidden and no collision
	//bIsActive change to call OnRep ----- change the state of client
	ApplyActiveState();//manually call in server
	ForceNetUpdate();
}

bool ASNetworkKey::DropKey(ASPlayerState* HolderJustNow, const FVector& DropLocation)
{
	UE_LOG(LogTemp, Log, TEXT("[DropKey] Key received drop request: Key=%s Authority=%s RequestedHolder=%s"),
		*GetNameSafe(this),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(HolderJustNow));

	//only server
	if (!HasAuthority() || !IsValid(HolderJustNow))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropKey] Key rejected request: no authority or invalid requested holder."));
		return false;
	}
	ASGameState* GameState = CachedGameState.Get();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropKey] Key rejected request: cached GameState is invalid."));
		return false;
	}
	if (GameState->GetPhase() != EGamePhase::SearchKey || bIsActive || !HolderJustNow->IsCarryingKey() || GameState->GetKeyHolder() != HolderJustNow)//only Inactive key can be dropped
	{
		UE_LOG(LogTemp, Warning, TEXT("[DropKey] Key rejected state: Phase=%d Active=%s Carrying=%s RecordedHolder=%s RequestedHolder=%s"),
			static_cast<int32>(GameState->GetPhase()),
			bIsActive ? TEXT("true") : TEXT("false"),
			HolderJustNow->IsCarryingKey() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(GameState->GetKeyHolder()),
			*GetNameSafe(HolderJustNow));
		return false;
	}
	
	//teleport hidden key to Droppped Location.
	SetActorLocation(DropLocation,false,nullptr,ETeleportType::ResetPhysics);
	
	//call logic
	HolderJustNow->SetCarryingKey(false);
	GameState->SetKeyHolder(nullptr);
	
	//reset key to active state
	bIsActive = true;//onrep -- client
	ApplyActiveState();//server
	ForceNetUpdate();
	UE_LOG(LogTemp, Log, TEXT("[DropKey] Key dropped successfully: Key=%s Location=%s"), *GetNameSafe(this), *DropLocation.ToCompactString());
	
	return true;
}

void ASNetworkKey::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASNetworkKey, bIsActive);
}
