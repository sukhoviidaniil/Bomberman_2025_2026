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
     * TODO(daniil): the tile layout only changes when a block is
     * destroyed, so this belongs in RenderFrame::constant_items behind a
     * dirty flag rather than being rebuilt every frame. sif's
     * FrameContext already carries a `redrawing` flag for exactly this;
     * nothing reads it yet.
     */
    class ArenaView {
    public:
        static void append_render_items(const logic::TileGrid& grid,
                                        sif::rnd::RenderFrame& frame,
                                        const sif::rnd::Camera& camera);
    };
}

#endif //BOMBERMAN_VIEW_ARENAVIEW_H
