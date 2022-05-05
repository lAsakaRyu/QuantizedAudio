// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuantizedAudio.h"
#include "QuantizedAudioSettings.generated.h"

#define DEFAULT_FADE_DURATION 0.5f

USTRUCT(BlueprintType)
struct FQuantizedAudioFadeSettings
{
	GENERATED_BODY()

public:
	/** Fade in duration for Quantized Audio operation. */
	UPROPERTY(Config, EditAnywhere, Category = "Quantized Audio")
	float FadeInDuration;

	/** Fade out duration for Quantized Audio operation. */
	UPROPERTY(Config, EditAnywhere, Category = "Quantized Audio")
	float FadeOutDuration;

	FQuantizedAudioFadeSettings() :
		FadeInDuration(DEFAULT_FADE_DURATION),
		FadeOutDuration(DEFAULT_FADE_DURATION)
	{};

	FQuantizedAudioFadeSettings(float InFadeInDuration, float InFadeOutDuration) :
		FadeInDuration(InFadeInDuration),
		FadeOutDuration(InFadeOutDuration)
	{};
};

/**
 * 
 */
UCLASS(config = Game, defaultconfig)
class QUANTIZEDAUDIO_API UQuantizedAudioSettings : public UObject
{
	GENERATED_BODY()
	
public:
	/** Default global track fade settings. */
	UPROPERTY(Config, EditAnywhere, Category = "Quantized Audio")
	FQuantizedAudioFadeSettings GlobalFadeSettings;

	/** Default specific track fade settings. */
	UPROPERTY(Config, EditAnywhere, Category = "Quantized Audio")
	TMap<FName, FQuantizedAudioFadeSettings> SpecificFadeSettings;

	/** Show Debug Logs. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	bool bShowDebugLog;

	UQuantizedAudioSettings() :
		GlobalFadeSettings(),
		SpecificFadeSettings(
			{
				{ QA_BGM, FQuantizedAudioFadeSettings(1.5f , 1.f) }
			}
		),
		bShowDebugLog(false)
	{};
};
