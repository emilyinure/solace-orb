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
    } // namespace drawings

    namespace spacing
    {
        inline TreeEntry* space_local;
    }

    namespace bindings
    {
        inline TreeEntry* lane_clear;
        inline TreeEntry* last_hit;
        inline TreeEntry* combo;
        inline TreeEntry* mixed;
        inline TreeEntry* auto_space;
        inline TreeEntry* flee;
    } // namespace bindings

    namespace champ
    {
        inline TreeEntry* akshan_aa;
    } // namespace bindings

    namespace humanizer
    {
        inline TreeEntry* min_move_delay;
        inline TreeEntry* max_move_delay;
    } // namespace humanizer

} // namespace settings