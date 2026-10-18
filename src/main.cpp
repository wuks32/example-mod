#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <chrono>

using namespace geode::prelude;

// 1 tick = 0.25 frame @ 60fps (240 TPS). update() takes frames, so a tick is 0.25, not 1/240.
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

static constexpr int LOOKAHEAD = 24;
static constexpr double MAX_DECISION_MS = 12.0;

struct Ev { bool down; int button; bool p1; long fireTick; long madeTick; };

static std::vector<Ev> g_in;
static std::vector<Ev> g_q;
static long  g_tick = 0;
static bool  g_fire = false;
static bool  g_held = false;
static bool  g_simulating = false;
static bool  g_fakeDead = false;
static bool  g_hasPos = false;
static float g_lastX = 0.f, g_lastY = 0.f, g_dx = 0.f, g_dy = 0.f;
static int   g_waveStreak = 0;

static PlayerObject*     g_fakeA = nullptr;
static PlayerObject*     g_fakeB = nullptr;
static PlayerCheckpoint* g_cpReal = nullptr;
static PlayerCheckpoint* g_cpT = nullptr;
static GJBaseGameLayer*  g_owner = nullptr;

static bool isClone(PlayerObject* p) {
    return p && (p == g_fakeA || p == g_fakeB);
}

static std::string stageText(const Stage& s) {
    if (s.all == 0) return fmt::format("{}: OFF (100% Raw Input)", s.name);
    float a = s.all / 4.f, w = s.wave / 4.f;
    std::string fr = (s.all == s.wave)
        ? fmt::format("{:.1f}f @ 60fps", a)
        : fmt::format("{:.1f}f / {:.1f}f @ 60fps", a, w);
    return fmt::format("{}: {} Ticks All | {} Ticks Wave ({})", s.name, s.all, s.wave, fr);
}

static void dropFakes() {
    auto rel = [](CCNode* n) { if (n) n->release(); };
    rel(g_fakeA); rel(g_fakeB); rel(g_cpReal); rel(g_cpT);
    g_fakeA = g_fakeB = nullptr;
    g_cpReal = g_cpT = nullptr;
    g_owner = nullptr;
}

class $modify(BufferKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6) {
            g_stage = (g_stage + 1) % STAGE_COUNT;
            log::info("[InputBuffer] F6 Cycle -> {}", stageText(STAGES[g_stage]));
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

class $modify(BufferPlayer, PlayerObject) {
    void playerDestroyed(bool noEffects) {
        if (g_simulating || isClone(this)) {
            g_fakeDead = true;
            this->m_isDead = true;
            return;
        }
        PlayerObject::playerDestroyed(noEffects);
    }

    void playDeathEffect() {
        if (g_simulating || isClone(this)) return;
        PlayerObject::playDeathEffect();
    }
};

class $modify(BufferPlay, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        if (g_simulating || isClone(player)) {
            g_fakeDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        g_in.clear();
        g_q.clear();
        g_tick = 0;
        g_held = false;
        g_hasPos = false;
        g_lastX = g_lastY = g_dx = g_dy = 0.f;
        g_waveStreak = 0;
    }

    void onQuit() {
        g_in.clear();
        g_q.clear();
        dropFakes();
        PlayLayer::onQuit();
    }
};

class $modify(BufferLayer, GJBaseGameLayer) {
    bool bufferActive() {
        if (STAGES[g_stage].all == 0) return false;
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return false;
        if (!m_player1 || m_player1->m_isDead) return false;
        return true;
    }

    bool isWave() { return m_player1->m_isDart || g_waveStreak >= 3; }

    // Screen must not flip because a clone touched a portal.
    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (g_simulating || isClone(player)) {
            if (player) player->flipGravity(flip, true);
            return;
        }
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
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
        g_in.push_back({down, button, isPlayer1, 0, 0});
    }

    bool ensureFakes() {
        if (!m_objectLayer) return false;
        GJBaseGameLayer* self = this;
        if (g_owner != self || !g_fakeA || !g_fakeB || !g_cpReal || !g_cpT) {
            dropFakes();
            g_fakeA = PlayerObject::create(1, 1, this, m_objectLayer, true);
            g_fakeB = PlayerObject::create(1, 1, this, m_objectLayer, true);
            g_cpReal = PlayerCheckpoint::create();
            g_cpT = PlayerCheckpoint::create();
            for (auto n : {(CCNode*)g_fakeA, (CCNode*)g_fakeB, (CCNode*)g_cpReal, (CCNode*)g_cpT}) {
                if (n) n->retain();
            }
            g_owner = self;
        }
        return g_fakeA && g_fakeB && g_cpReal && g_cpT;
    }

    void wake(PlayerObject* p) {
        if (!p) return;
        p->m_isDead = false;
        p->m_playEffects = false;
        p->m_ignoreDamage = false;
        p->setVisible(false);
        p->setOpacity(0);
        if (p->m_waveTrail) p->m_waveTrail->setVisible(false);
    }

    void fakeInput(PlayerObject* p, bool down) {
        if (down) p->pushButton(PlayerButton::Jump);
        else      p->releaseButton(PlayerButton::Jump);
    }

    // One real 240hz tick. Returns true if the clone dies on this tick.
    bool stepFake(PlayerObject* p) {
        const float dt = 0.25f;
        if (p->m_collisionLogTop) p->m_collisionLogTop->removeAllObjects();
        if (p->m_collisionLogBottom) p->m_collisionLogBottom->removeAllObjects();
        if (p->m_collisionLogLeft) p->m_collisionLogLeft->removeAllObjects();
        if (p->m_collisionLogRight) p->m_collisionLogRight->removeAllObjects();
        p->m_playEffects = false;
        p->setVisible(false);

        g_fakeDead = false;
        g_simulating = true;
        p->update(dt);
        int hit = GJBaseGameLayer::checkCollisions(p, dt, false);
        g_simulating = false;

        p->setVisible(false);
        if (p->m_waveTrail) p->m_waveTrail->setVisible(false);
        if (g_fakeDead || p->m_isDead || hit != 0) {
            g_fakeDead = true;
            return true;
        }
        return false;
    }

    bool trial(const Ev& ev, bool heldPre) {
        g_fakeB->loadFromCheckpoint(g_cpT);
        wake(g_fakeB);
        fakeInput(g_fakeB, heldPre);
        fakeInput(g_fakeB, ev.down);
        for (int i = 0; i < LOOKAHEAD; i++) {
            if (stepFake(g_fakeB)) return false;
        }
        return true;
    }

    void decide(Ev ev) {
        int limit = isWave() ? STAGES[g_stage].wave : STAGES[g_stage].all;

        if (!ensureFakes()) {
            log::info("[InputBuffer] could not create clone players, raw input");
            applyNow(ev.down, ev.button, ev.p1);
            return;
        }

        auto t0 = std::chrono::steady_clock::now();
        auto ms = [&]() {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - t0).count();
        };

        long minTick = g_tick;
        for (auto& q : g_q) minTick = std::max(minTick, q.fireTick);
        int startDelay = (int)(minTick - g_tick);
        if (startDelay < 0) startDelay = 0;

        struct Flags { EnhancedGameObject* o; bool a; bool b; bool c; };
        std::vector<Flags> saved;
        float reach = (limit + LOOKAHEAD + 4) * std::max(g_dx, 1.f) + 120.f;
        float px = m_player1->getPositionX();
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (!obj) continue;
            if (std::fabs(obj->getPositionX() - px) > reach) continue;
            auto eo = typeinfo_cast<EnhancedGameObject*>(obj);
            if (!eo) continue;
            saved.push_back({eo, eo->m_activated, eo->m_activatedByPlayer1, eo->m_activatedByPlayer2});
        }

        m_player1->saveToCheckpoint(g_cpReal);
        g_fakeA->loadFromCheckpoint(g_cpReal);
        wake(g_fakeA);
        bool heldPre = g_held;
        fakeInput(g_fakeA, heldPre);

        int chosen = -1;
        bool timedOut = false;
        std::string tried;

        for (int t = 0; t <= limit; t++) {
            for (auto& q : g_q) {
                if (q.fireTick - g_tick == t) {
                    fakeInput(g_fakeA, q.down);
                    heldPre = q.down;
                }
            }

            if (t >= startDelay) {
                g_fakeA->saveToCheckpoint(g_cpT);
                bool ok = trial(ev, heldPre);
                tried += ok ? 'S' : 'x';
                if (ok) { chosen = t; break; }
            }

            if (stepFake(g_fakeA)) { tried += '!'; break; }
            if (ms() > MAX_DECISION_MS) { timedOut = true; break; }
        }

        for (auto& f : saved) {
            f.o->m_activated = f.a;
            f.o->m_activatedByPlayer1 = f.b;
            f.o->m_activatedByPlayer2 = f.c;
        }
        g_simulating = false;

        log::info("[InputBuffer] SIM {} mode={} limit={} delays: {} ({:.1f} ms){}",
                  ev.down ? "PRESS" : "RELEASE", isWave() ? "WAVE" : "OTHER", limit,
                  tried, ms(), timedOut ? " TIMEOUT" : "");

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
        ev.madeTick = g_tick;
        ev.fireTick = g_tick + chosen;
        g_q.push_back(ev);
        log::info("[InputBuffer] BUFFERED {} @ tick {} -> fires in {} ticks",
                  ev.down ? "PRESS" : "RELEASE", g_tick, chosen);
    }

    void flushAllRaw() {
        for (auto& e : g_in) applyNow(e.down, e.button, e.p1);
        for (auto& e : g_q)  applyNow(e.down, e.button, e.p1);
        g_in.clear();
        g_q.clear();
    }

    void fireDue() {
        for (auto it = g_q.begin(); it != g_q.end();) {
            if (it->fireTick <= g_tick) {
                applyNow(it->down, it->button, it->p1);
                long saved = g_tick - it->madeTick;
                int lim = isWave() ? STAGES[g_stage].wave : STAGES[g_stage].all;
                log::info("[InputBuffer] *** SAVED +{} TICKS ({:.2f} frames @ 60fps) {}! "
                          "(Frame0: {} -> Delayed: {}) [Limit: {}] ***",
                          saved, saved / 4.f, it->down ? "PRESS" : "RELEASE",
                          it->madeTick, g_tick, lim);
                it = g_q.erase(it);
            } else ++it;
        }
    }

    void sampleMotion() {
        if (!m_player1) return;
        CCPoint pos = m_player1->getPosition();
        if (!g_hasPos) {
            g_lastX = pos.x;
            g_lastY = pos.y;
            g_hasPos = true;
            g_dx = g_dy = 0.f;
            return;
        }
        g_dx = pos.x - g_lastX;
        g_dy = pos.y - g_lastY;
        g_lastX = pos.x;
        g_lastY = pos.y;
        bool clean = false;
        if (g_dx > 0.01f) {
            float r = std::fabs(g_dy) / g_dx;
            clean = (std::fabs(r - 1.f) < 0.03f) || (std::fabs(r - 2.f) < 0.05f);
        }
        g_waveStreak = clean ? g_waveStreak + 1 : 0;
    }

    // Queue the click BEFORE vanilla processQueuedButtons, or it misses this tick.
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (!isHalfTick) {
            g_tick++;
            sampleMotion();
            if (!bufferActive()) {
                if (!g_in.empty() || !g_q.empty()) flushAllRaw();
            } else {
                fireDue();
                if (!g_in.empty()) {
                    std::vector<Ev> work;
                    work.swap(g_in);
                    for (auto& ev : work) decide(ev);
                }
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

$on_mod(Loaded) {
    log::info("====================================================");
    log::info("[InputBuffer] v7.0 LOADED (clone trajectory, click queued before physics)");
    log::info("[InputBuffer] Starting Mode: STAGE 1 (2 All / 2 Wave)");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF");
    log::info("====================================================");
}
