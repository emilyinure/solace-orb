#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"
namespace settings
{
    inline TreeTab* main_menu;
    namespace drawings
    {
        inline TreeEntry* enable;
        inline TreeEntry* color;
        inline TreeEntry* glow_inner_size;
        inline TreeEntry* glow_inner_strength;
        inline TreeEntry* glow_outer_size;
        inline TreeEntry* glow_outer_strength;
    }

    namespace bindings
    {
        inline TreeEntry* lane_clear;
        inline TreeEntry* last_hit;
        inline TreeEntry* combo;
        inline TreeEntry* mixed;
        inline TreeEntry* auto_space;
    } // namespace bindings

    namespace humanizer
    {
        inline TreeEntry* min_move_delay;
        inline TreeEntry* max_move_delay;
    }

} // namespace settings