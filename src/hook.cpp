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
    static inline std::atomic<int> g_suppressForceEquipClips{0};

    // The specific hkbClipGenerator instance being suppressed on the SkipInstant path
    static inline std::atomic<RE::hkbClipGenerator *> g_suppressedClip{nullptr};

    static bool IsEquipClip(std::string_view nm)
    {
        return Util::String::iContains(nm, "Equip");
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
                            RE::TESForm *a_lHandObject,
                            RE::TESForm *a_rHandObject)
{
    auto *tracker = AnimationEventTracker::GetSingleton();
    if (!tracker)
        return;

    tracker->SendAnimationEvent(a_this, "weaponDraw");

    if ((a_lHandObject && (a_lHandObject->IsWeapon() || a_lHandObject->IsArmor())) || (a_rHandObject && (a_rHandObject->IsWeapon() || a_rHandObject->IsArmor())))
    {
        tracker->SendAnimationEvent(a_this, "WeapEquip_Out");
        tracker->SendAnimationEvent(a_this, "WeapEquip_OutMoving");
    }
}

// ---------------------------------------------------------------------------
// Mode detection
// ---------------------------------------------------------------------------

EquipHook::EquipMode EquipHook::GetEquipMode(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    if (!a_playAnim)
        return EquipMode::Skip;

    if (!a_this || !a_this->AsActorState() || !a_this->AsActorState()->IsWeaponDrawn())
        return EquipMode::Normal;

    auto *rHandObj = a_this->GetEquippedObject(false);
    auto *lHandObj = a_this->GetEquippedObject(true);

    if ((lHandObj && rHandObj) && !CheckIsValidBoundObject(lHandObj) && !CheckIsValidBoundObject(rHandObj))
        return EquipMode::Normal;

    // Set this variable from your own mod/Papyrus whenever you want the
    // equip clip to be driven through the state machine instantly with
    // zero visual movement (inputs unlocked immediately).
    bool instantAnim = false;
    a_this->GetGraphVariableBool("InstantEquipAnim", instantAnim);
    if (instantAnim)
        return EquipMode::InstantAnim;

    // Uses playAnim=false so the graph never plays the clip at all.
    bool skipAnim = false;
    a_this->GetGraphVariableBool("SkipEquipAnimation", skipAnim);
    if (skipAnim)
        return EquipMode::Skip;

    return EquipMode::Normal;
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
        // Original mod path: suppress the animation entirely by passing false.
        // Then send the weapon-ready events after a short delay so the 3-D model
        auto *tracker = AnimationEventTracker::GetSingleton();
        tracker->Register();

        auto *rHandObj = a_this->GetEquippedObject(false);
        auto *lHandObj = a_this->GetEquippedObject(true);

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
        // New path: let the graph run (playAnim=true) so state transitions happen
        // normally and attack inputs are unlocked.  Activate_Hook will fast-forward
        // the clip and Generate_Hook will suppress bone movement for that instance.
        g_suppressForceEquipClips.store(1, std::memory_order_relaxed);
        _OnEquipItemPC(a_this, a_playAnim);
        break;

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
    if (g_suppressForceEquipClips.load(std::memory_order_relaxed) <= 0)
        return;

    std::string_view nm{a_this->animationName.c_str()};
    if (!IsEquipClip(nm))
        return;

    // Consume the flag immediately so only this specific clip instance is marked
    g_suppressForceEquipClips.store(0, std::memory_order_relaxed);
    g_suppressedClip.store(a_this, std::memory_order_relaxed);

    // Fast-forward the clip so the state machine transitions to "equip done" right
    // now, unlocking attack inputs.
    float duration = 2.0f;
    if (a_this->binding && a_this->binding->animation)
        duration = a_this->binding->animation->duration;

    a_this->mode = RE::hkbClipGenerator::PlaybackMode::kModeSinglePlay;
    _Update(a_this, a_context, duration + 0.01f);
    a_this->atEnd = true;
}

void EquipHook::Update_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context, float a_timestep)
{
    if (!a_this)
    {
        _Update(a_this, a_context, a_timestep);
        return;
    }

    // Suppress continued updates for the fast-forwarded clip.
    // The graph will call Deactivate naturally when it transitions away.
    if (a_this == g_suppressedClip.load(std::memory_order_relaxed) && a_this->atEnd)
        return;

    _Update(a_this, a_context, a_timestep);
}

void EquipHook::Deactivate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context)
{
    // Release the suppressed-instance pointer when the graph is done with the clip.
    if (a_this == g_suppressedClip.load(std::memory_order_relaxed))
        g_suppressedClip.store(nullptr, std::memory_order_relaxed);

    _Deactivate(a_this, a_context);
}

void EquipHook::Generate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context)
{
    // Produce no bone transforms for the suppressed clip
    if (a_this == g_suppressedClip.load(std::memory_order_relaxed))
        return;

    _Generate(a_this, a_context);
}