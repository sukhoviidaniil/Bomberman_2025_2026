/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_BOTBRAIN_H
#define BOMBERMAN_LOGIC_BOTBRAIN_H

#include <memory>
#include <string>
#include <vector>

#include "bomberman/logic/ai/BotBehaviour.h"

namespace bomberman::logic::ai {

/**
 * @brief How a bot weighs its goals against each other.
 *
 * The assignment lists "give each bot their own personality" as a
 * bonus, and with a priority chain a personality is exactly that: the
 * same behaviours in a different order. No new code, no new branch in
 * an existing behaviour.
 */
enum class BotPersonality {
    /// Survive, grab what is nearby, open the map, then fight.
    Balanced,
    /// Survive, then hunt - opens the map only when there is no target.
    Aggressive,
    /// Survive, then hoard power-ups; fights last.
    Collector
};

[[nodiscard]] BotPersonality personality_from_string(const std::string& value);
[[nodiscard]] std::string to_string(BotPersonality personality);

/**
 * @brief One bot's decision-making: an ordered list of behaviours.
 *
 * Asks each in turn and takes the first answer. That ordering *is* the
 * AI - it is the difference between a bot that runs from a bomb and one
 * that collects a power-up while standing in the blast - so it is data
 * rather than control flow, and can be swapped per bot or read out in a
 * test.
 *
 * Owns its behaviours through unique_ptr because they are polymorphic
 * and belong to exactly one brain; they hold no state of their own, so
 * nothing here needs to survive a round.
 */
class BotBrain {
public:
    explicit BotBrain(BotPersonality personality = BotPersonality::Balanced);

    BotBrain(const BotBrain&) = delete;
    BotBrain& operator=(const BotBrain&) = delete;
    BotBrain(BotBrain&&) = default;
    BotBrain& operator=(BotBrain&&) = default;

    /**
     * @brief The first action any behaviour asks for.
     *
     * @return An action with `reason` naming the behaviour that
     * produced it, or an empty action (Direction::None, no bomb) when
     * every behaviour declined - which only happens for a bot that is
     * boxed in with nowhere safe to go.
     */
    [[nodiscard]] BotAction decide(const BotContext& ctx) const;

    [[nodiscard]] BotPersonality personality() const;

    /// @brief Behaviour names in priority order; used by tests and docs.
    [[nodiscard]] std::vector<std::string> priorities() const;

private:
    BotPersonality personality_;
    std::vector<std::unique_ptr<BotBehaviour>> behaviours_;
};
} // namespace bomberman::logic::ai

#endif // BOMBERMAN_LOGIC_BOTBRAIN_H
