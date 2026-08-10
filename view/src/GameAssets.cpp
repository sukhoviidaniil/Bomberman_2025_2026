/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/GameAssets.h"

#include <stdexcept>

#include "bomberman/view/AssetNames.h"

#include "sif/asset/AssetRegistry.h"
#include "sif/asset/internal/AssetImporter.h"

namespace bomberman::view {
    using namespace assets;

    GameAssets::GameAssets() {
        // Listed explicitly rather than "everything in the registry":
        // a missing asset then fails here, at start-up, with a name -
        // instead of at the moment the first bomb is drawn.
        for (const std::string& name : {
                 ui_font,
                 player_walk_up, player_walk_down, player_walk_left, player_walk_right,
                 player_idle_up, player_idle_down, player_idle_left, player_idle_right,
                 player_die, bomb, explosion,
                 tile_floor, tile_wall, tile_block,
                 item_fire, item_bomb, item_skates,
                 sfx_explosion, sfx_bomb_place, sfx_pickup, sfx_death,
                 sfx_menu, sfx_victory, sfx_defeat}) {
            request(name);
        }
    }

    sif::intrnl::GUID GameAssets::guid_of(const std::string &name) {
        try {
            return sif::asset::AssetImporter::instance().get(name).meta.guid;
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "asset '" + name + "' is not in the registry (" + e.what() +
                "). Run tools/build_assets.py and regenerate assets/bin/registry.rgst.json.");
        }
    }

    void GameAssets::request(const std::string &name) {
        const sif::intrnl::GUID guid = guid_of(name);
        sif::asset::AssetRegistry::instance().request(guid);
        guids_.emplace(name, guid);
    }

    sif::asset::AssetHandle<sif::asset::Font> GameAssets::font(const std::string &name) const {
        return sif::asset::AssetRegistry::instance().get<sif::asset::Font>(guids_.at(name));
    }

    sif::asset::AssetHandle<sif::asset::Sound> GameAssets::sound(const std::string &name) const {
        return sif::asset::AssetRegistry::instance().get<sif::asset::Sound>(guids_.at(name));
    }

    sif::asset::AssetHandle<sif::asset::PrimitiveAnimation> GameAssets::animation(const std::string &name) const {
        return sif::asset::AssetRegistry::instance().get<sif::asset::PrimitiveAnimation>(guids_.at(name));
    }

    sif::asset::AssetHandle<void> GameAssets::sprite(const std::string &name) const {
        return sif::asset::AssetRegistry::instance().get<void>(guids_.at(name));
    }

    sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>
    GameAssets::player_walk(const logic::Direction direction) const {
        switch (direction) {
            case logic::Direction::Up:    return animation(player_walk_up);
            case logic::Direction::Left:  return animation(player_walk_left);
            case logic::Direction::Right: return animation(player_walk_right);
            default:                      return animation(player_walk_down);
        }
    }

    sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>
    GameAssets::player_idle(const logic::Direction direction) const {
        switch (direction) {
            case logic::Direction::Up:    return animation(player_idle_up);
            case logic::Direction::Left:  return animation(player_idle_left);
            case logic::Direction::Right: return animation(player_idle_right);
            default:                      return animation(player_idle_down);
        }
    }

    sif::asset::AssetHandle<void> GameAssets::tile(const logic::Tile t) const {
        switch (t) {
            case logic::Tile::Indestructible: return sprite(tile_wall);
            case logic::Tile::Destructible:   return sprite(tile_block);
            default:                          return sprite(tile_floor);
        }
    }

    sif::asset::AssetHandle<void> GameAssets::item(const logic::PowerUpKind kind) const {
        switch (kind) {
            case logic::PowerUpKind::ExtraBomb: return sprite(item_bomb);
            case logic::PowerUpKind::Skates:    return sprite(item_skates);
            default:                            return sprite(item_fire);
        }
    }
}
