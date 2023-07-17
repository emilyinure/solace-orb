#include "plugin_sdk.hpp"
#include "orb_walker.h"
#include "settings.h"
#include "health_pred.h"

PLUGIN_NAME("solace-orb test")
PLUGIN_TYPE(plugin_type::core)
const char* version = "1.0.2";

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
    if (myhero->get_champion() == champion_id::Aphelios)
    {
        // ApheliosCalibrumManager
        // ApheliosGravitumManager
    }

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

float get_ping()
{
    return ping->get_ping() / 1000.f;
}

void on_draw()
{
    if (orb.m_id != orbwalker->get_active_orbwalker())
        return;
    if (myhero->is_dead())
        return;
    if (settings::drawings::enable->get_bool())
    {
        float range = orb.get_auto_attack_range(myhero.get());
        draw_manager->add_circle_with_glow(myhero->get_position(), settings::drawings::color->get_color(), range, 3.f,
                                           glow_data{0.01f * settings::drawings::glow_inner_size->get_int(),
                                                     0.01f * settings::drawings::glow_inner_strength->get_int(),
                                                     0.01f * settings::drawings::glow_outer_size->get_int(),
                                                     0.01f * settings::drawings::glow_outer_strength->get_int()});
        // draw_manager->add_filled_circle(myhero->get_position(), range, 0x50FFFFFF);
    }

    for (auto& minion : entitylist->get_enemy_minions())
    {
        //if (orb.m_last_hit == minion)
        //    draw_manager->add_text_on_screen(minion->get_hpbar_pos() - vector(0, 40, 0),
        //                                     settings::drawings::color->get_color(), 15, "last_hit");
        //if (orb.m_wait == minion)
        //    draw_manager->add_text_on_screen(minion->get_hpbar_pos() - vector(0, 40, 0),
        //                                     settings::drawings::color->get_color(), 15, "wait");
        //if (orb.m_adjust == minion)
        //    draw_manager->add_text_on_screen(minion->get_hpbar_pos() - vector(0, 40, 0),
        //                                     settings::drawings::color->get_color(), 15, "adjust");
        float death_time = -1.f;
         auto predicted_health_when_attack = healthpred.get_last_health_before_death(minion, death_time);
         draw_manager->add_text_on_screen(minion->get_hpbar_pos() - vector(0, 40, 0),
                                          settings::drawings::color->get_color(), 15,
                                          std::to_string(predicted_health_when_attack).c_str());
         draw_manager->add_text_on_screen(minion->get_hpbar_pos() - vector(0, 20, 0),
         settings::drawings::color->get_color(), 15,
                                          std::to_string(death_time).c_str());
    }
}

void on_preupdate()
{
    if (orb.m_id != orbwalker->get_active_orbwalker())
        return;
    orb.m_enabled = false;
    healthpred.on_preupdate();
}

void on_update()
{
    healthpred.on_update();
    if (orb.m_id != orbwalker->get_active_orbwalker())
    {
        settings::main_menu->is_hidden() = true;
        return;
    }
    settings::main_menu->is_hidden() = false; // if (!orb.enabled)
    //{
    orb.combo_mode();
    orb.last_hit_mode();
    orb.lane_clear_mode();
    orb.mixed_mode();
    orb.flee_mode();
   
    //}
    orb.orbwalk(orb.get_target(), orb.m_move_pos);
}
void on_issue_order(game_object_script& target, vector& pos, _issue_order_type& _type, bool* process)
{
}
float last_cast = 0.f;
void on_stop_cast(game_object_script sender, spell_instance_script spell)
{
    if (sender && spell && sender->get_id() == myhero->get_id())
        orb.on_stop_cast(spell);
    //if (sender && spell && (sender->is_minion() || sender->is_ai_turret()))
    //{
    //    healthpred.reset(sender->get_id());
    //}
}

void on_network_packet(game_object_script object, uint32_t id, pkttype_e type, void* args)
{
    if (object && object->is_ai_turret())
    {
        // console->print(std::to_string(id).c_str());
    }
}

void on_create_object(game_object_script object)
{
}

void on_process_spellcast(game_object_script sender, spell_instance_script spell)
{
    if (sender && spell && sender->get_id() == myhero->get_id())
    {
        // console->print("cast");
        orb.on_spell_cast(spell);
    }

    if (sender && spell && (sender->is_minion() || sender->is_ai_turret()))
    {
        auto target = entitylist->get_object(spell->get_last_target_id());
        // if (sender->is_minion())
        //     console->print(std::to_string(sender->get_minion_type()).c_str());
        healthpred.on_missle_created(sender, target, spell);
    }
}

void on_do_cast(game_object_script sender, spell_instance_script spell)
{
}

void min_move_delay_changed(TreeEntry* tree)
{
    int val = tree->get_int();
    if (settings::humanizer::max_move_delay->get_int() < val)
        settings::humanizer::max_move_delay->set_int(val);
}

void max_move_delay_changed(TreeEntry* tree)
{
    int val = settings::humanizer::min_move_delay->get_int();
    if (tree->get_int() < val)
        tree->set_int(val);
}

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    settings::main_menu = menu->create_tab("solace.orb", "solace-orb beta");

    const auto drawings_tab = settings::main_menu->add_tab("solace.orb.drawings", "Drawings");
    {
        settings::drawings::enable = drawings_tab->add_checkbox("solace.orb.drawings.enable", "Enable", true);
        float color[4] = {62.f, 214.f, 148.f, 255.f};
        settings::drawings::color = drawings_tab->add_colorpick("solace.orb.drawings.color", "Color", color, true);
        settings::drawings::glow_inner_size =
            drawings_tab->add_slider("solace.orb.humanizer.glowinnersize", "Glow Inner Size", 12, 0, 100);
        settings::drawings::glow_inner_strength =
            drawings_tab->add_slider("solace.orb.humanizer.glowinnerstrength", "Glow Inner Strength", 95, 0, 100);
        settings::drawings::glow_outer_size =
            drawings_tab->add_slider("solace.orb.humanizer.glowoutersize", "Glow Outer Size", 0, 0, 100);
        settings::drawings::glow_outer_strength =
            drawings_tab->add_slider("solace.orb.humanizer.glowouterstrength", "Glow Outer Strength", 0, 0, 100);
    }

    const auto bindings_tab = settings::main_menu->add_tab("solace.orb.bindings", "Bindings");
    {
        settings::bindings::last_hit =
            bindings_tab->add_hotkey("solace.orb.bindings.last_hit", "Last Hit", TreeHotkeyMode::Hold, 88, false);
        settings::bindings::lane_clear =
            bindings_tab->add_hotkey("solace.orb.bindings.laneclear", "Lane Clear", TreeHotkeyMode::Hold, 86, false);
        settings::bindings::combo =
            bindings_tab->add_hotkey("solace.orb.bindings.combo", "Combo", TreeHotkeyMode::Hold, ' ', false);
        settings::bindings::mixed =
            bindings_tab->add_hotkey("solace.orb.bindings.mixed", "Mixed", TreeHotkeyMode::Hold, 160, false);
        settings::bindings::auto_space =
            bindings_tab->add_hotkey("solace.orb.bindings.autospace", "Auto Space", TreeHotkeyMode::Hold, 5, false);
        settings::bindings::flee =
            bindings_tab->add_hotkey("solace.orb.bindings.flee", "Flee", TreeHotkeyMode::Hold, 'z', false);
    }

    const auto champ_tab = settings::main_menu->add_tab("solace.orb.champ", "Champ");
    {
        settings::champ::akshan_aa =
            champ_tab->add_hotkey("solace.orb.champ.akshanaa", "Akshan Double AA", TreeHotkeyMode::Toggle, 0x0, false);
    }

    const auto spacing_tab = settings::main_menu->add_tab("solace.orb.spacing", "Spacing");
    {
        settings::spacing::space_local =
            spacing_tab->add_checkbox("solace.orb.spacing.spacelocal", "Space Local AA Range", false);
        auto blacklist_tab = spacing_tab->add_tab("solace.orb.spacing.blacklist", "Blacklist");
        std::string temp = "solace.orb.spacing.blacklist.";
        for (auto& i : entitylist->get_enemy_heroes())
        {
            orb.m_blacklisted_champs[i->get_network_id()] =
                blacklist_tab->add_checkbox((temp + i->get_base_skin_name()).c_str(), i->get_base_skin_name(), false);
        }
    }

    const auto humanizer_tab = settings::main_menu->add_tab("solace.orb.humanizer", "Humanizer");
    {
        settings::humanizer::min_move_delay =
            humanizer_tab->add_slider("solace.orb.humanizer.minmovedelay", "Minimum Move Delay", 50, 40, 1000);
        settings::humanizer::min_move_delay->add_property_change_callback(min_move_delay_changed);
        settings::humanizer::max_move_delay =
            humanizer_tab->add_slider("solace.orb.humanizer.maxmovedelay", "Maximum Move Delay", 80, 40, 1000);
        settings::humanizer::max_move_delay->add_property_change_callback(max_move_delay_changed);
    }

    settings::main_menu->add_separator("solace.orb.sep", "Message me on discord with issues");
    settings::main_menu->add_separator("solace.orb.sep2", "emily#4986");
    settings::main_menu->add_separator("solace.orb.ver", (std::string("Version ") + version).c_str());

    orb.m_is_akshan = myhero->get_champion() == champion_id::Akshan;
    orb.m_is_sett = myhero->get_champion() == champion_id::Sett;
    event_handler<events::on_env_draw>::add_callback(on_draw);
    event_handler<events::on_preupdate>::add_callback(on_preupdate, event_prority::highest);
    event_handler<events::on_update>::add_callback(on_update, event_prority::highest);
    event_handler<events::on_stop_cast>::add_callback(on_stop_cast, event_prority::highest);
    event_handler<events::on_do_cast>::add_callback(on_do_cast, event_prority::highest);
    event_handler<events::on_process_spell_cast>::add_callback(on_process_spellcast, event_prority::highest);
    event_handler<events::on_issue_order>::add_callback(on_issue_order, event_prority::highest);
    event_handler<events::on_create_object>::add_callback(on_create_object, event_prority::highest);
    event_handler<events::on_network_packet>::add_callback(on_network_packet, event_prority::highest);
    orb.m_id = orbwalker->add_orbwalker_callback(
        "solace-orb beta", last_hit_mode, mixed_mode, lane_clear_mode, combo_mode, flee_mode, none_mode, harass,
        reset_auto_attack_timer, get_target, get_last_target, get_last_aa_time, get_last_move_time,
        get_my_projectile_speed, can_attack, can_move, should_wait, move_to, orbwalk, set_attack, set_movement,
        set_orbwalking_target, set_orbwalking_point, get_orb_state);

    return true;
}

PLUGIN_API void on_sdk_unload()
{

    event_handler<events::on_env_draw>::remove_handler(on_draw);
    event_handler<events::on_preupdate>::remove_handler(on_preupdate);
    event_handler<events::on_update>::remove_handler(on_update);
    event_handler<events::on_do_cast>::remove_handler(on_process_spellcast);
    event_handler<events::on_issue_order>::remove_handler(on_issue_order);
    event_handler<events::on_create_object>::remove_handler(on_create_object);
    event_handler<events::on_network_packet>::remove_handler(on_network_packet);
    orbwalker->remove_orbwalker_callback(orb.m_id);
}