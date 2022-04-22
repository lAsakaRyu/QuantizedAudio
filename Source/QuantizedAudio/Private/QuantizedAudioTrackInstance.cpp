// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizedAudioTrackInstance.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackPDAsset.h"

void UQuantizedAudioTrackInstance::Init(FName InTrackName, FQuantizedAudioCue InAudioCue, bool bAutoStart)
{
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
		QuartzClockHandle->ResetTransportQuantized(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle);
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
		if (!AudioCue.TrackList[CurrentIndex].bIsLooping && !(AudioCue.TrackList.Num() == CurrentIndex))
			CurrentIndex++;

		UE_LOG(LogTemp, Warning, TEXT("[%s] Queuing Next."), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)));
		StartAudioTrackAtIndex(CurrentIndex);


	}
}

bool UQuantizedAudioTrackInstance::CreateAudioTrack(USoundBase* InSound)
{
	if (!AudioCue.TrackList[CurrentIndex].Track->IsValidLowLevel())
		return false;

	if (UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), InSound, 1.f, 1.f, 0.f, nullptr, false, true))
	{
		AudioComponent->SetUISound(true);
		AudioComponent->OnAudioFinished.AddDynamic(this, &UQuantizedAudioTrackInstance::CheckPendingDestroy);
		EQuartzCommandQuantization QuartzCommandQuantization = CurrentIndex == 0 ? EQuartzCommandQuantization::None : EQuartzCommandQuantization::Bar;
		FQuartzQuantizationBoundary QuartzQuantizationBoundary(QuartzCommandQuantization, 1.0f, EQuarztQuantizationReference::TransportRelative, true);
		OnQuartzCommandEvent.BindDynamic(this, &UQuantizedAudioTrackInstance::QuartzCommand);

		float FadeInDuration = CurrentIndex == 0 ? 0.5f : 0.f;
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
	TArray<UAudioComponent*> AudioTracksCopy = AudioTrackComponents;
	for (auto* AudioComponent : AudioTracksCopy)
	{
		if (AudioComponent->IsValidLowLevel()) {
			AudioComponent->FadeOut(1.f, 0);
		}
		AudioTrackComponents.Remove(AudioComponent);
	}
}

void UQuantizedAudioTrackInstance::ResumeAudioTrack()
{
	if (QuartzClockHandle)
	{
		CurrentIndex = 0;
		StartAudioTrackAtIndex(CurrentIndex);
	}
}

void UQuantizedAudioTrackInstance::CheckPendingDestroy()
{
	AudioTrackComponents.Remove(nullptr);

	for (auto* AudioComponent : AudioTrackComponents)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Checking Destroy : %s | Is Playing : %s"), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)), *(UKismetSystemLibrary::GetDisplayName(AudioComponent)), *FString(AudioComponent->IsPlaying()?"true":"false"));
	}

	if (QuartzClockHandle && QuartzClockHandle->IsClockRunning(GetWorld()) && AudioTrackComponents.Num() == 0)
	{
		QuartzClockHandle->StopClock(GetWorld(), false, QuartzClockHandle);
	}
}
