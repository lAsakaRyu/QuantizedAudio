// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTWSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizeAudioTrackInstance.h"

UQuartzClockHandle* UQuantizedAudioTWSubsystem::PlayQuantizedAudio(FName TrackName, class UQuantizedAudioTrackPDAsset* TrackAsset)
{
	UQuantizeAudioTrackInstance*  Instance = NewObject<UQuantizeAudioTrackInstance>(this);
	Instance->Init(TrackName, TrackAsset);
	AudioTrackInstanceMap.Add(TrackName, Instance);
	return Instance->QuartzClockHandle;
}

void UQuantizedAudioTWSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (UGameplayStatics::IsGamePaused(this) && !UQuartzSubsystem::Get(GetWorld())->IsTickableWhenPaused())
	{
		UQuartzSubsystem::Get(GetWorld())->Tick(DeltaTime);
	}
}

TStatId UQuantizedAudioTWSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UQuantizedAudioTWSubsystem, STATGROUP_Tickables);
}

