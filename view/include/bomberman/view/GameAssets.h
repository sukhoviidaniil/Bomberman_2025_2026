/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_GAMEASSETS_H
#define BOMBERMAN_VIEW_GAMEASSETS_H

#include <string>
#include <unordered_map>

#include "bomberman/logic/Direction.h"
#include "bomberman/logic/events/GameEvents.h"
#include "bomberman/logic/grid/Tile.h"

#include "sif/asset/AssetHandle.h"
#include "sif/asset/internal/Font.h"
#include "sif/asset/internal/PrimitiveAnimation.h"
#include "sif/asset/internal/Sound.h"

namespace bomberman::view {

    /**
     * @brief Resolves asset names to handles, once.
     *
     * Every lookup goes name -> GUID (through sif's importer) -> request ->
     * handle, and the handle is cached. Doing that per frame would be a
     * hash lookup and a registry call for every entity on screen; doing it
     * here means a view is constructed with the handles it needs and then
     * only ever dereferences them.
     *
     * Requesting an asset also *starts* its background load, so building
     * this object early is what gets the textures in flight before the
     * first frame. Views draw nothing while a handle is not ready yet,
     * which is why a slow disk shows an empty arena rather than a crash.
     */
    class GameAssets {
    public:
        /**
         * @brief Requests every asset the game needs.
         *
         * @throws std::runtime_error naming the asset if one is missing
         * from the registry - a typo in a descriptor is far easier to fix
         * when it is reported at start-up than when a sprite silently
         * fails to appear.
         */
        GameAssets();

        [[nodiscard]] sif::asset::AssetHandle<sif::asset::Font> font(const std::string& name) const;
        [[nodiscard]] sif::asset::AssetHandle<sif::asset::Sound> sound(const std::string& name) const;
        [[nodiscard]] sif::asset::AssetHandle<sif::asset::PrimitiveAnimation> animation(const std::string& name) const;

        /// @brief Type-erased handle to a SpriteSingle, ready for a render item.
        [[nodiscard]] sif::asset::AssetHandle<void> sprite(const std::string& name) const;

        // ===== Convenience mappings used by the views =====

        [[nodiscard]] sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>
        player_walk(logic::Direction direction) const;

        [[nodiscard]] sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>
        player_idle(logic::Direction direction) const;

        [[nodiscard]] sif::asset::AssetHandle<void> tile(logic::Tile tile) const;
        [[nodiscard]] sif::asset::AssetHandle<void> item(logic::PowerUpKind kind) const;

    private:
        /// @brief GUID for an asset name, or throws naming it.
        [[nodiscard]] static sif::intrnl::GUID guid_of(const std::string& name);

        void request(const std::string& name);

        std::unordered_map<std::string, sif::intrnl::GUID> guids_;
    };
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_GAMEASSETS_H
