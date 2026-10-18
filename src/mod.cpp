#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"

#include "d/d_kantera_icon_meter.h"
#include "d/d_item_data.h"
#include "d/d_menu_ring.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_pane_class.h"
#include "dusk/config_var.hpp"

#include <array>
#include <string_view>

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);

DEFINE_HOOK(&dMeter2Draw_c::draw, DrawHud);
DEFINE_HOOK(&dMeter2Draw_c::drawKanteraScreen, DrawGauge);
DEFINE_HOOK(&dDlst_KanteraIcon_c::draw, DrawOilGauge);
DEFINE_HOOK(&dMenu_Ring_c::getItemMaxNum, RingItemMax);
DEFINE_HOOK(&dMeter2_c::isArrowEquip, ArrowSoundEquip);
DEFINE_HOOK(&dMeter2_c::isPachinkoEquip, SeedSoundEquip);

using BoolConfig = dusk::config::ConfigVar<bool>;
static const BoolConfig* infinite_hearts;
static const BoolConfig* infinite_arrows;
static const BoolConfig* infinite_seeds;
static const BoolConfig* infinite_bombs;
static const BoolConfig* infinite_oil;
static const BoolConfig* infinite_oxygen;
static const BoolConfig* infinite_rupees;

static u8 (*get_selected_item)(int);
static u8 (*get_inventory_item)(int, bool);

static std::array<J2DPane*, 15> hidden_panes;
static size_t hidden_count;

template <typename T>
static ModResult resolve(const char* name, T& target) {
    void* address = nullptr;
    ModResult result = svc_hook->resolve(mod_ctx, name, &address, nullptr);
    if (result == MOD_OK) {
        target = reinterpret_cast<T>(address);
    } else {
        mods::log::error("failed to resolve {}", name);
    }
    return result;
}

static bool infinite_ammo(u8 item) {
    switch (item) {
    case dItemNo_BOW_e:
    case dItemNo_LIGHT_ARROW_e:
    case dItemNo_ARROW_LV1_e:
    case dItemNo_ARROW_LV2_e:
    case dItemNo_ARROW_LV3_e:
    case dItemNo_HAWK_ARROW_e:
        return infinite_arrows->getValue();
    case dItemNo_BOMB_BAG_LV1_e:
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
        return infinite_bombs->getValue();
    case dItemNo_BOMB_ARROW_e:
        // Bomb arrows are limited by whichever resource is still finite.
        return infinite_arrows->getValue() && infinite_bombs->getValue();
    case dItemNo_PACHINKO_e:
        return infinite_seeds->getValue();
    default:
        return false;
    }
}

static void hide_for_draw(J2DPane* pane) {
    if (pane != nullptr && pane->isVisible()) {
        hidden_panes[hidden_count++] = pane;
        pane->hide();
    }
}

static HookAction on_hud_pre(ModContext*, void* args, void*, void*) {
    auto* hud = mods::arg<dMeter2Draw_c*>(args, 0);
    hidden_count = 0;
    if (infinite_hearts->getValue()) {
        hide_for_draw(hud->mpLifeParent->getPanePtr());
    }
    if (infinite_rupees->getValue()) {
        for (auto& digit : hud->mpRupeeTexture) {
            for (auto* pane : digit) {
                hide_for_draw(pane->getPanePtr());
            }
        }
    }
    for (int button = 0; button < 2; ++button) {
        if (infinite_ammo(get_selected_item(button))) {
            for (auto* digit : hud->mpItemNumTex[button]) {
                hide_for_draw(digit);
            }
        }
    }
    return HOOK_CONTINUE;
}

static void on_hud_post(ModContext*, void*, void*, void*) {
    while (hidden_count != 0) {
        hidden_panes[--hidden_count]->show();
    }
}

static HookAction on_gauge_pre(ModContext*, void* args, void*, void*) {
    u8 type = mods::arg<u8>(args, 1);
    if ((type == 1 && infinite_oil->getValue()) ||
        (type == 2 && infinite_oxygen->getValue())) {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

static HookAction on_oil_gauge_pre(ModContext*, void*, void*, void*) {
    return infinite_oil->getValue() ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}

static HookAction on_ring_max_pre(ModContext*, void* args, void* retval, void*) {
    u8 slot = mods::arg<u8>(args, 1);
    if (infinite_ammo(get_inventory_item(slot, false))) {
        // The ring only draws ammo digits when the display maximum is nonzero.
        *static_cast<u8*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

static HookAction on_ammo_sound_equip_pre(ModContext*, void*, void* retval, void* userdata) {
    const auto* cheat = *static_cast<const BoolConfig**>(userdata);
    if (cheat->getValue()) {
        // These HUD-only equipment checks gate refill ticks, including on HUD teardown.
        *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    dusk::config::ConfigVarBase* (*get_config)(std::string_view) = nullptr;
    ModResult result = resolve("dusk::config::GetConfigVar", get_config);
    if (result != MOD_OK) {
        return result;
    }
    const struct {
        const char* name;
        const BoolConfig** value;
    } cheats[] = {
        {"game.infiniteHearts", &infinite_hearts},
        {"game.infiniteArrows", &infinite_arrows},
        {"game.infiniteSeeds", &infinite_seeds},
        {"game.infiniteBombs", &infinite_bombs},
        {"game.infiniteOil", &infinite_oil},
        {"game.infiniteOxygen", &infinite_oxygen},
        {"game.infiniteRupees", &infinite_rupees},
    };
    for (const auto& cheat : cheats) {
        auto* config = get_config(cheat.name);
        if (config == nullptr) {
            mods::log::error("missing cheat setting {}", cheat.name);
            return MOD_UNAVAILABLE;
        }
        *cheat.value = static_cast<const BoolConfig*>(config);
    }

    result = resolve("dComIfGp_getSelectItem", get_selected_item);
    if (result != MOD_OK) {
        return result;
    }
    result = resolve("dComIfGs_getItem", get_inventory_item);
    if (result != MOD_OK) {
        return result;
    }

    result = mods::hook::add_pre<DrawHud>(on_hud_pre);
    if (result != MOD_OK) {
        return result;
    }
    result = mods::hook::add_post<DrawHud>(on_hud_post);
    if (result != MOD_OK) {
        return result;
    }
    result = mods::hook::add_pre<DrawGauge>(on_gauge_pre);
    if (result != MOD_OK) {
        return result;
    }
    result = mods::hook::add_pre<DrawOilGauge>(on_oil_gauge_pre);
    if (result != MOD_OK) {
        return result;
    }
    result = mods::hook::add_pre<RingItemMax>(on_ring_max_pre);
    if (result != MOD_OK) {
        return result;
    }

    HookOptions arrow_sound_options = HOOK_OPTIONS_INIT;
    arrow_sound_options.userdata = &infinite_arrows;
    result = mods::hook::add_pre<ArrowSoundEquip>(on_ammo_sound_equip_pre, &arrow_sound_options);
    if (result != MOD_OK) {
        return result;
    }
    HookOptions seed_sound_options = HOOK_OPTIONS_INIT;
    seed_sound_options.userdata = &infinite_seeds;
    result = mods::hook::add_pre<SeedSoundEquip>(on_ammo_sound_equip_pre, &seed_sound_options);
    if (result != MOD_OK) {
        return result;
    }

    mods::log::info("Hide Infinite Counts initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
