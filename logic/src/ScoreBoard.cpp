/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-26
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/ScoreBoard.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "sif/io/from_JSON.h"

namespace bomberman::logic {

    void ScoreBoard::load(const std::string& filepath) {
        entries_.clear();

        if (!std::filesystem::exists(filepath)) {
            return; // first run: an empty board is correct
        }

        const nlohmann::json j = sif::io::get_json_data(filepath);
        if (!j.is_array()) {
            throw std::runtime_error("ScoreBoard: '" + filepath + "' is not a JSON array");
        }

        entries_.reserve(j.size());
        for (const auto& node : j) {
            ScoreEntry entry;
            entry.name = sif::io::get_checked<std::string>(node, "name", entry.name);
            entry.points = sif::io::get_checked<int>(node, "points", entry.points);
            entries_.push_back(entry);
        }

        std::stable_sort(entries_.begin(), entries_.end(),
                         [](const ScoreEntry& a, const ScoreEntry& b) { return a.points > b.points; });
        if (entries_.size() > capacity) {
            entries_.resize(capacity);
        }
    }

    void ScoreBoard::save(const std::string& filepath) const {
        nlohmann::json j = nlohmann::json::array();
        for (const ScoreEntry& entry : entries_) {
            nlohmann::json node = nlohmann::json::object();
            node["name"] = entry.name;
            node["points"] = entry.points;
            j.push_back(node);
        }

        // sif's writer creates the directory, writes to a temp file and
        // renames it into place, so an interrupted save cannot leave the
        // player with a truncated high-score table.
        sif::io::write_json_file(filepath, j);
    }

    bool ScoreBoard::qualifies(const int points) const {
        return entries_.size() < capacity || points > entries_.back().points;
    }

    bool ScoreBoard::submit(const ScoreEntry& entry) {
        if (!qualifies(entry.points)) {
            return false;
        }

        entries_.push_back(entry);
        std::stable_sort(entries_.begin(), entries_.end(),
                         [](const ScoreEntry& a, const ScoreEntry& b) { return a.points > b.points; });
        if (entries_.size() > capacity) {
            entries_.resize(capacity);
        }
        return true;
    }

    const std::vector<ScoreEntry>& ScoreBoard::entries() const {
        return entries_;
    }
} // namespace bomberman::logic
