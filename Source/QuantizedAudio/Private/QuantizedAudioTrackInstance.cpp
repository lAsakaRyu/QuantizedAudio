// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTrackInstance.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackPDAsset.h"

void UQuantizedAudioTrackInstance::Init(FName InTrackName, FQuantizedAudioCue InAudioCue, bool bAutoStart)
{
	if (QuartzClockHandle)
	{
		StopAudioTrack(); // TODO:Fade duration in Cue
	}

	TrackName = InTrackName;
	AudioCue = InAudioCue;

	QuartzSubsystem = UQuartzSubsystem::Get(GetWorld());

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to get QuartzSubystem!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	QuartzClockHandle = QuartzSubsystem->CreateNewClock(GetWorld(), InTrackName, AudioCue.QuartzClockSettings);

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to create Quartz Clock!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[%s] created successfully!"), *(UKismetSystemLibrary::GetDisplayName(this)));

	QuartzClockHandle->SetBeatsPerMinute(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle, AudioCue.BeatsPerMinute);

	if(bAutoStart)
		ResumeAudioTrack();
}

void UQuantizedAudioTrackInstance::QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name)
{
	if (!QuartzSubsystem || !QuartzClockHandle)
		return;

	UE_LOG(LogTemp, Warning, TEXT("%s : %s"), *(UEnum::GetValueAsString(EventType)), *Name.ToString());
	switch (EventType)
	{
	case EQuartzCommandDelegateSubType::CommandOnAboutToStart:
		/*
		*  bInitial is used to prevent the first beat to call Reset Transport to cause slight delay to the first beat.
		*/
		if (!bInitial)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] Reset Transport."), *(UKismetSystemLibrary::GetDisplayName(this)));
			QuartzClockHandle->ResetTransportQuantized(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle);
			bInitial = false;
		}
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
		UE_LOG(LogTemp, Warning, TEXT("[%s] Null!"), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)));
		UE_LOG(LogTemp, Warning, TEXT("[%s] Null!"), *(UKismetSystemLibrary::GetDisplayName(QuartzClockHandle)));
		return;
	}
		
	UE_LOG(LogTemp, Warning, TEXT("Bar : %s | Beat : %s"), *(FString::FromInt(NumBars)), *(FString::FromInt(Beat)));
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

		UE_LOG(LogTemp, Warning, TEXT("[%s] Queuing Next."), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)));
		StartAudioTrackAtIndex(CurrentIndex);
	}
}

bool UQuantizedAudioTrackInstance::CreateAudioTrack(USoundBase* InSound)
{
	if (!QuartzClockHandle->IsValidLowLevelFast())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Quartz Clock not initialized!"), *(UKismetSystemLibrary::GetDisplayName(this)));
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

		float FadeInDuration = bInitialLocal ? 0.5f : 0.f; // TODO:Fade duration in Cue
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
		UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid starting index of %d! TrackList size = %d"), *(UKismetSystemLibrary::GetDisplayName(this)),Index, AudioCue.TrackList.Num());
		return false;
	}
	return CreateAudioTrack(AudioCue.TrackList[Index].Track);
}

void UQuantizedAudioTrackInstance::StopAudioTrack()
{
	StopAudioTrackInternal(1.f); // TODO:Fade duration in Cue
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

void UQuantizedAudioTrackInstance::ResumeAudioTrack()
{
	if (QuartzClockHandle)
	{
		CurrentIndex = 0;
		bInitial = true;
		StartAudioTrackAtIndex(CurrentIndex);
	}
}

void UQuantizedAudioTrackInstance::CheckPendingDestroy()
{
	AudioTrackComponents.Remove(nullptr);
	TArray<UAudioComponent*> AudioTracksCopy = AudioTrackComponents;
	for (auto* AudioComponent : AudioTracksCopy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Checking Destroy : %s | Is Playing : %s"), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)), *(UKismetSystemLibrary::GetDisplayName(AudioComponent)), *FString(AudioComponent->IsPlaying()?"true":"false"));

		if (!AudioComponent->IsPlaying())
		{
			AudioTrackComponents.Remove(AudioComponent);
			if (!AudioComponent->IsBeingDestroyed())
				AudioComponent->DestroyComponent();
		}	
	}

	if (QuartzClockHandle && QuartzClockHandle->IsClockRunning(GetWorld()) && AudioTrackComponents.Num() == 0)
	{
		QuartzClockHandle->StopClock(GetWorld(), false, QuartzClockHandle);
	}
}
