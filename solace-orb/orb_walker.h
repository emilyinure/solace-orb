#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"
class orb_walker
{
    float m_last_order_time = 0.f;
    bool m_has_moved_since_last = true;
    float m_last_attack_time = 0.f;
    float m_last_move_time = 0.f;
    float m_move_timer = 0.f;
    float m_rand_time = 0.f;
    bool m_has_moved = false;
    bool m_can_move = true;
    float m_move_pause = 0.f;
    bool m_can_attack = true;
    uint32_t m_orb_state;
    game_object_script m_last_target = {};
    game_object_script m_target = {};

public:
    bool enabled = false;
    vector m_move_pos;
    uintptr_t id;

    float get_auto_attack_range(game_object* from, game_object* to = nullptr);
    bool is_in_auto_attack_range(game_object* from, game_object* to, float additional, bool spacing);

    bool last_hit_mode();
    bool mixed_mode();
    bool lane_clear_mode();
    bool find_jungle_target();
    bool find_turret_target();
    bool find_inhibitor_target();
    bool find_nexus_target();
    bool find_ward_target();
    bool find_champ_target();
    bool find_other_targets();
    float get_ping();
    bool combo_mode();
    bool flee_mode();
    bool none_mode();
    bool harass();
    bool reset_auto_attack_timer();
    game_object_script get_target();
    game_object_script get_last_target();
    float get_last_aa_time();
    float get_last_move_time();
    float get_my_projectile_speed();
    bool can_attack();
    bool can_move(float extra_windup);
    bool should_wait();
    void move_to(vector& pos);
    void orbwalk(game_object_script target, vector& pos);
    void set_attack(bool enable);
    void set_movement(bool enable);
    void set_orbwalking_target(game_object_script target);
    void set_orbwalking_point(vector const& pos);
    std::uint32_t get_orb_state();
};
