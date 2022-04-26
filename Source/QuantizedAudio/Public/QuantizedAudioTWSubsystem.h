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
	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay = "bAutoStart, bCustomFadeDuration"))
	UQuantizedAudioTrackInstance* PlayQuantizedAudioFromAsset(FName TrackName, class UQuantizedAudioTrackPDAsset* TrackAsset, bool bAutoStart = true, bool bCustomFadeDuration = false);

	UFUNCTION(BlueprintCallable, meta = (AdvancedDisplay = "bAutoStart, bCustomFadeDuration"))
	UQuantizedAudioTrackInstance* PlayQuantizedAudio(FName TrackName, FQuantizedAudioCue AudioCue, bool bAutoStart = true, bool bCustomFadeDuration = false);

	UFUNCTION(BlueprintCallable)
	void StopQuantizedAudio(FName TrackName);

	UFUNCTION(BlueprintCallable)
	void ResumeQuantizedAudio(FName TrackName);

	UPROPERTY(BlueprintReadOnly, Category="Quantized Audio | BGM")
	class UQuantizedAudioTrackPDAsset* CurrentBGMTrackAsset;
	UPROPERTY(BlueprintReadOnly, Category = "Quantized Audio | BGM")
	FName CurrentBGMTrackName;
	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | BGM")
	void SwapBGM(class UQuantizedAudioTrackPDAsset* BGMTrackAsset, bool bForceSwap = false);
	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | BGM")
	void PauseBGM();
	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | BGM")
	void ResumeBGM();
	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | BGM")
	void RestartBGM();

	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | Event Music")
	void StartEventBGM(class UQuantizedAudioTrackPDAsset* BGMTrackAsset);
	UFUNCTION(BlueprintCallable, Category = "Quantized Audio | Event Music")
	void EndEventBGM(bool bResumeBGM = true);

	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface
	virtual bool IsTickableWhenPaused() const override { return true; }
	
	UPROPERTY(BlueprintReadWrite)
	TMap<FName, class UQuantizedAudioTrackInstance*> AudioTrackInstanceMap;

protected:
	UFUNCTION()
	FName CompileTrackName(FName InTrackName);

	UFUNCTION()
	TArray<FName> FilterAudioTrackWithName(FName InTrackName, bool bShouldBeActive = false);

	TMap<FName,int32> TrackIndexes;
};
