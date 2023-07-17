#pragma once
#include "plugin_sdk.hpp"
#include <unordered_map>
struct active_attack {
    uint32_t m_target;
    uint32_t m_sender;
    float m_arrive_time = 0.f;
    spell_data_script spell;
    active_attack()
    {
    }
    active_attack(uint32_t target, uint32_t sender, float arrive_time)
        : m_target(target), m_sender(sender), m_arrive_time(arrive_time) {};
};
struct obj_info
{
    uint32_t m_local = -1;
    float m_last_time_attacked = -1.f;
    uint32_t m_target = -1;
};
class health_pred
{
    float turret_attack_delay = 0.07f;
    std::vector<active_attack> m_active_attacks;
    std::unordered_map<uint32_t, obj_info> m_object_target;

public:
    float get_death_time(game_object_script target);
    float get_last_health_before_death(game_object_script target, float &death_time);
    void on_update();
    void reset(uint32_t id);
    void on_preupdate();
    void on_missle_created(game_object_script sender, game_object_script target, spell_instance_script missle_data);
    float get_health_prediction(game_object_script target, float at_time);
} inline healthpred;
