#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>
#include <algorithm>
#include <chrono>

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

// How far ahead (ticks) we check that you survive. 8 ticks = 2 frames.
static constexpr int LOOKAHEAD = 8;
// If one decision takes longer than this, skip buffering for that input.
static constexpr double MAX_DECISION_MS = 25.0;

// ---------- Shared state ----------
struct Ev { bool down; int button; bool p1; long fireTick; long madeTick; };
static std::vector<Ev> g_in;    // freshly caught inputs, waiting for a decision
static std::vector<Ev> g_q;     // decided inputs waiting for their fire tick
static long g_tick = 0;
static bool g_sim = false;      // inside a lookahead simulation
static bool g_fire = false;     // we are applying an input ourselves
static bool g_simDead = false;  // player died during simulation
static bool g_busy = false;     // decision in progress (blocks re-entry)

// ---------- F6 cycling ----------
class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            log::info("[InputBuffer] F6 Cycle -> {} | {} Ticks ({:.1f}f @ 60fps)",
                      STAGES[g_stage].label, STAGES[g_stage].ticks,
                      ticksToFrames(STAGES[g_stage].ticks));
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

// ---------- Death handling + reset ----------
class $modify(BufferPlay, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        if (g_sim) {            // simulated death: record only, never really die
            g_simDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        g_in.clear();
        g_q.clear();
        g_tick = 0;
    }
};

// ---------- Buffer logic ----------
class $modify(BufferLayer, GJBaseGameLayer) {

    bool bufferActive() {
        if (bufferTicks() == 0) return false;
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return false;
        if (!m_player1 || m_player1->m_isDead) return false;
        return true;
    }

    // Just catch the input. NO simulation here (game is mid input-loop).
    void handleButton(bool down, int button, bool isPlayer1) {
        if (g_fire || g_sim || g_busy || !bufferActive()) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }
        g_in.push_back({down, button, isPlayer1, g_tick, g_tick});
    }

    void applyNow(const Ev& ev) {
        g_fire = true;
        GJBaseGameLayer::handleButton(ev.down, ev.button, ev.p1);
        g_fire = false;
    }

    // Rewind to cp, apply ev after `delay` ticks, run, report if player survived.
    bool survives(PlayLayer* pl, CheckpointObject* cp, const Ev& ev, int delay) {
        pl->loadFromCheckpoint(cp);
        g_sim = true;
        g_simDead = false;

        for (int t = 0; t <= delay + LOOKAHEAD && !g_simDead; t++) {
            g_fire = true;
            for (auto& q : g_q)
                if (q.fireTick - g_tick == t)
                    GJBaseGameLayer::handleButton(q.down, q.button, q.p1);
            if (t == delay)
                GJBaseGameLayer::handleButton(ev.down, ev.button, ev.p1);
            g_fire = false;

            GJBaseGameLayer::update(1.f / 240.f);
        }

        g_sim = false;
        return !g_simDead;
    }

    // Decide the delay for one caught input. Called outside the input loop.
    void decide(Ev ev) {
        auto pl = PlayLayer::get();
        int limit = bufferTicks();

        long minTick = g_tick;
        for (auto& q : g_q) minTick = std::max(minTick, q.fireTick);
        int startDelay = (int)(minTick - g_tick);

        auto t0 = std::chrono::steady_clock::now();
        auto elapsedMs = [&]() {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
        };

        CheckpointObject* cp = pl->createCheckpoint();   // autoreleased by the game
        int chosen = -1;
        bool timedOut = false;

        if (cp) {
            for (int d = startDelay; d <= limit; d++) {
                if (survives(pl, cp, ev, d)) { chosen = d; break; }
                if (elapsedMs() > MAX_DECISION_MS) { timedOut = true; break; }
            }
            pl->loadFromCheckpoint(cp);   // restore the real game state
        }

        if (timedOut) {
            log::info("[InputBuffer] decision too slow ({:.1f} ms), sending raw", elapsedMs());
            if (startDelay == 0) { applyNow(ev); return; }
        }

        if (chosen == 0 && startDelay == 0) {
            applyNow(ev);                       // already safe, no change
            return;
        }

        if (chosen < 0 && startDelay == 0) {
            log::info("[InputBuffer] {} @ tick {}: no safe delay within {} ticks, sending raw",
                      ev.down ? "PRESS" : "RELEASE", g_tick, limit);
            applyNow(ev);
            return;
        }

        int use = chosen < 0 ? startDelay : chosen;
        ev.fireTick = g_tick + use;
        g_q.push_back(ev);
        log::info("[InputBuffer] BUFFERED {} @ tick {} -> fires in {} ticks ({:.1f} ms)",
                  ev.down ? "PRESS" : "RELEASE", g_tick, use, elapsedMs());
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (g_sim || g_busy) return;   // simulation steps never touch real state

        if (!isHalfTick) g_tick++;

        if (!bufferActive()) {
            g_in.clear();
            g_q.clear();
            return;
        }

        // 1) Decide caught inputs (safe spot: outside the game's input loop).
        if (!g_in.empty()) {
            g_busy = true;
            std::vector<Ev> work;
            work.swap(g_in);
            for (auto& ev : work) decide(ev);
            g_busy = false;
        }

        // 2) Fire delayed inputs whose time has come.
        for (auto it = g_q.begin(); it != g_q.end();) {
            if (it->fireTick <= g_tick) {
                applyNow(*it);
                long saved = g_tick - it->madeTick;
                log::info("[InputBuffer] *** SAVED +{} TICKS ({:.2f} frames @ 60fps) {}! "
                          "(Frame0: {} -> Delayed: {}) [Limit: {}] ***",
                          saved, ticksToFrames((int)saved),
                          it->down ? "PRESS" : "RELEASE",
                          it->madeTick, g_tick, bufferTicks());
                it = g_q.erase(it);
            } else {
                ++it;
            }
        }
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v5.1 LOADED (lookahead buffer, crash-safe hooks)");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2 -> 4 -> 8 -> 16 -> OFF");
    log::info("====================================================");
}
