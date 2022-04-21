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


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class QUANTIZEDAUDIO_API UQuantizedAudioTrackPDAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FQuantizedAudioTrack> Tracks;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName = "BPM"))
	float BeatsPerMinute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FQuartzClockSettings QuartzClockSettings;
};
