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

#include "sif/render/elements/Rectangle.h"

namespace bomberman::view {
    namespace {
        const sif::intrnl::Color floor_color{38, 42, 52};
        const sif::intrnl::Color indestructible_color{96, 102, 118};
        const sif::intrnl::Color destructible_color{140, 106, 72};
    }

    void ArenaView::append_render_items(const logic::TileGrid &grid,
                                        sif::rnd::RenderFrame &frame,
                                        const sif::rnd::Camera &camera) {
        for (int row = 0; row < static_cast<int>(grid.rows()); ++row) {
            for (int col = 0; col < static_cast<int>(grid.columns()); ++col) {
                const logic::TilePos cell{row, col};

                auto item = std::make_unique<sif::rnd::Rectangle>();
                item->rect = camera.world_to_screen(grid.get_rect(cell));

                switch (grid.get_tile(cell)) {
                    case logic::Tile::Indestructible: item->color = indestructible_color; break;
                    case logic::Tile::Destructible:   item->color = destructible_color; break;
                    default:                          item->color = floor_color; break;
                }

                frame.constant_items.push_back(std::move(item));
            }
        }
    }
}
