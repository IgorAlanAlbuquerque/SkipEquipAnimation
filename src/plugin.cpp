#include "log.h"
#include "hook.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
    if(a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
        EquipHook::Hook();
    }
}
SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();

    auto* plugin  = SKSE::PluginDeclaration::GetSingleton();
    spdlog::info("{} v{} is loading...", plugin->GetName(), plugin->GetVersion());

    SKSE::Init(skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", MessageHandler)) {
        return false;
    }

    spdlog::info("{} by {} has finished loading. Support for more mods! {}", plugin->GetName(), plugin->GetAuthor(), plugin->GetSupportEmail());

    return true;
}
