#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

struct Stage { int all; int wave; };

static constexpr int STAGE_CUSTOM = 4;
static constexpr int STAGE_OFF = 5;
static constexpr int STAGE_COUNT = 6;
static constexpr float TICKS_PER_SEC = 240.f;

static int  g_stage = 0;
static float g_pending[2] = {-1.f, -1.f};   // seconds left before delayed release fires, -1 = none
static bool g_replaying = false;

static Stage getStage(int i) {
    switch (i) {
        case 0: return {2, 2};
        case 1: return {4, 4};
        case 2: return {8, 8};
        case 3: return {16, 8};
        case STAGE_CUSTOM: return {
            (int)Mod::get()->getSettingValue<int64_t>("custom-all"),
            (int)Mod::get()->getSettingValue<int64_t>("custom-wave")
        };
        default: return {0, 0};
    }
}

static std::string describe(int i) {
    if (i == STAGE_OFF) return "OFF";
    auto s = getStage(i);
    auto name = i == STAGE_CUSTOM ? std::string("CUSTOM") : fmt::format("STAGE {}", i + 1);
    float fa = s.all / 4.f, fw = s.wave / 4.f;
    if (s.all == s.wave) {
        return fmt::format("{}: {} Ticks All | {} Ticks Wave ({:.1f}f @ 60fps)", name, s.all, s.wave, fa);
    }
    return fmt::format("{}: {} Ticks All | {} Ticks Wave ({:.1f}f / {:.1f}f @ 60fps)", name, s.all, s.wave, fa, fw);
}

class $modify(BufferLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        log::info("[InputBuffer] INPUT {} button={} p1={} replaying={} stage={}",
                  down ? "PRESS" : "RELEASE", button, isPlayer1, g_replaying, g_stage);

        if (button != 1 || g_replaying || g_stage == STAGE_OFF) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        int idx = isPlayer1 ? 0 : 1;
        auto player = isPlayer1 ? m_player1 : m_player2;
        auto s = getStage(g_stage);
        int ticks = (player && player->m_isDart) ? s.wave : s.all;

        if (ticks <= 0) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        if (down) {
            // If a release is still waiting, deliver it now so this click registers as a fresh press
            if (g_pending[idx] >= 0.f) {
                log::info("[InputBuffer] new press while release pending -> flushing old release first");
                g_pending[idx] = -1.f;
                g_replaying = true;
                GJBaseGameLayer::handleButton(false, button, isPlayer1);
                g_replaying = false;
            }
            GJBaseGameLayer::handleButton(true, button, isPlayer1);
        } else {
            g_pending[idx] = ticks / TICKS_PER_SEC;
            log::info("[InputBuffer] release DELAYED {} ticks ({:.4f}s)", ticks, g_pending[idx]);
        }
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        for (int i = 0; i < 2; i++) {
            if (g_pending[i] < 0.f) continue;
            g_pending[i] -= dt;
            if (g_pending[i] <= 0.f) {
                g_pending[i] = -1.f;
                log::info("[InputBuffer] delayed release FIRED p{}", i + 1);
                g_replaying = true;
                GJBaseGameLayer::handleButton(false, 1, i == 0);
                g_replaying = false;
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        g_stage = Mod::get()->getSavedValue<int>("stage", 0);
        g_pending[0] = g_pending[1] = -1.f;
        log::info("==============================================================");
        log::info("[InputBuffer] v4.6 LOADED");
        log::info("[InputBuffer] Starting Mode: {}", describe(g_stage));
        log::info("[InputBuffer] Press F6 in-level to cycle: 2/2 -> 4/4 -> 8/8 -> 16/8 -> CUSTOM -> OFF");
        log::info("==============================================================");
        return true;
    }

    void resetLevel() {
        g_pending[0] = g_pending[1] = -1.f;
        log::info("[InputBuffer] level reset, pending releases cleared");
        PlayLayer::resetLevel();
    }
};

class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6 && PlayLayer::get()) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            Mod::get()->setSavedValue("stage", g_stage);
            auto msg = describe(g_stage);
            log::info("[InputBuffer] F6 Cycle -> {}", msg);
            if (Mod::get()->getSettingValue<bool>("show-popup")) {
                Notification::create(msg, NotificationIcon::None, 1.5f)->show();
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
