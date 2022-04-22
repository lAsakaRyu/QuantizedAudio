// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "QuantizedAudioTWSubsystem.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Quantized Audio"))
class QUANTIZEDAUDIO_API UQuantizedAudioTWSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	UQuartzClockHandle* PlayQuantizedAudioFromAsset(FName TrackName, class UQuantizedAudioTrackPDAsset* TrackAsset);

	UFUNCTION(BlueprintCallable)
	UQuartzClockHandle* PlayQuantizedAudio(FName TrackName, FQuantizedAudioCue AudioCue);

	UFUNCTION(BlueprintCallable)
	void StopQuantizedAudio(FName TrackName);

	UFUNCTION(BlueprintCallable)
	void ResumeQuantizedAudio(FName TrackName);

	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface
	virtual bool IsTickableWhenPaused() const override { return true; }
	
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, class UQuantizedAudioTrackInstance*> AudioTrackInstanceMap;


};
