#pragma once

using EventChecker = RE::BSEventNotifyControl;
class AnimationEventTracker : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
    static AnimationEventTracker *GetSingleton()
    {
        static AnimationEventTracker singleton;
        return &singleton;
    }

    bool Register();

    virtual EventChecker ProcessEvent(const RE::BSAnimationGraphEvent *a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent> *a_eventSource) override
    {
        return EventChecker::kContinue;
    };
    void SendAnimationEvent(RE::Actor *a_this, const RE::BSFixedString a_tag, const RE::BSFixedString a_payload = "");

    RE::BSTEventSource<RE::BSAnimationGraphEvent> *eventSource;
};