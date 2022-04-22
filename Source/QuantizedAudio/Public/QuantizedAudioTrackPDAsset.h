// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuantizedAudioTrackPDAsset.generated.h"

USTRUCT(BlueprintType)
struct FQuantizedAudioTrack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoundBase* Track;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsLooping;
};

USTRUCT(BlueprintType)
struct FQuantizedAudioCue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FQuantizedAudioTrack> TrackList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "BPM"))
	float BeatsPerMinute = 60.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FQuartzClockSettings QuartzClockSettings;
};


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class QUANTIZEDAUDIO_API UQuantizedAudioTrackPDAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FQuantizedAudioCue QuantizedAudioCue;

};
