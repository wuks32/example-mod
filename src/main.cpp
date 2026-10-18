#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>

using namespace geode::prelude;

// ---------- Stages (1 tick = 0.25 frames @ 60fps, 240 TPS) ----------
struct Stage { int ticks; const char* label; };
static const Stage STAGES[] = {
    {0,  "STAGE 5: OFF (100% Raw Input)"},
    {2,  "STAGE 1: 2 Ticks"},
    {4,  "STAGE 2: 4 Ticks"},
    {8,  "STAGE 3: 8 Ticks"},
    {16, "STAGE 4: 16 Ticks"},
};
static constexpr int STAGE_COUNT = 5;
static int g_stage = 0;

static int bufferTicks() { return STAGES[g_stage].ticks; }
static float ticksToFrames(int t) { return t / 4.0f; }

// ---------- Pending inputs ----------
struct PendingPress {
    int  button;
    bool player1;
    long startTick;
    int  limit;
    bool releaseQueued = false;
};

// ---------- F6 cycling ----------
class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6 && PlayLayer::get()) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            log::info("[InputBuffer] F6 Cycle -> {} | {} Ticks ({:.1f}f @ 60fps)",
                      STAGES[g_stage].label, STAGES[g_stage].ticks,
                      ticksToFrames(STAGES[g_stage].ticks));
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

// ---------- Buffer logic ----------
class $modify(BufferLayer, GJBaseGameLayer) {
    struct Fields {
        std::vector<PendingPress> pending;
        long tickCounter = 0;
        bool firing = false;   // our own re-fired inputs must not be re-buffered
    };

    bool bufferActive() {
        if (bufferTicks() == 0) return false;
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return false;
        if (!Mod::get()->getSettingValue<bool>("allow-normal-mode") && !pl->m_isPracticeMode)
            return false;
        return true;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto f = m_fields.self();

        if (f->firing || !bufferActive()) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        PlayerObject* pl = isPlayer1 ? m_player1 : m_player2;

        if (down) {
            // Always pass the click through (orbs, pads, other modes keep working).
            GJBaseGameLayer::handleButton(down, button, isPlayer1);

            // Only buffer clicks made while airborne. Grounded clicks just work.
            if (pl && !pl->m_isOnGround) {
                f->pending.push_back({button, isPlayer1, f->tickCounter, bufferTicks()});
                log::info("[InputBuffer] BUFFERED click @ tick {} (waiting up to {} ticks)",
                          f->tickCounter, bufferTicks());
            }
            return;
        }

        // Release: if this button's press is still waiting, hold the release back
        // until the press has fired, so short taps still make a full jump.
        for (auto& p : f->pending) {
            if (p.button == button && p.player1 == isPlayer1) {
                p.releaseQueued = true;
                return;
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (isHalfTick) return;

        auto f = m_fields.self();
        f->tickCounter++;

        if (f->pending.empty()) return;

        if (!bufferActive()) {
            f->pending.clear();
            return;
        }

        f->firing = true;
        for (auto it = f->pending.begin(); it != f->pending.end();) {
            PlayerObject* pl = it->player1 ? m_player1 : m_player2;
            long elapsed = f->tickCounter - it->startTick;
            bool done = false;

            if (pl && pl->m_isOnGround) {
                // Landed: fire the click now.
                GJBaseGameLayer::handleButton(true, it->button, it->player1);
                if (it->releaseQueued)
                    GJBaseGameLayer::handleButton(false, it->button, it->player1);
                log::info("[InputBuffer] *** SAVED +{} TICKS ({:.2f} frames @ 60fps) PRESS! "
                          "(Frame0: {} -> Delayed: {}) [Limit: {}] ***",
                          elapsed, ticksToFrames((int)elapsed),
                          it->startTick, f->tickCounter, it->limit);
                done = true;
            } else if (elapsed >= it->limit) {
                // Window ran out before landing: too early, give up.
                if (it->releaseQueued)
                    GJBaseGameLayer::handleButton(false, it->button, it->player1);
                log::info("[InputBuffer] EXPIRED (clicked too early, never landed in {} ticks)",
                          it->limit);
                done = true;
            }

            if (done) it = f->pending.erase(it);
            else ++it;
        }
        f->firing = false;
    }
};

// ---------- Reset on restart ----------
class $modify(PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        auto f = static_cast<BufferLayer*>(static_cast<GJBaseGameLayer*>(this))->m_fields.self();
        f->pending.clear();
        f->tickCounter = 0;
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v4.6 LOADED");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2 -> 4 -> 8 -> 16 -> OFF");
    log::info("====================================================");
}
