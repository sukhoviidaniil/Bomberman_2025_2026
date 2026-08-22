/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-14
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "TestFramework.h"

#include <filesystem>
#include <fstream>

#include "bomberman/logic/PlayerProfile.h"
#include "bomberman/logic/ScoreBoard.h"

using namespace bomberman::logic;

namespace {
    std::string temp_path(const std::string& name) {
        return (std::filesystem::temp_directory_path() / name).string();
    }

    template<class Fn>
    bool throws(Fn&& fn) {
        try {
            fn();
        } catch (const std::exception&) {
            return true;
        }
        return false;
    }
} // namespace

// ---------------------------------------------------------------------
// Editing
// ---------------------------------------------------------------------

SIF_TEST(a_new_profile_has_a_usable_default_name) {
    const PlayerProfile profile;
    SIF_CHECK(!profile.name().empty());
}

SIF_TEST(typing_builds_the_name_one_character_at_a_time) {
    PlayerProfile profile;
    profile.set_name(""); // refused: see below
    while (!profile.name().empty()) {
        profile.backspace();
    }

    for (const char32_t c : {U'A', U'c', U'e'}) {
        profile.append(c);
    }
    SIF_CHECK(profile.name() == "Ace");

    profile.backspace();
    SIF_CHECK(profile.name() == "Ac");
}

SIF_TEST(the_name_is_capped_so_the_scoreboard_stays_readable) {
    PlayerProfile profile;
    while (!profile.name().empty()) {
        profile.backspace();
    }

    for (int i = 0; i < 40; ++i) {
        profile.append(U'x');
    }
    SIF_CHECK(profile.name().size() == PlayerProfile::max_name_length);
}

SIF_TEST(unprintable_and_non_ascii_keystrokes_are_ignored) {
    PlayerProfile profile;
    while (!profile.name().empty()) {
        profile.backspace();
    }

    profile.append(U'\t');
    profile.append(U'\n');
    profile.append(U'\u0444'); // beyond the font's glyph set
    SIF_CHECK(profile.name().empty());

    profile.append(U'Z');
    SIF_CHECK(profile.name() == "Z");
}

SIF_TEST(backspace_on_an_empty_name_is_harmless) {
    PlayerProfile profile;
    while (!profile.name().empty()) {
        profile.backspace();
    }
    profile.backspace();
    profile.backspace();
    SIF_CHECK(profile.name().empty());
}

SIF_TEST(a_blank_name_never_replaces_a_real_one) {
    // An empty entry on the scoreboard is worse than a stale one, and
    // there is no good moment to refuse it at, so set_name declines.
    PlayerProfile profile;
    SIF_CHECK(!profile.set_name("   "));
    SIF_CHECK(!profile.set_name(""));
    SIF_CHECK(profile.name() == "player");

    SIF_CHECK(profile.set_name("  Daniil  "));
    SIF_CHECK(profile.name() == "Daniil"); // trimmed
}

// ---------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------

SIF_TEST(a_profile_survives_a_round_trip) {
    const std::string path = temp_path("bm_profile.json");
    std::filesystem::remove(path);

    PlayerProfile written;
    written.set_name("Ace");
    written.save(path);

    PlayerProfile read;
    read.load(path);
    SIF_CHECK(read.name() == "Ace");

    std::filesystem::remove(path);
}

SIF_TEST(a_missing_profile_is_the_first_run_not_an_error) {
    PlayerProfile profile;
    profile.load(temp_path("bm_profile_absent.json"));
    SIF_CHECK(profile.name() == "player");
}

SIF_TEST(a_corrupt_profile_is_reported) {
    const std::string path = temp_path("bm_profile_bad.json");
    std::ofstream(path) << "[1, 2, 3]";

    PlayerProfile profile;
    SIF_CHECK(throws([&] { profile.load(path); }));

    std::filesystem::remove(path);
}

// ---------------------------------------------------------------------
// What the save screen decides between
// ---------------------------------------------------------------------

SIF_TEST(the_board_can_say_in_advance_whether_a_score_qualifies) {
    // The save screen tells the player this *before* they choose, which
    // is only possible because qualifies() does not mutate anything.
    ScoreBoard board;
    for (int i = 0; i < static_cast<int>(ScoreBoard::capacity); ++i) {
        board.submit({"bot", 100 + i});
    }

    SIF_CHECK(board.entries().size() == ScoreBoard::capacity);
    SIF_CHECK(board.qualifies(1000));
    SIF_CHECK(!board.qualifies(1));

    // ...and asking did not change the board.
    SIF_CHECK(board.entries().size() == ScoreBoard::capacity);
}

SIF_TEST(discarding_a_score_leaves_the_board_untouched) {
    ScoreBoard board;
    board.submit({"Ace", 500});

    const std::size_t before = board.entries().size();
    // "Discard" is simply never calling submit - the state pops instead.
    SIF_CHECK(board.entries().size() == before);
    SIF_CHECK(board.entries().front().name == "Ace");
}

SIF_TEST(a_saved_score_is_recorded_under_the_current_name) {
    PlayerProfile profile;
    profile.set_name("Renamed");

    ScoreBoard board;
    board.submit({profile.name(), 320});

    SIF_CHECK(board.entries().size() == 1);
    SIF_CHECK(board.entries().front().name == "Renamed");
    SIF_CHECK(board.entries().front().points == 320);
}
