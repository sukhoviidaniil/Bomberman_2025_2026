/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/ArenaView.h"

#include <memory>

#include "bomberman/view/AssetNames.h"

#include "sif/render/elements/Sprite.h"

namespace bomberman::view {
    namespace {
        void push_tile(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                       const sif::asset::AssetHandle<void>& sprite,
                       const sif::intrnl::Rect& world_rect, const float height_scale) {
            if (!sprite.ready()) {
                return;
            }

            // Walls and blocks are taller than the floor tile they stand
            // on; growing them upwards keeps their base aligned with the
            // grid while the extra height reads as depth.
            sif::intrnl::Rect rect = world_rect;
            const float extra = world_rect.height * (height_scale - 1.f);
            rect.y -= extra;
            rect.height += extra;

            auto item = std::make_unique<sif::rnd::Sprite>();
            item->rect = camera.world_to_screen(rect);
            item->asset = sprite;
            item->kind = sif::asset::AssetType::SpriteSingle;

            frame.constant_items.push_back(std::move(item));
        }
    }

    void ArenaView::append_render_items(const logic::TileGrid &grid, const GameAssets &assets,
                                        sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) {
        const sif::asset::AssetHandle<void> floor = assets.sprite(assets::tile_floor);

        // Two passes, because a single one would let a wall in row 3 be
        // painted over by the floor of row 4.
        for (int row = 0; row < static_cast<int>(grid.rows()); ++row) {
            for (int col = 0; col < static_cast<int>(grid.columns()); ++col) {
                push_tile(frame, camera, floor, grid.get_rect({row, col}), 1.f);
            }
        }

        for (int row = 0; row < static_cast<int>(grid.rows()); ++row) {
            for (int col = 0; col < static_cast<int>(grid.columns()); ++col) {
                const logic::TilePos cell{row, col};
                const logic::Tile tile = grid.get_tile(cell);
                if (tile == logic::Tile::Free) {
                    continue;
                }
                push_tile(frame, camera, assets.tile(tile), grid.get_rect(cell), 1.18f);
            }
        }
    }
}
