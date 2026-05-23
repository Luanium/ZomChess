#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <memory>

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // Load nhạc từ memory (dữ liệu nhúng trong executable)
    bool loadMusicFromMemory(const std::string& name, const void* data, std::size_t size);
    // Load sound effect từ memory
    bool loadSoundFromMemory(const std::string& name, const void* data, std::size_t size);

    void playSound(const std::string& name);
    // Chuyển nhạc, bỏ qua nếu đang phát đúng track đó rồi
    void playMusic(const std::string& name, bool loop = true);
    void stopMusic();
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::string currentTrackName;
    std::unordered_map<std::string, sf::SoundBuffer> soundBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> musicFiles;
    sf::Sound sound;
    sf::Music* currentMusic = nullptr; // raw pointer vào musicFiles, không sở hữu
    float musicVolume = 50.0f;
    float soundVolume = 70.0f;
};

#endif // AUDIOMANAGER_H
