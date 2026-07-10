#ifndef RAYLIB_AUDIOMANAGER_REAL_H
#define RAYLIB_AUDIOMANAGER_REAL_H

// Real AudioManager backed by Raylib's raudio module.
// Same public interface as src/AudioManager.h so GameState.cpp compiles
// unchanged. SFX are generated procedurally per-event (ported from
// SoundSynth.h's SFML synthesis logic) instead of a single generic beep.

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <random>
#include <algorithm>

namespace RaylibSoundSynth {

static constexpr int SAMPLE_RATE = 44100;

static inline float envelope(float t, float dur, float attack, float decay) {
    if (t < attack) return t / attack;
    if (t > dur - decay) return (dur - t) / decay;
    return 1.0f;
}

static inline float noisef(std::mt19937& rng) {
    static std::uniform_real_distribution<float> d(-1.f, 1.f);
    return d(rng);
}

static Sound bufferToSound(const std::vector<float>& samples) {
    std::vector<short> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float s = std::max(-1.f, std::min(1.f, samples[i]));
        pcm[i] = (short)(s * 32767.f);
    }
    Wave wave = { 0 };
    wave.frameCount = (unsigned int)pcm.size();
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = pcm.data();
    Sound s = LoadSoundFromWave(wave); // copies data internally
    return s;
}

static Sound makeFootstep() {
    std::mt19937 rng(42);
    float dur = 0.12f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float env = std::exp(-t * 40.f);
        float thud = std::sin(2.f * PI * 120.f * t) * 0.5f + noisef(rng) * 0.5f;
        s[i] = thud * env * 0.6f;
    }
    return bufferToSound(s);
}

static Sound makePistol() {
    std::mt19937 rng(1);
    float dur = 0.25f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack = noisef(rng) * std::exp(-t * 80.f);
        float body  = std::sin(2.f * PI * 80.f * t) * std::exp(-t * 25.f);
        float tail  = noisef(rng) * std::exp(-t * 12.f) * 0.15f;
        s[i] = (crack * 0.7f + body * 0.5f + tail) * 0.9f;
    }
    return bufferToSound(s);
}

static Sound makeShotgun() {
    std::mt19937 rng(2);
    float dur = 0.55f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float blast = noisef(rng) * std::exp(-t * 30.f);
        float boom  = std::sin(2.f * PI * 55.f * t) * std::exp(-t * 15.f);
        float tail  = noisef(rng) * std::exp(-t * 6.f) * 0.2f;
        s[i] = (blast * 0.8f + boom * 0.6f + tail) * 0.95f;
    }
    return bufferToSound(s);
}

static Sound makeExplosion() {
    std::mt19937 rng(3);
    float dur = 1.2f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack  = noisef(rng) * std::exp(-t * 60.f);
        float rumble = noisef(rng) * std::exp(-t * 4.f) * 0.5f;
        float freq   = 80.f * std::exp(-t * 3.f);
        float boom   = std::sin(2.f * PI * freq * t) * std::exp(-t * 5.f);
        s[i] = (crack * 0.6f + rumble * 0.5f + boom * 0.7f) * 0.95f;
    }
    return bufferToSound(s);
}

static Sound makeGrenadeLand() {
    std::mt19937 rng(4);
    float dur = 0.18f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float env  = std::exp(-t * 35.f);
        float clink = std::sin(2.f * PI * 1200.f * t) * 0.5f
                    + std::sin(2.f * PI * 1800.f * t) * 0.3f
                    + noisef(rng) * 0.2f;
        s[i] = clink * env * 0.7f;
    }
    return bufferToSound(s);
}

static Sound makeMinePlant() {
    std::mt19937 rng(5);
    float dur = 0.22f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float click = noisef(rng) * std::exp(-t * 120.f) * 0.8f;
        float bt = t - 0.05f;
        float beep = (bt > 0.f) ? std::sin(2.f * PI * 880.f * bt) * std::exp(-bt * 20.f) * 0.5f : 0.f;
        s[i] = click + beep;
    }
    return bufferToSound(s);
}

static Sound makeZombieBite() {
    std::mt19937 rng(6);
    float dur = 0.20f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.7f + raw * 0.3f;
        float env = std::exp(-t * 25.f);
        float crunch = lp * 0.6f + raw * 0.4f;
        float growl = std::sin(2.f * PI * 180.f * t) * std::exp(-t * 15.f) * 0.3f;
        s[i] = (crunch * env + growl) * 0.8f;
    }
    return bufferToSound(s);
}

static Sound makeZombieScratch() {
    std::mt19937 rng(7);
    float dur = 0.28f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        hp = hp * 0.85f + raw * 0.15f;
        float hi = raw - hp;
        float env = envelope(t, dur, 0.02f, 0.08f);
        float freq = 600.f + 1400.f * (t / dur);
        float scrape = std::sin(2.f * PI * freq * t) * 0.2f + hi * 0.8f;
        s[i] = scrape * env * 0.7f;
    }
    return bufferToSound(s);
}

static Sound makeLightning() {
    std::mt19937 rng(8);
    float dur = 1.5f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack   = noisef(rng) * std::exp(-t * 100.f);
        float thunder = noisef(rng) * std::exp(-t * 3.5f) * 0.4f;
        float rumble  = std::sin(2.f * PI * 40.f * t) * std::exp(-t * 2.5f) * 0.3f;
        s[i] = (crack + thunder + rumble) * 0.9f;
    }
    return bufferToSound(s);
}

static Sound makeFire() {
    std::mt19937 rng(9);
    float dur = 1.0f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp1 = 0.f, lp2 = 0.f;
    std::uniform_real_distribution<float> popDist(0.f, 1.f);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp1 = lp1 * 0.92f + raw * 0.08f;
        lp2 = lp2 * 0.85f + lp1 * 0.15f;
        float band = raw - lp2;
        float pop = (popDist(rng) > 0.998f) ? noisef(rng) * 0.8f : 0.f;
        float env = 0.6f + 0.4f * std::sin(2.f * PI * 1.5f * t);
        s[i] = (band * 0.5f + pop) * env * 0.7f;
    }
    return bufferToSound(s);
}

static Sound makeWind() {
    std::mt19937 rng(10);
    float dur = 1.2f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.97f + raw * 0.03f;
        float env = envelope(t, dur, 0.15f, 0.25f);
        float wobble = 1.f + 0.1f * std::sin(2.f * PI * 3.f * t);
        s[i] = lp * env * wobble * 0.9f;
    }
    return bufferToSound(s);
}

static Sound makeElectricity() {
    std::mt19937 rng(11);
    float dur = 0.6f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        hp = hp * 0.6f + raw * 0.4f;
        float hi = raw - hp;
        float hum = std::sin(2.f * PI * 120.f * t) * 0.15f;
        float stutter = 0.5f + 0.5f * std::sin(2.f * PI * 40.f * t);
        float env = envelope(t, dur, 0.01f, 0.15f);
        s[i] = (hi * 0.7f + hum) * stutter * env * 0.85f;
    }
    return bufferToSound(s);
}

static Sound makeKnife() {
    std::mt19937 rng(12);
    float dur = 0.18f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.8f + raw * 0.2f;
        float freq = 800.f - 600.f * (t / dur);
        float whoosh = std::sin(2.f * PI * freq * t) * 0.3f + (raw - lp) * 0.7f;
        float env = envelope(t, dur, 0.01f, 0.06f);
        float impact_t = t - (dur * 0.75f);
        float impact = (impact_t > 0.f)
            ? std::sin(2.f * PI * 200.f * impact_t) * std::exp(-impact_t * 60.f) * 0.5f
            : 0.f;
        s[i] = (whoosh * env + impact) * 0.8f;
    }
    return bufferToSound(s);
}

static Sound makeMolotov() {
    std::mt19937 rng(13);
    float dur = 0.5f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.93f + raw * 0.07f;
        float whoosh = lp * envelope(t, 0.25f, 0.05f, 0.1f);
        float gt = t - 0.25f;
        float glass = (gt > 0.f && gt < 0.08f) ? noisef(rng) * std::exp(-gt * 80.f) * 0.6f : 0.f;
        float ct = t - 0.30f;
        float crackle = (ct > 0.f) ? (raw - lp) * std::exp(-ct * 8.f) * 0.5f : 0.f;
        s[i] = (whoosh + glass + crackle) * 0.85f;
    }
    return bufferToSound(s);
}

static Sound makeRain() {
    std::mt19937 rng(14);
    float dur = 1.5f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.88f + raw * 0.12f;
        float env = envelope(t, dur, 0.3f, 0.4f);
        s[i] = lp * env * 0.6f;
    }
    return bufferToSound(s);
}

static Sound makeIcePick() {
    std::mt19937 rng(15);
    float dur = 0.15f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        hp = hp * 0.4f + raw * 0.6f;
        float hi = raw - hp;
        float tone = std::sin(2.f * PI * 2500.f * t) * std::exp(-t * 80.f);
        float shatter = hi * std::exp(-t * 20.f);
        s[i] = (tone * 0.4f + shatter * 0.6f) * 0.85f;
    }
    return bufferToSound(s);
}

static Sound makeLootPickup() {
    std::mt19937 rng(18);
    float dur = 0.18f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float clink = std::sin(2.f * PI * 1400.f * t) * std::exp(-t * 45.f) * 0.5f
                    + std::sin(2.f * PI * 2200.f * t) * std::exp(-t * 60.f) * 0.3f;
        float rustle = noisef(rng) * std::exp(-t * 22.f) * 0.2f;
        float chirp_t = t - 0.06f;
        float chirp = (chirp_t > 0.f)
            ? std::sin(2.f * PI * (900.f + 600.f * chirp_t / 0.12f) * chirp_t) * std::exp(-chirp_t * 35.f) * 0.35f
            : 0.f;
        s[i] = (clink + rustle + chirp) * 0.85f;
    }
    return bufferToSound(s);
}

static Sound makeZombieDeath() {
    std::mt19937 rng(16);
    float dur = 0.70f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp1 = 0.f, lp2 = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp1 = lp1 * 0.60f + raw * 0.40f;
        lp2 = lp2 * 0.80f + lp1 * 0.20f;
        float rattle_band = lp1 - lp2;
        float rattle_env  = std::exp(-t * 18.f);
        float rattle = rattle_band * rattle_env * 0.65f;
        float gurgle_lp = lp2 * 0.5f;
        float gurgle_mod = 0.5f + 0.5f * std::sin(2.f * PI * 14.f * t);
        float gurgle_env = std::exp(-t * 6.f) * (t < 0.05f ? t / 0.05f : 1.f);
        float gurgle = gurgle_lp * gurgle_mod * gurgle_env * 0.5f;
        float mt = t - 0.10f;
        float moan = 0.f;
        if (mt > 0.f) {
            float freq = 220.f * std::exp(-mt * 2.8f) + 60.f;
            float moan_env = std::exp(-mt * 4.5f) * (mt < 0.04f ? mt / 0.04f : 1.f);
            float vibrato = 1.f + 0.04f * std::sin(2.f * PI * 6.f * mt);
            moan = std::sin(2.f * PI * freq * vibrato * mt) * moan_env * 0.55f;
        }
        float click = raw * std::exp(-t * 200.f) * 0.3f;
        s[i] = (rattle + gurgle + moan + click) * 0.92f;
    }
    return bufferToSound(s);
}

static Sound makeZombieLoot() {
    std::mt19937 rng(17);
    float dur = 0.30f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noisef(rng);
        lp = lp * 0.75f + raw * 0.25f;
        float phase = t / dur;
        float freq = 120.f + 80.f * std::sin(PI * phase);
        float grunt = std::sin(2.f * PI * freq * t) * 0.5f;
        float smack = lp * std::exp(-t * 30.f) * 0.4f;
        float env = envelope(t, dur, 0.03f, 0.12f);
        s[i] = (grunt + smack) * env * 0.85f;
    }
    return bufferToSound(s);
}

// Simple heal chime (used by human loot pickup HP/Stamina potions; not in SoundSynth.h
// originally under this name, but GameState.cpp calls sfx("heal")).
static Sound makeHeal() {
    float dur = 0.35f; int n = (int)(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float freq = 660.f + 440.f * (t / dur);
        float tone = std::sin(2.f * PI * freq * t) * std::exp(-t * 3.5f);
        float shimmer = std::sin(2.f * PI * freq * 2.01f * t) * std::exp(-t * 5.f) * 0.3f;
        s[i] = (tone + shimmer) * 0.7f;
    }
    return bufferToSound(s);
}

} // namespace RaylibSoundSynth

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    bool loadMusicFromMemory(const std::string& name, const void* data, std::size_t size) {
        ensureInit();
        Music m = LoadMusicStreamFromMemory(".ogg", (const unsigned char*)data, (int)size);
        if (m.stream.buffer == nullptr) return false;
        musicTracks[name] = m;
        return true;
    }

    void playMusic(const std::string& name, bool loop = true) {
        if (name == currentTrack && currentlyPlaying) return;
        stopMusic();
        auto it = musicTracks.find(name);
        if (it == musicTracks.end()) return;
        ensureInit();
        it->second.looping = loop;
        PlayMusicStream(it->second);
        SetMusicVolume(it->second, musicVolume / 100.0f);
        currentTrack = name;
        currentlyPlaying = true;
    }

    void stopMusic() {
        if (currentlyPlaying) {
            auto it = musicTracks.find(currentTrack);
            if (it != musicTracks.end()) StopMusicStream(it->second);
        }
        currentlyPlaying = false;
        currentTrack.clear();
    }

    void setMusicVolume(float volume) {
        musicVolume = volume;
        if (currentlyPlaying) {
            auto it = musicTracks.find(currentTrack);
            if (it != musicTracks.end()) SetMusicVolume(it->second, musicVolume / 100.0f);
        }
    }

    void updateMusic() {
        if (currentlyPlaying) {
            auto it = musicTracks.find(currentTrack);
            if (it != musicTracks.end()) UpdateMusicStream(it->second);
        }
    }

    bool loadSoundFromMemory(const std::string&, const void*, std::size_t) { return true; }
    bool loadSoundFromBuffer(const std::string&, int) { return true; }

    void playSound(const std::string& name) {
        ensureInit();
        ensureSfxRegistry();
        auto it = sounds.find(name);
        if (it == sounds.end()) return; // unknown sound name: silently ignore
        SetSoundVolume(it->second, soundVolume / 100.0f);
        PlaySound(it->second);
    }

    void setSoundVolume(float volume) {
        soundVolume = volume;
    }

    ~AudioManager() {
        for (auto& kv : sounds) UnloadSound(kv.second);
        for (auto& kv : musicTracks) UnloadMusicStream(kv.second);
        if (initialized) CloseAudioDevice();
    }

private:
    AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool initialized = false;
    bool sfxRegistered = false;
    float soundVolume = 70.0f;
    float musicVolume = 50.0f;
    std::string currentTrack;
    bool currentlyPlaying = false;
    std::unordered_map<std::string, Sound> sounds;
    std::unordered_map<std::string, Music> musicTracks;

    void ensureInit() {
        if (!initialized) {
            InitAudioDevice();
            initialized = true;
        }
    }

    // Build the full per-event SFX set once, matching SoundSynth.h's naming exactly
    // so every sfx("...") call in GameState.cpp resolves to a distinct, matching sound.
    void ensureSfxRegistry() {
        if (sfxRegistered) return;
        sfxRegistered = true;
        using namespace RaylibSoundSynth;
        sounds["footstep"]      = makeFootstep();
        sounds["pistol"]        = makePistol();
        sounds["shotgun"]       = makeShotgun();
        sounds["explosion"]     = makeExplosion();
        sounds["grenade_land"]  = makeGrenadeLand();
        sounds["mine_plant"]    = makeMinePlant();
        sounds["zombie_bite"]   = makeZombieBite();
        sounds["zombie_scratch"]= makeZombieScratch();
        sounds["lightning"]     = makeLightning();
        sounds["fire"]          = makeFire();
        sounds["wind"]          = makeWind();
        sounds["electricity"]   = makeElectricity();
        sounds["knife"]         = makeKnife();
        sounds["molotov"]       = makeMolotov();
        sounds["rain"]          = makeRain();
        sounds["ice_pick"]      = makeIcePick();
        sounds["zombie_death"]  = makeZombieDeath();
        sounds["zombie_loot"]   = makeZombieLoot();
        sounds["loot_pickup"]   = makeLootPickup();
        sounds["heal"]          = makeHeal();
    }
};

#endif // RAYLIB_AUDIOMANAGER_REAL_H
