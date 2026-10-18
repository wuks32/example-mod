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

struct Stage {
    int all;
    int wave;
    const char* name;
};

static const Stage STAGES[] = {
    {2,   2,   "STAGE 1"},
    {4,   4,   "STAGE 2"},
    {8,   8,   "STAGE 3"},
    {16,  8,   "STAGE 4"},
    {32,  32,  "STAGE 5"},
    {64,  64,  "STAGE 6"},
    {128, 128, "STAGE 7"},
    {0,   0,   "STAGE 8"},
};
static constexpr int STAGE_COUNT = 8;
static int g_stage = 0;

static constexpr float STEP_DT = 0.25f;
// A cube is in the air ~0.4s. 120 steps is long enough to see it land on the spikes.
static constexpr int CUBE_HORIZON = 120;
// How early a cube click can be and still be held. ~0.4s, same feel as the wave clip.
static constexpr int CUBE_LIMIT = 96;
static constexpr int NEED = 4;
static constexpr double MAX_DECISION_MS = 14.0;

struct Ev {
    bool down;
    int button;
    bool p1;
    long fireTick;
    long madeTick;
};

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
static float g_groundY = 0.f;
static bool  g_groundYValid = false;
static double g_jumpVel = 0.0;
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

static bool cubeLike(PlayerObject* p) {
    return p && !p->m_isShip && !p->m_isBall && !p->m_isBird && !p->m_isDart
        && !p->m_isSpider && !p->m_isSwing;
}

static bool onGround(PlayerObject* p) {
    return p && (p->m_isOnGround || p->m_isOnGround2 || p->m_isOnGround3 ||
                 p->m_isOnGround4 || p->m_isOnSlope);
}

static bool touchKills(PlayerObject* p) {
    return p && (p->m_isDart || p->m_isShip || p->m_isBird || p->m_isSwing);
}

static bool hazardId(int id) {
    switch (id) {
        case 8: case 39: case 103: case 392:
        case 9: case 61: case 243: case 244:
        case 88: case 89: case 98:
        case 135: case 145: case 205: case 216: case 217: case 459:
        case 1715: case 1716: case 1717: case 1718:
        case 1719: case 1720: case 1721: case 1731:
            return true;
        default:
            return false;
    }
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
        g_groundYValid = false;
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

    bool isWave() { return m_player1 && (m_player1->m_isDart || g_waveStreak >= 3); }

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

    void hideClone(PlayerObject* p) {
        if (!p) return;
        p->setVisible(false);
        p->setOpacity(0);
        p->m_playEffects = false;
        if (p->m_waveTrail) p->m_waveTrail->setVisible(false);
        if (p->m_regularTrail) p->m_regularTrail->setVisible(false);
        if (p->m_shipStreak) p->m_shipStreak->setVisible(false);
    }

    void bury(PlayerObject* p) {
        if (!p) return;
        hideClone(p);
        p->stopParticles();
        p->setPosition({-100000.f, -100000.f});
    }

    void wake(PlayerObject* p) {
        if (!p) return;
        p->m_isDead = false;
        p->m_ignoreDamage = false;
        p->m_playEffects = false;
    }

    void syncGroundState(PlayerObject* dst, PlayerObject* src) {
        if (!dst || !src) return;
        dst->m_isOnGround  = src->m_isOnGround;
        dst->m_isOnGround2 = src->m_isOnGround2;
        dst->m_isOnGround3 = src->m_isOnGround3;
        dst->m_isOnGround4 = src->m_isOnGround4;
        dst->m_isOnSlope   = src->m_isOnSlope;
    }

    void syncHold(PlayerObject* p, bool held) {
        p->m_jumpBuffered = held;
        p->m_holdingButtons[(int)PlayerButton::Jump] = held;
    }

    void ensureCubeJump(PlayerObject* p, bool wasGrounded) {
        if (!wasGrounded || !cubeLike(p)) return;
        double mag = std::fabs(g_jumpVel);
        if (mag < 1.0) return;
        double sign = p->m_isUpsideDown ? -1.0 : 1.0;
        if (sign > 0.0 && p->m_yVelocity < mag * 0.5) p->m_yVelocity = mag;
        if (sign < 0.0 && p->m_yVelocity > -mag * 0.5) p->m_yVelocity = -mag;
        p->m_isOnGround = false;
        p->m_jumpBuffered = true;
    }

    void applyEdge(PlayerObject* p, bool& held, bool down) {
        if (down == held) {
            syncHold(p, held);
            return;
        }
        if (down) {
            bool grounded = onGround(p);
            p->m_holdingButtons[(int)PlayerButton::Jump] = false;
            p->m_jumpBuffered = false;
            if (grounded) {
                p->m_isOnGround = true;
                p->m_isOnGround2 = true;
            }
            p->pushButton(PlayerButton::Jump);
            p->m_jumpBuffered = true;
            ensureCubeJump(p, grounded);
        } else {
            p->m_holdingButtons[(int)PlayerButton::Jump] = true;
            p->m_jumpBuffered = true;
            p->releaseButton(PlayerButton::Jump);
        }
        held = down;
    }

    bool hazardTouch(PlayerObject* p) {
        float scale = p->m_vehicleSize > 0.1f ? p->m_vehicleSize : 1.f;
        float half = 6.5f * scale;
        CCPoint c = p->getPosition();
        CCRect r(c.x - half, c.y - half, half * 2.f, half * 2.f);

        for (auto* obj : m_activeObjects) {
            if (!obj) continue;
            if (std::fabs(obj->getPositionX() - c.x) > 80.f) continue;
            if (std::fabs(obj->getPositionY() - c.y) > 80.f) continue;
            if (!hazardId(obj->m_objectID) && obj->m_objectType != GameObjectType::Hazard) continue;
            CCRect s = obj->getObjectRect();
            float ix = s.size.width * 0.3f;
            float iy = s.size.height * 0.3f;
            s.origin.x += ix;
            s.origin.y += iy;
            s.size.width -= ix * 2.f;
            s.size.height -= iy * 2.f;
            if (s.size.width < 2.f || s.size.height < 2.f) continue;
            if (r.intersectsRect(s)) return true;
        }
        return false;
    }

    bool stepFake(PlayerObject* p, bool snapToFloor) {
        if (p->m_collisionLogTop)    p->m_collisionLogTop->removeAllObjects();
        if (p->m_collisionLogBottom) p->m_collisionLogBottom->removeAllObjects();
        if (p->m_collisionLogLeft)   p->m_collisionLogLeft->removeAllObjects();
        if (p->m_collisionLogRight)  p->m_collisionLogRight->removeAllObjects();
        p->m_playEffects = false;
        p->m_ignoreDamage = false;

        float x0 = p->getPositionX();
        g_fakeDead = false;
        g_simulating = true;
        p->update(STEP_DT);
        int hit = GJBaseGameLayer::checkCollisions(p, STEP_DT, true);
        g_simulating = false;

        // Classic cube X is advanced by the layer, not by PlayerObject::update.
        // Wave already moves itself, so this only fills in a missing step.
        float moved = p->getPositionX() - x0;
        if (g_dx > 0.4f && moved < g_dx * 0.5f)
            p->setPositionX(x0 + g_dx);

        if (snapToFloor && g_groundYValid && cubeLike(p) && p->m_yVelocity <= 0.0) {
            if (p->getPositionY() < g_groundY - 0.5f) {
                p->setPositionY(g_groundY);
                p->m_yVelocity = 0.0;
                p->m_isOnGround = true;
                p->m_isOnGround2 = true;
            }
        }

        bool spiked = hazardTouch(p);
        hideClone(p);

        bool died = g_fakeDead || p->m_isDead || spiked;
        if (!died && hit != 0 && touchKills(p)) died = true;
        if (died) {
            g_fakeDead = true;
            p->m_isDead = true;
        }
        return died;
    }

    int trial(const Ev& ev, bool heldPre, int horizon, bool cubePress) {
        g_fakeB->loadFromCheckpoint(g_cpT);
        wake(g_fakeB);
        syncGroundState(g_fakeB, g_fakeA);
        bool held = heldPre;
        applyEdge(g_fakeB, held, ev.down);
        int lived = 0;
        for (; lived < horizon; lived++) {
            if (stepFake(g_fakeB, false)) return lived;
        }
        return horizon;
    }

    void decide(Ev ev) {
        bool wave = isWave();
        bool cubePress = !wave && ev.down && cubeLike(m_player1);
        int stageLimit = wave ? STAGES[g_stage].wave : STAGES[g_stage].all;
        int limit = cubePress
            ? (stageLimit >= 64 ? stageLimit : std::max(stageLimit, CUBE_LIMIT))
            : stageLimit;
        int horizon = cubePress ? CUBE_HORIZON
                    : wave      ? std::min(24, std::max(12, stageLimit + 8))
                                : std::min(12, std::max(8, stageLimit + 4));
        double budget = limit >= 64 ? 40.0 : MAX_DECISION_MS;

        if (!ensureFakes()) {
            applyNow(ev.down, ev.button, ev.p1);
            return;
        }

        auto* pl = PlayLayer::get();
        bool savedIgnore = false;
        bool savedEnabled = false;
        if (pl) {
            savedIgnore = pl->m_ignoreDamage;
            savedEnabled = pl->m_isIgnoreDamageEnabled;
            pl->m_ignoreDamage = false;
            pl->m_isIgnoreDamageEnabled = false;
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
        syncGroundState(g_fakeA, m_player1);
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
                bool jumpable = !cubePress || onGround(g_fakeA);
                if (jumpable) {
                    g_fakeA->saveToCheckpoint(g_cpT);
                    int life = trial(ev, heldPre, horizon, cubePress);
                    tried += (life >= horizon) ? 'S' : 'x';
                    if (life > bestLife) {
                        bestLife = life;
                        bestT = t;
                    }
                    if ((wave || cubePress) && life >= horizon) {
                        chosen = t;
                        break;
                    }
                    if (!wave && !cubePress && life >= NEED) {
                        chosen = t;
                        break;
                    }
                } else {
                    tried += '.';
                }
            }

            if (stepFake(g_fakeA, true)) { tried += '!'; break; }
            if (ms() > budget) { timedOut = true; break; }
        }

        for (auto& f : saved) {
            f.o->m_activated = f.a;
            f.o->m_activatedByPlayer1 = f.b;
            f.o->m_activatedByPlayer2 = f.c;
        }
        if (pl) {
            pl->m_ignoreDamage = savedIgnore;
            pl->m_isIgnoreDamageEnabled = savedEnabled;
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
        bool grounded = onGround(m_player1);
        if (grounded) {
            g_groundY = m_player1->getPositionY();
            g_groundYValid = true;
        } else if (std::fabs(m_player1->m_yVelocity) > 1.0) {
            if (std::fabs(m_player1->m_yVelocity) > std::fabs(g_jumpVel))
                g_jumpVel = m_player1->m_yVelocity;
        }

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
    log::info("[InputBuffer] v10.7 LOADED");
    log::info("[InputBuffer] F6: 2 -> 4 -> 8 -> 16/8 -> 32 -> 64 -> 128 -> OFF");
    log::info("====================================================");
}
