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

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "sif/asset/AssetRegistry.h"
#include "sif/asset/internal/IAssetLoader.h"
#include "sif/asset/internal/Sound.h"

using namespace sif;

namespace {
    /// Records which thread actually ran the load.
    class ThreadRecordingLoader final : public asset::IAssetLoader {
    public:
        ThreadRecordingLoader(std::atomic<std::thread::id>& where, const bool main_thread)
            : where_(where), main_thread_(main_thread) {}

        std::atomic<int> runs{0};

    private:
        void load_asset(asset::AssetRecord& record, const std::string&, const uint64_t token) override {
            where_ = std::this_thread::get_id();
            ++runs;
            record.set_data_if_current(token, std::make_shared<int>(1));
        }

        [[nodiscard]] bool runs_on_main_thread() const override { return main_thread_; }

        std::atomic<std::thread::id>& where_;
        bool main_thread_;
    };

    intrnl::GUID register_asset(const intrnl::GUID guid, const asset::AssetType type) {
        asset::data::AssetMetaData meta;
        meta.guid = guid;
        meta.type = type;
        meta.asset_name = "threaded_" + guid.string();
        asset::AssetRegistry::instance().add_AssetRecord(asset::AssetDesc("t.asset.json", meta));
        return guid;
    }
}

SIF_TEST(a_main_thread_loader_runs_on_the_thread_that_pumps) {
    // The whole point: OpenAL and OpenGL cannot be touched from a worker
    // thread while the main thread is drawing or playing, so a loader can
    // ask to be run by the frame loop instead.
    asset::AssetRegistry& registry = asset::AssetRegistry::instance();

    std::atomic<std::thread::id> where{};
    auto loader = std::make_unique<ThreadRecordingLoader>(where, true);
    ThreadRecordingLoader* raw = loader.get();
    registry.register_loader(asset::AssetType::Sound, std::move(loader));

    const intrnl::GUID guid = register_asset(intrnl::GUID(0xB00301), asset::AssetType::Sound);
    registry.request(guid);

    // Nothing happens until somebody pumps: the load is queued, not lost.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    SIF_CHECK(raw->runs == 0);

    registry.pump(std::chrono::milliseconds(100));

    SIF_CHECK(raw->runs == 1);
    SIF_CHECK(where.load() == std::this_thread::get_id());
    SIF_CHECK(registry.get<asset::Sound>(guid).ready());
}

SIF_TEST(an_ordinary_loader_still_runs_in_the_background) {
    // The main-thread route must not become the only route: image and
    // data decoding is pure CPU work and should stay parallel.
    asset::AssetRegistry& registry = asset::AssetRegistry::instance();

    std::atomic<std::thread::id> where{};
    auto loader = std::make_unique<ThreadRecordingLoader>(where, false);
    ThreadRecordingLoader* raw = loader.get();
    registry.register_loader(asset::AssetType::SpriteSingle, std::move(loader));

    const intrnl::GUID guid = register_asset(intrnl::GUID(0xB00302), asset::AssetType::SpriteSingle);
    registry.request(guid);

    SIF_CHECK(registry.wait_for_idle(std::chrono::seconds(5)));
    SIF_CHECK(raw->runs == 1);
    SIF_CHECK(where.load() != std::this_thread::get_id());
}

SIF_TEST(pumping_with_nothing_queued_is_harmless) {
    // Called every frame, so the empty case is the common case.
    asset::AssetRegistry::instance().pump();
    asset::AssetRegistry::instance().pump();
    SIF_CHECK(true);
}

SIF_TEST(wait_for_idle_does_not_block_on_work_only_pump_can_do) {
    // wait_for_idle runs on the same thread as pump(), so waiting for a
    // main-thread load to finish would be waiting for itself.
    asset::AssetRegistry& registry = asset::AssetRegistry::instance();

    std::atomic<std::thread::id> where{};
    auto loader = std::make_unique<ThreadRecordingLoader>(where, true);
    registry.register_loader(asset::AssetType::Sound, std::move(loader));

    const intrnl::GUID guid = register_asset(intrnl::GUID(0xB00303), asset::AssetType::Sound);
    registry.request(guid);

    const auto start = std::chrono::steady_clock::now();
    const bool idle = registry.wait_for_idle(std::chrono::seconds(2));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    SIF_CHECK(idle);
    SIF_CHECK(elapsed < std::chrono::seconds(1)); // returned at once, not after the timeout

    registry.pump(std::chrono::milliseconds(100));
}
