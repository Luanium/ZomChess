#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <array>

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // ── Music ────────────────────────────────────────────────────────────────
    bool loadMusicFromMemory(const std::string& name, const void* data, std::size_t size);
    // Play music; skips if the same track is already playing
    void playMusic(const std::string& name, bool loop = true);
    void stopMusic();
    void setMusicVolume(float volume);

    // ── Sound effects ────────────────────────────────────────────────────────
    bool loadSoundFromMemory(const std::string& name, const void* data, std::size_t size);
    // Load directly from a pre-built SoundBuffer (used by SoundSynth)
    bool loadSoundFromBuffer(const std::string& name, sf::SoundBuffer buffer);
    // Play a sound effect; uses a pool of channels so sounds don't cut each other
    void playSound(const std::string& name);
    void setSoundVolume(float volume);

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // ── Music state ──────────────────────────────────────────────────────────
    std::string currentTrackName;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicFiles;
    sf::Music* currentMusic = nullptr;  // raw ptr into musicFiles (no ownership)
    float musicVolume = 50.0f;

    // ── Sound state ──────────────────────────────────────────────────────────
    std::unordered_map<std::string, sf::SoundBuffer> soundBuffers;
    // Pool of 16 channels so overlapping sounds don't cut each other
    static constexpr int CHANNEL_COUNT = 16;
    std::array<sf::Sound, CHANNEL_COUNT> channels;
    int nextChannel = 0;
    float soundVolume = 70.0f;
};

#endif // AUDIOMANAGER_H
