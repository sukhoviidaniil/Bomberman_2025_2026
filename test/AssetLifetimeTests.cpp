/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-12
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "TestFramework.h"

#include <memory>

#include "sif/asset/AssetHandle.h"
#include "sif/asset/AssetRegistry.h"
#include "sif/asset/internal/data/AssetDataNode.h"
#include "sif/asset/internal/data/PrimitiveAnimationNode.h"
#include "sif/asset/internal/Sound.h"

using namespace sif;

namespace {
    /// A Sound that reports when it is destroyed, so a dangling handle
    /// becomes an observable fact rather than a suspicion.
    class WatchedSound final : public asset::Sound {
    public:
        explicit WatchedSound(bool& alive_flag) : alive_(alive_flag) { alive_ = true; }
        ~WatchedSound() override { alive_ = false; }

        [[nodiscard]] float duration_seconds() const override { return 1.f; }

    private:
        bool& alive_;
    };

    /// Registers an asset and marks it Ready with the given data.
    intrnl::GUID ready_asset(const intrnl::GUID guid, std::shared_ptr<void> data) {
        asset::AssetRegistry& registry = asset::AssetRegistry::instance();

        asset::data::AssetMetaData meta;
        meta.guid = guid;
        meta.type = asset::AssetType::Sound;
        meta.asset_name = "watched_" + guid.string();
        registry.add_AssetRecord(asset::AssetDesc("watched.asset.json", meta));

        const asset::AssetHandle<void> handle = registry.get<void>(guid);
        if (const auto record = handle.record().lock()) {
            record->set_data(std::move(data));
            record->set_state(asset::AssetState::Ready);
        }
        return guid;
    }
}

// ---------------------------------------------------------------------
// Asset data nodes
// ---------------------------------------------------------------------

SIF_TEST(deleting_a_data_node_through_the_base_runs_the_derived_destructor) {
    // Without a virtual destructor on AssetDataNode this is undefined
    // behaviour, and it really did misbehave: AddressSanitizer caught the
    // asset tool freeing 200 bytes of a 232-byte PrimitiveAnimationNode,
    // which corrupts the allocator and turns some *later*, unrelated
    // allocation into a crash.
    SIF_CHECK(std::has_virtual_destructor_v<asset::data::AssetDataNode>);

    std::unique_ptr<asset::data::AssetDataNode> node =
        std::make_unique<asset::data::PrimitiveAnimationNode>();
    node.reset(); // must not corrupt anything
    SIF_CHECK(node == nullptr);
}

// ---------------------------------------------------------------------
// Handle lifetime
// ---------------------------------------------------------------------

SIF_TEST(lock_keeps_the_asset_alive_even_if_the_record_lets_go) {
    bool alive = false;
    const intrnl::GUID guid = ready_asset(
        intrnl::GUID(0xA55E01),
        std::static_pointer_cast<void>(std::shared_ptr<asset::Sound>(
            std::make_shared<WatchedSound>(alive))));

    const asset::AssetHandle<asset::Sound> handle =
        asset::AssetRegistry::instance().get<asset::Sound>(guid);

    SIF_CHECK(alive);

    // Hold it, then have the record drop its own reference - the way a
    // background reload would.
    const std::shared_ptr<asset::Sound> held = handle.lock();
    SIF_CHECK(held != nullptr);

    if (const auto record = handle.record().lock()) {
        record->set_data(nullptr);
    }

    // Still alive: that is the whole point of lock().
    SIF_CHECK(alive);
    SIF_CHECK(held->duration_seconds() == 1.f);
}

SIF_TEST(lock_reports_nothing_for_an_asset_that_is_not_ready) {
    asset::AssetRegistry& registry = asset::AssetRegistry::instance();

    asset::data::AssetMetaData meta;
    meta.guid = intrnl::GUID(0xA55E02);
    meta.type = asset::AssetType::Sound;
    meta.asset_name = "never_loaded";
    registry.add_AssetRecord(asset::AssetDesc("never.asset.json", meta));

    const asset::AssetHandle<asset::Sound> handle = registry.get<asset::Sound>(meta.guid);

    SIF_CHECK(!handle.ready());
    SIF_CHECK(handle.lock() == nullptr);
    SIF_CHECK(handle.get() == nullptr);
}

SIF_TEST(a_default_constructed_handle_is_safely_empty) {
    // What every "the assets are still loading" path hits, and the reason
    // a key press during start-up must not be able to crash anything.
    const asset::AssetHandle<asset::Sound> handle;

    SIF_CHECK(!handle.ready());
    SIF_CHECK(handle.lock() == nullptr);
    SIF_CHECK(handle.get() == nullptr);
    SIF_CHECK(handle.record().lock() == nullptr);
}
