// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTrackInstance.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackPDAsset.h"

bool UQuantizedAudioTrackInstance::Init(FName InTrackName, FQuantizedAudioCue InAudioCue, bool bAutoStart)
{
	if (QuartzClockHandle)
	{
		QA_LOG(Warning, TEXT("%s: Track Instance is running!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}

	TrackName = InTrackName;
	AudioCue = InAudioCue;
	bIsPlaying = false;

	QuartzSubsystem = UQuartzSubsystem::Get(GetWorld());

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Failed to get QuartzSubystem!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}

	if(bAutoStart)
		return ResumeAudioTrack();

	return true;
}

bool UQuantizedAudioTrackInstance::CheckClockHandle()
{
	if (!QuartzClockHandle)
	{
		QuartzClockHandle = QuartzSubsystem->CreateNewClock(GetWorld(), TrackName, AudioCue.QuartzClockSettings);

		if (!QuartzClockHandle->IsValidLowLevel())
		{
			UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Failed to create Quartz Clock!"), *(UKismetSystemLibrary::GetDisplayName(this)));
			return false;
		}
		QA_LOG(Warning, TEXT("%s: Clock created successfully!"), *(UKismetSystemLibrary::GetDisplayName(this)));
	}
	return QuartzClockHandle->IsValidLowLevelFast();
}

void UQuantizedAudioTrackInstance::QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name)
{
	if (!QuartzSubsystem || !QuartzClockHandle)
		return;

	QA_LOG(Warning, TEXT("%s : %s"), *(UKismetSystemLibrary::GetDisplayName(this)), *(UEnum::GetValueAsString(EventType)));
	switch (EventType)
	{
	case EQuartzCommandDelegateSubType::CommandOnAboutToStart:
		/*
		*  bInitial is used to prevent the first beat to call Reset Transport to cause slight delay to the first beat.
		*/
		if (!bInitial)
		{
			QA_LOG(Warning, TEXT("[%s] Reset Transport."), *(UKismetSystemLibrary::GetDisplayName(this)));
			QuartzClockHandle->ResetTransportQuantized(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle);
		}
		bInitial = false;
		break;
	case EQuartzCommandDelegateSubType::CommandOnQueued:
		if (bIsPlaying)
		{
			QA_LOG(Warning, TEXT("%s: Resume Clock."), *(UKismetSystemLibrary::GetDisplayName(this)));
			QuartzClockHandle->ResumeClock(this, QuartzClockHandle);
		}
		break;
	case EQuartzCommandDelegateSubType::CommandOnCanceled:
		//QA_LOG(Warning, TEXT("%s: Cancelled Command."), *(UKismetSystemLibrary::GetDisplayName(this)));
		//CheckPendingDestroy();
		break;
	}
}

void UQuantizedAudioTrackInstance::QuartzMetronome(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	if (!QuartzSubsystem || !QuartzClockHandle || !AudioCue.TrackList.IsValidIndex(CurrentIndex))
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Null!"), *QA_FUNC_LINE);
		return;
	}
		
	QA_LOG(Warning, TEXT("%s: Bar : %s | Beat : %s"), *(UKismetSystemLibrary::GetDisplayName(this)) , *(FString::FromInt(NumBars)), *(FString::FromInt(Beat)));
	int32 WaitBars = FMath::RoundToInt(AudioCue.TrackList[CurrentIndex].Track->Duration / 60.f * AudioCue.BeatsPerMinute / AudioCue.QuartzClockSettings.TimeSignature.NumBeats);

	//UE_LOG(LogTemp, Warning, TEXT("[%s] Waiting for %s"), *(FString::FromInt(WaitBars)));
	if (NumBars % WaitBars == 0 && Beat == 1)
	{
		if (!AudioCue.TrackList[CurrentIndex].bIsLooping && ((AudioCue.TrackList.Num() - 1) == CurrentIndex))
			return;

		if (!AudioCue.TrackList[CurrentIndex].bIsLooping && !((AudioCue.TrackList.Num() - 1) == CurrentIndex))
			CurrentIndex++;

		if (AudioTrackComponents.Num() > 0 && AudioCue.TrackList[CurrentIndex].Track->IsLooping())
			return;

		if (bIsPlaying)
		{
			QA_LOG(Warning, TEXT("%s: Queuing Next."), *(UKismetSystemLibrary::GetDisplayName(this)));
			StartAudioTrackAtIndex(CurrentIndex);
		}
		else
		{
			QA_LOG(Warning, TEXT("%s: Skipping Play. bIsPlaying = false"), *(UKismetSystemLibrary::GetDisplayName(this)));
		}
	}
}

bool UQuantizedAudioTrackInstance::CreateAudioTrack(USoundBase* InSound)
{
	if (!QuartzClockHandle)
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Quartz Clock not initialized!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}

	if (!InSound->IsValidLowLevelFast())
		return false;

	InSound->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;

	if (UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(GetOuter(), InSound, 1.f, 1.f, 0.f))
	{
		bIsPlaying = true;
		bool bInitialLocal = bInitial;
		AudioComponent->SetUISound(true);
		AudioComponent->OnAudioFinished.AddDynamic(this, &UQuantizedAudioTrackInstance::CheckPendingDestroy);
		EQuartzCommandQuantization QuartzCommandQuantization = /*AudioTrackComponents.Num() == 0 ? EQuartzCommandQuantization::None : */EQuartzCommandQuantization::Bar;
		FQuartzQuantizationBoundary QuartzQuantizationBoundary(QuartzCommandQuantization, 1.0f, EQuarztQuantizationReference::TransportRelative, true);
		OnQuartzCommandEvent.BindDynamic(this, &UQuantizedAudioTrackInstance::QuartzCommand);

		float FadeInDuration = bInitialLocal ? AudioCue.FadeSetting.FadeInDuration : 0.f;
		AudioComponent->PlayQuantized(this, QuartzClockHandle, QuartzQuantizationBoundary, OnQuartzCommandEvent, 0.f, FadeInDuration, 1.f);
		OnQuartzMetronomeEvent.BindDynamic(this, &UQuantizedAudioTrackInstance::QuartzMetronome);
		QuartzClockHandle->SubscribeToQuantizationEvent(GetWorld(), EQuartzCommandQuantization::Beat, OnQuartzMetronomeEvent, QuartzClockHandle);

		AudioTrackComponents.Add(AudioComponent);
	}
	return true;
}

bool UQuantizedAudioTrackInstance::StartAudioTrackAtIndex(int32 Index)
{
	if (!AudioCue.TrackList.IsValidIndex(Index))
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("[%s] Invalid starting index of %d! TrackList size = %d"), *(UKismetSystemLibrary::GetDisplayName(this)), Index, AudioCue.TrackList.Num());
		return false;
	}
	return CreateAudioTrack(AudioCue.TrackList[Index].Track);
}

void UQuantizedAudioTrackInstance::StopAudioTrack(bool bStopsImmediately)
{
	StopAudioTrackInternal(bStopsImmediately ? 0.f : AudioCue.FadeSetting.FadeOutDuration);
}

void UQuantizedAudioTrackInstance::StopAudioTrackInternal(float FadeOutDuration)
{
	TArray<UAudioComponent*> AudioTracksCopy = AudioTrackComponents;
	for (auto* AudioComponent : AudioTracksCopy)
	{
		if (AudioComponent->IsValidLowLevelFast()) {
	
			AudioComponent->FadeOut(FadeOutDuration, 0);
		}
		AudioTrackComponents.Remove(AudioComponent);
	}
	bIsPlaying = false;
}

bool UQuantizedAudioTrackInstance::ResumeAudioTrack()
{
	if (CheckClockHandle())
	{
		QuartzClockHandle->SetBeatsPerMinute(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle, AudioCue.BeatsPerMinute);
		CurrentIndex = 0;
		bInitial = true;
		return StartAudioTrackAtIndex(CurrentIndex);
	}
	return false;
}

void UQuantizedAudioTrackInstance::CheckPendingDestroy()
{
	AudioTrackComponents.Remove(nullptr);
	TArray<UAudioComponent*> AudioTracksCopy = AudioTrackComponents;
	for (auto* AudioComponent : AudioTracksCopy)
	{
		QA_LOG(Warning, TEXT("%s: Checking Destroy : %s | Is Playing : %s"), *(UKismetSystemLibrary::GetDisplayName(this)), *(UKismetSystemLibrary::GetDisplayName(AudioComponent)), *FString(AudioComponent->IsPlaying() ? "true" : "false"));

		if (!AudioComponent->IsPlaying())
		{
			AudioTrackComponents.Remove(AudioComponent);
			if (!AudioComponent->IsBeingDestroyed())
				AudioComponent->DestroyComponent();
		}	
	}

	if (QuartzClockHandle && AudioTrackComponents.Num() == 0)
	{
		QA_LOG(Warning, TEXT("%s: Destroying Handle."), *(UKismetSystemLibrary::GetDisplayName(this)));
		QuartzClockHandle->StopClock(this, false, QuartzClockHandle);
		QuartzClockHandle->ConditionalBeginDestroy();
		QuartzClockHandle = nullptr;
	}
}
