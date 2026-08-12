/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-26
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_STATEMANAGER_H
#define BOMBERMAN_VIEW_STATEMANAGER_H

#include <memory>
#include <vector>

#include "bomberman/view/state/State.h"

namespace bomberman::view {

    class Game;

    /**
     * @brief The state stack.
     *
     * Push/pop rather than replace, so pausing is "push PausedState" and
     * resuming is "pop it" - the level underneath is untouched and
     * continues exactly where it stopped. During a pause the stack really
     * does hold Menu, Level and Paused, which is what the original
     * assignment described and what the Pac-Man implementation did not
     * do: there, entering a level popped the menu first, so the stack
     * never had more than two entries and the pattern's whole point was
     * lost.
     *
     * Transitions are deferred to the end of the frame. A state that
     * pushes a successor from inside its own update() would otherwise be
     * destroyed while its stack frame is still running - a use-after-free
     * that only shows up under a debug allocator.
     */
    class StateManager {
    public:
        explicit StateManager(Game& game);

        StateManager(const StateManager&) = delete;
        StateManager& operator=(const StateManager&) = delete;

        /// @brief The game this stack belongs to (window size, audio, assets).
        [[nodiscard]] Game& game() const;

        void push(std::unique_ptr<State> state);

        /// @brief Pops the top state; ignored when the stack is empty.
        void pop();

        /// @brief Pops everything down to (and including) the given depth.
        void pop_to_depth(std::size_t depth);

        /// @brief Empties the stack, which ends the game loop.
        void quit();

        /**
         * @brief Runs one frame: update, then apply pending transitions.
         */
        void update(float dt);

        void on_key(sif::event::input::Key key);

        void on_text(char32_t character);

        /**
         * @brief Applies queued transitions immediately.
         *
         * Needed exactly once, right after the initial state is pushed:
         * transitions are deferred to the end of a frame, so before the
         * first frame the stack would still be empty and running() would
         * report false - the loop would exit before drawing anything.
         */
        void flush();

        void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const;

        /// @brief False once the stack is empty; the Game loop stops.
        [[nodiscard]] bool running() const;

        [[nodiscard]] std::size_t depth() const;

    private:
        void apply_pending();

        enum class Action { Push, Pop, PopToDepth, Quit };

        struct Pending {
            Action action = Action::Push;
            std::unique_ptr<State> state;
            std::size_t depth = 0;
        };

        Game& game_;
        std::vector<std::unique_ptr<State>> stack_;
        std::vector<Pending> pending_;
    };
}

#endif //BOMBERMAN_VIEW_STATEMANAGER_H
