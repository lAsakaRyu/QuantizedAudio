// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuantizeAudioTrackInstance.generated.h"

/**
 * 
 */
UCLASS()
class QUANTIZEDAUDIO_API UQuantizeAudioTrackInstance : public UObject
{
	GENERATED_BODY()
public:
	FName TrackName;

	UPROPERTY()
	class UQuantizedAudioTrackPDAsset* TrackAsset;

	void Init(FName InTrackName, class UQuantizedAudioTrackPDAsset* InTrackAsset);

	UPROPERTY()
	FOnQuartzCommandEventBP OnQuartzCommandEvent;
	UFUNCTION()
	void QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name);

	UPROPERTY()
	FOnQuartzMetronomeEventBP OnQuartzMetronomeEvent;
	UFUNCTION()
	void QuartzMetronome(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);

	void CreateAudioTrack();

	UPROPERTY()
	class UQuartzSubsystem* QuartzSubsystem;
	UPROPERTY()
	class UQuartzClockHandle* QuartzClockHandle;

	int32 CurrentIndex;

	UPROPERTY()
	TMap<int32, UAudioComponent*> AudioTrackComponents;

};
