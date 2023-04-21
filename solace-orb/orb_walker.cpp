#include "orb_walker.h"
#include "settings.h"

float orb_walker::get_ping()
{
    return ping->get_ping() / 2000.f;
};

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
            to_position = to_position + to->get_pathing_direction() * get_ping() * 1.1f;
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

    return from_position.distance_squared(to_position) < attack_range * attack_range;
}

int IntersectRaySphere(vector p, vector d, vector s, float r, vector& q)
{
    float t = 0.f;
    vector m = p - s;
    float b = m.dot_product(d);
    float c = m.dot_product(m) - r * r;

    // Exit if r’s origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0f && b > 0.0f)
        return 0;
    float discr = b * b - c;

    // A negative discriminant corresponds to ray missing sphere
    if (discr < 0.0f)
        return 0;

    // Ray now found to intersect sphere, compute smallest t value of intersection
    t = -b - sqrt(discr);

    // If t is negative, ray started inside sphere so clamp t to zero
    if (t < 0.0f)
        t = 0.0f;
    q = p + d * t;

    return 1;
}
bool orb_walker::space_enemy_champs()
{
    if (!settings::auto_space->get_bool())
        return false;
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;
        vector to_position = i->get_position();
        vector from_position = myhero->get_position();

        if (i->is_ai_hero())
            to_position = i->get_path_controller()->get_position_on_path();

        if (myhero->is_ai_hero())
            from_position = myhero->get_path_controller()->get_position_on_path();

        auto diff = (to_position - from_position).normalized();

        float space = i->get_move_speed() * get_ping();
        if (!can_attack())
        {
            float delta_time =
                (m_last_attack_time + myhero->get_attack_delay() / 2.f) - (gametime->get_time() + get_ping());

            space += fabsf(delta_time) * myhero->get_move_speed();
        }

        if (is_in_auto_attack_range(i.get(), myhero.get(), space, true))
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
        else
        {
            if (i->get_path_controller()->is_moving())
            {
                to_position = to_position + i->get_pathing_direction() * get_ping() * 1.1f;
            }
            auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
            auto dist = get_auto_attack_range(myhero.get(), i.get());
            auto wanted_dif = (to_position - pos);
            auto wanted_dif_len = wanted_dif.length();
            if (wanted_dif_len <= dist)
            {
                float dist_out_of_aa_range = (diff.length() - dist);
                vector new_pos;
                float new_space = 0.f;
                if (!can_attack())
                {
                    float delta_time =
                        (m_last_attack_time + myhero->get_attack_delay() / 2.f) - (gametime->get_time() + get_ping());

                    new_space += fabsf(delta_time) * myhero->get_move_speed();
                }
                if (IntersectRaySphere(from_position, (pos - from_position).normalized(), to_position,
                                       dist + new_space - 3.f, new_pos))
                {
                    new_pos.z = navmesh->get_height_for_position(new_pos.x, new_pos.y);
                    set_orbwalking_point(new_pos);
                }
            }
        }
    }
    return false;
}

bool orb_walker::find_champ_target_special()
{
    float lowest_health_after_attack = -1.f;
    bool ret = false;
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;
        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            auto damage = damagelib->get_auto_attack_damage(myhero, i, true);
            float after = i->get_real_health() - damage;
            if (after < lowest_health_after_attack || lowest_health_after_attack < 0.5f)
            {
                lowest_health_after_attack = fmaxf(0.f, after);
                set_orbwalking_target(i);
                ret = true;
            }
        }
    }
    return ret;
}

bool orb_walker::can_attack()
{
    float game_time = gametime->get_time() + get_ping();
    if (game_time <= m_last_attack_time)
        return false;
    return game_time > m_last_attack_time + myhero->get_attack_delay();
}

bool orb_walker::can_move(float extra_windup)
{
    if (myhero->is_winding_up())
        return false;

    float game_time = gametime->get_time() + get_ping();
    if (game_time <= m_last_attack_time)
        return false;
    return game_time > m_last_attack_time + myhero->get_attack_cast_delay();
}

bool orb_walker::should_wait() // idrk what this is for lol
{
    return false;
    if (myhero->is_winding_up())
        return true;
    return m_move_pause > gametime->get_time();
    // console->print(__FUNCTION__);
    auto active = myhero->get_active_spell();

    return !!active;
    if (!active)
        return true;
    return false;
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
    console->print(__FUNCTION__);
    return m_last_attack_time;
    return false;
}

float orb_walker::get_last_move_time()
{
    console->print(__FUNCTION__);
    return m_last_move_time;
    return false;
}

float orb_walker::get_my_projectile_speed()
{
    return myhero->get_auto_attack()->MissileSpeed() ;
    console->print(__FUNCTION__);
    return false;
}

float orb_walker::get_projectile_travel_time(const game_object_script &to)
{
    vector to_position = to->get_position();
    vector from_position = myhero->get_position();

    if (to->is_ai_hero())
        to_position = to->get_path_controller()->get_position_on_path();

    if (myhero->is_ai_hero())
        from_position = myhero->get_path_controller()->get_position_on_path();

    return (to_position - from_position).length() / get_my_projectile_speed();
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
    m_last_attack_time = gametime->get_time() + get_ping();
    attack_delay_on_attack = myhero->get_attack_delay();
    attack_cast_delay_on_attack = myhero->get_attack_cast_delay();
    m_last_order_time = gametime->get_time() + get_ping();
    m_move_pause = gametime->get_time() + get_ping();
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
    bool ret = false;
    float lowest_health_after_attack = -1.f;
    for (auto& i : entitylist->get_enemy_heroes())
    {
        if (target_filter(i.get()))
            continue;

        if (is_in_auto_attack_range(myhero.get(), i.get()))
        {
            auto damage = damagelib->get_auto_attack_damage(myhero, i, true);
            float after = i->get_real_health() - damage;
            if (after < lowest_health_after_attack || lowest_health_after_attack < 0.5f)
            {
                lowest_health_after_attack = fmaxf(after, 0.f);
                set_orbwalking_target(i);
                ret = true;
            }
        }
    }
    return ret;
}

bool orb_walker::find_other_targets()
{
    return find_jungle_target() || find_turret_target() || find_inhibitor_target() || find_nexus_target() ||
           find_champ_target() ||
           find_ward_target(); // sort of abusing short cicuiting to keep code clean and not waste calls aswell
}



bool orb_walker::last_hit_mode()
{
    if (settings::last_hit->get_bool())
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
                auto damage = damagelib->get_auto_attack_damage(myhero, i, true);
                float proj_travel_time = get_projectile_travel_time(i);
                auto predicted_health_when_attack = health_prediction->get_health_prediction(
                    i, get_ping() + myhero->get_attack_cast_delay() + proj_travel_time);
                
                if (predicted_health_when_attack <= damage && predicted_health_when_attack > 0.f)
                {
                    set_orbwalking_target(i);
                    auto predicted_health_after_attack = health_prediction->get_health_prediction(
                        i, get_ping() + myhero->get_attack_delay() + myhero->get_attack_cast_delay() * 2 +
                               proj_travel_time);
                    if (predicted_health_after_attack <= 0.f)
                        break;
                }
            }
        }
        enabled = true;
        return true;
        // console->print(__FUNCTION__);
    }
    return false;
}
bool orb_walker::mixed_mode()
{
    if (settings::mixed->get_bool())
    {
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        enabled = true;
        bool found_to_wait = false;
        bool found_last_hit = false;
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::lane_clear;

        if (!space_enemy_champs())
        {
            for (auto& i : entitylist->get_enemy_minions()) // lane clear logic
            {
                if (target_filter(i.get()))
                    continue;

                if (is_in_auto_attack_range(myhero.get(), i.get()))
                {
                    auto damage = damagelib->get_auto_attack_damage(myhero, i, true);

                    float proj_travel_time = get_projectile_travel_time(i);
                    auto predicted_health_when_attack = health_prediction->get_health_prediction(
                        i, get_ping() + myhero->get_attack_cast_delay() + proj_travel_time);
                    if (predicted_health_when_attack <= damage) // can we last hit them
                    {
                        set_orbwalking_target(i);
                        found_last_hit = true;
                        auto predicted_health_when_attack = health_prediction->get_health_prediction(
                            i, get_ping() + myhero->get_attack_delay() + myhero->get_attack_cast_delay() * 2 +
                                   proj_travel_time);
                        if (predicted_health_when_attack <= 0.f) // will they be dead if we dont attack now
                            break;
                    }
                    else
                    {
                        auto predicted_health_when_next_attack = health_prediction->get_lane_clear_health_prediction(
                            i, get_ping() + myhero->get_attack_delay() + myhero->get_attack_cast_delay() * 2 +
                                   proj_travel_time);

                        if (found_last_hit)
                            continue;
                        if (found_to_wait) // already waiting on something else, in the future ill priorities certain
                                           // minions
                            continue;
                        if (predicted_health_when_next_attack <=
                            0) // they will die before we can a second time, lets wait
                        {
                            found_to_wait = true; // lets wait
                            set_orbwalking_target(nullptr);
                            continue;
                        }
                    }
                    set_orbwalking_target(i);
                }
            }
        }


        if (!found_to_wait && !found_last_hit && find_champ_target_special())
            m_orb_state = orbwalker_state_flags::combo;
        return true;
    }
    return false;
}

bool orb_walker::lane_clear_mode()
{
    if (settings::lane_clear->get_bool())
    {
        bool found_to_wait = false;
        bool found_last_hit = false;
        set_orbwalking_target(nullptr);
        m_orb_state = orbwalker_state_flags::lane_clear;

        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        enabled = true;

        for (auto& i : entitylist->get_enemy_minions()) // lane clear logic
        {
            if (target_filter(i.get()))
                continue;

            if (is_in_auto_attack_range(myhero.get(), i.get()))
            {
                auto damage = damagelib->get_auto_attack_damage(myhero, i, true);

                float proj_travel_time = get_projectile_travel_time(i);
                auto predicted_health_when_attack = health_prediction->get_health_prediction(
                    i, get_ping() + myhero->get_attack_cast_delay() + proj_travel_time);
                if (predicted_health_when_attack <= damage) // can we last hit them
                {
                    set_orbwalking_target(i);
                    found_last_hit = true;
                    auto predicted_health_when_attack = health_prediction->get_health_prediction(
                        i, get_ping() + myhero->get_attack_delay() * 2 + myhero->get_attack_cast_delay() * 2 +
                               proj_travel_time);
                    if (predicted_health_when_attack <= 0.f) // will they be dead if we dont attack now
                        break;
                }
                else
                {
                    auto predicted_health_when_next_attack = health_prediction->get_health_prediction(
                        i, get_ping() + myhero->get_attack_delay() * 2 + myhero->get_attack_cast_delay() * 2 +
                               proj_travel_time);

                    if (found_last_hit)
                        continue;
                    if (found_to_wait) // already waiting on something else, in the future ill priorities certain
                                       // minions
                        continue;
                    if (predicted_health_when_next_attack <= 0) // they will die before we can a second time, lets wait
                    {
                        found_to_wait = true; // lets wait
                        set_orbwalking_target(nullptr);
                        continue;
                    }
                }
                set_orbwalking_target(i);
            }
        }

        if (!get_target() && !found_to_wait)
        {
            find_other_targets();
        }

        return true;
    }
    return false;
}

bool orb_walker::combo_mode() // needs rework to account for priority
{
    console->print("combo");
    if (settings::combo->get_bool())
    {
        set_orbwalking_target(nullptr);
        enabled = true;
        m_orb_state = orbwalker_state_flags::combo;
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_point(pos);
        space_enemy_champs();
        find_champ_target_special();
        return true;
    }
    return false;
}

bool orb_walker::flee_mode()
{
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
    if (m_move_timer + m_rand_time > gametime->get_time())
        return;
    m_rand_time = 1.f / ((rand() % 3) + 9.f);
    m_move_timer = gametime->get_time();
    myhero->issue_order(pos, true, false);
}

void orb_walker::orbwalk(game_object_script target, vector& pos)
{
    if (!enabled)
        return;
    if (evade->is_evading())
    {
        reset_auto_attack_timer();
        return;
    }

    bool found = false;
    if (target && target->is_valid())
    {
        if (m_can_attack && can_attack())
        {
            bool should_attack = true;

            event_handler<events::on_before_attack_orbwalker>::invoke(target, &should_attack);
            
            if (should_attack)
            {
                found = true;
                m_move_timer = 0.f;
                myhero->issue_order(target, true, false);

                event_handler<events::on_after_attack_orbwalker>::invoke(target);

                m_has_moved_since_last = false;
                
                m_last_attack_time = gametime->get_time() + get_ping() + (1.f/SERVER_TICKRATE);
                attack_delay_on_attack = myhero->get_attack_delay();
                attack_cast_delay_on_attack = myhero->get_attack_cast_delay();
                m_last_order_time = gametime->get_time() + get_ping();
                m_move_pause = gametime->get_time() + get_ping();
            }
        }
    }

    if (m_can_move && can_move(0) && !found)
    {
        move_to(pos);
        m_has_moved_since_last = true;
        m_last_move_time = gametime->get_time() + get_ping();
        m_last_order_time = gametime->get_time() + get_ping();
    }
}
