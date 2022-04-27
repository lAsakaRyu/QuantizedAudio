// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuantizedAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "QuantizedAudioTrackPDAsset.generated.h"

USTRUCT(BlueprintType)
struct FQuantizedAudioTrack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoundBase* Track;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsLooping;

	FQuantizedAudioTrack() :
		Track(nullptr),
		bIsLooping(false)
	{
	}

	bool operator==(const FQuantizedAudioTrack& Other) const
	{
		return Track == Other.Track;
	}

	bool operator!=(const FQuantizedAudioTrack& Other) const
	{
		return Track != Other.Track;
	}
};

USTRUCT(BlueprintType)
struct FQuantizedAudioCue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FQuantizedAudioTrack> TrackList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "BPM"))
	float BeatsPerMinute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FQuartzClockSettings QuartzClockSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FQuantizedAudioFadeSettings FadeSetting;

	FQuantizedAudioCue() :
		TrackList(),
		BeatsPerMinute(60.f),
		QuartzClockSettings(),
		FadeSetting()
	{}

	bool operator==(const FQuantizedAudioCue& Other) const
	{
		return TrackList == Other.TrackList;
	}

	bool operator!=(const FQuantizedAudioCue& Other) const
	{
		return TrackList != Other.TrackList;
	}
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
