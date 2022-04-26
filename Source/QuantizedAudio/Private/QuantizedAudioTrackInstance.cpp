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

	QuartzSubsystem = UQuartzSubsystem::Get(GetWorld());

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Failed to get QuartzSubystem!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}

	QuartzClockHandle = QuartzSubsystem->CreateNewClock(GetWorld(), InTrackName, AudioCue.QuartzClockSettings);

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Failed to create Quartz Clock!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}
	QA_LOG(Warning, TEXT("%s: created successfully!"), *(UKismetSystemLibrary::GetDisplayName(this)));

	QuartzClockHandle->SetBeatsPerMinute(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle, AudioCue.BeatsPerMinute);

	if(bAutoStart)
		return ResumeAudioTrack();

	return true;
}

void UQuantizedAudioTrackInstance::QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name)
{
	if (!QuartzSubsystem || !QuartzClockHandle)
		return;

	QA_LOG(Warning, TEXT("%s : %s"), *(UEnum::GetValueAsString(EventType)), *Name.ToString());
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
		//UE_LOG(LogTemp, Warning, TEXT("Resume Clock"));
		QuartzClockHandle->ResumeClock(GetWorld(), QuartzClockHandle);
		break;
	case EQuartzCommandDelegateSubType::CommandOnCanceled:
		CheckPendingDestroy();
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
		
	QA_LOG(Warning, TEXT("Bar : %s | Beat : %s"), *(FString::FromInt(NumBars)), *(FString::FromInt(Beat)));
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

		QA_LOG(Warning, TEXT("%s: Queuing Next."), *(UKismetSystemLibrary::GetDisplayName(this)));
		StartAudioTrackAtIndex(CurrentIndex);
	}
}

bool UQuantizedAudioTrackInstance::CreateAudioTrack(USoundBase* InSound)
{
	if (!QuartzClockHandle->IsValidLowLevelFast())
	{
		UE_LOG(LogQuantizedAudio, Warning, TEXT("%s: Quartz Clock not initialized!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return false;
	}

	if (!AudioCue.TrackList[CurrentIndex].Track->IsValidLowLevelFast())
		return false;

	if (UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), InSound, 1.f, 1.f, 0.f/*, nullptr, false, true*/))
	{
		bool bInitialLocal = bInitial;
		AudioComponent->SetUISound(true);
		AudioComponent->OnAudioFinished.AddDynamic(this, &UQuantizedAudioTrackInstance::CheckPendingDestroy);
		EQuartzCommandQuantization QuartzCommandQuantization = /*AudioTrackComponents.Num() == 0 ? EQuartzCommandQuantization::None : */EQuartzCommandQuantization::Bar;
		FQuartzQuantizationBoundary QuartzQuantizationBoundary(QuartzCommandQuantization, 1.0f, EQuarztQuantizationReference::TransportRelative, true);
		OnQuartzCommandEvent.BindDynamic(this, &UQuantizedAudioTrackInstance::QuartzCommand);

		float FadeInDuration = bInitialLocal ? AudioCue.FadeSetting.FadeInDuration : 0.f;
		AudioComponent->PlayQuantized(GetWorld(), QuartzClockHandle, QuartzQuantizationBoundary, OnQuartzCommandEvent, 0.f, FadeInDuration, 1.f);
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

void UQuantizedAudioTrackInstance::StopAudioTrack()
{
	StopAudioTrackInternal(AudioCue.FadeSetting.FadeOutDuration);
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
}

bool UQuantizedAudioTrackInstance::ResumeAudioTrack()
{
	if (QuartzClockHandle)
	{
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
		QuartzClockHandle->StopClock(GetWorld(), false, QuartzClockHandle);
		QuartzClockHandle->ConditionalBeginDestroy();
		QuartzClockHandle = nullptr;
	}
}
