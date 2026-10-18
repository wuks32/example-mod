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

static int   g_stage = 0;
static bool  g_active[2]       = {false, false};  // a buffered click is waiting
static float g_window[2]       = {0.f, 0.f};      // seconds left in the buffer window
static bool  g_held[2]         = {false, false};  // is the real button physically down
static bool  g_releaseNext[2]  = {false, false};  // release a click we fired ourselves
static bool  g_wasTouching[2]  = {false, false};
static bool  g_wasOnGround[2]  = {false, false};
static bool  g_replaying = false;

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
    float fa = s.all / 4.f;
    return fmt::format("{}: {} Ticks buffer ({:.1f}f @ 60fps, wave unaffected)", name, s.all, fa);
}

static void resetState() {
    for (int i = 0; i < 2; i++) {
        g_active[i] = false;
        g_held[i] = false;
        g_releaseNext[i] = false;
    }
}

class $modify(BufferLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (button != 1 || g_replaying || g_stage == STAGE_OFF) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        int idx = isPlayer1 ? 0 : 1;
        auto player = isPlayer1 ? m_player1 : m_player2;
        int ticks = getStage(g_stage).all;

        if (!player || player->m_isDart || ticks <= 0) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        g_held[idx] = down;

        if (down) {
            bool touching = !player->m_touchedRings.empty();
            bool ground = player->m_isOnGround;
            if (!touching && !ground) {
                // click happened with nothing to use it on yet -> keep it for a few ticks
                g_active[idx] = true;
                g_window[idx] = ticks / TICKS_PER_SEC;
                g_wasTouching[idx] = false;
                g_wasOnGround[idx] = false;
                log::info("[InputBuffer] early click BUFFERED for {} ticks ({:.4f}s)", ticks, g_window[idx]);
            } else {
                g_active[idx] = false;
            }
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        for (int i = 0; i < 2; i++) {
            auto player = (i == 0) ? m_player1 : m_player2;

            // finish a click we fired ourselves while the real button was up
            if (g_releaseNext[i]) {
                g_releaseNext[i] = false;
                if (!g_held[i]) {
                    g_replaying = true;
                    GJBaseGameLayer::handleButton(false, 1, i == 0);
                    g_replaying = false;
                }
            }

            if (!g_active[i] || !player) continue;

            g_window[i] -= dt;
            bool touching = !player->m_touchedRings.empty();
            bool ground = player->m_isOnGround;
            bool opportunity = (touching && !g_wasTouching[i]) || (ground && !g_wasOnGround[i]);
            g_wasTouching[i] = touching;
            g_wasOnGround[i] = ground;

            if (opportunity) {
                g_active[i] = false;
                log::info("[InputBuffer] buffered click FIRED p{} (ring={} ground={})", i + 1, touching, ground);
                g_replaying = true;
                if (g_held[i]) {
                    GJBaseGameLayer::handleButton(false, 1, i == 0);
                    GJBaseGameLayer::handleButton(true, 1, i == 0);
                } else {
                    GJBaseGameLayer::handleButton(true, 1, i == 0);
                    g_releaseNext[i] = true;
                }
                g_replaying = false;
            } else if (g_window[i] <= 0.f) {
                g_active[i] = false;
                log::info("[InputBuffer] buffered click expired p{}", i + 1);
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        g_stage = Mod::get()->getSavedValue<int>("stage", 0);
        resetState();
        log::info("==============================================================");
        log::info("[InputBuffer] v4.6 LOADED (press buffer)");
        log::info("[InputBuffer] Starting Mode: {}", describe(g_stage));
        log::info("[InputBuffer] Press F6 in-level to cycle: 2 -> 4 -> 8 -> 16 -> CUSTOM -> OFF");
        log::info("==============================================================");
        return true;
    }

    void resetLevel() {
        resetState();
        PlayLayer::resetLevel();
    }
};

class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6 && PlayLayer::get()) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            Mod::get()->setSavedValue("stage", g_stage);
            resetState();
            auto msg = describe(g_stage);
            log::info("[InputBuffer] F6 Cycle -> {}", msg);
            if (Mod::get()->getSettingValue<bool>("show-popup")) {
                Notification::create(msg, NotificationIcon::None, 1.5f)->show();
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
