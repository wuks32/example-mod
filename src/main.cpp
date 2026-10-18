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

// Survive this many ticks and the click is good enough. Don't wait for a later, "safer" one.
static constexpr int NEED = 4;
static constexpr double MAX_DECISION_MS = 6.0;

struct Ev { bool down; int button; bool p1; long fireTick; long madeTick; };

static std::vector<Ev> g_in;
static std::vector<Ev> g_q;
static long  g_tick = 0;
static bool  g_fire = false;
static bool  g_held = false;
static bool  g_simulating = false;
static bool  g_inFlip = false;
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

static const char* modeName(PlayerObject* p) {
    if (!p) return "NONE";
    if (p->m_isDart) return "WAVE";
    if (p->m_isShip) return "SHIP";
    if (p->m_isBird) return "UFO";
    if (p->m_isBall) return "BALL";
    if (p->m_isRobot) return "ROBOT";
    if (p->m_isSpider) return "SPIDER";
    if (p->m_isSwing) return "SWING";
    return "CUBE";
}

// Cube and robot jump off a surface. Everyone else acts on the press itself.
static bool cubeLike(PlayerObject* p) {
    return p && !p->m_isShip && !p->m_isBall && !p->m_isBird && !p->m_isDart
        && !p->m_isSpider && !p->m_isSwing;
}

static bool onGround(PlayerObject* p) {
    return p->m_isOnGround || p->m_isOnGround2 || p->m_isOnGround3 || p->m_isOnGround4 || p->m_isOnSlope;
}

// Wave, ship, ufo and swing die when they touch a solid. Cube and ball do not.
static bool touchKills(PlayerObject* p) {
    return p->m_isDart || p->m_isShip || p->m_isBird || p->m_isSwing;
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

    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (g_simulating || isClone(player)) {
            if (player && !g_inFlip) {
                g_inFlip = true;
                player->flipGravity(flip, true);
                g_inFlip = false;
            }
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

    void hush(PlayerObject* p) {
        if (!p) return;
        p->setVisible(false);
        p->setOpacity(0);
        p->m_playEffects = false;
        p->m_isHidden = true;
        if (p->m_waveTrail) p->m_waveTrail->setVisible(false);
        if (p->m_regularTrail) p->m_regularTrail->setVisible(false);
        if (p->m_shipStreak) p->m_shipStreak->setVisible(false);
    }

    void bury(PlayerObject* p) {
        if (!p) return;
        hush(p);
        p->stopParticles();
        p->setPosition({-100000.f, -100000.f});
    }

    void wake(PlayerObject* p) {
        if (!p) return;
        p->m_isDead = false;
        p->m_ignoreDamage = false;
        hush(p);
    }

    void syncHold(PlayerObject* p, bool held) {
        p->m_jumpBuffered = held;
        p->m_holdingButtons[(int)PlayerButton::Jump] = held;
    }

    void applyEdge(PlayerObject* p, bool& held, bool down) {
        if (down == held) {
            syncHold(p, held);
            return;
        }
        if (down) {
            p->m_holdingButtons[(int)PlayerButton::Jump] = false;
            p->pushButton(PlayerButton::Jump);
        } else {
            p->m_holdingButtons[(int)PlayerButton::Jump] = true;
            p->m_jumpBuffered = true;
            p->releaseButton(PlayerButton::Jump);
        }
        held = down;
    }

    bool stepFake(PlayerObject* p) {
        const float dt = 0.25f;
        if (p->m_collisionLogTop) p->m_collisionLogTop->removeAllObjects();
        if (p->m_collisionLogBottom) p->m_collisionLogBottom->removeAllObjects();
        if (p->m_collisionLogLeft) p->m_collisionLogLeft->removeAllObjects();
        if (p->m_collisionLogRight) p->m_collisionLogRight->removeAllObjects();
        hush(p);

        g_fakeDead = false;
        g_simulating = true;
        p->update(dt);
        int hit = GJBaseGameLayer::checkCollisions(p, dt, false);
        g_simulating = false;
        hush(p);

        bool died = g_fakeDead || p->m_isDead;
        if (!died && hit) {
            if (touchKills(p) || !onGround(p)) died = true;
        }
        if (died) g_fakeDead = true;
        return died;
    }

    int trial(const Ev& ev, bool heldPre, int horizon) {
        g_fakeB->loadFromCheckpoint(g_cpT);
        wake(g_fakeB);
        bool held = heldPre;
        applyEdge(g_fakeB, held, ev.down);
        int lived = 0;
        for (; lived < horizon; lived++) {
            if (stepFake(g_fakeB)) return lived;
        }
        return horizon;
    }

    void decide(Ev ev) {
        int limit = isWave() ? STAGES[g_stage].wave : STAGES[g_stage].all;
        int horizon = std::min(12, std::max(8, limit + 4));

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
        static std::vector<Flags> saved;
        saved.clear();
        for (auto obj : m_activeObjects) {
            if (!obj) continue;
            auto eo = typeinfo_cast<EnhancedGameObject*>(obj);
            if (!eo) continue;
            saved.push_back({eo, eo->m_activated, eo->m_activatedByPlayer1, eo->m_activatedByPlayer2});
        }

        m_player1->saveToCheckpoint(g_cpReal);
        g_fakeA->loadFromCheckpoint(g_cpReal);
        wake(g_fakeA);
        bool heldPre = g_held;
        syncHold(g_fakeA, heldPre);

        int bestT = -1;
        int bestLife = -1;
        int chosen = -1;
        bool timedOut = false;
        std::string tried;

        for (int t = 0; t <= limit; t++) {
            for (auto& q : g_q) {
                if (q.fireTick - g_tick == t) applyEdge(g_fakeA, heldPre, q.down);
            }

            if (t >= startDelay) {
                g_fakeA->saveToCheckpoint(g_cpT);
                int life = trial(ev, heldPre, horizon);
                tried += (life >= NEED) ? 'S' : 'x';
                if (life > bestLife) {
                    bestLife = life;
                    bestT = t;
                }
                // Cube/robot: an air click is not the click. Wait for the surface.
                bool wantGround = ev.down && cubeLike(g_fakeA);
                if (life >= NEED && (!wantGround || onGround(g_fakeA))) {
                    chosen = t;
                    break;
                }
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
        bury(g_fakeA);
        bury(g_fakeB);

        if (chosen < 0) chosen = bestT;

        log::info("[InputBuffer] SIM {} mode={} limit={} delays: {} pick={} lived={}/{} ({:.1f} ms){}",
                  ev.down ? "PRESS" : "RELEASE", modeName(m_player1), limit,
                  tried, chosen, std::max(0, bestLife), horizon, ms(), timedOut ? " TIMEOUT" : "");

        if (chosen < 0) {
            if (startDelay == 0) applyNow(ev.down, ev.button, ev.p1);
            else {
                ev.madeTick = g_tick;
                ev.fireTick = g_tick + startDelay;
                g_q.push_back(ev);
            }
            return;
        }
        if (chosen == 0 && startDelay == 0) {
            applyNow(ev.down, ev.button, ev.p1);
            return;
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
    log::info("[InputBuffer] v9.0 LOADED (short delay, all modes, hidden clones)");
    log::info("[InputBuffer] Starting Mode: STAGE 1 (2 All / 2 Wave)");
    log::info("[InputBuffer] Press F6 in-level to cycle: 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF");
    log::info("====================================================");
}
