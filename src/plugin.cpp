#include "log.h"
#include "hook.h"
#include "event.h"

void MessageHandler(SKSE::MessagingInterface::Message *a_msg)
{
    switch (a_msg->type)
    {
    case SKSE::MessagingInterface::kDataLoaded:
        EquipHook::Hook();
        break;
    case SKSE::MessagingInterface::kPostLoad:
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
    case SKSE::MessagingInterface::kPostLoadGame:
    case SKSE::MessagingInterface::kNewGame:
        EquipHook::ResetState();
        if (auto animationEventTracker = AnimationEventTracker::GetSingleton(); animationEventTracker)
            animationEventTracker->Register();
        break;
    }
}

SKSEPluginInfo(SKSE::PluginDeclaration::PluginDeclarationInfo{
		.Version = { 1, 0, 9, 0 },
		.Name = "SkipEquipAnimation",
		.Author = "AnArchos",
		.SupportEmail = "patreon.com/AnArchos",
		.StructCompatibility = ::SKSE::StructCompatibility::Independent,
		.RuntimeCompatibility = ::SKSE::VersionIndependence::AddressLibrary,
		.MinimumSKSEVersion = { 2, 0, 0, 2 }
	}
);
SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
    REL::Module::reset();

    SetupLog();

    auto *plugin = SKSE::PluginDeclaration::GetSingleton();
    spdlog::info("{} v{} is loading...", plugin->GetName(), plugin->GetVersion());

    SKSE::Init(a_skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", MessageHandler))
    {
        return false;
    }

    spdlog::info("{} by {} has finished loading. Support for more mods! {}", plugin->GetName(), plugin->GetAuthor(), plugin->GetSupportEmail());

    return true;
}
