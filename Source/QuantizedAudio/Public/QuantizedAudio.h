// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "QuantizedAudioSettings.h"

#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

DECLARE_LOG_CATEGORY_EXTERN(LogQuantizedAudio, Log, All);

#define QA_FUNC (FString(__FUNCTION__))              // Current Class Name + Function Name where this is called
#define QA_LINE (FString::FromInt(__LINE__))         // Current Line Number in the code where this is called
#define QA_FUNC_LINE (QA_FUNC + "(" + QA_LINE + ")") // Current Class and Line Number where this is called!

#define QA_LOG(Verbosity, Format, ...) \
	if(UQuantizedAudioSettings* QuantizedAudioSettings = FQuantizedAudioModule::Get().GetSettings()) { \
		if(QuantizedAudioSettings->bShowDebugLog) { \
			UE_LOG(LogQuantizedAudio, Verbosity, Format, __VA_ARGS__) \
		} \
	} \

#define QA_LOG(Verbosity, Format, ...) \
	if(UQuantizedAudioSettings* QuantizedAudioSettings = FQuantizedAudioModule::Get().GetSettings()) { \
		if(QuantizedAudioSettings->bShowDebugLog) { \
			UE_LOG(LogQuantizedAudio, Verbosity, Format, __VA_ARGS__) \
		} \
	} \

class FQuantizedAudioModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline FQuantizedAudioModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FQuantizedAudioModule>("QuantizedAudio");
	}

	/** Getter for internal settings object to support runtime configuration changes */
	UQuantizedAudioSettings* GetSettings() const;

protected:
	/** Module settings */
	UQuantizedAudioSettings* ModuleSettings;
};
