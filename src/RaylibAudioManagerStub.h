#ifndef RAYLIB_AUDIOMANAGER_STUB_H
#define RAYLIB_AUDIOMANAGER_STUB_H

// No-op AudioManager stand-in for the Raylib build.
// Same public interface as the real AudioManager (src/AudioManager.h)
// so GameState.cpp compiles unchanged. Real Raylib audio will replace
// this later.

#include <string>

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    bool loadMusicFromMemory(const std::string&, const void*, std::size_t) { return true; }
    void playMusic(const std::string&, bool = true) {}
    void stopMusic() {}
    void setMusicVolume(float) {}

    bool loadSoundFromMemory(const std::string&, const void*, std::size_t) { return true; }
    void playSound(const std::string&) {}
    void setSoundVolume(float) {}

private:
    AudioManager() = default;
};

#endif // RAYLIB_AUDIOMANAGER_STUB_H
