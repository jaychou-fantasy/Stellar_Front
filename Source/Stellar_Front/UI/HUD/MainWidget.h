// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainWidget.generated.h"

/**
 * 
 */
UCLASS()
class STELLAR_FRONT_API UMainWidget : public UUserWidget
{
	GENERATED_BODY()
	
	
public:
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	void UpdateAmmo(int32 MagAmmo,int32 TotalAmmo);
};
