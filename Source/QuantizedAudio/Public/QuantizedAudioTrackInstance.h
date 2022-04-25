// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuantizedAudioTrackPDAsset.h"
#include "QuantizedAudioTrackInstance.generated.h"

/**
 * 
 */
UCLASS()
class QUANTIZEDAUDIO_API UQuantizedAudioTrackInstance : public UObject
{
	GENERATED_BODY()
public:
	FName TrackName;
	bool bInitial;

	UPROPERTY()
	FQuantizedAudioCue AudioCue;

	void Init(FName InTrackName, FQuantizedAudioCue InAudioCue, bool bAutoStart = true);

	UPROPERTY()
	FOnQuartzCommandEventBP OnQuartzCommandEvent;
	UFUNCTION()
	void QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name);

	UPROPERTY()
	FOnQuartzMetronomeEventBP OnQuartzMetronomeEvent;
	UFUNCTION()
	void QuartzMetronome(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);

	bool CreateAudioTrack(USoundBase* InSound);
	bool StartAudioTrackAtIndex(int32 Index);
	void StopAudioTrack();
	void ResumeAudioTrack();

	UPROPERTY()
	class UQuartzSubsystem* QuartzSubsystem;
	UPROPERTY()
	class UQuartzClockHandle* QuartzClockHandle;

	int32 CurrentIndex;

	UPROPERTY()
	TArray<UAudioComponent*> AudioTrackComponents;

	UFUNCTION()
	void CheckPendingDestroy();

protected:
	void StopAudioTrackInternal(float FadeOutDuration);
};
