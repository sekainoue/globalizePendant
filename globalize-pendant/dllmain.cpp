#include "pch.h"
#include "loader.h"

#include <MinHook.h>

using namespace loader;

// 0x1451c2400 -> sMhGUI
// 0x1410f6eb0 -> get uGUIEquipBox
// equipBox + 0x4408 -> Weapons array
// equipBox + 0x9228 -> Weapons count
// equipCharm + 0x3DC8 -> Selected weapon
// weapon + 0x64 -> Equipped pendant id
// equipCharm + 0x36A8 + index * 0x8 -> Pendant
// pendant + 0x20 -> id

const auto target_function = (void*)0x14050d700;
void(*target_function_original)(uintptr_t, int, void*) = nullptr;

// void FUN_14050d700(uGUIEquipCharm *param_1,int pendantIndex,undefined8 param_3)
void on_pendant_changed(uintptr_t equip_charm, int pendant_index, void* unk) {
    uintptr_t pendant = *(uintptr_t*)(equip_charm + 0x36A8 + pendant_index * 0x8);
    int new_pendant_id = *(int16_t*)(pendant + 0x20);

    uintptr_t selected_weapon = *(uintptr_t*)(equip_charm + 0x3DC8);
    if (selected_weapon == 0) {
        LOG(INFO) << "No weapon selected";
        return target_function_original(equip_charm, pendant_index, unk);
    }

    const auto get_equip_box = (uintptr_t(*)(uintptr_t))0x1410f6eb0;
    uintptr_t gui_singleton = *(uintptr_t*)0x1451c2400;
    uintptr_t equip_box = get_equip_box(gui_singleton);

    uintptr_t* weapons = (uintptr_t*)(equip_box + 0x4408);
    int weapon_count = *(int*)(equip_box + 0x9228);

    int equipped_pendant_id = *(int*)(selected_weapon + 0x64);
    if (new_pendant_id == equipped_pendant_id) {
        // unequip pendant from all weapons
        for (int i = 0; i < weapon_count; i++) {
            if (weapons[i] != selected_weapon) {
                *(int*)(weapons[i] + 0x64) = -1;
            }
        }
    } else {
        // equip selected pendant on all weapons
        for (int i = 0; i < weapon_count; i++) {
            if (weapons[i] != selected_weapon) {
                *(int*)(weapons[i] + 0x64) = new_pendant_id;
            }
        }
    }

    return target_function_original(equip_charm, pendant_index, unk);
}


static void load() {
    MH_Initialize();
    MH_CreateHook(target_function, on_pendant_changed, (void**)&target_function_original);
    MH_EnableHook(target_function);
}


BOOL APIENTRY DllMain(HMODULE Dll, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        LOG(INFO) << "Loading GlobalPendant";
        load();
    }

    return TRUE;
}


