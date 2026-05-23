#include "AudioManager.h"
#include <iostream>

// ── Music ────────────────────────────────────────────────────────────────────

bool AudioManager::loadMusicFromMemory(const std::string& name, const void* data, std::size_t size) {
    auto music = std::make_unique<sf::Music>();
    if (music->openFromMemory(data, size)) {
        musicFiles[name] = std::move(music);
        std::cout << "[Audio] Loaded music '" << name << "' (" << size << " bytes)\n";
        return true;
    }
    std::cerr << "[Audio] Failed to load music '" << name << "' from memory\n";
    return false;
}

void AudioManager::playMusic(const std::string& name, bool loop) {
    // Don't restart if already playing this track
    if (name == currentTrackName && currentMusic &&
        currentMusic->getStatus() == sf::Music::Playing) {
        return;
    }
    if (currentMusic) {
        currentMusic->stop();
        currentMusic = nullptr;
    }
    currentTrackName = "";

    auto it = musicFiles.find(name);
    if (it != musicFiles.end() && it->second) {
        currentMusic = it->second.get();
        currentMusic->setVolume(musicVolume);
        currentMusic->setLoop(loop);
        currentMusic->play();
        currentTrackName = name;
        std::cout << "[Audio] Playing music: " << name << "\n";
    } else {
        std::cerr << "[Audio] Music not found: " << name << "\n";
    }
}

void AudioManager::stopMusic() {
    if (currentMusic) {
        currentMusic->stop();
        currentMusic = nullptr;
    }
    currentTrackName = "";
}

void AudioManager::setMusicVolume(float volume) {
    musicVolume = std::max(0.f, std::min(100.f, volume));
    if (currentMusic) currentMusic->setVolume(musicVolume);
}

// ── Sound effects ─────────────────────────────────────────────────────────────

bool AudioManager::loadSoundFromMemory(const std::string& name, const void* data, std::size_t size) {
    sf::SoundBuffer buf;
    if (buf.loadFromMemory(data, size)) {
        soundBuffers[name] = std::move(buf);
        return true;
    }
    std::cerr << "[Audio] Failed to load sound '" << name << "' from memory\n";
    return false;
}

bool AudioManager::loadSoundFromBuffer(const std::string& name, sf::SoundBuffer buffer) {
    soundBuffers[name] = std::move(buffer);
    return true;
}

void AudioManager::playSound(const std::string& name) {
    auto it = soundBuffers.find(name);
    if (it == soundBuffers.end()) return;

    // Find a free channel (not playing), or steal the oldest one
    int chosen = nextChannel;
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        int idx = (nextChannel + i) % CHANNEL_COUNT;
        if (channels[idx].getStatus() != sf::Sound::Playing) {
            chosen = idx;
            break;
        }
    }
    nextChannel = (chosen + 1) % CHANNEL_COUNT;

    channels[chosen].setBuffer(it->second);
    channels[chosen].setVolume(soundVolume);
    channels[chosen].play();
}

void AudioManager::setSoundVolume(float volume) {
    soundVolume = std::max(0.f, std::min(100.f, volume));
}
