#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>

using namespace geode::prelude;

// ---------- Stages ----------
struct Stage { int ticks; const char* label; };
static const Stage STAGES[] = {
    {0,  "OFF (100% Raw Input)"},
    {2,  "STAGE 1: 2 Ticks"},
    {4,  "STAGE 2: 4 Ticks"},
    {8,  "STAGE 3: 8 Ticks"},
    {16, "STAGE 4: 16 Ticks"},
};
static constexpr int STAGE_COUNT = 5;
static int g_stage = 0;

static int bufferTicks() { return STAGES[g_stage].ticks; }

// ---------- Pending inputs ----------
struct PendingPress {
    int  button;
    bool player1;
    int  ticksLeft;
    bool releaseQueued = false;
};

// ---------- F6 cycling ----------
class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6 && PlayLayer::get()) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            log::info("[InputBuffer] F6 Cycle -> {} | buffering {} ticks",
                      STAGES[g_stage].label, STAGES[g_stage].ticks);
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

// ---------- Buffer logic ----------
class $modify(BufferLayer, GJBaseGameLayer) {
    struct Fields {
        std::vector<PendingPress> pending;
        bool firing = false;   // bypass flag so our own re-fired inputs aren't re-buffered
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

        if (down) {
            // Pass it through now (works if the player can act immediately)...
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            // ...and also remember it so it fires on landing if clicked early.
            f->pending.push_back({button, isPlayer1, bufferTicks()});
            return;
        }

        // Release: if a press for this button is still waiting, delay the release
        // until after that press has fired so short taps still jump.
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

        // Only count full ticks so the tick windows stay accurate.
        if (isHalfTick) return;

        auto f = m_fields.self();
        if (f->pending.empty()) return;

        if (!bufferActive()) {
            f->pending.clear();
            return;
        }

        f->firing = true;
        for (auto it = f->pending.begin(); it != f->pending.end();) {
            PlayerObject* pl = it->player1 ? m_player1 : m_player2;
            bool fired = false;

            if (pl && pl->m_isOnGround) {
                GJBaseGameLayer::handleButton(true, it->button, it->player1);
                if (it->releaseQueued)
                    GJBaseGameLayer::handleButton(false, it->button, it->player1);
                fired = true;
                log::info("[InputBuffer] Saved {} ticks", it->ticksLeft);
            }

            if (fired || --it->ticksLeft <= 0) {
                // Window expired: make sure a queued release isn't lost.
                if (!fired && it->releaseQueued)
                    GJBaseGameLayer::handleButton(false, it->button, it->player1);
                it = f->pending.erase(it);
            } else {
                ++it;
            }
        }
        f->firing = false;
    }
};

// ---------- Reset on restart ----------
class $modify(PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        static_cast<BufferLayer*>(static_cast<GJBaseGameLayer*>(this))->m_fields->pending.clear();
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v4.6 LOADED");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2 -> 4 -> 8 -> 16 -> OFF");
    log::info("====================================================");
}
