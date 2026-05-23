// Fill out your copyright notice in the Description page of Project Settings.

#include "SAttributeComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "SGameMode.h"

static TAutoConsoleVariable<float> CVarDamageMultiplier(TEXT("su.DamageMultiplier"),1.0f,TEXT("Gloabal Damage Modifier for Attribute Component"),ECVF_Cheat);

USAttributeComponent::USAttributeComponent()
{
	HealthMax = 100.0f;
	Health = HealthMax;
	
	//Component-->RepByDefault
	SetIsReplicatedByDefault(true);
}

USAttributeComponent* USAttributeComponent::GetAttributeComp(AActor* FromActor)
{
	if (FromActor)
	{
		return Cast<USAttributeComponent>(FromActor->GetComponentByClass(USAttributeComponent::StaticClass()));
	}
	return nullptr;
}

bool USAttributeComponent::IsActorAlive(AActor* CheckActor)
{
	//in self-comp,there no need to "USAttributeComponent::GetAttributeComp"
	USAttributeComponent* AttributeComp = GetAttributeComp(CheckActor);
	if (AttributeComp)
	{
		return AttributeComp->IsAlive();
	}
	return false;
}

bool USAttributeComponent::IsAlive() const
{
	return Health > 0.0f;
}

bool USAttributeComponent::IsFullHealth() const
{
	return Health == HealthMax;
}

float USAttributeComponent::GetHealthMax() const
{
	return HealthMax;
}

float USAttributeComponent::GetHealth() const
{
	return Health;
}

bool USAttributeComponent::Kill(AActor* Instigator)
{
	//HealthMax is a protected property.While GetHealthMax() is a public function
	return ApplyHealthchange(Instigator,-GetHealthMax());
}


bool USAttributeComponent::ApplyHealthchange(AActor* InstigatorActor, float Delta)
{
	//God Mode
	if (!GetOwner()->CanBeDamaged() && Delta < 0.0f)
	{
		return false;
	}
	//CVar_DamageMultiplier
	if (Delta < 0.0f)
	{
		float DamageMultiplier  = CVarDamageMultiplier.GetValueOnGameThread();
		Delta *= DamageMultiplier;
	}
	
	//true logic
	float OldHealth = Health;
	float NewHealth = FMath::Clamp(Health+Delta,0.0f,HealthMax);
	float ActualDelta = NewHealth - OldHealth;
	//Only Server can make Health Change
	if (GetOwner()->HasAuthority())
	{
		//in server, we grant Health the NewValue;
		Health  = NewHealth;
		if (ActualDelta != 0)
		{
			MulticastHealthChanged(InstigatorActor,Health,ActualDelta);
			UE_LOG(LogTemp, Log, TEXT("ApplyHealthChange: Owner=%s NewHealth=%f Delta=%f"), *GetNameSafe(GetOwner()), Health, ActualDelta);
		}
	}
	
	//Died
	if (ActualDelta < 0 && Health <= 0.0f)
	{
		ASGameMode *GameMode = GetWorld()->GetAuthGameMode<ASGameMode>();
		if (GameMode)
		{
			//@fixme::Add "On Actor Killed" when we accomplish it in GameModebase
		}
	}
	
	return ActualDelta != 0;
	//return true if there was an actuall HealthChange, false vice versa.
	//even on client,that you didn't actually make damage,the "Actual Delta is not equal to 0,return true"
	//that you return true, but you didn't make damage.But tells that you really "TRY to make damage"
}


void USAttributeComponent::MulticastHealthChanged_Implementation(AActor* Instigator, float NewHealth, float Delta)
{
	OnHealthChanged.Broadcast(Instigator,this,Health,Delta);
}

void USAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USAttributeComponent,Health);
	DOREPLIFETIME(USAttributeComponent,HealthMax);
}