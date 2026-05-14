#include "hook.h"
#include "event.h"
#include "util.h"

#include <atomic>
#include <chrono>
#include <string_view>
#include <thread>

using namespace std::literals;
using namespace Util;

namespace
{
    static inline std::vector<RE::hkbClipGenerator *> g_pendingClips;
    static inline std::vector<RE::hkbClipGenerator *> g_suppressedClips;

    bool IsEquipClip(std::string_view nm)
    {
        return Util::String::iContains(nm, "Equip");
    }

    RE::Actor *GetActorFromContext(const RE::hkbContext &a_context)
    {
        if (!a_context.character)
            return nullptr;

        const auto *graph = reinterpret_cast<const RE::BShkbAnimationGraph *>(
            reinterpret_cast<const std::byte *>(a_context.character) - 0xC0);

        return graph->holder;
    }
}

// ---------------------------------------------------------------------------
// Helpers shared by both paths
// ---------------------------------------------------------------------------

static bool CheckIsValidBoundObject(const RE::TESForm *a_object)
{
    if (!a_object)
        return false;
    if (a_object->IsMagicItem())
        return true;
    return a_object->As<RE::TESObjectWEAP>() != nullptr || a_object->As<RE::TESObjectARMO>() != nullptr;
}

// Sends the animation events that normally fire at the end of an equip animation.
static void SendEquipEvents(RE::Actor *a_this,
                            RE::TESForm const *a_lHandObject,
                            RE::TESForm const *a_rHandObject)
{
    auto *tracker = AnimationEventTracker::GetSingleton();
    if (!tracker)
        return;

    tracker->SendAnimationEvent(a_this, "weaponDraw");

    if ((a_lHandObject && (a_lHandObject->IsWeapon() || a_lHandObject->IsArmor())) ||
        (a_rHandObject && (a_rHandObject->IsWeapon() || a_rHandObject->IsArmor())))
    {
        tracker->SendAnimationEvent(a_this, "WeapEquip_Out");
        tracker->SendAnimationEvent(a_this, "WeapEquip_OutMoving");
    }
}

// ---------------------------------------------------------------------------
// Mode detection
// ---------------------------------------------------------------------------

EquipHook::EquipMode EquipHook::GetEquipMode(const RE::PlayerCharacter *a_this, bool a_playAnim)
{
    using enum EquipHook::EquipMode;
    if (a_this)
    {
        bool instantAnim = false;
        a_this->GetGraphVariableBool("InstantEquipAnim", instantAnim);
        if (instantAnim && a_playAnim)
            return InstantAnim;

        bool skipAnim = false;
        a_this->GetGraphVariableBool("SkipEquipAnimation", skipAnim);
        if (skipAnim && a_playAnim)
            return Skip;
    }
    return Normal;
}

// ---------------------------------------------------------------------------
// Vtable hook: PlayerCharacter::OnEquipItem
// ---------------------------------------------------------------------------

void EquipHook::OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    switch (GetEquipMode(a_this, a_playAnim))
    {
    case EquipMode::Skip:
    {
        auto *tracker = AnimationEventTracker::GetSingleton();
        tracker->Register();

        auto const *rHandObj = a_this->GetEquippedObject(false);
        auto const *lHandObj = a_this->GetEquippedObject(true);

        int delay = 300;
        bool skip3D = false;
        a_this->GetGraphVariableInt("LoadBoundObjectDelay", delay);
        a_this->GetGraphVariableBool("Skip3DLoading", skip3D);
        if (delay < static_cast<int>(*g_deltaTimeRealTime * 1000.f))
            delay = static_cast<int>(*g_deltaTimeRealTime * 1000.f);

        _OnEquipItemPC(a_this, false);

        if (!skip3D)
        {
            std::jthread([=]()
                         {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                SendEquipEvents(a_this, lHandObj, rHandObj); })
                .detach();
        }
        break;
    }

    case EquipMode::InstantAnim:
    {
        _OnEquipItemPC(a_this, a_playAnim);
        break;
    }

    default:
        _OnEquipItemPC(a_this, a_playAnim);
        break;
    }
}

// ---------------------------------------------------------------------------
// Vtable hooks: hkbClipGenerator
// ---------------------------------------------------------------------------
void EquipHook::Activate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context)
{
    _Activate(a_this, a_context);

    if (!a_this)
        return;

    auto const *player = RE::PlayerCharacter::GetSingleton();
    if (GetActorFromContext(a_context) != player)
        return;

    if (const std::string_view name{a_this->animationName.c_str()}; !IsEquipClip(name))
        return;

    bool instantAnim = false;
    player->GetGraphVariableBool("InstantEquipAnim", instantAnim);
    if (!instantAnim)
        return;

    g_pendingClips.push_back(a_this);
}

void EquipHook::Update_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context, float a_timestep)
{
    if (a_this)
    {
        for (auto const *clip : g_suppressedClips)
        {
            if (clip == a_this)
            {
                if (a_this->atEnd)
                    return;

                float duration = 2.0f;
                if (a_this->binding && a_this->binding->animation)
                    duration = a_this->binding->animation->duration;

                a_this->mode = RE::hkbClipGenerator::PlaybackMode::kModeSinglePlay;
                _Update(a_this, a_context, duration + 0.01f);
                a_this->atEnd = true;
                return;
            }
        }

        if (auto it = std::ranges::find(g_pendingClips, a_this); it != g_pendingClips.end())
        {
            g_pendingClips.erase(it);

            float duration = 2.0f;
            if (a_this->binding && a_this->binding->animation)
                duration = a_this->binding->animation->duration;

            a_this->mode = RE::hkbClipGenerator::PlaybackMode::kModeSinglePlay;
            _Update(a_this, a_context, duration + 0.01f);
            a_this->atEnd = true;

            g_suppressedClips.push_back(a_this);
            return;
        }
    }

    _Update(a_this, a_context, a_timestep);
}

void EquipHook::Deactivate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context)
{
    if (auto it = std::ranges::find(g_pendingClips, a_this); it != g_pendingClips.end())
        g_pendingClips.erase(it);

    if (auto it2 = std::ranges::find(g_suppressedClips, a_this); it2 != g_suppressedClips.end())
        g_suppressedClips.erase(it2);

    if (!a_this)
    {
        _Deactivate(a_this, a_context);
        return;
    }

    _Deactivate(a_this, a_context);
}

void EquipHook::ResetState()
{
    g_pendingClips.clear();
    g_suppressedClips.clear();
}