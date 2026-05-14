#pragma once
#include "event.h"

class EquipHook
{
public:
    static void Hook()
    {
        REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{RE::VTABLE_PlayerCharacter[0]};
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_hkbClipGenerator[0]};

        _OnEquipItemPC = PlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemPC);
        _Activate = vtbl.write_vfunc(0x04, Activate_Hook);
        _Update = vtbl.write_vfunc(0x05, Update_Hook);
        _Deactivate = vtbl.write_vfunc(0x07, Deactivate_Hook);
    }
    static void ResetState();

private:
    enum class EquipMode
    {
        Normal,      // play animation normally
        Skip,        // playAnim=false + delayed SendEquipEvents
        InstantAnim, // playAnim=true, fast-forward clip, suppress Generate
    };

    static void OnEquipItemPC(RE::PlayerCharacter *a_this, bool a_playAnim);
    static void Activate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context);
    static void Update_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context, float a_timestep);
    static void Deactivate_Hook(RE::hkbClipGenerator *a_this, const RE::hkbContext &a_context);

    static EquipMode GetEquipMode(const RE::PlayerCharacter *a_this, bool a_playAnim);

    static inline REL::Relocation<decltype(OnEquipItemPC)> _OnEquipItemPC;
    static inline REL::Relocation<decltype(Activate_Hook)> _Activate;
    static inline REL::Relocation<decltype(Update_Hook)> _Update;
    static inline REL::Relocation<decltype(Deactivate_Hook)> _Deactivate;
};