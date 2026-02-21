#pragma once

using EventChecker = RE::BSEventNotifyControl;

class EquipHook
{
public:
    static void Hook()
    {
        REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{RE::VTABLE_PlayerCharacter[0]};
        //REL::Relocation<std::uintptr_t> NonPlayerCharacterVtbl{ RE::VTABLE_Actor[0] };
        REL::Relocation<std::uintptr_t> animHolderVtbl{RE::VTABLE_PlayerCharacter[3]};
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_hkbClipGenerator[0]};

        _OnEquipItemPC = PlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemPC);
        //_OnEquipItemNPC     = NonPlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemNPC);
        _NotifyAnimationGraph = animHolderVtbl.write_vfunc(0x01, NotifyAnimationGraph_Hook);
        _Update = vtbl.write_vfunc(0x05, Update_Hook);
        _Activate = vtbl.write_vfunc(0x04, Activate_Hook);
    }

private:
    static void OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim);
    //static void OnEquipItemNPC(RE::Actor* a_this, bool a_playAnim);
    static bool NotifyAnimationGraph_Hook(RE::IAnimationGraphManagerHolder *a_this, const RE::BSFixedString &a_event);
    static void Update_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context, float a_timestep);
    static void Activate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context);

    static inline REL::Relocation<decltype(OnEquipItemPC)> _OnEquipItemPC;
    //static inline REL::Relocation<decltype(OnEquipItemNPC)> _OnEquipItemNPC;
    static inline REL::Relocation<decltype(NotifyAnimationGraph_Hook)> _NotifyAnimationGraph;
    static inline REL::Relocation<decltype(Update_Hook)> _Update;
    static inline REL::Relocation<decltype(Activate_Hook)> _Activate;

    static bool SkipAnim(RE::PlayerCharacter *a_this, bool a_playAnim);

    static inline bool _skipAnim;
};