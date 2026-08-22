/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/ai/BotBrain.h"

#include <stdexcept>

#include "bomberman/logic/ai/Behaviours.h"

namespace bomberman::logic::ai {

    BotPersonality personality_from_string(const std::string& value) {
        if (value == "balanced")
            return BotPersonality::Balanced;
        if (value == "aggressive")
            return BotPersonality::Aggressive;
        if (value == "collector")
            return BotPersonality::Collector;
        throw std::runtime_error("unknown bot personality '" + value +
                                 "' (expected balanced, aggressive or collector)");
    }

    std::string to_string(const BotPersonality personality) {
        switch (personality) {
        case BotPersonality::Aggressive:
            return "aggressive";
        case BotPersonality::Collector:
            return "collector";
        default:
            return "balanced";
        }
    }

    BotBrain::BotBrain(const BotPersonality personality) : personality_(personality) {
        // Survival is first in every personality. A bot that dies to its
        // own bomb is not a personality, it is a bug.
        behaviours_.push_back(std::make_unique<SurviveBehaviour>());

        switch (personality_) {
        case BotPersonality::Aggressive:
            behaviours_.push_back(std::make_unique<HuntBehaviour>(8));
            behaviours_.push_back(std::make_unique<CollectPowerUpBehaviour>(4));
            behaviours_.push_back(std::make_unique<BreakBlocksBehaviour>());
            break;

        case BotPersonality::Collector:
            behaviours_.push_back(std::make_unique<CollectPowerUpBehaviour>(12));
            behaviours_.push_back(std::make_unique<BreakBlocksBehaviour>());
            behaviours_.push_back(std::make_unique<HuntBehaviour>(3));
            break;

        case BotPersonality::Balanced:
        default:
            behaviours_.push_back(std::make_unique<CollectPowerUpBehaviour>());
            behaviours_.push_back(std::make_unique<BreakBlocksBehaviour>());
            behaviours_.push_back(std::make_unique<HuntBehaviour>());
            break;
        }

        // Always last: something to do when nothing else applies.
        behaviours_.push_back(std::make_unique<WanderBehaviour>());
    }

    BotAction BotBrain::decide(const BotContext& ctx) const {
        for (const auto& behaviour : behaviours_) {
            if (std::optional<BotAction> action = behaviour->decide(ctx)) {
                return *action;
            }
        }
        return {};
    }

    BotPersonality BotBrain::personality() const {
        return personality_;
    }

    std::vector<std::string> BotBrain::priorities() const {
        std::vector<std::string> names;
        names.reserve(behaviours_.size());
        for (const auto& behaviour : behaviours_) {
            names.push_back(behaviour->name());
        }
        return names;
    }
} // namespace bomberman::logic::ai
