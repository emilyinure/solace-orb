#include "orb_walker.h"
#include "health_pred.h"

float health_pred::get_death_time(game_object_script target)
{
    float health = target->get_real_health();

    return 0.0f;
}
obj_info *get_lowest(std::vector<obj_info>& attacks)
{
    obj_info *attack;
    float best_time = FLT_MAX;
    for (auto i = 0; i < attacks.size(); i++)
    {
        auto info = &attacks[i];
        auto attacker = entitylist->get_object(info->m_local);
        if (info->m_last_time_attacked + attacker->get_attack_delay() < best_time)
        {
            best_time = info->m_last_time_attacked + attacker->get_attack_delay();
            attack = info;
        }
    }
    return attack;
}

float health_pred::get_last_health_before_death(game_object_script target, float& death_time)
{
    std::vector<obj_info> attackers_info = {};
    auto target_id = target->get_id();
    for (auto& i : m_object_target)
    {
        auto attacker = entitylist->get_object(i.first);
        if (i.second.m_target == target_id)
        {
            attackers_info.push_back(i.second);
        }
    }

    std::vector<active_attack> attacks;
    for (auto& i : m_active_attacks)
        if (i.m_target == target_id && i.m_arrive_time < gametime->get_prec_time())
            attacks.push_back(i);

    float temp_health = target->get_real_health(true);
    if (!attackers_info.empty())
    {
        while (temp_health > 0.f)
        {
            auto info = get_lowest(attackers_info);
            auto attacker = entitylist->get_object(info->m_local);
            float next_hit_time =
                info->m_last_time_attacked + attacker->get_attack_delay() + attacker->get_attack_cast_delay();
            if (attacker->is_lane_minion() && attacker->get_minion_type() != 4)
                next_hit_time += orb_walker::get_projectile_travel_time(target, attacker);
            attacks.push_back(active_attack(info->m_target, info->m_local, next_hit_time));

            float damage = attacker->get_auto_attack_damage(target, true);
            temp_health -= damage;
            info->m_last_time_attacked += attacker->get_attack_delay();
        }
    }

    struct
    {
        bool operator()(active_attack& a, active_attack& b) const
        {
            auto a_attacker = entitylist->get_object(a.m_sender);
            auto b_attacker = entitylist->get_object(b.m_sender);
            return a.m_arrive_time <
                   b.m_arrive_time;
        }
    } custom_less;

    std::sort(attacks.begin(), attacks.end(), custom_less);
    float health = target->get_real_health(true);
    for (auto& i : attacks)
    {
        if (i.m_target == target_id)
        {
            auto attacker = entitylist->get_object(i.m_sender);
            if (attacker)
            {
                float damage = attacker->get_auto_attack_damage(target, true);
                if (health <= damage)
                {
                    death_time = i.m_arrive_time;
                    return health;
                }
                health -= damage;
            }
        }
    }

    return -1.f;
}

void health_pred::on_update()
{
    for (auto i = m_active_attacks.begin(); i < m_active_attacks.end(); i++)
        if (i->m_arrive_time < gametime->get_prec_time())
            i = m_active_attacks.erase(i) - 1;
}

void health_pred::reset(uint32_t id)
{
    m_object_target[id].m_target = -1;
}
void health_pred::on_preupdate()
{
    for (auto& i : m_object_target)
    {
        auto attacker = entitylist->get_object(i.first);
        auto reciever = entitylist->get_object(i.second.m_target);
        if (!attacker || !attacker->is_valid() || !attacker->is_visible() || attacker->is_dead() || !reciever ||
            !reciever->is_valid() || reciever->is_dead() || !reciever->is_attack_allowed_on_target() ||
            !reciever->is_visible() || !orb_walker::is_in_auto_attack_range(attacker.get(), reciever.get(), 0, false))
        {

            m_object_target[i.first].m_target = -1;
            continue;
        }
        //auto spell = attacker->get_active_spell();
        //if (!spell || spell->get_last_target_id() != i.second.m_target)
        //    m_object_target[i.first].m_target = -1;
    }
    for (auto i = m_active_attacks.begin(); i < m_active_attacks.end(); i++)
        if (i->m_arrive_time < gametime->get_prec_time())
            i = m_active_attacks.erase(i) - 1;
}

void health_pred::on_missle_created(game_object_script sender, game_object_script target,
                                    spell_instance_script missle_data)
{
    if (sender && target)
    {
        float real_time = gametime->get_time() - (ping->get_ping() / 2000.f) + (1.f / 30.f);
        float arrive_time = real_time + target->get_attack_cast_delay();

        if (!sender->is_minion() || sender->get_minion_type() != 4)
            arrive_time += orb_walker::get_projectile_travel_time(target, sender, missle_data->get_spell_data());
        m_active_attacks.push_back(active_attack{sender->get_id(), target->get_id(), arrive_time});
        auto target_info = &m_object_target[sender->get_id()];
        target_info->m_last_time_attacked = real_time;
        target_info->m_target = target->get_id();
        target_info->m_local = sender->get_id();
    }
}

float health_pred::get_health_prediction(game_object_script target, float at_time)
{
    std::vector<obj_info> attackers_info = {};
    auto target_id = target->get_id();
    for (auto& i : m_object_target)
    {
        auto attacker = entitylist->get_object(i.first);
        if (i.second.m_target == target_id)
        {
            attackers_info.push_back(i.second);
        }
    }

    std::vector<active_attack> attacks;
    for (auto& i : m_active_attacks)
        if (i.m_target == target_id)
            attacks.push_back(i);
    float temp_health = target->get_real_health(true);
    if (!attackers_info.empty())
    {
        while (temp_health > 0.f)
        {
            auto info = get_lowest(attackers_info);
            auto attacker = entitylist->get_object(info->m_local);
            float next_hit_time =
                info->m_last_time_attacked + attacker->get_attack_delay() + attacker->get_attack_cast_delay();
            if (attacker->is_lane_minion() && attacker->get_minion_type() != 4)
                next_hit_time += orb_walker::get_projectile_travel_time(target, attacker);
            attacks.push_back(active_attack(info->m_target, info->m_local, next_hit_time));

            float damage = attacker->get_auto_attack_damage(target, true);
            temp_health -= damage;
            info->m_last_time_attacked += attacker->get_attack_delay();
        }
    }

    struct
    {
        bool operator()(active_attack& a, active_attack& b) const
        {
            auto a_attacker = entitylist->get_object(a.m_sender);
            auto b_attacker = entitylist->get_object(b.m_sender);
            return a.m_arrive_time < b.m_arrive_time;
        }
    } custom_less;

    std::sort(attacks.begin(), attacks.end(), custom_less);
    float health = target->get_real_health(true);
    for (auto& i : attacks)
    {
        if (i.m_target == target_id)
        {
            auto attacker = entitylist->get_object(i.m_sender);
            if (attacker)
            {
                if (i.m_arrive_time > at_time)
                {
                    //console->print("found health_pred");
                    return health;
                }
                float damage = attacker->get_auto_attack_damage(target, true);
                health -= damage;
            }
        }
    }
    return health;
}