/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-14
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_PLAYERPROFILE_H
#define BOMBERMAN_LOGIC_PLAYERPROFILE_H

#include <string>

namespace bomberman::logic {

/**
 * @brief What the game remembers about the person playing it.
 *
 * One field today, but a file of its own rather than a key in
 * config.json: the configuration is *input* that a player edits and
 * the game only reads, while this is state the game writes back.
 * Mixing the two means a save eventually overwrites a hand-edited
 * comment or reformats somebody's file.
 */
class PlayerProfile {
public:
    /// @brief Longest name the scoreboard can show without truncating.
    static constexpr std::size_t max_name_length = 12;

    PlayerProfile() = default;

    [[nodiscard]] const std::string& name() const;

    /**
     * @brief Sets the name, trimmed and capped.
     *
     * Anything that would leave the field blank keeps the previous
     * name instead: an empty entry on the scoreboard is worse than a
     * stale one, and there is no sensible moment to refuse it at.
     *
     * @return true if the name actually changed.
     */
    bool set_name(std::string value);

    /// @brief Appends one typed character, if there is room for it.
    void append(char32_t character);

    /// @brief Removes the last character.
    void backspace();

    /**
     * @brief Reads the profile, or leaves the default if there is none.
     *
     * A missing file is the first run, not an error.
     *
     * @throws std::runtime_error if the file exists but cannot be parsed.
     */
    void load(const std::string& filepath);

    /// @throws std::runtime_error if the file cannot be written.
    void save(const std::string& filepath) const;

private:
    std::string name_ = "player";
};
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_PLAYERPROFILE_H
