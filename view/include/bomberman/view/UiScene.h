/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-14
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_UISCENE_H
#define BOMBERMAN_VIEW_UISCENE_H

#include <memory>
#include <string>
#include <vector>

#include "sif/internal/Color.h"
#include "sif/layout_engine/elements/Text.h"
#include "sif/layout_engine/elements/UIElement.h"
#include "sif/render/Camera.h"
#include "sif/render/RenderFrame.h"

namespace bomberman::view {

/**
 * @brief One screen, loaded from a *.ui.xml file.
 *
 * Wraps the whole authoring pipeline - Tokenizer, Parser, UIFactory -
 * and the per-frame measure/layout/collect dance, so a state's job
 * shrinks to "load this scene, then set these labels".
 *
 * @par Why the screens are files
 * They used to be `text(frame, ctx, "PAUSED", 280.f, 48, colour)`
 * calls with hand-tuned pixel offsets and an approximate centring
 * formula, which meant every layout change was a rebuild and every
 * label was centred by eye. The layout engine measures text with the
 * real font metrics, so the same scene is correct at any window size -
 * and a designer can move things without touching C++.
 */
class UiScene {
public:
    UiScene() = default;

    /**
     * @brief Loads a serialized scene from the data directory.
     *
     * @param scene_file File name inside <data>/bin/scenes/.
     * @throws std::runtime_error if the file is missing or malformed,
     * naming the file - a scene is content, and content breaks.
     */
    explicit UiScene(const std::string& scene_file);

    [[nodiscard]] bool loaded() const;

    /// @brief Advances animations and button transitions.
    void update(float dt);

    /// @brief Lays the tree out for the current screen size and draws it.
    void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const;

    /**
     * @brief Sets the text of a named element.
     *
     * A missing name is ignored rather than fatal: a scene is data,
     * and a renamed label should not crash a running game.
     */
    void set_text(const std::string& element_name, const std::string& value);

    void set_color(const std::string& element_name, sif::intrnl::Color color);

    /// @brief Shows or hides a named element (it keeps its layout space).
    void set_visible(const std::string& element_name, bool visible);

    [[nodiscard]] sif::ui::UIElement* root() const;

private:
    std::unique_ptr<sif::ui::UIElement> root_;
};

/**
 * @brief Keyboard selection over a list of named scene elements.
 *
 * The scenes describe *what* the options are; this decides which one
 * is highlighted and recolours it. Keeping it out of the scene file
 * means the same menu markup works whether it is driven by a keyboard,
 * a mouse or an AI demo.
 */
class MenuNav {
public:
    MenuNav() = default;

    /// @param item_names Element names, in the order they appear.
    explicit MenuNav(std::vector<std::string> item_names);

    void move(int delta);

    [[nodiscard]] const std::string& current() const;
    [[nodiscard]] std::size_t index() const;
    [[nodiscard]] bool empty() const;

    /// @brief Recolours the items so the selected one stands out.
    void apply(UiScene& scene, sif::intrnl::Color selected, sif::intrnl::Color normal) const;

private:
    std::vector<std::string> items_;
    std::size_t index_ = 0;
};
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_UISCENE_H
