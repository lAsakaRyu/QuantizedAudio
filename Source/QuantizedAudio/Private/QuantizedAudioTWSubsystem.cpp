// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTWSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackInstance.h"
#include "QuantizedAudioTrackPDAsset.h" 
#include "QuantizedAudio.h"
#include "QuantizedAudioSettings.h"

#define QA_BGM "BGMusic"
#define QA_EVENT "EventMusic"

UQuantizedAudioTrackInstance* UQuantizedAudioTWSubsystem::PlayQuantizedAudioFromAsset(FName TrackName, UQuantizedAudioTrackPDAsset* TrackAsset, bool bAutoStart, bool bCustomFadeDuration)
{
	if (!TrackAsset)
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("[%s] Invalid track asset"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return nullptr;
	}
	
	return PlayQuantizedAudio(TrackName, TrackAsset->QuantizedAudioCue, bAutoStart, bCustomFadeDuration);
}

UQuantizedAudioTrackInstance* UQuantizedAudioTWSubsystem::PlayQuantizedAudio(FName TrackName, FQuantizedAudioCue AudioCue, bool bAutoStart, bool bCustomFadeDuration)
{
	FName CompiledTrackName = CompileTrackName(TrackName);
	UQuantizedAudioTrackInstance* Instance = AudioTrackInstanceMap.FindRef(CompiledTrackName);
	if (!Instance)
	{
		Instance = NewObject<UQuantizedAudioTrackInstance>(this);
		AudioTrackInstanceMap.Add(CompiledTrackName, Instance);
		if (!bCustomFadeDuration)
		{
			if (UQuantizedAudioSettings* QuantizedAudioSettings = FQuantizedAudioModule::Get().GetSettings())
			{
				if (FQuantizedAudioFadeSettings* FadeSetting = QuantizedAudioSettings->SpecificFadeSettings.Find(TrackName))
				{
					AudioCue.FadeSetting = *FadeSetting;
				}
				else
				{
					AudioCue.FadeSetting = QuantizedAudioSettings->GlobalFadeSettings;
				}
			}
		}
	}
	Instance->Init(CompiledTrackName, AudioCue, bAutoStart);
	
	return Instance;
}

void UQuantizedAudioTWSubsystem::StopQuantizedAudio(FName TrackName)
{
	UQuantizedAudioTrackInstance* Instance = AudioTrackInstanceMap.FindRef(TrackName);
	if (Instance)
	{
		Instance->StopAudioTrack();
	}
}

void UQuantizedAudioTWSubsystem::ResumeQuantizedAudio(FName TrackName)
{
	UQuantizedAudioTrackInstance* Instance = AudioTrackInstanceMap.FindRef(TrackName);
	if (Instance)
	{
		Instance->ResumeAudioTrack();
	}
}

void UQuantizedAudioTWSubsystem::SwapBGM(UQuantizedAudioTrackPDAsset* BGMTrackAsset, bool bForceSwap)
{
	if (!BGMTrackAsset)
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("[%s] Invalid track asset"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}
		
	if (bForceSwap || !CurrentBGMTrackAsset || (BGMTrackAsset->QuantizedAudioCue != CurrentBGMTrackAsset->QuantizedAudioCue))
	{
		for (auto& TrackName : FilterAudioTrackWithName(QA_BGM))
		{
			StopQuantizedAudio(TrackName);
		}
		if (UQuantizedAudioTrackInstance* NewInstance = PlayQuantizedAudioFromAsset(QA_BGM, BGMTrackAsset, false))
		{
			CurrentBGMTrackAsset = BGMTrackAsset;
			CurrentBGMTrackName = NewInstance->TrackName;
			if (NewInstance && FilterAudioTrackWithName(QA_EVENT, true).Num() == 0)
			{
				NewInstance->ResumeAudioTrack();
			}
		}
	}
}

void UQuantizedAudioTWSubsystem::PauseBGM()
{
	StopQuantizedAudio(CurrentBGMTrackName);
}

void UQuantizedAudioTWSubsystem::ResumeBGM()
{
	UQuantizedAudioTrackInstance* Instance = AudioTrackInstanceMap.FindRef(CurrentBGMTrackName);
	if (Instance && Instance->AudioTrackComponents.Num() == 0)
	{
		Instance->ResumeAudioTrack();
	}
}

void UQuantizedAudioTWSubsystem::RestartBGM()
{
	ResumeQuantizedAudio(CurrentBGMTrackName);
}

void UQuantizedAudioTWSubsystem::StartEventBGM(UQuantizedAudioTrackPDAsset* BGMTrackAsset)
{
	PauseBGM();

	for (auto& TrackName : FilterAudioTrackWithName(QA_EVENT))
	{
		StopQuantizedAudio(TrackName);
	}

	PlayQuantizedAudioFromAsset(QA_EVENT, BGMTrackAsset);
}

void UQuantizedAudioTWSubsystem::EndEventBGM(bool bResumeBGM)
{
	for (auto& TrackName : FilterAudioTrackWithName(QA_EVENT))
	{
		StopQuantizedAudio(TrackName);
	}

	if (bResumeBGM)
		ResumeBGM();
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

FName UQuantizedAudioTWSubsystem::CompileTrackName(FName InTrackName)
{
	for (auto& TrackName : FilterAudioTrackWithName(InTrackName))
	{
		auto* TrackInstance = AudioTrackInstanceMap.FindRef(TrackName);
		if (TrackInstance && !TrackInstance->QuartzClockHandle)
			return TrackInstance->TrackName;
	}
	int32 NewIndex = TrackIndexes.FindOrAdd(InTrackName, -1) + 1;
	TrackIndexes.Add(InTrackName, NewIndex);
	FString NewTrackName = InTrackName.ToString();
	NewTrackName.Append("_").Append(FString::FromInt(NewIndex));
	return *NewTrackName;
}

TArray<FName> UQuantizedAudioTWSubsystem::FilterAudioTrackWithName(FName InTrackName, bool bShouldBeActive)
{
	TArray<FName> OutTrackNames;
	for (auto& TrackName : AudioTrackInstanceMap)
	{
		if (TrackName.Key.ToString().Contains(InTrackName.ToString()))
		{
			if (!bShouldBeActive || (TrackName.Value && TrackName.Value->AudioTrackComponents.Num() > 0))
			{
				OutTrackNames.AddUnique(TrackName.Key);
			}
		}
	}
	return OutTrackNames;
}

