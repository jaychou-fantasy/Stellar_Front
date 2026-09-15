#pragma once

#include "CoreMinimal.h"
#include "Framework/Match/SGameState.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Interaction/SGameplayInterface.h"
#include "SNetworkKey.generated.h"

class ASPlayerState;
class UStaticMeshComponent;

UCLASS()
class STELLAR_FRONT_API ASNetworkKey : public AActor , public ISGameplayInterface
{
	GENERATED_BODY()

public:
	ASNetworkKey();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	
	//calling true logic	
	bool DropKey(ASPlayerState* HolderJustNow,const FVector &DropLocation);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetworkKey")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive, BlueprintReadOnly, Category = "NetworkKey")
	bool bIsActive = false;

	UFUNCTION()
	void HandlePhaseChanged(EGamePhase NewPhase);

	UFUNCTION()
	void OnRep_IsActive();
	
	void ApplyActiveState();

	TWeakObjectPtr<ASGameState> CachedGameState;
};
