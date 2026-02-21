#pragma once

using EventChecker = RE::BSEventNotifyControl;

class EquipHook
{
public:
    static void Hook()
    {
        REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{RE::VTABLE_PlayerCharacter[0]};
        //REL::Relocation<std::uintptr_t> NonPlayerCharacterVtbl{ RE::VTABLE_Actor[0] };
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_hkbClipGenerator[0]};

        _OnEquipItemPC = PlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemPC);
        //_OnEquipItemNPC     = NonPlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemNPC);
        _Update = vtbl.write_vfunc(0x05, Update_Hook);
        _Activate = vtbl.write_vfunc(0x04, Activate_Hook);
    }

private:
    static void OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim);
    //static void OnEquipItemNPC(RE::Actor* a_this, bool a_playAnim);
    static void Update_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context, float a_timestep);
    static void Activate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context);

    static inline REL::Relocation<decltype(OnEquipItemPC)> _OnEquipItemPC;
    //static inline REL::Relocation<decltype(OnEquipItemNPC)> _OnEquipItemNPC;
    static inline REL::Relocation<decltype(Update_Hook)> _Update;
    static inline REL::Relocation<decltype(Activate_Hook)> _Activate;

    static bool SkipAnim(RE::PlayerCharacter *a_this, bool a_playAnim);

    static inline bool _skipAnim;
};