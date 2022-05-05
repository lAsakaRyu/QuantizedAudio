// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuantizedAudio.h"
#include "QuantizedAudioSettings.h"
#include "Developer/Settings/Public/ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FQuantizedAudioModule"

DEFINE_LOG_CATEGORY(LogQuantizedAudio);

void FQuantizedAudioModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	ModuleSettings = NewObject<UQuantizedAudioSettings>(GetTransientPackage(), "QuantizedAudioSettings", RF_Standalone);
	ModuleSettings->AddToRoot();

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Quantized Audio",
			LOCTEXT("RuntimeSettingsName", "Quantized Audio"),
			LOCTEXT("RuntimeSettingsDescription", "Configure Quantized Audio plugin settings"),
			ModuleSettings);
	}

	UE_LOG(LogQuantizedAudio, Log, TEXT("%s: Quantized Audio module started"), *QA_FUNC_LINE);
}

void FQuantizedAudioModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Quantized Audio");
	}

	if (!GExitPurge)
	{
		ModuleSettings->RemoveFromRoot();
	}
	else
	{
		ModuleSettings = nullptr;
	}
}

UQuantizedAudioSettings* FQuantizedAudioModule::GetSettings() const
{
	check(ModuleSettings);
	return ModuleSettings;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FQuantizedAudioModule, QuantizedAudio)