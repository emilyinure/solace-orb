#include "orb_walker.h"
#include "settings.h"
#include "health_pred.h"

bool orb_walker::is_reset(spell_data_script spell)
{
    auto name = spell->get_name_cstr();
    auto name_hash = spell->get_name_hash();
    return name_hash == spell_hash("GravesMove") || name_hash == spell_hash("AatroxE") ||
           strstr(name, "GravesAutoAttackRecoilCastE");
}

bool orb_walker::ignore_spell(spell_data_script spell)
{
    auto name = spell->get_name_cstr();
    return strstr(name, "Summoner") || strstr(name, "Trinket") || spell->get_name_hash() == spell_hash("TwitchR");
}

bool orb_walker::special_spell_conditions(spell_instance_script spell)
{
    if (!spell->is_auto_attack() && (m_is_akshan || m_is_sett))
    {
        m_last_left_attack = -1.f;
        m_double_attack = 0;
    }

    float cast_start = gametime->get_time() - ping->get_ping() / 2000.f + (1.f / 30.f);
    float end_cast = spell->get_attack_cast_delay();
    float end_attack = spell->get_attack_delay();
    console->print(spell->get_spell_data()->get_name_cstr());
    switch (spell->get_spell_data()->get_name_hash())
    {
        case spell_hash("KalistaBasicAttack"):
            add_cast(cast_start, 0.f, end_attack / 2.f, spell->get_spell_data()->get_name_hash());
            return true;
        case spell_hash("AkshanBasicAttack"):
        case spell_hash("AkshanCritAttack"):
            add_cast(cast_start, end_cast, end_attack, spell->get_spell_data()->get_name_hash());
            m_last_left_attack = gametime->get_prec_time();
            if (settings::champ::akshan_aa->get_bool())
                m_double_attack = 1;
            return true;
        case spell_hash("SettBasicAttack"):
        case spell_hash("SettBasicAttack3"):
            m_last_left_attack = cast_start;
            m_double_attack = 1;
            add_cast(cast_start, end_cast, end_cast, spell->get_spell_data()->get_name_hash());
            return true;
        case spell_hash("XayahQ"):
            add_cast(cast_start, end_cast, end_attack * 3, spell->get_spell_data()->get_name_hash());
            return true;
        case spell_hash("KaisaE"):
            set_can_move_until(end_cast + cast_start);
            return false;
        default:
            if (spell->is_auto_attack())
            {
                if (m_is_sett)
                {
                    m_last_left_attack = -1.f;
                    m_double_attack = 0;
                    add_cast(cast_start, end_cast, end_cast, spell->get_spell_data()->get_name_hash());
                }
                if (m_is_akshan)
                {
                    add_cast(cast_start, end_cast, myhero->get_attack_delay(),
                             spell->get_spell_data()->get_name_hash());
                    m_last_left_attack = -1.f;
                    m_double_attack = 0;
                    return true;
                }
            }
            return false;
    }
    return false;
}

float orb_walker::get_ping()
{
    return (ping->get_ping() / 2000.f);
};

void orb_walker::add_cast(float cast_start, float end_cast, float end_attack, uint32_t hash)
{
    // console->print(std::to_string(end_cast).c_str());
    end_cast += cast_start - get_ping();
    end_attack += cast_start - get_ping();
    m_wait_for_cast = 0;

    for (auto i = 0; i < spell_info.size(); i++)
    {
        auto spell = &spell_info[i];
        if (cast_start < spell->cast_end || cast_start > spell->attack_end)
        {
            spell_info.erase(spell_info.begin() + i);
            i--;
        }
    }
    spell_info.push_back(spell_time_info{end_cast, end_attack, hash});
}

void orb_walker::on_spell_cast(spell_instance_script& spell)
{
    float cast_start = gametime->get_time() - (ping->get_ping() / 2000.f) + (1.f / 30.f); // startTime
    float end_cast = spell->get_attack_cast_delay();
    float end_attack = spell->get_attack_delay();

    auto spell_data = spell->get_spell_data();

    if (special_spell_conditions(spell))
        return;

    if (!spell->is_auto_attack())
    {
        if (ignore_spell(spell_data))
            return;
        if (is_reset(spell_data))
        {
            reset_auto_attack_timer();
            return;
        }
        add_cast(cast_start, end_cast, end_cast, spell->get_spell_data()->get_name_hash());
        return;
    }

    add_cast(cast_start, end_cast, end_attack, spell->get_spell_data()->get_name_hash());
}

void orb_walker::on_stop_cast(spell_instance_script& spell)
{
    for (auto i = 0; i < spell_info.size(); i++)
    {
        auto& info = spell_info[i];
        if (info.name_hash == spell->get_spell_data()->get_name_hash())
        {
            spell_info.erase(spell_info.begin() + i);
            i--;
        }
    }
}

float orb_walker::get_next_attack_time()
{
    float attack_time = 0;
    for (auto& i : spell_info)
        attack_time = fmaxf(attack_time, i.attack_end);
    return attack_time;
}

float orb_walker::get_finish_cast_time()
{
    float cast_time = 0;
    for (auto& i : spell_info)
        cast_time = fmaxf(cast_time, i.cast_end);
    return cast_time;
}

void orb_walker::on_issue_order(game_object_script& target, vector& pos, _issue_order_type& _type, bool* process)
{
}

float orb_walker::get_auto_attack_range(game_object* from, game_object* to)
{
    if (from == nullptr)
        return 0.f;

    auto attack_range = from->get_bounding_radius();
    if (to)
        attack_range += to->get_bounding_radius();

    if (from->is_ai_turret())
        attack_range += 775.f;
    else if (from->is_ai_hero() && from->get_champion() == champion_id::Zeri)
        attack_range += 500.f;
    else
        attack_range += from->get_attack_range();

    if (to)
    {
        if (from->get_champion() == champion_id::Caitlyn && to->has_buff(buff_hash("caitlynyordletrapinternal")))
            attack_range = 1300.f;

        if (from->get_champion() == champion_id::Aphelios &&
            to->has_buff(buff_hash("aphelioscalibrumbonusrangedebuff")))
            attack_range = 1800.f;

        if (from->get_champion() == champion_id::Samira && !to->has_buff(buff_hash("samirapcooldown")) &&
            to->has_buff_type({buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Charm, buff_type::Flee,
                               buff_type::Taunt, buff_type::Asleep, buff_type::Suppression}))
        {
            attack_range += 300.f;
        }
    }
    return attack_range;
}

bool orb_walker::is_in_auto_attack_range(game_object* from, game_object* to, float additional = 0.f,
                                         bool spacing = false)
{
    if (to == nullptr || from == nullptr)
        return false;

    if (from->get_champion() == champion_id::Azir)
    {
        if (to->is_ai_hero() || to->is_ai_minion())
        {
            bool is_allowed_object = true;
            if (to->is_ai_minion())
            {
                if (!to->is_ward())
                {
                    auto hash = buff_hash_real(to->get_base_skin_name().c_str());

                    if (hash == buff_hash("TeemoMushroom") || hash == buff_hash("JhinTrap") ||
                        hash == buff_hash("NidaleeSpear"))
                        is_allowed_object = false;
                }
                else
                    is_allowed_object = false;
            }

            if (is_allowed_object)
            {
                for (auto& it : entitylist->get_other_minion_objects())
                {
                    if (it->is_dead())
                        continue;

                    if (it->get_object_owner() != from->get_base())
                        continue;

                    if (it->get_name() != "AzirSoldier")
                        continue;

                    if (it->get_position().distance(from->get_position()) > 730.f)
                        continue;

                    if (it->get_attack_range() == 0.f)
                        continue;

                    if (it->is_in_auto_attack_range_native(to, it->get_bounding_radius() * -2.f - 15.f))
                        return true;
                }
            }
        }
    }

    auto attack_range = get_auto_attack_range(from, to);

    vector to_position = to->get_position();
    vector from_position = from->get_position();

    if (to->is_ai_hero())
        to_position = to->get_path_controller()->get_position_on_path();

    if (from->is_ai_hero())
        from_position = from->get_path_controller()->get_position_on_path();

    if (!spacing && to->is_ai_hero())
    {
        if (to->get_path_controller()->is_moving())
        {
            to_position = to_position + to->get_pathing_direction() * (get_ping() + SERVER_TICK_INTERVAL);
        }
    }

    if (!spacing && from->is_ai_hero())
    {
        if (from->get_path_controller()->is_moving())
        {
            from_position = from_position + from->get_pathing_direction() * (get_ping() + SERVER_TICK_INTERVAL);
        }
    }

    if (spacing)
    {

        float dir = from->get_pathing_direction().angle_between(to_position - from_position);
        if (dir < 10.f)
        {
            attack_range += 15.f;
        }
        else
            attack_range -= 100.f;
        attack_range = fmaxf(attack_range, get_auto_attack_range(to, from) - 5.f);
    }
    attack_range += additional;
    return from_position.distance_squared(to_position) < attack_range * attack_range;
}

bool orb_walker::space_enemy_champs()
{
    if (!settings::bindings::auto_space->get_bool())
        return false;
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;

        auto id = i->get_network_id();
        auto num = m_blacklisted_champs.count(id);
        if (num && m_blacklisted_champs.at(id)->get_bool())
            continue;

        vector to_position = i->get_position();
        vector from_position = myhero->get_position();

        if (i->is_ai_hero())
            to_position = i->get_path_controller()->get_position_on_path();

        if (myhero->is_ai_hero())
            from_position = myhero->get_path_controller()->get_position_on_path();

        auto diff = (to_position - from_position).normalized();

        float space = 0.f;
        float time = gametime->get_prec_time() + get_ping();
        float attack_time = get_next_attack_time();
        float move_time = get_finish_cast_time();
        if (attack_time > time)
        {
            float new_time = ((attack_time - move_time)) / 2.f;
            if (new_time + move_time > time)
            {
                // console->print(std::to_string(myhero->get_move_speed()).c_str());
                space += fabsf(new_time) * myhero->get_move_speed();
            }
        }
        // if (attack_time > time && time > move_time)
        //{
        //     float new_time = (attack_time - move_time) / 2.f;
        //     float delta_time = (attack_time - time);
        //
        //     delta_time = new_time - fabsf(delta_time - new_time);
        //
        //     space += fabsf(delta_time) * myhero->get_move_speed();
        // }

        if (is_in_auto_attack_range(i.get(), myhero.get(), space, settings::spacing::space_local->get_bool()))
        {
            if (math::IsZero(diff.length()))
            {
                continue;
            }
            auto new_pos = myhero->get_position() + diff * -200.f;
            new_pos.z = navmesh->get_height_for_position(new_pos.x, new_pos.y);

            set_orbwalking_point(new_pos);

            return true;
        }
    }
    return false;
}

bool orb_walker::find_champ_target_special()
{
    std::vector<game_object_script> targets = {};
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            targets.push_back(i);
        }
    }
    if (auto target = target_selector->get_target(targets, damage_type::physical))
    {
        set_orbwalking_target(target);
        return true;
    }
    return false;
}

bool orb_walker::can_attack()
{
    auto active_spell = myhero->get_active_spell();
    if (active_spell && active_spell->is_channeling())
        return false;
    if (myhero->is_winding_up())
        return false;
    float game_time = gametime->get_prec_time() + get_ping();
    if (game_time < m_wait_for_cast)
        return false;
    float delta_time = game_time - get_next_attack_time();
    return delta_time > 0.f;
}

bool orb_walker::can_move(float extra_windup)
{
    float game_time = gametime->get_prec_time() + get_ping();
    if (myhero->is_winding_up() && !(game_time < m_can_move_until))
        return false;
    if (game_time < m_wait_for_cast)
        return false;

    return (game_time > get_finish_cast_time() + extra_windup) || (game_time < m_can_move_until);
}

bool orb_walker::should_wait() // idrk what this is for lol
{
    if (m_wait_for_cast)
        return true;
    if (myhero->is_winding_up())
        return true;

    float game_time = gametime->get_prec_time() + get_ping();
    return game_time > get_finish_cast_time();
}

std::uint32_t orb_walker::get_orb_state()
{
    return m_orb_state;
}

game_object_script orb_walker::get_target()
{
    return m_target;
}

game_object_script orb_walker::get_last_target()
{
    return m_last_target;
}

float orb_walker::get_last_aa_time()
{
    return m_last_attack_time;
}

float orb_walker::get_last_move_time()
{
    return m_last_move_time;
}

float orb_walker::get_my_projectile_speed()
{
    return myhero->get_auto_attack()->MissileSpeed();
}

float orb_walker::get_projectile_travel_time(const game_object_script& to, game_object_script from,
                                             spell_data_script spell)
{
    if (from == nullptr)
        from = myhero;
    vector to_position = to->get_position();
    vector from_position = from->get_position();

    if (to->is_ai_hero())
        to_position = to->get_path_controller()->get_position_on_path();

    if (from->is_ai_hero())
        from_position = from->get_path_controller()->get_position_on_path();

    if (spell == nullptr)
        spell = from->get_auto_attack();

    return ((to_position - from_position).length() - to->get_bounding_radius()) / spell->MissileSpeed();
}

void orb_walker::set_attack(bool enable)
{
    m_can_attack = enable;
}

void orb_walker::set_movement(bool enable)
{
    m_can_move = enable;
}

void orb_walker::set_orbwalking_target(game_object_script target)
{
    m_last_target = m_target;
    m_target = target;
}

void orb_walker::set_orbwalking_point(vector const& pos)
{
    m_move_pos = pos;
}

bool orb_walker::reset_auto_attack_timer()
{
    m_rand_time = 0.f;

    spell_info.clear();

    return false;
}

bool orb_walker::target_filter(game_object* target)
{
    return !target || !target->is_attack_allowed_on_target() || !target->is_visible() || target->get_health() <= 0;
}

bool orb_walker::find_jungle_target()
{
    bool ret = false;
    float greatest_max_health_found = 0;
    for (auto& i : entitylist->get_jugnle_mobs_minions())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            float max_health = i->get_max_health();
            if (greatest_max_health_found < max_health)
            {
                greatest_max_health_found = max_health;
                set_orbwalking_target(i);
                ret = true;
            }
        }
    }
    return ret;
}

bool orb_walker::find_turret_target()
{
    for (auto& i : entitylist->get_enemy_turrets())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            set_orbwalking_target(i);
            return true;
        }
    }
    return false;
}

bool orb_walker::find_inhibitor_target()
{
    for (auto& i : entitylist->get_enemy_inhibitors())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            set_orbwalking_target(i);
            return true;
        }
    }
    return false;
}

bool orb_walker::find_nexus_target()
{
    for (auto& i : entitylist->get_all_nexus())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            set_orbwalking_target(i);
            return true;
        }
    }
    return false;
}

bool orb_walker::find_ward_target()
{
    for (auto& i : entitylist->get_enemy_wards())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            set_orbwalking_target(i);
            return true;
        }
    }
    return false;
}

bool orb_walker::find_champ_target() // needs to be reworked to have certain priority options
{
    std::vector<game_object_script> targets = {};
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            targets.push_back(i);
        }
    }
    if (auto target = target_selector->get_target(targets, damage_type::physical))
    {
        set_orbwalking_target(target);
        return true;
    }
    return false;
}

bool orb_walker::find_other_targets()
{
    return find_jungle_target() || find_champ_target() ||
           find_ward_target(); // sort of abusing short cicuiting to keep code clean and not waste calls aswell
}

bool orb_walker::last_hit_mode()
{
    if (settings::bindings::last_hit->get_bool())
    {
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::last_hit;

        for (auto& i : entitylist->get_enemy_minions())
        {
            if (target_filter(i.get()))
                continue;

            if (is_in_auto_attack_range(myhero.get(), i.get()))
            {
                auto damage = myhero->get_auto_attack_damage(i, true);
                float death_time = -1.f;
                auto death_health = healthpred.get_last_health_before_death(i, death_time);

                float proj_travel_time = get_projectile_travel_time(i);
                auto predicted_health_when_attack = healthpred.get_health_prediction(
                    i, gametime->get_prec_time() + myhero->get_attack_cast_delay() + proj_travel_time + get_ping());

                if (predicted_health_when_attack <= damage && predicted_health_when_attack > 0.f)
                {
                    set_orbwalking_target(i);
                    float next_hit_time = gametime->get_prec_time() + myhero->get_attack_delay() +
                                          myhero->get_attack_cast_delay() + proj_travel_time + get_ping();
                    if (death_time > 0.f && next_hit_time > death_time) // will they be dead if we dont attack now
                        break;
                }
            }
        }
        m_enabled = true;
        return true;
    }
    return false;
}
bool orb_walker::mixed_mode()
{
    if (settings::bindings::mixed->get_bool())
    {
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        m_enabled = true;
        bool found_last_hit = false;
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::last_hit;
        if (find_turret_target() || find_inhibitor_target() || find_nexus_target())
            return true;
        if (!space_enemy_champs())
        {
            for (auto& i : entitylist->get_enemy_minions()) // lane clear logic
            {
                if (target_filter(i.get()))
                    continue;

                if (is_in_auto_attack_range(myhero.get(), i.get()))
                {
                    auto damage = myhero->get_auto_attack_damage(i, true);
                    float death_time = -1.f;
                    auto death_health = healthpred.get_last_health_before_death(i, death_time);

                    float proj_travel_time = get_projectile_travel_time(i);
                    auto predicted_health_when_attack = healthpred.get_health_prediction(
                        i, myhero->get_attack_cast_delay() + proj_travel_time + get_ping());
                    if (predicted_health_when_attack <= damage) // can we last hit them
                    {
                        set_orbwalking_target(i);
                        found_last_hit = true;
                        float next_hit_time = gametime->get_prec_time() + myhero->get_attack_delay() +
                                              myhero->get_attack_cast_delay() + proj_travel_time + get_ping();
                        if (death_time > 0.f && next_hit_time > death_time) // will they be dead if we dont attack now
                            break;
                    }
                }
            }
        }

        if (!found_last_hit && find_champ_target_special())
            m_orb_state = orbwalker_state_flags::combo;
        return true;
    }
    return false;
}

enum minion_type
{
    melee = 4,
    caster,
    cannon
};

bool orb_walker::lane_clear_mode2()
{

    if (settings::bindings::lane_clear->get_bool())
    {
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::lane_clear;

        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        m_enabled = true;

        game_object_script minion_with_turret_agro = {};
        std::vector<game_object_script> minions_under_tower = {};
        std::vector<game_object_script> minions_with_agro = {};
        std::vector<game_object_script> minions_no_agro = {};

        for (auto& i : entitylist->get_enemy_minions()) // lane clear logic
        {
            if (target_filter(i.get()))
                continue;

            if (!is_in_auto_attack_range(myhero.get(), i.get()))
                continue;
            if (health_prediction->has_turret_aggro(i))
            {
                minion_with_turret_agro = i;
                continue;
            }

            if (i->is_under_enemy_turret())
            {
                minions_under_tower.push_back(i);
                continue;
            }

            if (health_prediction->has_minion_aggro(i))
                minions_with_agro.push_back(i);
            else
                minions_no_agro.push_back(i);
        }

        bool found_to_last_hit = false;
        bool forced_to_last_hit = false;
        bool found_to_wait = false;
        bool forced_health_adjust = false;

        auto my_damage = damagelib->get_auto_attack_damage(myhero, minion_with_turret_agro, true);
        for (auto& i : minions_with_agro)
        {
            auto original_health = i->get_health();

            float proj_travel_time = get_projectile_travel_time(i);
            float pred_time = myhero->get_attack_cast_delay() + proj_travel_time;
            auto predicted_health_when_attack = health_prediction->get_health_prediction(i, pred_time);
            if (predicted_health_when_attack < my_damage) // can we last hit them
            {
                pred_time += myhero->get_attack_cast_delay() + myhero->get_attack_delay();
                auto predicted_health_when_attack = health_prediction->get_health_prediction(i, pred_time);
                found_to_last_hit = true;

                set_orbwalking_target(i);

                if (predicted_health_when_attack < 0.f) // will they be dead if we dont attack now
                {
                    forced_to_last_hit = true;
                    break;
                }
            }
            else
            {
                if (found_to_last_hit)
                    continue;
                if (found_to_wait) // already waiting on something else, in the future ill priorities certain
                                   // minions
                    continue;

                pred_time += myhero->get_attack_cast_delay() + myhero->get_attack_delay();
                auto predicted_health_when_next_attack = health_prediction->get_health_prediction(i, pred_time);

                if (fabsf(original_health - predicted_health_when_next_attack) > 0.5f)
                {
                    // predicted_health_when_next_attack -= damage;

                    if (predicted_health_when_next_attack < 0)
                    {
                        found_to_wait = true; // lets wait
                        set_orbwalking_target(nullptr);
                        continue;
                    }
                }
            }
            set_orbwalking_target(i);
        }

        bool forced_to_wait = false;
        if (minion_with_turret_agro)
        {
            auto turret = health_prediction->get_aggro_turret(minion_with_turret_agro);
            auto turret_damage = damagelib->get_auto_attack_damage(turret, minion_with_turret_agro, true);
            auto next_turret_hit_time = turret->get_attack_cast_delay() +
                                        health_prediction->turret_aggro_start_time(minion_with_turret_agro) +
                                        get_projectile_travel_time(minion_with_turret_agro, turret);

            float proj_travel_time = get_projectile_travel_time(minion_with_turret_agro);
            float pred_time = myhero->get_attack_cast_delay() + proj_travel_time;
            auto predicted_health_when_attack =
                health_prediction->get_health_prediction(minion_with_turret_agro, pred_time);
            if (predicted_health_when_attack < my_damage) // can we last hit them
            {
                found_to_last_hit = true;
                set_orbwalking_target(minion_with_turret_agro);
            }
            else
            {

                if (minion_with_turret_agro->get_real_health() < turret_damage)
                {
                    float next_hit_time = gametime->get_prec_time() + myhero->get_attack_cast_delay() +
                                          get_projectile_travel_time(minion_with_turret_agro);
                    float new_health = minion_with_turret_agro->get_real_health();
                    for (; next_hit_time < next_turret_hit_time; next_hit_time += myhero->get_attack_delay())
                    {
                        new_health -= my_damage;
                        if (new_health < 0)
                        {
                            found_to_last_hit = true;
                            set_orbwalking_target(minion_with_turret_agro);
                        }
                    }
                }
                else if (health_prediction->get_health_prediction(minion_with_turret_agro,
                                                                  pred_time + myhero->get_attack_delay()) < 0)
                {
                    forced_to_wait = true;
                    set_orbwalking_target(nullptr);
                }
            }

            if (!forced_to_wait && !found_to_last_hit && !minions_under_tower.empty())
            {
                float best = -1;
                game_object_script turret;
                for (auto& i : entitylist->get_ally_turrets())
                {
                    float dist = i->get_position().distance_squared(minions_under_tower.front()->get_position());
                    if (best > dist || best < 0)
                    {
                        best = dist;
                        turret = i;
                    }
                }
                struct
                {
                    game_object_script turret;
                    bool operator()(game_object_script a, game_object_script b) const
                    {
                        if (a->get_minion_type() == cannon && b->get_minion_type() != cannon)
                            return true;
                        if (b->get_minion_type() == cannon && a->get_minion_type() != cannon)
                            return false;
                        if (a->get_minion_type() == melee && b->get_minion_type() != melee)
                            return true;
                        if (b->get_minion_type() == melee && a->get_minion_type() != melee)
                            return false;
                        return a->get_position().distance_squared(turret->get_position()) <
                               b->get_position().distance_squared(turret->get_position());
                    }
                } customLess;
                customLess.turret = turret;
                std::sort(minions_under_tower.begin(), minions_under_tower.end(), customLess);

                game_object_script furthest_caster;
                for (auto& i : minions_under_tower)
                {
                    // float time = 0;
                    // float last_health = healthpred.get_last_health_before_death(i, time);
                    //
                    // if (last_health > my_damage)
                    //{
                    //     forced_health_adjust = true;
                    //     set_orbwalking_target(i);
                    //     break;
                    // }

                    auto turret_damage = damagelib->get_auto_attack_damage(turret, i, true);
                    auto turret_attack_health = i->get_real_health();
                    while (turret_attack_health > my_damage)
                        turret_attack_health -= turret_damage;

                    if (turret_attack_health <= 0)
                    {
                        forced_health_adjust = true;
                        set_orbwalking_target(i);
                        break;
                    }
                }
                if (!forced_health_adjust)
                {
                    if (minions_under_tower.size() > 2)
                    {
                        auto new_target = minions_under_tower.back();
                        float proj_travel_time = get_projectile_travel_time(new_target);
                        float pred_time = myhero->get_attack_cast_delay() + proj_travel_time;
                        auto predicted_health_when_attack =
                            health_prediction->get_health_prediction(new_target, pred_time);
                        if (predicted_health_when_attack < my_damage) // can we last hit them
                        {
                            found_to_last_hit = true;
                            set_orbwalking_target(new_target);
                        }
                        else if (health_prediction->get_health_prediction(new_target,
                                                                          pred_time + myhero->get_attack_delay()) < 0)
                        {
                            forced_to_wait = true;
                            set_orbwalking_target(nullptr);
                        }
                        else
                        {
                            set_orbwalking_target(new_target);
                        }
                    }
                }
            }
        }

        if (!forced_health_adjust && !found_to_last_hit && !forced_to_last_hit && !forced_to_wait)
        {
            find_turret_target() || find_inhibitor_target() || find_nexus_target();
        }
        if (!get_target() && !found_to_wait && !forced_to_wait)
        {
            if (!minions_no_agro.empty())
                set_orbwalking_target(minions_no_agro.front());
            else
                find_other_targets();
        }
        return true;
    }
    return false;
}

bool orb_walker::lane_clear_mode()
{
    m_last_hit = nullptr;
    m_wait = nullptr;
    m_adjust = nullptr;
    if (settings::bindings::lane_clear->get_bool())
    {
        bool found_to_wait = false;
        bool found_last_hit = false;
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::lane_clear;

        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        m_enabled = true;

        for (auto& i : entitylist->get_enemy_minions()) // lane clear logic
        {
            if (target_filter(i.get()))
                continue;

            if (is_in_auto_attack_range(myhero.get(), i.get()))
            {
                auto damage = myhero->get_auto_attack_damage(i, true);
                auto original_health = i->get_health();
                float death_time = -1.f;
                auto death_health = healthpred.get_last_health_before_death(i, death_time);

                float proj_travel_time = get_projectile_travel_time(i);
                auto predicted_health_when_attack = healthpred.get_health_prediction(
                    i, gametime->get_prec_time() + myhero->get_attack_cast_delay() + proj_travel_time + get_ping());
                if (predicted_health_when_attack < damage) // can we last hit them
                {
                    m_last_hit = i;
                    // console->print("last_hit");
                    set_orbwalking_target(i);
                    found_last_hit = true;
                    float next_hit_time = gametime->get_prec_time() + myhero->get_attack_delay() +
                                          myhero->get_attack_cast_delay() + proj_travel_time + get_ping();
                    if (death_time > 0.f && next_hit_time > death_time) // will they be dead if we dont attack now
                        break;
                }
                else
                {
                    if (found_last_hit)
                        continue;
                    if (found_to_wait) // already waiting on something else, in the future ill priorities certain
                                       // minions
                        continue;

                    float next_hit_time = gametime->get_prec_time() + myhero->get_attack_delay() * 2 +
                                          myhero->get_attack_cast_delay() + proj_travel_time + get_ping();

                    if (death_time > 0.f)
                    {
                        // predicted_health_when_next_attack -= damage;
                        if (death_health > damage)
                        {
                            m_adjust = i;
                            found_to_wait = true;
                            set_orbwalking_target(i);
                            // console->print("adjust health");
                        }
                        else if (death_time < next_hit_time + 0.05)
                        {
                            m_wait = i;
                            // console->print("wait");
                            found_to_wait = true; // lets wait
                            set_orbwalking_target(nullptr);
                            continue;
                        }
                    }
                }
                if (!get_target())
                    set_orbwalking_target(i);
            }
        }

        if (!found_last_hit)
        {
            find_turret_target() || find_inhibitor_target() || find_nexus_target();
        }

        if (!get_target() && !found_to_wait)
        {
            find_other_targets();
        }

        return true;
    }
    return false;
}

bool orb_walker::combo_mode()
{
    if (settings::bindings::combo->get_bool())
    {
        m_enabled = true;
        m_orb_state = orbwalker_state_flags::combo;
        set_orbwalking_target(nullptr);

        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);

        if (!space_enemy_champs())
            find_champ_target_special();
        return true;
    }
    return false;
}

bool orb_walker::flee_mode()
{
    if (settings::bindings::flee->get_bool())
    {
        m_enabled = true;
        m_orb_state = orbwalker_state_flags::flee;
        set_orbwalking_target(nullptr);

        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        return true;
    }
    return false;
}

bool orb_walker::none_mode()
{
    return false;
}

bool orb_walker::harass()
{
    return false;
}

void orb_walker::move_to(vector& pos)
{
    if (m_move_timer + m_rand_time > gametime->get_prec_time())
        return;

    int min_time = settings::humanizer::min_move_delay->get_int();
    if (settings::humanizer::max_move_delay->get_int() < min_time)
        settings::humanizer::max_move_delay->set_int(min_time);
    int delta_time = settings::humanizer::max_move_delay->get_int() - min_time;

    int new_time = (rand() % (delta_time + 1)) + min_time;

    m_rand_time = new_time / 1000.f;
    m_move_timer = gametime->get_prec_time();
    myhero->issue_order(pos, true, false);
}

void orb_walker::orbwalk(game_object_script target, vector& pos)
{
    if (!m_enabled)
        return;
    if (evade->is_evading())
    {
        m_wait_for_cast = 0;
        return;
    }
    if (myhero->get_active_spell())
    {
        if (myhero->get_active_spell()->is_channeling())
            console->print("channeling");
    }
    bool found = false;
    bool valid_target = target && target->is_valid();
    if (valid_target)
    {
        if (m_can_attack && can_attack())
        {
            bool should_attack = true;

            event_handler<events::on_before_attack_orbwalker>::invoke(target, &should_attack);

            if (should_attack)
            {
                console->print("attack");
                found = true;
                m_move_timer = 0.f;
                myhero->issue_order(target, true, false);
                if (m_double_attack == 0 && (!m_is_akshan || settings::champ::akshan_aa->get_bool()))
                    m_double_attack = 1;
                event_handler<events::on_after_attack_orbwalker>::invoke(target);
                m_wait_for_cast = gametime->get_prec_time() + 0.3;
                // next_attack_time = end_attack;

                m_has_moved_since_last = false;
                m_rand_time = 0.f;
                m_last_attack_time = gametime->get_prec_time();
            }
        }
    }

    if (m_last_left_attack + 2.f < gametime->get_prec_time())
        m_double_attack = 0;

    if (m_can_move && (m_double_attack != 1 || !valid_target) && can_move(SERVER_TICK_INTERVAL) && !found)
    {
        move_to(pos);
        m_has_moved_since_last = true;
        m_last_move_time = gametime->get_prec_time();
    }
}
