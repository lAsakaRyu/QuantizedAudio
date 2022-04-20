// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuantizedAudioGISubsystem.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName = "Quantized Audio"))
class QUANTIZEDAUDIO_API UQuantizedAudioGISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	bool PlayQuantizedAudio();
};
