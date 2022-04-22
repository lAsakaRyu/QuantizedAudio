// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTWSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackInstance.h"
#include "QuantizedAudioTrackPDAsset.h" 

UQuartzClockHandle* UQuantizedAudioTWSubsystem::PlayQuantizedAudioFromAsset(FName TrackName, UQuantizedAudioTrackPDAsset* TrackAsset)
{
	if (!TrackAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid track asset"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return nullptr;
	}
	
	return PlayQuantizedAudio(TrackName, TrackAsset->QuantizedAudioCue);
}

UQuartzClockHandle* UQuantizedAudioTWSubsystem::PlayQuantizedAudio(FName TrackName, FQuantizedAudioCue AudioCue)
{
	UQuantizedAudioTrackInstance* Instance = NewObject<UQuantizedAudioTrackInstance>(this);
	Instance->Init(TrackName, AudioCue);
	AudioTrackInstanceMap.Add(TrackName, Instance);
	return Instance->QuartzClockHandle;
}

void UQuantizedAudioTWSubsystem::StopQuantizedAudio(FName TrackName)
{
	if (UQuantizedAudioTrackInstance* Instance = *AudioTrackInstanceMap.Find(TrackName))
	{
		Instance->StopAudioTrack();
	}
}

void UQuantizedAudioTWSubsystem::ResumeQuantizedAudio(FName TrackName)
{
	if (UQuantizedAudioTrackInstance* Instance = *AudioTrackInstanceMap.Find(TrackName))
	{
		Instance->ResumeAudioTrack();
	}
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

