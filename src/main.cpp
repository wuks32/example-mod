// Geode 5.x / GD 2.2081: practice macro tool (record, replay, freeze/step, checkpoint-aware)
// Keys: F6 record | F7 play | F8 stop+save | F9 freeze toggle | F10 single step | F11 load macro
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>

using namespace geode::prelude;

// ---------------------------------------------------------------- data

struct InputEvent {
    uint32_t step;
    uint8_t  button;   // 1 = jump (platformer: 2/3 = left/right)
    uint8_t  down;     // 0/1
    uint8_t  player2;  // 0/1
};

struct HashSample { uint32_t step; uint32_t hash; };

struct Macro {
    uint32_t formatVersion = 1;
    int32_t  levelId = 0;
    std::vector<InputEvent> events;
    std::vector<HashSample> hashes;   // desync detector samples
};

enum class Mode { Off, Record, Play };

struct CheckpointExtra { uint32_t step; };

// ---------------------------------------------------------------- engine

class Engine {
public:
    static Engine& get() { static Engine e; return e; }

    Mode     mode = Mode::Off;
    bool     frozen = false;
    bool     stepRequested = false;
    bool     tainted = false;      // any use of the tool taints the run
    uint32_t step = 0;
    size_t   evCursor = 0;
    size_t   hashCursor = 0;
    Macro    macro;
    std::unordered_map<CheckpointObject*, CheckpointExtra> cpExtra;

    static constexpr uint32_t kHashInterval = 16;
    static constexpr uint32_t kMagic = 0x4D435247; // "GRCM"

    // ---- helpers
    static uint32_t hashState(PlayerObject* p) {
        uint32_t h = 2166136261u;
        auto mix = [&](float f) {
            uint32_t b; std::memcpy(&b, &f, 4);
            h = (h ^ b) * 16777619u;
        };
        mix(p->getPositionX());
        mix(p->getPositionY());
        mix(static_cast<float>(p->m_yVelocity));
        return h;
    }

    void seekTo(uint32_t s) {
        step = s;
        auto it = std::lower_bound(macro.events.begin(), macro.events.end(), s,
            [](const InputEvent& e, uint32_t v) { return e.step < v; });
        evCursor = it - macro.events.begin();
        auto hit = std::lower_bound(macro.hashes.begin(), macro.hashes.end(), s,
            [](const HashSample& e, uint32_t v) { return e.step < v; });
        hashCursor = hit - macro.hashes.begin();
    }

    void truncateAfter(uint32_t s) {
        while (!macro.events.empty() && macro.events.back().step >= s) macro.events.pop_back();
        while (!macro.hashes.empty() && macro.hashes.back().step >= s) macro.hashes.pop_back();
    }

    void toast(char const* msg, NotificationIcon icon = NotificationIcon::Success) {
        Notification::create(msg, icon, 1.0f)->show();
    }

    // ---- control
    void startRecord() {
        macro = Macro{};
        macro.events.reserve(100000);
        if (auto pl = PlayLayer::get()) macro.levelId = pl->m_level->m_levelID;
        mode = Mode::Record; tainted = true;
        toast("Recording: restart the level");
    }
    void startPlay() {
        if (macro.events.empty()) { toast("No macro loaded", NotificationIcon::Error); return; }
        mode = Mode::Play; tainted = true;
        toast("Playback: restart the level");
    }
    void stop() { mode = Mode::Off; toast("Stopped"); }

    std::filesystem::path path() { return Mod::get()->getSaveDir() / "macro.bin"; }

    void save() {
        std::ofstream f(path(), std::ios::binary);
        if (!f) { toast("Save failed", NotificationIcon::Error); return; }
        uint32_t magic = kMagic, ver = macro.formatVersion;
        uint32_t ne = macro.events.size(), nh = macro.hashes.size();
        f.write((char*)&magic, 4); f.write((char*)&ver, 4);
        f.write((char*)&macro.levelId, 4);
        f.write((char*)&ne, 4); f.write((char*)&nh, 4);
        f.write((char*)macro.events.data(), ne * sizeof(InputEvent));
        f.write((char*)macro.hashes.data(), nh * sizeof(HashSample));
        toast("Macro saved");
    }

    void load() {
        std::ifstream f(path(), std::ios::binary);
        uint32_t magic = 0, ver = 0, ne = 0, nh = 0;
        Macro m;
        if (!f) { toast("No macro file", NotificationIcon::Error); return; }
        f.read((char*)&magic, 4); f.read((char*)&ver, 4);
        f.read((char*)&m.levelId, 4);
        f.read((char*)&ne, 4); f.read((char*)&nh, 4);
        if (!f || magic != kMagic || ver != m.formatVersion || ne > 5'000'000 || nh > 5'000'000) {
            toast("Bad macro file", NotificationIcon::Error); return;
        }
        m.events.resize(ne); m.hashes.resize(nh);
        f.read((char*)m.events.data(), ne * sizeof(InputEvent));
        f.read((char*)m.hashes.data(), nh * sizeof(HashSample));
        if (!f) { toast("Truncated macro file", NotificationIcon::Error); return; }
        macro = std::move(m);
        toast("Macro loaded");
    }

    // ---- per-step logic (called from the processCommands hook)
    void beginStep(GJBaseGameLayer* gl) {
        if (mode != Mode::Play) return;
        while (evCursor < macro.events.size() && macro.events[evCursor].step <= step) {
            auto const& e = macro.events[evCursor++];
            // Original function, bypasses our own hook so it doesn't re-record.
            gl->GJBaseGameLayer::handleButton(e.down != 0, e.button, e.player2 == 0);
        }
    }

    void endStep(GJBaseGameLayer* gl) {
        step++;
        if (step % kHashInterval != 0 || !gl->m_player1) return;
        uint32_t h = hashState(gl->m_player1);
        if (mode == Mode::Record) {
            macro.hashes.push_back({ step, h });
        } else if (mode == Mode::Play) {
            while (hashCursor < macro.hashes.size() && macro.hashes[hashCursor].step < step) hashCursor++;
            if (hashCursor < macro.hashes.size() && macro.hashes[hashCursor].step == step
                && macro.hashes[hashCursor].hash != h) {
                mode = Mode::Off;
                toast("DESYNC detected, playback stopped", NotificationIcon::Error);
                log::warn("desync at step {}", step);
            }
        }
    }

    void onFullReset()  { step = 0; evCursor = 0; hashCursor = 0; cpExtra.clear(); }
};

// ---------------------------------------------------------------- hooks

static bool isLivePlayLayer(GJBaseGameLayer* gl) {
    return PlayLayer::get() && static_cast<GJBaseGameLayer*>(PlayLayer::get()) == gl; // excludes editor
}

class $modify(ToolBaseLayer, GJBaseGameLayer) {
    // Single choke point for press/release (real clicks reach it when queued input is processed).
    void handleButton(bool down, int button, bool isPlayer1) { // VERIFY signature
        auto& e = Engine::get();
        if (e.mode == Mode::Record && isLivePlayLayer(this) && !static_cast<PlayLayer*>(PlayLayer::get())->m_isPaused) {
            e.macro.events.push_back({ e.step, (uint8_t)button, (uint8_t)down, (uint8_t)!isPlayer1 });
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    // One call per fixed physics step in 2.2+.
    void processCommands(float dt) { // VERIFY: may be processCommands(float, bool, bool) on 2.2081
        auto& e = Engine::get();
        if (!isLivePlayLayer(this)) { GJBaseGameLayer::processCommands(dt); return; }

        if (e.frozen) {
            if (!e.stepRequested) return;     // skip the whole simulation tick
            e.stepRequested = false;
        }
        e.beginStep(this);
        GJBaseGameLayer::processCommands(dt);
        e.endStep(this);
    }
};

class $modify(ToolPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        auto& e = Engine::get();

        uint32_t target = 0;
        bool fromCheckpoint = false;
        if (m_isPracticeMode && m_checkpointArray && m_checkpointArray->count() > 0) {
            auto cp = static_cast<CheckpointObject*>(m_checkpointArray->lastObject());
            if (auto it = e.cpExtra.find(cp); it != e.cpExtra.end()) {
                target = it->second.step;
                fromCheckpoint = true;
            }
        }
        if (!fromCheckpoint) e.cpExtra.clear();

        if (e.mode == Mode::Record) e.truncateAfter(target); // drop "future" inputs after a rewind
        e.seekTo(target);
        // NOTE: held-button state at a checkpoint is not reconciled here. Record
        // from a released state, or extend CheckpointExtra with the held flags.
    }

    CheckpointObject* createCheckpoint() { // VERIFY: may differ per version
        auto cp = PlayLayer::createCheckpoint();
        if (cp) Engine::get().cpExtra[cp] = { Engine::get().step };
        return cp;
    }

    void removeCheckpoint(bool last) {
        if (m_checkpointArray && m_checkpointArray->count() > 0) {
            auto cp = static_cast<CheckpointObject*>(
                last ? m_checkpointArray->lastObject() : m_checkpointArray->objectAtIndex(0));
            Engine::get().cpExtra.erase(cp);
        }
        PlayLayer::removeCheckpoint(last);
    }

    void levelComplete() {
        // Tainted runs must never be submittable. Test mode makes the game treat the
        // completion as non-recordable. VERIFY this behaviour on 2.2081.
        if (Engine::get().tainted) m_isTestMode = true;
        PlayLayer::levelComplete();
    }

    void onQuit() {
        auto& e = Engine::get();
        e.onFullReset();
        e.mode = Mode::Off;
        e.frozen = false;
        e.tainted = false;   // the next level starts clean
        PlayLayer::onQuit();
    }
};

// Keybinds (only active while a level is open)
class $modify(ToolKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) { // VERIFY: older versions have no timestamp
        if (down && !repeat && PlayLayer::get()) {
            auto& e = Engine::get();
            switch (key) {
                case KEY_F6:  e.startRecord(); break;
                case KEY_F7:  e.startPlay();   break;
                case KEY_F8:  if (e.mode == Mode::Record) e.save(); e.stop(); break;
                case KEY_F9:  e.frozen = !e.frozen; e.tainted = true;
                              e.toast(e.frozen ? "Frozen" : "Unfrozen"); break;
                case KEY_F10: if (e.frozen) e.stepRequested = true; break;
                case KEY_F11: e.load(); break;
                default: break;
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
z
