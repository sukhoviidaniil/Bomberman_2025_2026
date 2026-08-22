/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-19
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_STATE_H
#define BOMBERMAN_VIEW_STATE_H

#include "sif/asset/AssetHandle.h"
#include "sif/asset/internal/Font.h"
#include "sif/event/events/input_Keyboard.h"
#include "sif/render/Camera.h"
#include "sif/render/RenderFrame.h"

namespace bomberman::view {

    class StateManager;

    /**
     * @brief Everything a state needs in order to draw.
     *
     * Passed in rather than reached for: a state has no pointer to the
     * Game, so it cannot accidentally grow a dependency on the window,
     * the audio device or the asset registry just because it wanted a
     * font. Adding a shared drawing resource later means adding a field
     * here, and the compiler lists every state that has to care.
     */
    struct DrawContext {
        const sif::rnd::Camera& camera;
        sif::asset::AssetHandle<sif::asset::Font> font;
    };

    /**
     * @brief One screen of the game: menu, level, pause, game over.
     *
     * A state decides for itself when to hand over ("It should be the
     * responsibilities of the individual states to decide when to switch
     * to what new state, not the StateManager, which should theoretically
     * not be aware which concrete state is currently running"), so the
     * manager below only ever sees State*.
     *
     * Note that the retake's list of required patterns no longer includes
     * State. It is kept because the game genuinely has four screens and a
     * pause that must resume where it left off - a stack models that in a
     * few dozen lines - but it is now a design choice to defend, not a box
     * to tick.
     */
    class State {
    public:
        virtual ~State() = default;

        State(const State&) = delete;
        State& operator=(const State&) = delete;

        /// @brief Called once when the state becomes the top of the stack.
        virtual void on_enter(StateManager& manager);

        /// @brief Called when another state is pushed on top of this one.
        virtual void on_pause();

        /// @brief Called when the state on top of this one is popped.
        virtual void on_resume();

        /**
         * @brief Advances the state by one frame.
         *
         * Only the top state is updated: a paused level must genuinely
         * stop, not keep simulating behind the pause screen.
         */
        virtual void update(StateManager& manager, float dt) = 0;

        virtual void on_key(StateManager& manager, sif::event::input::Key key) = 0;

        /**
         * @brief A character the user typed.
         *
         * Separate from on_key because a key is a position on the keyboard
         * and a character is what the layout produced from it; only the
         * screens that accept text care, so the default does nothing.
         */
        virtual void on_text(StateManager& manager, char32_t character);

        virtual void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const = 0;

        /**
         * @brief True if the state below should also be drawn.
         *
         * The pause screen is an overlay: the level stays visible behind
         * it, which is only possible because the stack kept it.
         */
        [[nodiscard]] virtual bool draws_below() const;

    protected:
        State() = default;
    };
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_STATE_H
