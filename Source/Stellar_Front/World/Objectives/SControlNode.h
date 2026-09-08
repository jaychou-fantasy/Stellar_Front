// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Match/SGameState.h"
#include "Framework/Player/SPlayerState.h"
#include "GameFramework/Actor.h"
#include "SControlNode.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class STELLAR_FRONT_API ASControlNode : public AActor
{
	GENERATED_BODY()

public:
	ASControlNode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	float GetCaptureProgress() const { return CaptureProgress; }
	
	UFUNCTION(BlueprintPure)
	ETeam GetControllingTeam() const { return ControllingTeam; }
	
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ControlNode")
	TObjectPtr<USphereComponent> CaptureArea;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "ControlNode")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ControlNode")
	ETeam ControllingTeam = ETeam::None;//@fixme: for now , it can only be Red Team

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ControlNode", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CaptureProgress = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ControlNode", meta = (ClampMin = "0.1"))
	float CaptureDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ControlNode", meta = (ClampMin = "0.05"))
	float CaptureTickInterval = 0.2f;
	
	
	void UpdateCapture();

	UFUNCTION()
	void HandlePhaseChanged(EGamePhase NewPhase);

	FTimerHandle CaptureTimerHandle;
	TWeakObjectPtr<ASGameState> CachedGameState;
	bool bCompleted = false;
};
