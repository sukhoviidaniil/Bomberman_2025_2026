/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-14
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/PlayerProfile.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "sif/io/from_JSON.h"

namespace bomberman::logic {
namespace {
std::string trim(std::string value) {
    const auto not_space = [](const unsigned char c) { return std::isspace(c) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}
} // namespace

const std::string& PlayerProfile::name() const { return name_; }

bool PlayerProfile::set_name(std::string value) {
    value = trim(std::move(value));
    if (value.size() > max_name_length) {
        value.resize(max_name_length);
        value = trim(std::move(value));
    }
    if (value.empty() || value == name_) {
        return false;
    }
    name_ = std::move(value);
    return true;
}

void PlayerProfile::append(const char32_t character) {
    if (name_.size() >= max_name_length) {
        return;
    }
    // ASCII only: the scoreboard font is a fixed set of glyphs and a
    // half-rendered name is worse than a rejected keystroke. Anything
    // beyond it is silently ignored rather than written as bytes the
    // renderer cannot draw.
    if (character < 32 || character > 126) {
        return;
    }
    name_.push_back(static_cast<char>(character));
}

void PlayerProfile::backspace() {
    if (!name_.empty()) {
        name_.pop_back();
    }
}

void PlayerProfile::load(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        return; // first run
    }

    const nlohmann::json j = sif::io::get_json_data(filepath);
    if (!j.is_object()) {
        throw std::runtime_error("player profile: '" + filepath + "' must contain an object");
    }

    // Not set_name(): a stored name is accepted as it is, so a file
    // written by an older version cannot be silently "corrected" into
    // something the player never chose.
    std::string stored = sif::io::get_checked<std::string>(j, "name", name_);
    stored = trim(std::move(stored));
    if (!stored.empty()) {
        if (stored.size() > max_name_length) {
            stored.resize(max_name_length);
        }
        name_ = std::move(stored);
    }
}

void PlayerProfile::save(const std::string& filepath) const {
    nlohmann::json j = nlohmann::json::object();
    j["name"] = name_;

    // sif's writer creates the directory, writes to a temp file and
    // renames it, so an interrupted save cannot truncate the profile.
    sif::io::write_json_file(filepath, j);
}
} // namespace bomberman::logic
