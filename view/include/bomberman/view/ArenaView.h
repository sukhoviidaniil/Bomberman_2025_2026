/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_ARENAVIEW_H
#define BOMBERMAN_VIEW_ARENAVIEW_H

#include "bomberman/logic/grid/TileGrid.h"
#include "bomberman/view/GameAssets.h"

#include "sif/render/Camera.h"
#include "sif/render/RenderFrame.h"

namespace bomberman::view {

/**
 * @brief Draws the static part of the arena: the tiles themselves.
 *
 * Separate from EntityView because tiles are not entities - they have
 * no behaviour, no events and no lifetime of their own; they are a
 * property of the grid.
 *
 * Floor is drawn under everything, then walls and blocks on top, so a
 * block sprite that overhangs its tile (they are 51x53 on a 50x45
 * floor) covers the neighbour instead of being covered by it.
 *
 * TODO(daniil): the tile layout only changes when a block is
 *  destroyed, so this belongs in RenderFrame::constant_items behind a
 *  dirty flag rather than being rebuilt every frame.
 *  sif::rnd::FrameContext already carries a `redrawing` flag that
 *  nothing reads yet.
 */
class ArenaView {
public:
    static void append_render_items(const logic::TileGrid& grid, const GameAssets& assets, sif::rnd::RenderFrame& frame,
                                    const sif::rnd::Camera& camera);
};
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_ARENAVIEW_H
