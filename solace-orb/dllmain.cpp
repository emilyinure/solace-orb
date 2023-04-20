#include "../plugin_sdk/plugin_sdk.hpp"
#include "orb_walker.h"
#include "settings.h"

PLUGIN_NAME("solace-orb beta")
PLUGIN_TYPE(plugin_type::core)

orb_walker orb;

bool last_hit_mode()
{
    return orb.last_hit_mode();
}
bool mixed_mode()
{
    return orb.mixed_mode();
}
bool lane_clear_mode()
{

    return orb.lane_clear_mode();
}
bool combo_mode()
{

    return orb.combo_mode();
}
bool flee_mode()
{

    return orb.flee_mode();
}
bool none_mode()
{

    return orb.none_mode();
}
bool harass()
{

    return orb.harass();
}
bool reset_auto_attack_timer()
{

    return orb.reset_auto_attack_timer();
}
game_object_script get_target()
{

    return orb.get_target();
}
game_object_script get_last_target()
{
    return orb.get_last_target();
}
float get_last_aa_time()
{

    return orb.get_last_aa_time();
}
float get_last_move_time()
{

    return orb.get_last_move_time();
}
float get_my_projectile_speed()
{

    return orb.get_my_projectile_speed();
}
bool can_attack()
{

    return orb.can_attack();
}
bool can_move(float extra_windup)
{

    return orb.can_move(extra_windup);
}
bool should_wait()
{

    return orb.should_wait();
}
void move_to(vector& pos)
{
    orb.move_to(pos);
}
void orbwalk(game_object_script target, vector& pos)
{
    orb.orbwalk(target, pos);
}
void set_attack(bool enable)
{
    orb.set_attack(enable);
}
void set_movement(bool enable)
{
    orb.set_movement(enable);
}
void set_orbwalking_target(game_object_script target)
{
    orb.set_orbwalking_target(target);
}
void set_orbwalking_point(vector const& pos)
{
    orb.set_orbwalking_point(pos);
}
std::uint32_t get_orb_state()
{
    return orb.get_orb_state();
}

float get_auto_attack_range(game_object* from)
{
    auto attack_range = from->get_bounding_radius();

    if (from->is_ai_turret())
        attack_range += 775.f;
    else if (from->is_ai_hero() && from->get_champion() == champion_id::Zeri)
        attack_range += 500.f;
    else
        attack_range += from->get_attack_range();

    return attack_range;
}

void on_menu()
{
}

void on_draw()
{
    if (orb.id != orbwalker->get_active_orbwalker())
        return;
    if (myhero->is_dead())
        return;
    if (settings::drawings::enable->get_bool())
    {
        float range = orb.get_auto_attack_range(myhero.get());
        draw_manager->add_circle(myhero->get_position(), range, 0xA0FFFFFF);
        draw_manager->add_filled_circle(myhero->get_position(), range, 0x50FFFFFF);
    }
}

void on_preupdate()
{
    if (orb.id != orbwalker->get_active_orbwalker())
        return;
    orb.enabled = false;
    orb.mixed_mode();
    orb.combo_mode();
    orb.last_hit_mode();
    orb.lane_clear_mode();
}

void on_update()
{
    if (orb.id != orbwalker->get_active_orbwalker())
        return;
    orb.orbwalk(orb.get_target(), orb.m_move_pos);
}

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    auto main_menu = menu->create_tab("solace.orb", "solace-orb beta");
    const auto drawings_tab = main_menu->add_tab("solace.orb.drawings", "Drawings");
    settings::drawings::enable = drawings_tab->add_checkbox("solace.orb.drawings.enable", "Enable", true);

    
    const auto bindings_tab = main_menu->add_tab("solace.orb.bindings", "Bindings");
    settings::last_hit =
        bindings_tab->add_hotkey("solace.orb.bindings.last_hit", "Last Hit", TreeHotkeyMode::Hold, 88, false);
    settings::lane_clear =
        bindings_tab->add_hotkey("solace.orb.bindings.laneclear", "Lane Clear", TreeHotkeyMode::Hold, 86, false);
    settings::combo =
        bindings_tab->add_hotkey("solace.orb.bindings.combo", "Combo", TreeHotkeyMode::Hold, 32, false);
    settings::mixed = bindings_tab->add_hotkey("solace.orb.bindings.mixed", "Mixed", TreeHotkeyMode::Hold, 160, false);

    event_handler<events::on_env_draw>::add_callback(on_draw);
    event_handler<events::on_preupdate>::add_callback(on_preupdate, event_prority::highest);
    event_handler<events::on_update>::add_callback(on_update);
    orb.id = orbwalker->add_orbwalker_callback(
        "solace-orb beta", last_hit_mode, mixed_mode, lane_clear_mode, combo_mode, flee_mode, none_mode, harass,
        reset_auto_attack_timer, get_target, get_last_target, get_last_aa_time, get_last_move_time,
        get_my_projectile_speed, can_attack, can_move, should_wait, move_to, orbwalk, set_attack, set_movement,
        set_orbwalking_target, set_orbwalking_point, get_orb_state);

    return true;
}

PLUGIN_API void on_sdk_unload()
{
    event_handler<events::on_env_draw>::remove_handler(on_draw);
    orbwalker->remove_orbwalker_callback(orb.id);
}