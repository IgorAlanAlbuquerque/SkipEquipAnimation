#include "hook.h"
#include "util.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <string_view>

using namespace std::literals;
using namespace Util;

namespace
{
    static inline std::atomic<std::int64_t> g_skipUntilMs{0};
    static inline std::atomic<int> g_suppressForceEquipClips{0};

    inline std::int64_t NowMs()
    {
        using clock = std::chrono::steady_clock;
        return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
    }

    inline bool InSkipWindow()
    {
        return NowMs() <= g_skipUntilMs.load(std::memory_order_relaxed);
    }

    static bool contains_icase(std::string_view s, std::string_view needle)
    {
        if (needle.empty() || s.size() < needle.size())
            return false;

        for (size_t i = 0; i + needle.size() <= s.size(); ++i)
        {
            bool ok = true;
            for (size_t j = 0; j < needle.size(); ++j)
            {
                unsigned char a = (unsigned char)s[i + j];
                unsigned char b = (unsigned char)needle[j];
                if (std::tolower(a) != std::tolower(b))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return true;
        }
        return false;
    }

    static bool IsEquipClip(std::string_view nm)
    {
        return contains_icase(nm, "Equip");
    }
}

void EquipHook::OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    SkipAnim(a_this, a_playAnim);
    _OnEquipItemPC(a_this, a_playAnim);
}

/*
void EquipHook::OnEquipItemNPC(RE::Actor* a_this, bool a_playAnim)
{
    _OnEquipItemNPC(a_this, !SkipAnim(a_this, a_playAnim));
}
*/

bool CheckIsValidBoundObject(const RE::TESForm *a_object)
{
    if (!a_object)
        return false;
    if (a_object->IsMagicItem())
        return true;
    return a_object->As<RE::TESObjectWEAP>() != nullptr || a_object->As<RE::TESObjectARMO>() != nullptr;
}

bool EquipHook::SkipAnim(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    bool skipAnim = !a_playAnim;
    if (a_this && a_this->AsActorState() && a_this->AsActorState()->IsWeaponDrawn())
    {
        auto rHandObj = a_this->GetEquippedObject(false);
        auto lHandObj = a_this->GetEquippedObject(true);

        if (!(lHandObj && rHandObj) || CheckIsValidBoundObject(lHandObj) || CheckIsValidBoundObject(rHandObj))
        {
            a_this->GetGraphVariableBool("SkipEquipAnimation", skipAnim);
            if (skipAnim)
            {
                g_skipUntilMs.store(NowMs() + 500, std::memory_order_relaxed);
                g_suppressForceEquipClips.store(1, std::memory_order_relaxed);
            }
        }
    }
    _skipAnim = skipAnim;
    return skipAnim;
}

void EquipHook::Update_Hook(RE::hkbClipGenerator* a_this, const RE::hkbContext& a_context, float a_timestep)
{
    if (!a_this) {
        _Update(a_this, a_context, a_timestep);
        return;
    }

    std::string_view nm{ a_this->animationName.c_str() };
    if (IsEquipClip(nm) && a_this->atEnd) return;

    _Update(a_this, a_context, a_timestep);
}

void EquipHook::Activate_Hook(RE::hkbClipGenerator* a_this, const RE::hkbContext& a_context)
{
    _Activate(a_this, a_context);

    if (!a_this) return;

    const int sup = g_suppressForceEquipClips.load(std::memory_order_relaxed);
    if (sup <= 0) return;

    std::string_view nm{ a_this->animationName.c_str() };
    if (!IsEquipClip(nm)) return;

    g_suppressForceEquipClips.store(0, std::memory_order_relaxed);

    float duration = 2.0f;
    if (a_this->binding && a_this->binding->animation) {
        duration = a_this->binding->animation->duration;
    }

    a_this->mode = RE::hkbClipGenerator::PlaybackMode::kModeSinglePlay;
    _Update(a_this, a_context, duration + 0.01f);
    a_this->atEnd = true;
}