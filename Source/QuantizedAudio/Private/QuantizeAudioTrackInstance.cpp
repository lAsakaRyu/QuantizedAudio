// Fill out your copyright notice in the Description page of Project Settings.


#include "QuantizeAudioTrackInstance.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "QuantizedAudioTrackPDAsset.h"

void UQuantizeAudioTrackInstance::Init(FName InTrackName, UQuantizedAudioTrackPDAsset* InTrackAsset)
{
	if (!InTrackAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid Track Asset"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	TrackName = InTrackName;
	TrackAsset = InTrackAsset;
	CurrentIndex = 0;

	QuartzSubsystem = UQuartzSubsystem::Get(GetWorld());

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to get QuartzSubystem!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	QuartzClockHandle = QuartzSubsystem->CreateNewClock(GetWorld(), InTrackName, TrackAsset->QuartzClockSettings);

	if (!QuartzSubsystem->IsValidLowLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to create Quartz Clock!"), *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[%s] created successfully!"), *(UKismetSystemLibrary::GetDisplayName(this)));

	QuartzClockHandle->SetBeatsPerMinute(GetWorld(), FQuartzQuantizationBoundary(), FOnQuartzCommandEventBP(), QuartzClockHandle, 110);
	CreateAudioTrack();
}

void UQuantizeAudioTrackInstance::QuartzCommand(EQuartzCommandDelegateSubType EventType, FName Name)
{
	if (!QuartzSubsystem || !QuartzClockHandle)
		return;

	UE_LOG(LogTemp, Warning, TEXT("%s : %s"), *(UEnum::GetValueAsString(EventType)), *Name.ToString());
	switch (EventType)
	{
	case EQuartzCommandDelegateSubType::CommandOnQueued:
		UE_LOG(LogTemp, Warning, TEXT("Resume Clock"));
		QuartzClockHandle->ResumeClock(GetWorld(), QuartzClockHandle);
		break;
	}
}

void UQuantizeAudioTrackInstance::QuartzMetronome(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	if (!QuartzSubsystem || !QuartzClockHandle || !TrackAsset->Tracks.IsValidIndex(CurrentIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Null!"), *(UKismetSystemLibrary::GetDisplayName(QuartzSubsystem)));
		UE_LOG(LogTemp, Warning, TEXT("[%s] Null!"), *(UKismetSystemLibrary::GetDisplayName(QuartzClockHandle)));
		return;
	}
		

	UE_LOG(LogTemp, Warning, TEXT("Bar : %s | Beat : %s"), *(FString::FromInt(NumBars)), *(FString::FromInt(Beat)));
	int32 WaitBars = FMath::RoundToInt(TrackAsset->Tracks[CurrentIndex].Track->Duration / 60.f * TrackAsset->BeatsPerMinute / TrackAsset->QuartzClockSettings.TimeSignature.NumBeats);


	UE_LOG(LogTemp, Warning, TEXT("[%s] Waiting for %s"), *(FString::FromInt(WaitBars)));
	if (NumBars % WaitBars == 0 && Beat == 1)
	{
		if (!TrackAsset->Tracks[CurrentIndex].bIsLooping && !(TrackAsset->Tracks.Num() == CurrentIndex))
			CurrentIndex++;
		CreateAudioTrack();
		FQuartzQuantizationBoundary QuartzQuantizationBoundary(EQuartzCommandQuantization::Bar, 1.0f, EQuarztQuantizationReference::TransportRelative, true);
		QuartzClockHandle->ResetTransportQuantized(GetWorld(), QuartzQuantizationBoundary, FOnQuartzCommandEventBP(), QuartzClockHandle);
	}
}

void UQuantizeAudioTrackInstance::CreateAudioTrack()
{
	if (!TrackAsset/* || !TrackAsset->Tracks.IsValidIndex(CurrentIndex) || !TrackAsset->Tracks[CurrentIndex].Track->IsValidLowLevel()*/)
		return;

	UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), TrackAsset->Tracks[CurrentIndex].Track);
	FQuartzQuantizationBoundary QuartzQuantizationBoundary(EQuartzCommandQuantization::Bar, 1.0f, EQuarztQuantizationReference::TransportRelative, true);
	OnQuartzCommandEvent.BindDynamic(this, &UQuantizeAudioTrackInstance::QuartzCommand);
	AudioComponent->PlayQuantized(GetWorld(), QuartzClockHandle, QuartzQuantizationBoundary, OnQuartzCommandEvent);
	OnQuartzMetronomeEvent.BindDynamic(this, &UQuantizeAudioTrackInstance::QuartzMetronome);
	QuartzClockHandle->SubscribeToQuantizationEvent(GetWorld(), EQuartzCommandQuantization::Beat, OnQuartzMetronomeEvent, QuartzClockHandle);
}
