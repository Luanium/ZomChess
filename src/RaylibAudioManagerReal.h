#ifndef RAYLIB_AUDIOMANAGER_REAL_H
#define RAYLIB_AUDIOMANAGER_REAL_H

// Real AudioManager backed by Raylib's raudio module.
// Same public interface as src/AudioManager.h so GameState.cpp compiles
// unchanged. Sounds are generated procedurally (short beeps/noise bursts)
// since the original SoundSynth.h produces sf::SoundBuffer (SFML-only).

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstdlib>

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // Music: load OGG data from memory (reuses the same embedded byte arrays
    // as the SFML build) via Raylib's Music streaming API.
    bool loadMusicFromMemory(const std::string& name, const void* data, std::size_t size) {
        ensureInit();
        Music m = LoadMusicStreamFromMemory(".ogg", (const unsigned char*)data, (int)size);
        if (m.stream.buffer == nullptr) return false;
        musicTracks[name] = m;
        return true;
    }

    void playMusic(const std::string& name, bool loop = true) {
        if (name == currentTrack && currentlyPlaying) return; // already playing this track
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

    // Call this once per frame from the main loop to keep music streaming.
    void updateMusic() {
        if (currentlyPlaying) {
            auto it = musicTracks.find(currentTrack);
            if (it != musicTracks.end()) UpdateMusicStream(it->second);
        }
    }

    bool loadSoundFromMemory(const std::string&, const void*, std::size_t) { return true; }

    // Called by SoundSynth::registerAll in the SFML build; not used here.
    // Kept for interface compatibility but unused in Raylib build.
    bool loadSoundFromBuffer(const std::string&, int /*placeholder*/) { return true; }

    void playSound(const std::string& name) {
        ensureInit();
        auto it = sounds.find(name);
        if (it == sounds.end()) {
            Sound s = generateBeep(name);
            sounds[name] = s;
            it = sounds.find(name);
        }
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

    // Very small procedural sound generator: pick a distinct short tone
    // per sound name so different actions are at least audibly different.
    Sound generateBeep(const std::string& name) {
        int sampleRate = 44100;
        float duration = 0.12f;
        int sampleCount = (int)(sampleRate * duration);

        // Derive a base frequency from the name so each sound is distinct
        unsigned long hash = 0;
        for (char c : name) hash = hash * 31 + (unsigned char)c;
        float freq = 220.0f + (float)(hash % 660);

        short* data = (short*)malloc(sizeof(short) * sampleCount);
        for (int i = 0; i < sampleCount; ++i) {
            float t = (float)i / sampleRate;
            float envelope = 1.0f - (t / duration);
            float sample = sinf(2.0f * PI * freq * t) * envelope;
            data[i] = (short)(sample * 12000.0f);
        }

        Wave wave = { 0 };
        wave.frameCount = sampleCount;
        wave.sampleRate = sampleRate;
        wave.sampleSize = 16;
        wave.channels = 1;
        wave.data = data;

        Sound s = LoadSoundFromWave(wave);
        free(data);
        return s;
    }
};

#endif // RAYLIB_AUDIOMANAGER_REAL_H
