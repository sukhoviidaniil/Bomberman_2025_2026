/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_ASSETNAMES_H
#define BOMBERMAN_VIEW_ASSETNAMES_H

#include <string>

/**
 * @file
 *
 * The asset names the representation layer asks for, in one place.
 *
 * They are the `asset_name` fields of the *.asset.json descriptors that
 * tools/build_assets.py writes, so renaming an asset is a two-file change
 * (the script and this header) instead of a hunt through string literals.
 * GUIDs deliberately do not appear anywhere in the C++: the registry maps
 * name to GUID, which is what lets the art be reorganised without touching
 * code.
 */
namespace bomberman::view::assets {

    inline const std::string ui_font = "UI";

    // Characters: one animation per direction, plus a death animation.
    inline const std::string player_walk_up = "player_walk_up";
    inline const std::string player_walk_down = "player_walk_down";
    inline const std::string player_walk_left = "player_walk_left";
    inline const std::string player_walk_right = "player_walk_right";

    inline const std::string player_idle_up = "player_idle_up";
    inline const std::string player_idle_down = "player_idle_down";
    inline const std::string player_idle_left = "player_idle_left";
    inline const std::string player_idle_right = "player_idle_right";

    inline const std::string player_die = "player_die";

    inline const std::string bomb = "bomb";
    inline const std::string explosion = "explosion";

    inline const std::string tile_floor = "tile_floor";
    inline const std::string tile_wall = "tile_wall";
    inline const std::string tile_block = "tile_block";

    inline const std::string item_fire = "item_fire";
    inline const std::string item_bomb = "item_bomb";
    inline const std::string item_skates = "item_skates";

    inline const std::string sfx_explosion = "sfx_explosion";
    inline const std::string sfx_bomb_place = "sfx_bomb_place";
    inline const std::string sfx_pickup = "sfx_pickup";
    inline const std::string sfx_death = "sfx_death";
    inline const std::string sfx_menu = "sfx_menu";
    inline const std::string sfx_victory = "sfx_victory";
    inline const std::string sfx_defeat = "sfx_defeat";
} // namespace bomberman::view::assets

#endif // BOMBERMAN_VIEW_ASSETNAMES_H
