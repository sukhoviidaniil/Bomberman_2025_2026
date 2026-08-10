/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_AUDIODIRECTOR_H
#define BOMBERMAN_VIEW_AUDIODIRECTOR_H

#include <memory>

#include "bomberman/logic/Config.h"
#include "bomberman/view/GameAssets.h"

#include "sif/audio/AudioPlayer.h"
#include "sif/event/Event_Bus.h"
#include "sif/event/Observer.h"

namespace bomberman::view {

    /**
     * @brief Turns gameplay events into sound.
     *
     * The audio counterpart of a view, and an Observer for the same
     * reason: the World announces that a bomb went off, and something in
     * the representation layer decides that this is worth a bang. No rule
     * in the logic library mentions a sound file, so the game stays
     * playable - and testable - with no audio device at all.
     *
     * One class rather than a play() call sprinkled through the states
     * because "what does this game sound like" is then answerable by
     * reading forty lines in one file.
     */
    class AudioDirector final : public sif::event::Observer {
    public:
        /**
         * @param world_bus Gameplay events for one round.
         * @param audio Output; borrowed, must outlive this object.
         * @param assets Where the sound handles come from.
         * @param config Volumes, and whether sound is on at all.
         */
        AudioDirector(const std::shared_ptr<sif::event::Event_Bus>& world_bus,
                      sif::audio::AudioPlayer& audio,
                      const GameAssets& assets,
                      const logic::AudioConfig& config);

        /// @brief Plays a one-shot sound by asset name, honouring the volume settings.
        void play(const std::string& asset_name, float volume_scale = 1.f) const;

    private:
        sif::audio::AudioPlayer& audio_;
        const GameAssets& assets_;
        logic::AudioConfig config_;
    };
}

#endif //BOMBERMAN_VIEW_AUDIODIRECTOR_H
