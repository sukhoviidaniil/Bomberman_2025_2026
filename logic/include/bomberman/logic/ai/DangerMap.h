/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_DANGERMAP_H
#define BOMBERMAN_LOGIC_DANGERMAP_H

#include <limits>
#include <memory>
#include <vector>

#include "bomberman/logic/entity/Bomb.h"
#include "bomberman/logic/entity/Explosion.h"
#include "bomberman/logic/grid/TileGrid.h"

namespace bomberman::logic::ai {

    /**
     * @brief Where the fire will be, and in how long.
     *
     * Every bot behaviour is a question about this: "am I standing
     * somewhere that is about to explode", "is that power-up worth walking
     * to", "can I get out after placing this bomb". Computing it once per
     * decision and sharing it beats each behaviour re-deriving blast rays
     * from the bomb list, and it keeps the blast rules in one place - they
     * must match World::spread_blast exactly or the bots will confidently
     * walk into fire.
     *
     * A cell holds the time until the *earliest* blast that reaches it, so
     * "safe" is simply "no blast is coming" and "how urgent" falls out of
     * the same number.
     */
    class DangerMap {
    public:
        /// @brief Returned for a cell no blast reaches.
        static constexpr float never = std::numeric_limits<float>::infinity();

        DangerMap() = default;

        /**
         * @brief Recomputes the map from the live bombs and the fire.
         *
         * Both halves matter. Bombs are the future; explosions are the
         * present, and leaving them out is a mistake that looks like
         * working code: the tick after a bomb is removed, its cells read as
         * safe while the fire is still burning there for another half
         * second, and every bot walks straight into it. That is exactly how
         * the first version of this class killed all three bots on the
         * frame their own first bomb went off.
         *
         * @param grid The arena; walls stop rays, destructible blocks
         * absorb them (the tile itself still burns).
         * @param bombs Every bomb currently ticking.
         * @param explosions Every tile currently on fire.
         */
        void rebuild(const TileGrid& grid,
                     const std::vector<std::shared_ptr<Bomb>>& bombs,
                     const std::vector<std::shared_ptr<Explosion>>& explosions);

        /**
         * @brief Adds one hypothetical bomb without touching the real map.
         *
         * This is what lets a bot ask "if I drop a bomb here, can I still
         * get out?" before committing to it - the single most important
         * question a Bomberman AI asks, and the one whose absence makes a
         * bot look suicidal rather than merely bad.
         */
        [[nodiscard]] DangerMap with_bomb(const TileGrid& grid, const TilePos& cell,
                                          unsigned int radius, float fuse_seconds) const;

        /// @brief Seconds until this cell is on fire; `never` if it is not.
        [[nodiscard]] float seconds_until_blast(const TilePos& cell) const;

        /// @brief True if no blast reaches this cell at all.
        [[nodiscard]] bool safe(const TilePos& cell) const;

        /**
         * @brief True if the cell survives at least `seconds` from now.
         *
         * Used when a path is worth walking even though its far end will
         * eventually burn: what matters is whether it burns before the bot
         * gets there.
         */
        [[nodiscard]] bool safe_for(const TilePos& cell, float seconds) const;

    private:
        void add_bomb(const TileGrid& grid, const TilePos& cell,
                      unsigned int radius, float fuse_seconds);

        void mark(const TilePos& cell, float seconds);

        [[nodiscard]] std::size_t index(const TilePos& cell) const;
        [[nodiscard]] bool contains(const TilePos& cell) const;

        int rows_ = 0;
        int columns_ = 0;
        std::vector<float> seconds_; ///< row-major, `never` where safe
    };
}

#endif //BOMBERMAN_LOGIC_DANGERMAP_H
