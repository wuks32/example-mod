#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>
#include <algorithm>

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

// How far ahead (in ticks) we check that you survive. 12 ticks = 3 frames.
static constexpr int LOOKAHEAD = 12;

// ---------- Shared state ----------
struct Ev { bool down; int button; bool p1; long fireTick; long madeTick; };
static std::vector<Ev> g_q;     // delayed inputs waiting to fire
static long g_tick = 0;         // real tick counter
static bool g_sim = false;      // inside a lookahead simulation
static bool g_fire = false;     // we are firing an input ourselves
static bool g_simDead = false;  // player died during simulation

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
        if (g_sim) {            // simulated death: just record it, don't actually die
            g_simDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        g_q.clear();
        g_tick = 0;
    }
};

// ---------- Buffer logic ----------
class $modify(BufferLayer, GJBaseGameLayer) {

    bool bufferActive() {
        if (bufferTicks() == 0) return false;
        auto pl = PlayLayer::get();
        return pl && static_cast<GJBaseGameLayer*>(pl) == this;
    }

    // Simulate: rewind to cp, apply ev after `delay` ticks, run, check for death.
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

    void handleButton(bool down, int button, bool isPlayer1) {
        if (g_fire || g_sim || !bufferActive()) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto pl = PlayLayer::get();
        Ev ev{down, button, isPlayer1, g_tick, g_tick};

        // Keep input order: never schedule before an input that's already waiting.
        long minTick = g_tick;
        for (auto& q : g_q) minTick = std::max(minTick, q.fireTick);
        int startDelay = (int)(minTick - g_tick);
        int limit = bufferTicks();

        CheckpointObject* cp = pl->createCheckpoint();
        int chosen = -1;

        if (cp) {
            for (int d = startDelay; d <= limit; d++) {
                if (survives(pl, cp, ev, d)) { chosen = d; break; }
            }
            pl->loadFromCheckpoint(cp);   // restore the real game state
        }

        if (chosen <= 0 && startDelay == 0) {
            // Safe right now (or nothing safe found): behave like normal input.
            if (chosen < 0)
                log::info("[InputBuffer] {} @ tick {}: no safe delay within {} ticks, sending raw",
                          down ? "PRESS" : "RELEASE", g_tick, limit);
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        int use = chosen < 0 ? startDelay : chosen;
        ev.fireTick = g_tick + use;
        g_q.push_back(ev);
        log::info("[InputBuffer] BUFFERED {} @ tick {} -> fires in {} ticks",
                  down ? "PRESS" : "RELEASE", g_tick, use);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (g_sim || isHalfTick) return;   // simulation steps never touch real state
        g_tick++;

        if (g_q.empty()) return;

        if (!bufferActive()) { g_q.clear(); return; }

        g_fire = true;
        for (auto it = g_q.begin(); it != g_q.end();) {
            if (it->fireTick <= g_tick) {
                GJBaseGameLayer::handleButton(it->down, it->button, it->p1);
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
        g_fire = false;
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v5.0 LOADED (lookahead buffer, all gamemodes)");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2 -> 4 -> 8 -> 16 -> OFF");
    log::info("====================================================");
}
