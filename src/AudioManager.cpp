#include "AudioManager.h"
#include <iostream>

bool AudioManager::loadMusicFromMemory(const std::string& name, const void* data, std::size_t size) {
    auto music = std::make_unique<sf::Music>();
    if (music->openFromMemory(data, size)) {
        musicFiles[name] = std::move(music);
        std::cout << "[Audio] Loaded music '" << name << "' from memory (" << size << " bytes)" << std::endl;
        return true;
    }
    std::cerr << "[Audio] Failed to load music '" << name << "' from memory" << std::endl;
    return false;
}

bool AudioManager::loadSoundFromMemory(const std::string& name, const void* data, std::size_t size) {
    sf::SoundBuffer buffer;
    if (buffer.loadFromMemory(data, size)) {
        soundBuffers[name] = std::move(buffer);
        return true;
    }
    std::cerr << "[Audio] Failed to load sound '" << name << "' from memory" << std::endl;
    return false;
}

void AudioManager::playSound(const std::string& name) {
    auto it = soundBuffers.find(name);
    if (it != soundBuffers.end()) {
        sound.setBuffer(it->second);
        sound.setVolume(soundVolume);
        sound.play();
    }
}

void AudioManager::playMusic(const std::string& name, bool loop) {
    // Không restart nếu đang phát đúng track này rồi
    if (name == currentTrackName && currentMusic &&
        currentMusic->getStatus() == sf::Music::Playing) {
        return;
    }

    // Dừng nhạc hiện tại (raw pointer, không delete — map vẫn giữ ownership)
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
        std::cout << "[Audio] Playing music: " << name << std::endl;
    } else {
        std::cerr << "[Audio] Music not found: " << name << std::endl;
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
    musicVolume = std::max(0.0f, std::min(100.0f, volume));
    if (currentMusic) {
        currentMusic->setVolume(musicVolume);
    }
}

void AudioManager::setSoundVolume(float volume) {
    soundVolume = std::max(0.0f, std::min(100.0f, volume));
}
