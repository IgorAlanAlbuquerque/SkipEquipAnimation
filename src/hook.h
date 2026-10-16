#pragma once
#include "event.h"

class EquipHook
{
public:
    static void Hook()
    {
        REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{RE::VTABLE_PlayerCharacter[0]};
        REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_hkbClipGenerator[0]};

        // VTABLE_PlayerCharacter[3] = IAnimationGraphManagerHolder vtable
        // (inherited offset 0x38: TESForm[0], BSHandleRefObject[1], BSTEventSink<BSAnimGraphEvent>[2], IAnimationGraphManagerHolder[3])
        REL::Relocation<std::uintptr_t> animGraphHolderVtbl{RE::VTABLE_PlayerCharacter[3]};

        // Investigation 3a: hook hkbStateMachine::Update to track state changes
        REL::Relocation<std::uintptr_t> smVtbl{RE::VTABLE_hkbStateMachine[0]};

        _OnEquipItemPC        = PlayerCharacterVtbl.write_vfunc(0xB2, OnEquipItemPC);
        _Activate             = vtbl.write_vfunc(0x04, Activate_Hook);
        _Update               = vtbl.write_vfunc(0x05, Update_Hook);
        _Deactivate           = vtbl.write_vfunc(0x07, Deactivate_Hook);
        _NotifyAnimationGraph = animGraphHolderVtbl.write_vfunc(0x01, NotifyAnimationGraph_Hook);
        _SM_Update            = smVtbl.write_vfunc(0x05, SM_Update_Hook);
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

    // Investigation: logs every event the engine sends to the player's behavior graph
    static bool NotifyAnimationGraph_Hook(RE::IAnimationGraphManagerHolder *a_this, const RE::BSFixedString &a_eventName);

    // Investigation 3a: tracks hkbStateMachine state changes for the player
    static void SM_Update_Hook(RE::hkbStateMachine *a_this, const RE::hkbContext &a_context, float a_timestep);

    static EquipMode GetEquipMode(const RE::PlayerCharacter *a_this, bool a_playAnim);

    static inline REL::Relocation<decltype(OnEquipItemPC)>             _OnEquipItemPC;
    static inline REL::Relocation<decltype(Activate_Hook)>             _Activate;
    static inline REL::Relocation<decltype(Update_Hook)>               _Update;
    static inline REL::Relocation<decltype(Deactivate_Hook)>           _Deactivate;
    static inline REL::Relocation<decltype(NotifyAnimationGraph_Hook)> _NotifyAnimationGraph;
    static inline REL::Relocation<decltype(SM_Update_Hook)>            _SM_Update;
};