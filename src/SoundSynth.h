#pragma once
// SoundSynth.h — Procedural PCM sound synthesis, no external files needed.
// All sounds are generated mathematically and loaded into sf::SoundBuffer.

#define _USE_MATH_DEFINES
#include <cmath>

#include <SFML/Audio.hpp>
#include <vector>
#include <random>
#include <algorithm>

namespace SoundSynth {

static constexpr int SAMPLE_RATE = 44100;

// ─── helpers ────────────────────────────────────────────────────────────────

// Envelope: linear attack → linear decay
static inline float envelope(float t, float dur, float attack, float decay) {
    if (t < attack)              return t / attack;
    if (t > dur - decay)         return (dur - t) / decay;
    return 1.0f;
}

// White noise sample
static inline float noise(std::mt19937& rng) {
    static std::uniform_real_distribution<float> d(-1.f, 1.f);
    return d(rng);
}

// Build a SoundBuffer from a float PCM vector (mono, normalised to [-1,1])
static sf::SoundBuffer buildBuffer(const std::vector<float>& samples) {
    std::vector<sf::Int16> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float s = std::max(-1.f, std::min(1.f, samples[i]));
        pcm[i] = static_cast<sf::Int16>(s * 32767.f);
    }
    
    sf::SoundBuffer buf;
    buf.loadFromSamples(pcm.data(), pcm.size(), 1, SAMPLE_RATE);
    return buf;
}

// ─── Footstep ───────────────────────────────────────────────────────────────
// Soft thud: short burst of filtered noise with quick decay
static sf::SoundBuffer makeFootstep() {
    std::mt19937 rng(42);
    const float dur = 0.12f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float env = std::exp(-t * 40.f);
        // Low-frequency thud: mix noise with a 120 Hz sine
        float thud = std::sin(2.f * M_PI * 120.f * t) * 0.5f + noise(rng) * 0.5f;
        s[i] = thud * env * 0.6f;
    }
    return buildBuffer(s);
}

// ─── Pistol shot ────────────────────────────────────────────────────────────
// Sharp crack: high-energy noise burst with fast decay + low-end punch
static sf::SoundBuffer makePistol() {
    std::mt19937 rng(1);
    const float dur = 0.25f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack = noise(rng) * std::exp(-t * 80.f);          // sharp crack
        float body  = std::sin(2.f * M_PI * 80.f * t)            // low body
                    * std::exp(-t * 25.f);
        float tail  = noise(rng) * std::exp(-t * 12.f) * 0.15f;  // reverb tail
        s[i] = (crack * 0.7f + body * 0.5f + tail) * 0.9f;
    }
    return buildBuffer(s);
}

// ─── Shotgun blast ──────────────────────────────────────────────────────────
// Louder, wider crack with longer reverb tail
static sf::SoundBuffer makeShotgun() {
    std::mt19937 rng(2);
    const float dur = 0.55f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float blast = noise(rng) * std::exp(-t * 30.f);
        float boom  = std::sin(2.f * M_PI * 55.f * t) * std::exp(-t * 15.f);
        float tail  = noise(rng) * std::exp(-t * 6.f) * 0.2f;
        s[i] = (blast * 0.8f + boom * 0.6f + tail) * 0.95f;
    }
    return buildBuffer(s);
}

// ─── Explosion ──────────────────────────────────────────────────────────────
// Deep rumble + sharp initial crack + long decay
static sf::SoundBuffer makeExplosion() {
    std::mt19937 rng(3);
    const float dur = 1.2f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack  = noise(rng) * std::exp(-t * 60.f);
        float rumble = noise(rng) * std::exp(-t * 4.f) * 0.5f;
        // Pitch-descending sine for the "boom"
        float freq   = 80.f * std::exp(-t * 3.f);
        float boom   = std::sin(2.f * M_PI * freq * t) * std::exp(-t * 5.f);
        s[i] = (crack * 0.6f + rumble * 0.5f + boom * 0.7f) * 0.95f;
    }
    return buildBuffer(s);
}

// ─── Grenade bounce / land ──────────────────────────────────────────────────
// Metallic clink: two short tones with quick decay
static sf::SoundBuffer makeGrenadeLand() {
    std::mt19937 rng(4);
    const float dur = 0.18f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float env  = std::exp(-t * 35.f);
        float clink = std::sin(2.f * M_PI * 1200.f * t) * 0.5f
                    + std::sin(2.f * M_PI * 1800.f * t) * 0.3f
                    + noise(rng) * 0.2f;
        s[i] = clink * env * 0.7f;
    }
    return buildBuffer(s);
}

// ─── Mine plant ─────────────────────────────────────────────────────────────
// Mechanical click + soft beep
static sf::SoundBuffer makeMinePlant() {
    std::mt19937 rng(5);
    const float dur = 0.22f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        // Click at t=0
        float click = noise(rng) * std::exp(-t * 120.f) * 0.8f;
        // Beep at t=0.05
        float bt = t - 0.05f;
        float beep = (bt > 0.f) ? std::sin(2.f * M_PI * 880.f * bt) * std::exp(-bt * 20.f) * 0.5f : 0.f;
        s[i] = click + beep;
    }
    return buildBuffer(s);
}

// ─── Zombie bite ────────────────────────────────────────────────────────────
// Wet crunch: noise burst with mid-frequency emphasis
static sf::SoundBuffer makeZombieBite() {
    std::mt19937 rng(6);
    const float dur = 0.20f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    // Simple one-pole low-pass state
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.7f + raw * 0.3f;  // low-pass filter
        float env = std::exp(-t * 25.f);
        float crunch = lp * 0.6f + raw * 0.4f;
        // Add a low growl
        float growl = std::sin(2.f * M_PI * 180.f * t) * std::exp(-t * 15.f) * 0.3f;
        s[i] = (crunch * env + growl) * 0.8f;
    }
    return buildBuffer(s);
}

// ─── Zombie scratch / claw ──────────────────────────────────────────────────
// High-frequency scrape: filtered noise sweep
static sf::SoundBuffer makeZombieScratch() {
    std::mt19937 rng(7);
    const float dur = 0.28f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f; // high-pass state
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        hp = hp * 0.85f + raw * 0.15f;
        float hi = raw - hp;  // high-pass
        float env = envelope(t, dur, 0.02f, 0.08f);
        // Pitch-rising scrape
        float freq = 600.f + 1400.f * (t / dur);
        float scrape = std::sin(2.f * M_PI * freq * t) * 0.2f + hi * 0.8f;
        s[i] = scrape * env * 0.7f;
    }
    return buildBuffer(s);
}

// ─── Lightning strike ───────────────────────────────────────────────────────
// Instantaneous crack + rolling thunder
static sf::SoundBuffer makeLightning() {
    std::mt19937 rng(8);
    const float dur = 1.5f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float crack   = noise(rng) * std::exp(-t * 100.f);
        float thunder = noise(rng) * std::exp(-t * 3.5f) * 0.4f;
        // Low rumble
        float rumble  = std::sin(2.f * M_PI * 40.f * t) * std::exp(-t * 2.5f) * 0.3f;
        s[i] = (crack + thunder + rumble) * 0.9f;
    }
    return buildBuffer(s);
}

// ─── Fire crackling ─────────────────────────────────────────────────────────
// Continuous crackle: band-pass filtered noise with random pops
static sf::SoundBuffer makeFire() {
    std::mt19937 rng(9);
    const float dur = 1.0f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp1 = 0.f, lp2 = 0.f;
    std::uniform_real_distribution<float> popDist(0.f, 1.f);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        // Band-pass: two cascaded low-passes
        lp1 = lp1 * 0.92f + raw * 0.08f;
        lp2 = lp2 * 0.85f + lp1 * 0.15f;
        float band = raw - lp2;
        // Random pops
        float pop = (popDist(rng) > 0.998f) ? noise(rng) * 0.8f : 0.f;
        float env = 0.6f + 0.4f * std::sin(2.f * M_PI * 1.5f * t); // slow amplitude swell
        s[i] = (band * 0.5f + pop) * env * 0.7f;
    }
    return buildBuffer(s);
}

// ─── Wind ───────────────────────────────────────────────────────────────────
// Whoosh: low-pass noise with slow amplitude modulation
static sf::SoundBuffer makeWind() {
    std::mt19937 rng(10);
    const float dur = 1.2f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.97f + raw * 0.03f;  // heavy low-pass = wind-like
        float env = envelope(t, dur, 0.15f, 0.25f);
        // Slight pitch wobble
        float wobble = 1.f + 0.1f * std::sin(2.f * M_PI * 3.f * t);
        s[i] = lp * env * wobble * 0.9f;
    }
    return buildBuffer(s);
}

// ─── Electricity / shock ────────────────────────────────────────────────────
// Buzzing arc: high-frequency noise + 60 Hz hum
static sf::SoundBuffer makeElectricity() {
    std::mt19937 rng(11);
    const float dur = 0.6f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        hp = hp * 0.6f + raw * 0.4f;
        float hi = raw - hp;  // high-pass = buzzy
        float hum = std::sin(2.f * M_PI * 120.f * t) * 0.15f;
        // Stutter: amplitude modulated at ~40 Hz
        float stutter = 0.5f + 0.5f * std::sin(2.f * M_PI * 40.f * t);
        float env = envelope(t, dur, 0.01f, 0.15f);
        s[i] = (hi * 0.7f + hum) * stutter * env * 0.85f;
    }
    return buildBuffer(s);
}

// ─── Knife / blade swing ────────────────────────────────────────────────────
// Quick whoosh + impact thud
static sf::SoundBuffer makeKnife() {
    std::mt19937 rng(12);
    const float dur = 0.18f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.8f + raw * 0.2f;
        // Whoosh: mid-frequency sweep
        float freq = 800.f - 600.f * (t / dur);
        float whoosh = std::sin(2.f * M_PI * freq * t) * 0.3f + (raw - lp) * 0.7f;
        float env = envelope(t, dur, 0.01f, 0.06f);
        // Impact thud at end
        float impact_t = t - (dur * 0.75f);
        float impact = (impact_t > 0.f)
            ? std::sin(2.f * M_PI * 200.f * impact_t) * std::exp(-impact_t * 60.f) * 0.5f
            : 0.f;
        s[i] = (whoosh * env + impact) * 0.8f;
    }
    return buildBuffer(s);
}

// ─── Molotov throw / fire whoosh ────────────────────────────────────────────
// Soft whoosh + glass break + ignition crackle
static sf::SoundBuffer makeMolotov() {
    std::mt19937 rng(13);
    const float dur = 0.5f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.93f + raw * 0.07f;
        // Whoosh
        float whoosh = lp * envelope(t, 0.25f, 0.05f, 0.1f);
        // Glass shatter at t=0.25
        float gt = t - 0.25f;
        float glass = (gt > 0.f && gt < 0.08f)
            ? noise(rng) * std::exp(-gt * 80.f) * 0.6f
            : 0.f;
        // Ignition crackle after glass
        float ct = t - 0.30f;
        float crackle = (ct > 0.f)
            ? (raw - lp) * std::exp(-ct * 8.f) * 0.5f
            : 0.f;
        s[i] = (whoosh + glass + crackle) * 0.85f;
    }
    return buildBuffer(s);
}

// ─── Rain ───────────────────────────────────────────────────────────────────
// White noise with soft envelope — steady rainfall
static sf::SoundBuffer makeRain() {
    std::mt19937 rng(14);
    const float dur = 1.5f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.88f + raw * 0.12f;
        float env = envelope(t, dur, 0.3f, 0.4f);
        s[i] = lp * env * 0.6f;
    }
    return buildBuffer(s);
}

// ─── Ice Pick ───────────────────────────────────────────────────────────────
// Sharp high-pitched strike: fast transient and high-pass noise
static sf::SoundBuffer makeIcePick() {
    std::mt19937 rng(15);
    const float dur = 0.15f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float hp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        hp = hp * 0.4f + raw * 0.6f;
        float hi = raw - hp;
        
        // sharp strike tone
        float tone = std::sin(2.f * M_PI * 2500.f * t) * std::exp(-t * 80.f);
        // shatter noise
        float shatter = hi * std::exp(-t * 20.f);
        
        s[i] = (tone * 0.4f + shatter * 0.6f) * 0.85f;
    }
    return buildBuffer(s);
}

// ─── Human loot pickup (weapon/ammo) ────────────────────────────────────────
// Crisp metallic clink + soft rustle — the sound of picking up gear.
// Short and satisfying, distinct from the "heal" chime used for potions.
static sf::SoundBuffer makeLootPickup() {
    std::mt19937 rng(18);
    const float dur = 0.18f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        // Metallic clink: two high-frequency tones with fast decay
        float clink = std::sin(2.f * M_PI * 1400.f * t) * std::exp(-t * 45.f) * 0.5f
                    + std::sin(2.f * M_PI * 2200.f * t) * std::exp(-t * 60.f) * 0.3f;
        // Soft rustle: short filtered noise tail
        float rustle = noise(rng) * std::exp(-t * 22.f) * 0.2f;
        // Rising pitch chirp to give it a "found it!" feel
        float chirp_t = t - 0.06f;
        float chirp = (chirp_t > 0.f)
            ? std::sin(2.f * M_PI * (900.f + 600.f * chirp_t / 0.12f) * chirp_t)
              * std::exp(-chirp_t * 35.f) * 0.35f
            : 0.f;
        s[i] = (clink + rustle + chirp) * 0.85f;
    }
    return buildBuffer(s);
}
// Harsh rattling exhale: starts with a raspy throat-scrape burst (the "death rattle"),
// then transitions into a low descending moan as air leaves — unmistakably undead.
static sf::SoundBuffer makeZombieDeath() {
    std::mt19937 rng(16);
    const float dur = 0.70f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);

    float lp1 = 0.f, lp2 = 0.f; // two-stage low-pass for gurgle texture

    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);

        // ── Death rattle: band-passed noise burst in first 0.15 s ──────────
        lp1 = lp1 * 0.60f + raw * 0.40f;
        lp2 = lp2 * 0.80f + lp1 * 0.20f;
        float rattle_band = lp1 - lp2; // narrow band ~500–1500 Hz
        float rattle_env  = std::exp(-t * 18.f); // fast decay
        float rattle = rattle_band * rattle_env * 0.65f;

        // ── Wet gurgle: amplitude-modulated low-pass noise ─────────────────
        float gurgle_lp = lp2 * 0.5f;
        // Modulate at ~14 Hz to simulate the "bubbling" gurgle rhythm
        float gurgle_mod = 0.5f + 0.5f * std::sin(2.f * M_PI * 14.f * t);
        float gurgle_env = std::exp(-t * 6.f) * (t < 0.05f ? t / 0.05f : 1.f); // tiny attack
        float gurgle = gurgle_lp * gurgle_mod * gurgle_env * 0.5f;

        // ── Death moan: pitch-descending sine, delayed 0.1 s ───────────────
        float mt = t - 0.10f;
        float moan = 0.f;
        if (mt > 0.f) {
            // Starts at ~220 Hz (zombie vocal range), descends to ~60 Hz
            float freq = 220.f * std::exp(-mt * 2.8f) + 60.f;
            float moan_env = std::exp(-mt * 4.5f) * (mt < 0.04f ? mt / 0.04f : 1.f);
            // Add slight vibrato for organic "last-breath" quality
            float vibrato = 1.f + 0.04f * std::sin(2.f * M_PI * 6.f * mt);
            moan = std::sin(2.f * M_PI * freq * vibrato * mt) * moan_env * 0.55f;
        }

        // ── Sharp click transient at t=0 (body impact) ────────────────────
        float click = raw * std::exp(-t * 200.f) * 0.3f;

        s[i] = (rattle + gurgle + moan + click) * 0.92f;
    }
    return buildBuffer(s);
}

// ─── Clever Zombie loot pickup ──────────────────────────────────────────────
// Eerie satisfied grunt + wet smacking sound — zombie is pleased to find loot.
// Short, punchy, clearly non-human. Different from the human "heal" chime.
static sf::SoundBuffer makeZombieLoot() {
    std::mt19937 rng(17);
    const float dur = 0.30f;
    int n = static_cast<int>(dur * SAMPLE_RATE);
    std::vector<float> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; ++i) {
        float t = (float)i / SAMPLE_RATE;
        float raw = noise(rng);
        lp = lp * 0.75f + raw * 0.25f;
        // Satisfied low grunt: rising then falling tone
        float phase = t / dur;
        float freq = 120.f + 80.f * std::sin(M_PI * phase); // 120→200→120 Hz arc
        float grunt = std::sin(2.f * M_PI * freq * t) * 0.5f;
        // Wet smack transient at start
        float smack = lp * std::exp(-t * 30.f) * 0.4f;
        float env = envelope(t, dur, 0.03f, 0.12f);
        s[i] = (grunt + smack) * env * 0.85f;
    }
    return buildBuffer(s);
}

// ─── Register all sounds into AudioManager ──────────────────────────────────
// Call once during initAudio()
static void registerAll(class AudioManager& audio) {
    audio.loadSoundFromBuffer("footstep",    makeFootstep());
    audio.loadSoundFromBuffer("pistol",      makePistol());
    audio.loadSoundFromBuffer("shotgun",     makeShotgun());
    audio.loadSoundFromBuffer("explosion",   makeExplosion());
    audio.loadSoundFromBuffer("grenade_land",makeGrenadeLand());
    audio.loadSoundFromBuffer("mine_plant",  makeMinePlant());
    audio.loadSoundFromBuffer("zombie_bite", makeZombieBite());
    audio.loadSoundFromBuffer("zombie_scratch", makeZombieScratch());
    audio.loadSoundFromBuffer("lightning",   makeLightning());
    audio.loadSoundFromBuffer("fire",        makeFire());
    audio.loadSoundFromBuffer("wind",        makeWind());
    audio.loadSoundFromBuffer("electricity", makeElectricity());
    audio.loadSoundFromBuffer("knife",       makeKnife());
    audio.loadSoundFromBuffer("molotov",     makeMolotov());
    audio.loadSoundFromBuffer("rain",        makeRain());
    audio.loadSoundFromBuffer("ice_pick",    makeIcePick());
    audio.loadSoundFromBuffer("zombie_death",  makeZombieDeath());
    audio.loadSoundFromBuffer("zombie_loot",   makeZombieLoot());
    audio.loadSoundFromBuffer("loot_pickup",   makeLootPickup());
}

} // namespace SoundSynth
