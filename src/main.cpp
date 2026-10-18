#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

// ---------- Stages (1 tick = 0.25 frames @ 60fps, 240 TPS) ----------
struct Stage { int all; int wave; const char* name; };
static const Stage STAGES[] = {
    {2,  2, "STAGE 1"},
    {4,  4, "STAGE 2"},
    {8,  8, "STAGE 3"},
    {16, 8, "STAGE 4"},
    {0,  0, "STAGE 5"},
};
static constexpr int STAGE_COUNT = 5;
static int g_stage = 0;

static std::string stageText(const Stage& s) {
    if (s.all == 0) return fmt::format("{}: OFF (100% Raw Input)", s.name);
    float a = s.all / 4.f, w = s.wave / 4.f;
    std::string fr = (s.all == s.wave)
        ? fmt::format("{:.1f}f @ 60fps", a)
        : fmt::format("{:.1f}f / {:.1f}f @ 60fps", a, w);
    return fmt::format("{}: {} Ticks All | {} Ticks Wave ({})", s.name, s.all, s.wave, fr);
}

static constexpr int LOOKAHEAD = 10;

// ---------- Shared state ----------
struct Ev   { bool down; int button; bool p1; long fireTick; long madeTick; };
struct Land { int button; bool p1; long startTick; int limit; bool releaseQueued; };

static std::vector<Ev>   g_in;
static std::vector<Ev>   g_q;
static std::vector<Land> g_land;
static long  g_tick = 0;
static bool  g_fire = false;
static bool  g_held = false;
static float g_lastX = 0.f, g_lastY = 0.f;
static float g_dx = 0.f, g_dy = 0.f;
static int   g_waveStreak = 0;     // consecutive ticks moving in a clean wave line

// ---------- F6 cycling ----------
class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            log::info("[InputBuffer] F6 Cycle -> {}", stageText(STAGES[g_stage]));
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

// ---------- Reset ----------
class $modify(BufferPlay, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        g_in.clear(); g_q.clear(); g_land.clear();
        g_tick = 0; g_held = false;
        g_lastX = g_lastY = 0.f; g_dx = g_dy = 0.f;
        g_waveStreak = 0;
    }
};

// ---------- Buffer logic ----------
class $modify(BufferLayer, GJBaseGameLayer) {

    bool bufferActive() {
        if (STAGES[g_stage].all == 0) return false;
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return false;
        if (!m_player1 || m_player1->m_isDead) return false;
        return true;
    }

    bool isWave() {
        return m_player1->m_isDart || g_waveStreak >= 3;
    }

    void applyNow(bool down, int button, bool p1) {
        g_fire = true;
        GJBaseGameLayer::handleButton(down, button, p1);
        g_fire = false;
        if (button == 1 && p1) g_held = down;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (g_fire || button != 1 || !isPlayer1 || !bufferActive()) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            if (!g_fire && button == 1 && isPlayer1) g_held = down;
            return;
        }
        g_in.push_back({down, button, isPlayer1, g_tick, g_tick});
    }

    // ---------- WAVE ----------
    void decideWave(Ev ev) {
        int limit = STAGES[g_stage].wave;
        CCPoint p = m_player1->getPosition();
        float dx = g_dx > 0.01f ? g_dx : (m_player1->m_playerSpeed * 346.2f / 240.f);
        float size = m_player1->m_vehicleSize > 0.f ? m_player1->m_vehicleSize : 1.f;
        float slope = size < 1.f ? 2.f : 1.f;
        float half = 4.5f * size;
        bool upsideDown = m_player1->m_isUpsideDown;

        float reachX = dx * (limit + LOOKAHEAD + 2) + 60.f;
        float reachY = reachX * slope + 60.f;
        std::vector<CCRect> boxes;
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            auto t = obj->m_objectType;
            if (t != GameObjectType::Solid && t != GameObjectType::Hazard) continue;
            auto op = obj->getPosition();
            if (std::fabs(op.x - p.x) > reachX || std::fabs(op.y - p.y) > reachY) continue;
            boxes.push_back(obj->getObjectRect());
        }

        long minTick = g_tick;
        for (auto& q : g_q) minTick = std::max(minTick, q.fireTick);
        int startDelay = (int)(minTick - g_tick);

        bool curHeld = g_held;
        auto pathSafe = [&](int d) -> bool {
            float x = p.x, y = p.y;
            for (int t = 1; t <= d + LOOKAHEAD; t++) {
                bool held = curHeld;
                for (auto& q : g_q)
                    if (q.fireTick - g_tick < t) held = q.down;
                if (t > d) held = ev.down;
                bool up = (held != upsideDown);
                x += dx;
                y += (up ? 1.f : -1.f) * dx * slope;
                CCRect w(x - half, y - half, half * 2.f, half * 2.f);
                for (auto& b : boxes)
                    if (b.intersectsRect(w)) return false;
            }
            return true;
        };

        int chosen = -1;
        std::string tried;
        for (int d = startDelay; d <= limit; d++) {
            bool ok = pathSafe(d);
            tried += ok ? "S" : "x";
            if (ok) { chosen = d; break; }
        }
        log::info("[InputBuffer] WAVE {} dx={:.2f} boxes={} delays tried: {}",
                  ev.down ? "PRESS" : "RELEASE", dx, boxes.size(), tried);

        if (chosen == 0 && startDelay == 0) {
            applyNow(ev.down, ev.button, ev.p1);
            return;
        }
        if (chosen < 0) {
            if (startDelay == 0) {
                applyNow(ev.down, ev.button, ev.p1);
                return;
            }
            chosen = startDelay;
        }
        ev.fireTick = g_tick + chosen;
        g_q.push_back(ev);
        log::info("[InputBuffer] BUFFERED {} @ tick {} -> fires in {} ticks",
                  ev.down ? "PRESS" : "RELEASE", g_tick, chosen);
    }

    // ---------- Modes that can land (cube, robot, spider, ball) ----------
    void decideGround(Ev ev) {
        int limit = STAGES[g_stage].all;

        // Ship / UFO / swing never "land": no landing buffer for them.
        bool canLand = !(m_player1->m_isShip || m_player1->m_isBird || m_player1->m_isSwing);
        if (!canLand) {
            applyNow(ev.down, ev.button, ev.p1);
            return;
        }

        if (!ev.down) {
            for (auto& l : g_land)
                if (l.button == ev.button && l.p1 == ev.p1) { l.releaseQueued = true; return; }
            applyNow(ev.down, ev.button, ev.p1);
            return;
        }
        applyNow(ev.down, ev.button, ev.p1);
        if (!m_player1->m_isOnGround) {
            g_land.push_back({ev.button, ev.p1, g_tick, limit, false});
            log::info("[InputBuffer] BUFFERED click @ tick {} (waiting up to {} ticks)",
                      g_tick, limit);
        }
    }

    void flushAllRaw() {
        for (auto& e : g_in) applyNow(e.down, e.button, e.p1);
        for (auto& e : g_q)  applyNow(e.down, e.button, e.p1);
        for (auto& l : g_land) if (l.releaseQueued) applyNow(false, l.button, l.p1);
        g_in.clear(); g_q.clear(); g_land.clear();
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        if (isHalfTick) return;

        g_tick++;
        if (m_player1) {
            CCPoint pos = m_player1->getPosition();
            g_dx = pos.x - g_lastX;
            g_dy = pos.y - g_lastY;
            g_lastX = pos.x;
            g_lastY = pos.y;

            // Wave-like movement: constant 45 deg (or mini ~63 deg) line.
            bool clean = false;
            if (g_dx > 0.01f) {
                float r = std::fabs(g_dy) / g_dx;
                clean = (std::fabs(r - 1.f) < 0.03f) || (std::fabs(r - 2.f) < 0.05f);
            }
            g_waveStreak = clean ? g_waveStreak + 1 : 0;
        }

        if (!bufferActive()) {
            if (!g_in.empty() || !g_q.empty() || !g_land.empty()) flushAllRaw();
            return;
        }

        if (!g_in.empty()) {
            std::vector<Ev> work;
            work.swap(g_in);
            bool wave = isWave();
            log::info("[InputBuffer] click: mode={} isDart={} streak={} onGround={}",
                      wave ? "WAVE" : "OTHER", m_player1->m_isDart,
                      g_waveStreak, m_player1->m_isOnGround);
            for (auto& ev : work) {
                if (wave) decideWave(ev);
                else      decideGround(ev);
            }
        }

        for (auto it = g_q.begin(); it != g_q.end();) {
            if (it->fireTick <= g_tick) {
                applyNow(it->down, it->button, it->p1);
                long saved = g_tick - it->madeTick;
                log::info("[InputBuffer] *** SAVED +{} TICKS ({:.2f} frames @ 60fps) {}! "
                          "(Frame0: {} -> Delayed: {}) [Limit: {}] ***",
                          saved, saved / 4.f, it->down ? "PRESS" : "RELEASE",
                          it->madeTick, g_tick, STAGES[g_stage].wave);
                it = g_q.erase(it);
            } else ++it;
        }

        for (auto it = g_land.begin(); it != g_land.end();) {
            PlayerObject* pl = it->p1 ? m_player1 : m_player2;
            long elapsed = g_tick - it->startTick;
            bool done = false;
            if (pl && pl->m_isOnGround) {
                applyNow(true, it->button, it->p1);
                if (it->releaseQueued) applyNow(false, it->button, it->p1);
                log::info("[InputBuffer] *** SAVED +{} TICKS ({:.2f} frames @ 60fps) PRESS! "
                          "(Frame0: {} -> Delayed: {}) [Limit: {}] ***",
                          elapsed, elapsed / 4.f, it->startTick, g_tick, it->limit);
                done = true;
            } else if (elapsed >= it->limit) {
                if (it->releaseQueued) applyNow(false, it->button, it->p1);
                log::info("[InputBuffer] EXPIRED (never landed within {} ticks)", it->limit);
                done = true;
            }
            if (done) it = g_land.erase(it); else ++it;
        }
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v5.3 LOADED (wave detection fix)");
    log::info("[InputBuffer] Starting Mode: STAGE 1 (2 All / 2 Wave)");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF");
    log::info("====================================================");
}
