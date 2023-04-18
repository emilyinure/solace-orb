#include "orb_walker.h"


bool is_in_auto_attack_range_native(game_object* from, game_object* to, float additional = 0.f, bool reverse = false)
{
    if (to == nullptr)
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

    auto attack_range = from->get_bounding_radius() + to->get_bounding_radius() + additional;

    if (from->is_ai_turret())
        attack_range += 775.f;
    else if (from->is_ai_hero() && from->get_champion() == champion_id::Zeri)
        attack_range += 500.f;
    else
        attack_range += from->get_attack_range();

    vector to_position = to->get_position();
    vector from_position = from->get_position();

    if (to->is_ai_hero())
        to_position = to->get_path_controller()->get_position_on_path();

    if (from->is_ai_hero())
        from_position = from->get_path_controller()->get_position_on_path();

    if (from->is_ai_hero() && to->is_ai_hero())
    {

        if (from->get_path_controller()->is_moving())
        {
            float dir;
            if (!reverse)
            {
                float dir = to->get_pathing_direction().angle_between(from_position - to_position);
                if (dir > 89)
                    attack_range -= 12;
            }
        }
    }
    if (reverse)
    {

        float dir = from->get_pathing_direction().angle_between(to_position - from_position);
        if (dir < 10.f)
            attack_range += 40.f;
        else
            attack_range -= 40.f;
    }

    if (from->get_champion() == champion_id::Caitlyn && to->has_buff(buff_hash("caitlynyordletrapinternal")))
        attack_range = 1300.f;

    if (from->get_champion() == champion_id::Aphelios && to->has_buff(buff_hash("aphelioscalibrumbonusrangedebuff")))
        attack_range = 1800.f;

    if (from->get_champion() == champion_id::Samira && !to->has_buff(buff_hash("samirapcooldown")) &&
        to->has_buff_type({buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Charm, buff_type::Flee,
                           buff_type::Taunt, buff_type::Asleep, buff_type::Suppression}))
    {
        attack_range += 300.f;
    }

    return from_position.distance_squared(to_position) < attack_range * attack_range;
}


#define get_ping() (ping->get_ping() / 2000.f)
bool orb_walker::last_hit_mode()
{
    bool found = false;
    bool found_to_wait = false;
    bool found_last_hit = false;
    bool last_hit_or_die = false;
    if (keyboard_state->is_pressed(keyboard_game::x))
    {
        m_orb_state = orbwalker_state_flags::last_hit;

        // console->print(__FUNCTION__);
        game_object_script target{};
        // if (can_attack())
        // {
        for (auto& i : entitylist->get_all_minions())
        {
            if (i->is_ally() || !i->is_valid() || i->is_dead())
                continue;
            if (!i->is_attack_allowed_on_target())
                continue;
            if (!i->is_visible())
                continue;
            if (i->get_health() <= 0)
                continue;

            if (is_in_auto_attack_range_native(myhero.get(), i.get()))
            {
                auto damage = damagelib->get_auto_attack_damage(myhero, i, true);

                auto predicted_health_when_attack =
                    health_prediction->get_health_prediction(i, myhero->get_attack_cast_delay());
                if (predicted_health_when_attack <= damage)
                {
                    target = i;
                    found_last_hit = true;
                    found = true;
                    auto predicted_health_when_attack = health_prediction->get_health_prediction(
                        i, myhero->get_attack_delay() * 2 + myhero->get_attack_cast_delay() * 2);
                    if (predicted_health_when_attack <= 0.f)
                    {
                        last_hit_or_die = true;
                        break;
                    }
                }
            }
        }
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_target(target);
        set_orbwalking_point(pos);
        enabled = true;
        return true;
        // console->print(__FUNCTION__);
    }
    return false;
}

bool orb_walker::mixed_mode()
{
    // console->print(__FUNCTION__);
    return false;
}

bool orb_walker::lane_clear_mode()
{
    bool found = false;
    bool found_to_wait = false;
    bool found_last_hit = false;
    bool last_hit_or_die = false;
    if (keyboard_state->is_pressed(keyboard_game::v))
    {
        console->print(__FUNCTION__);
        m_orb_state = orbwalker_state_flags::combo;

        // console->print(__FUNCTION__);
        game_object_script target{};
        // if (can_attack())
        // {
        for (auto& i : entitylist->get_enemy_minions())
        {
            if (!i->is_valid() || i->is_dead())
                continue;
            if (!i->is_attack_allowed_on_target())
                continue;
            if (!i->is_visible())
                continue;
            if (i->get_health() <= 0)
                continue;

            if (is_in_auto_attack_range_native(myhero.get(), i.get()))
            {
                auto damage = damagelib->get_auto_attack_damage(myhero, i, true);

                auto predicted_health_when_attack =
                    health_prediction->get_health_prediction(i, myhero->get_attack_cast_delay());
                if (predicted_health_when_attack <= damage)
                {
                    target = i;
                    found_last_hit = true;
                    found = true;
                    auto predicted_health_when_attack = health_prediction->get_health_prediction(
                        i, myhero->get_attack_delay() * 2 + myhero->get_attack_cast_delay() * 2);
                    if (predicted_health_when_attack <= 0.f)
                    {
                        last_hit_or_die = true;
                        break;
                    }
                }
                else
                {
                    auto predicted_health_when_next_attack = health_prediction->get_health_prediction(
                        i, myhero->get_attack_delay() * 2 + myhero->get_attack_cast_delay() * 2);

                    if (found_last_hit)
                        continue;
                    if (found_to_wait)
                        continue;
                    if (predicted_health_when_next_attack <= damage)
                    {
                        found_to_wait = true;
                        target = nullptr;
                        continue;
                    }
                }
                target = i;
                found = true;
            }
        }
        if (!target && !found_to_wait)
        {
            float greatest_max_health_found = -1.f;
            if (!found)
                for (auto& i : entitylist->get_jugnle_mobs_minions())
                {
                    if (!i->is_valid() || i->is_dead())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;
                    if (i->get_health() <= 0)
                        continue;
                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        float max_health = i->get_max_health();
                        if (greatest_max_health_found < max_health)
                        {
                            greatest_max_health_found = max_health;
                            target = i;
                            found = true;
                        }
                    }
                }
            if (!found)
                for (auto& i : entitylist->get_enemy_turrets())
                {
                    if (!i->is_valid() || i->is_dead())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;
                    if (i->get_health() <= 0)
                        continue;
                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        target = i;
                        found = true;
                    }
                }
            if (!found)
                for (auto& i : entitylist->get_enemy_inhibitors())
                {
                    if (!i->is_valid() || i->is_dead())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;
                    if (i->get_health() <= 0)
                        continue;
                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        target = i;
                        found = true;
                    }
                }
            if (!found)
                for (auto& i : entitylist->get_all_nexus())
                {
                    if (!i->is_valid() || i->is_ally())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;
                    if (i->get_health() <= 0)
                        continue;
                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        target = i;
                        found = true;
                    }
                }
            float lowest_health_after_attack = -1.f;
            if (!found)
                for (auto& i : entitylist->get_enemy_heroes())
                {
                    if (!i->is_valid() || i->is_dead())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;

                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        auto damage = damagelib->get_auto_attack_damage(myhero, i, true);
                        float after = i->get_real_health() - damage;
                        if (after < lowest_health_after_attack || lowest_health_after_attack < 0.5f)
                        {
                            lowest_health_after_attack = fmaxf(after, 0.f);
                            target = i;
                        }
                    }
                }
            if (!found)
                for (auto& i : entitylist->get_enemy_wards())
                {
                    if (!i->is_valid() || i->is_dead())
                        continue;
                    if (!i->is_attack_allowed_on_target())
                        continue;
                    if (!i->is_visible())
                        continue;
                    if (i->get_health() <= 0)
                        continue;
                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                    {
                        target = i;
                        found = true;
                    }
                }
        }
        //}
        // if (found_last_hit)
        //{
        //    if (last_hit_or_die)
        //        console->print("lasthit or die");
        //    else
        //        console->print("lasthit");
        //}
        // else if (found_to_wait)
        //    console->print("wait");
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_target(target);
        set_orbwalking_point(pos);
        enabled = true;
        return true;
    }
    return found;
}

bool orb_walker::combo_mode()
{
    if (keyboard_state->is_pressed(keyboard_game::space))
    {
        float lowest_health_after_attack = -1.f;

        game_object_script target{};
        for (auto& i : entitylist->get_enemy_heroes())
        {
            if (!i->is_valid() || i->is_dead())
                continue;
            if (!i->is_attack_allowed_on_target())
                continue;
            if (!i->is_visible())
                continue;
            if (keyboard_state->is_pressed(keyboard_game::mouse5))
            {
                if (is_in_auto_attack_range_native(i.get(), myhero.get(), 10.f, true))
                {
                    auto diff = ((myhero->get_position() - i->get_position()).normalized());
                    if (math::IsZero(diff.length()))
                    {
                        continue;
                    }
                    auto new_pos = myhero->get_position() + diff * 200.f;
                    new_pos.z = navmesh->get_height_for_position(new_pos.x, new_pos.y);

                    set_orbwalking_point(new_pos);

                    if (is_in_auto_attack_range_native(myhero.get(), i.get()))
                        set_orbwalking_target(i);
                    else
                        set_orbwalking_target(nullptr);
                    enabled = true;
                    return true;
                }
            }
            if (is_in_auto_attack_range_native(myhero.get(), i.get()))
            {
                auto damage = damagelib->get_auto_attack_damage(myhero, i, true);
                float after = i->get_real_health() - damage;
                if (after < lowest_health_after_attack || lowest_health_after_attack < 0.5f)
                {
                    lowest_health_after_attack = fmaxf(0.f, after);
                    target = i;
                }
            }
        }
        m_orb_state = orbwalker_state_flags::combo;
        auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
        set_orbwalking_target(target);
        set_orbwalking_point(pos);
        enabled = true;
        return true;
    }
    // console->print(__FUNCTION__);
    return false;
}

bool orb_walker::flee_mode()
{
    // console->print(__FUNCTION__);
    return false;
}

bool orb_walker::none_mode()
{
    // console->print(__FUNCTION__);
    return false;
}

bool orb_walker::harass()
{
    // console->print(__FUNCTION__);
    return false;
}

bool orb_walker::reset_auto_attack_timer()
{
    m_move_timer = 0.f;
    m_last_attack_time = 0.f;
    return false;
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
    console->print(__FUNCTION__);
    return false;
}

bool orb_walker::can_attack()
{
    if (!m_can_attack)
        return false;
    float game_time = gametime->get_time() + get_ping();
    if (game_time <= m_last_attack_time)
        return false;
    return game_time > m_last_attack_time + myhero->get_attack_delay();
}

bool orb_walker::can_move(float extra_windup)
{
    if (!m_can_move)
        return false;
    if (myhero->is_winding_up())
        return false;

    float game_time = gametime->get_time() + get_ping();
    if (game_time <= m_last_attack_time)
        return false;
    return game_time > m_last_attack_time + myhero->get_attack_cast_delay();
}

bool orb_walker::should_wait()
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

void orb_walker::move_to(vector& pos)
{
    // console->print("move");
    if (m_move_timer + m_rand_time > gametime->get_time())
        return;
    m_rand_time = 1.f / ((rand() % 5) + 5.f);
    m_move_timer = gametime->get_time();
    myhero->issue_order(pos, true, false);
    // console->print(__FUNCTION__);
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
        if (can_attack())
        {
            bool should_attack = true;

            //if (!has_recur)
                event_handler<events::on_before_attack_orbwalker>::invoke(target, &should_attack);
            if (should_attack)
            {
                found = true;
                m_move_timer = 0.f;
                myhero->issue_order(target, true, false);

                event_handler<events::on_after_attack_orbwalker>::invoke(target);

                m_has_moved_since_last = false;
                m_last_attack_time = gametime->get_time() + get_ping();
                m_last_order_time = gametime->get_time() + get_ping();
                m_move_pause = gametime->get_time() + get_ping();
            }
        }
    }

    if (can_move(0) && !found)
    {
        move_to(pos);
        m_has_moved_since_last = true;
        m_last_move_time = gametime->get_time() + get_ping();
        m_last_order_time = gametime->get_time() + get_ping();
    }
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

std::uint32_t orb_walker::get_orb_state()
{
    return m_orb_state;
}
