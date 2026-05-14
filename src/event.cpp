#include "event.h"
/**/
bool AnimationEventTracker::Register()
{
    bool foundEventSource = eventSource != nullptr;
    if (!foundEventSource)
    {
        const auto pc = RE::PlayerCharacter::GetSingleton();

        RE::BSAnimationGraphManagerPtr graphManager;
        if (pc)
            pc->GetAnimationGraphManager(graphManager);
        else
            spdlog::debug("Can't found Player Character");

        if (graphManager)
        {
            for (auto &animationGraph : graphManager->graphs)
            {
                eventSource = animationGraph->GetEventSource<RE::BSAnimationGraphEvent>();
                if (eventSource)
                {
                    spdlog::info("Registered {}", typeid(RE::BSAnimationGraphEvent).name());
                    return true;
                }
            }
        }

        if (!eventSource)
        {
            spdlog::info("Failed to register {}", typeid(RE::BSAnimationGraphEvent).name());
        }else{
            spdlog::info("Event Source registered {}", typeid(RE::BSAnimationGraphEvent).name());
        }
    }
    return foundEventSource;
}

void AnimationEventTracker::SendAnimationEvent(RE::Actor *a_this, const RE::BSFixedString a_tag, const RE::BSFixedString a_payload)
{
    if (a_this && eventSource)
    {
        RE::BSAnimationGraphEvent event = {a_tag, a_this, a_payload};
        a_this->ProcessEvent(&event, eventSource);
    }
}