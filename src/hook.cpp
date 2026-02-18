#include "hook.h"
#include "event.h"
#include "util.h"
using namespace Util;

void EquipHook::OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    _OnEquipItemPC(a_this, !SkipAnim(a_this, a_playAnim));
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
#ifdef WORK_WITH_MAGIC_OBJECTS
    if (a_object->IsMagicItem())
        return true;
#endif

    return a_object->As<RE::TESObjectWEAP>() != nullptr || a_object->As<RE::TESObjectARMO>() != nullptr; //  && (uint32_t)a_object->As<RE::TESObjectWEAP>()->GetWeaponType() <= 6u)
}

void SendEquipEvents(RE::Actor *a_this, RE::TESForm *a_lHandObject, RE::TESForm *a_rHandObject, bool changePose)
{
    if (!a_this || !AnimationEventTracker::GetSingleton())
        return;

    auto *tracker = AnimationEventTracker::GetSingleton();

    bool rIsSpell = a_rHandObject && a_rHandObject->IsMagicItem();
    bool lIsSpell = a_lHandObject && a_lHandObject->IsMagicItem();
    bool rIsWeapon = a_rHandObject && (a_rHandObject->IsWeapon() || a_rHandObject->IsArmor());
    bool lIsWeapon = a_lHandObject && (a_lHandObject->IsWeapon() || a_lHandObject->IsArmor());

    bool hasSpell = lIsSpell || rIsSpell;
    bool hasWeapon = lIsWeapon || rIsWeapon;

    tracker->SendAnimationEvent(a_this, "weaponDraw");
#ifdef WORK_WITH_MAGIC_OBJECTS
    if (hasSpell)
    {
        tracker->SendAnimationEvent(a_this, "Magic_Equip_Out");
        tracker->SendAnimationEvent(a_this, "Magic_Equip_OutMoving");
    }
#endif
    if (hasWeapon)
    {
        tracker->SendAnimationEvent(a_this, "WeapEquip_Out");
        tracker->SendAnimationEvent(a_this, "WeapEquip_OutMoving");
    }
}

bool EquipHook::SkipAnim(RE::PlayerCharacter *a_this, bool a_playAnim)
{
    bool skipAnim = !a_playAnim;
    if (AnimationEventTracker::GetSingleton()->Register() && a_this && a_this->AsActorState() && a_this->AsActorState()->IsWeaponDrawn())
    {
        auto rHandObj = a_this->GetEquippedObject(false);
        auto lHandObj = a_this->GetEquippedObject(true);

        if (!(lHandObj && rHandObj) || CheckIsValidBoundObject(lHandObj) || CheckIsValidBoundObject(rHandObj))
        {
            int delay = 300;
            bool skip3D = false;
            bool changePose = false;
            a_this->GetGraphVariableBool("SkipEquipAnimation", skipAnim);
            a_this->GetGraphVariableInt("LoadBoundObjectDelay", delay);
            a_this->GetGraphVariableBool("Skip3DLoading", skip3D);
            a_this->GetGraphVariableBool("ChangePose", changePose);
            if (delay < (int)(*g_deltaTimeRealTime * 1000.f))
                delay = (int)(*g_deltaTimeRealTime * 1000.f); // the loading process will start next frame.
            if (!skip3D && skipAnim)
            {
                std::jthread delayedEquipThread([=]()
                                                {
                    if (skipAnim) {
                        spdlog::debug("equip anim skipped");
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        SendEquipEvents(a_this, lHandObj, rHandObj, changePose);
                        spdlog::debug("weapon 3d model called");
                    } });
                delayedEquipThread.detach();
            }
        }
    }
    _skipAnim = skipAnim;
    return skipAnim;
}