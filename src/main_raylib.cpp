#include "raylib.h"
#include "GameState.h"
#include "RaylibAudioManagerReal.h"
#include "embedded/menu_theme.h"
#include "embedded/battle_theme.h"
#include "embedded/victory_theme.h"
#include "embedded/defeat_theme.h"
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstring>
namespace fs = std::filesystem;

// ── GameState audio method implementations for Raylib build ──────────────────
// (GameState.cpp only defines these under #ifndef RAYLIB_BUILD, using the
// SFML-based AudioManager. Raylib uses RaylibAudioManagerReal.h's AudioManager,
// so we provide equivalent implementations here.)
void GameState::initAudio() {
    AudioManager& audio = AudioManager::getInstance();
    audio.loadMusicFromMemory("menu",    menu_theme_ogg,    menu_theme_ogg_len);
    audio.loadMusicFromMemory("battle",  battle_theme_ogg,  battle_theme_ogg_len);
    audio.loadMusicFromMemory("victory", victory_theme_ogg, victory_theme_ogg_len);
    audio.loadMusicFromMemory("defeat",  defeat_theme_ogg,  defeat_theme_ogg_len);
    audio.setMusicVolume(music_volume);
}

void GameState::playBackgroundMusic(const std::string& track) {
    if (!music_enabled) { AudioManager::getInstance().stopMusic(); return; }
    AudioManager::getInstance().playMusic(track, true);
}

void GameState::stopBackgroundMusic() {
    AudioManager::getInstance().stopMusic();
}

void GameState::setMusicVolume(float volume) {
    music_volume = std::max(0.0f, std::min(100.0f, volume));
    AudioManager::getInstance().setMusicVolume(music_volume);
}

void GameState::setSfxEnabled(bool enabled) {
    sfx_enabled = enabled;
    AudioManager::getInstance().setSoundVolume(enabled ? 70.0f : 0.0f);
}

// ============================================================
// This file mirrors the core game-loop structure of src/main.cpp
// (the SFML/ImGui version) as closely as possible, so the update
// order and input handling logic stay identical. Only the SFML
// draw calls and ImGui UI have been replaced with Raylib calls.
// ============================================================

static Color zombieColor(ZombieType t) {
    switch (t) {
        case ZombieType::Fast:      return (Color){55, 168, 255, 255};
        case ZombieType::Exploding: return (Color){220, 110, 15, 255};
        case ZombieType::Vampire:   return (Color){130, 30, 130, 255};
        case ZombieType::Sick:      return (Color){210, 190, 65, 255};
        default:                    return (Color){45, 175, 90, 255};
    }
}

static Color terrainColor(Terrain t) {
    switch (t) {
        case Terrain::Wall:   return (Color){60, 62, 66, 255};
        case Terrain::Water:  return (Color){35, 75, 115, 255};
        case Terrain::Forest: return (Color){34, 110, 48, 255};
        case Terrain::Ice:    return (Color){160, 210, 240, 255};
        case Terrain::Fire:   return (Color){220, 100, 20, 255};
        default:              return (Color){105, 60, 35, 255};
    }
}

// Returns the display color for a cell, blending from the old terrain color to
// the new one if a terrain transition animation is currently active on it.
static Color getTerrainDisplayColor(const GameState& state, int x, int y) {
    Color target = terrainColor(state.grid[x][y]);
    for (const auto& trans : state.terrain_transitions) {
        if (trans.pos.x != x || trans.pos.y != y) continue;
        if (trans.to_terrain != state.grid[x][y]) continue;
        float progress = trans.timer / trans.max_duration;
        progress = std::max(0.0f, std::min(1.0f, progress));
        Color from = terrainColor(trans.from_terrain);
        Color to = target;
        return (Color){
            (unsigned char)(from.r + progress * (to.r - from.r)),
            (unsigned char)(from.g + progress * (to.g - from.g)),
            (unsigned char)(from.b + progress * (to.b - from.b)),
            255
        };
    }
    return target;
}

// Returns true and fills outX/outY (grid-space float coords) if a wind push
// animation is active for this entity/item; otherwise returns false.
static bool getWindAnimGridPos(const GameState& state, bool isHuman, size_t idx,
                                bool isLoot, size_t lootIdx, bool isGrenade, size_t grenadeIdx,
                                float& outX, float& outY) {
    for (const auto& anim : state.wind_push_animations) {
        bool match = false;
        if (isHuman && anim.is_human) match = true;
        else if (isLoot && anim.is_loot && anim.loot_idx == lootIdx) match = true;
        else if (isGrenade && anim.is_grenade && anim.grenade_idx == grenadeIdx) match = true;
        else if (!isHuman && !isLoot && !isGrenade && !anim.is_human && !anim.is_loot && !anim.is_grenade && anim.zombie_idx == idx) match = true;
        if (!match) continue;
        float t = std::min(1.0f, anim.timer / anim.duration);
        // ease-out for a natural slide
        t = 1.0f - (1.0f - t) * (1.0f - t);
        outX = anim.from.x + (anim.to.x - anim.from.x) * t;
        outY = anim.from.y + (anim.to.y - anim.from.y) * t;
        return true;
    }
    return false;
}

// During WarpBolt FX's early phase, visually keep entities at their pre-swap
// positions so the swap only "appears" to happen once the singularity collapses.
static Position getWarpDisplayPos(const GameState& state, Position actualPos) {
    if (state.active_fx.type != FXType::WarpBolt) return actualPos;
    float t = 1.0f - (state.active_fx.timer / state.active_fx.max_duration); // 0 -> 1
    const float SWAP_POINT = 0.5f; // matches DARK_PEAK in the FX render code
    if (t >= SWAP_POINT) return actualPos; // after swap point: show real (already-swapped) position

    Position origin = state.active_fx.warp_origin_before;
    Position dest = state.active_fx.warp_dest_before;
    if (actualPos == origin) return dest;
    if (actualPos == dest) return origin;
    return actualPos;
}

// ── Shared checkbox drawer: bigger box, checkmark style (2 crossing lines) instead of solid fill ──
// Returns true if the box was clicked this frame (caller still owns the toggle logic).
static bool drawCheckbox(Vector2 mouse, bool clicked, float x, float y, bool checked, const char* label, int fontSize = 15) {
    const float boxSize = 22.0f;
    Rectangle box = { x, y, boxSize, boxSize };
    DrawRectangleRec(box, (Color){35, 35, 38, 255});
    DrawRectangleLinesEx(box, 2, (Color){110, 110, 110, 255});
    if (checked) {
        DrawLineEx((Vector2){x + 4, y + 11}, (Vector2){x + 9, y + 17}, 3.0f, (Color){60, 220, 120, 255});
        DrawLineEx((Vector2){x + 9, y + 17}, (Vector2){x + 18, y + 5}, 3.0f, (Color){60, 220, 120, 255});
    }
    if (label && label[0]) {
        DrawText(label, (int)(x + boxSize + 8), (int)(y + (boxSize - fontSize) / 2.0f - 1), fontSize, RAYWHITE);
    }
    Rectangle hitArea = { x, y, boxSize + (label && label[0] ? 8 + MeasureText(label, fontSize) : 0), boxSize };
    return clicked && CheckCollisionPointRec(mouse, hitArea);
}

// ── Splash screen: dark tactical theme matching the game's vibe ──────────────
static void runSplashScreen(Font& gameFont) {
    const float DURATION = 6.0f;
    float elapsed = 0.0f;

    while (elapsed < DURATION) {
        if (WindowShouldClose()) return;
        if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) break;

        float dt = GetFrameTime();
        elapsed += dt;

        float alpha = 1.0f;
        if (elapsed < 0.4f) alpha = elapsed / 0.4f;
        unsigned char a = (unsigned char)(alpha * 255);

        BeginDrawing();
        ClearBackground((Color){12, 13, 15, 255});

        int W = GetScreenWidth();
        int H = GetScreenHeight();

        // ── Background: dim pixel grid (tactical board vibe) ──
        {
            int cell = 28;
            for (int gx = 0; gx * cell < W; ++gx) {
                for (int gy = 0; gy * cell < H; ++gy) {
                    bool lit = ((gx + gy) % 7 == 0);
                    Color c = lit ? (Color){40, 70, 55, a} : (Color){20, 22, 24, a};
                    DrawRectangle(gx * cell, gy * cell, cell - 1, cell - 1, c);
                }
            }
            // Scanline
            int scanY = (int)(fmodf(elapsed * 120.0f, (float)H));
            DrawRectangle(0, scanY, W, 2, (Color){60, 220, 140, (unsigned char)(a * 0.35f)});
            // Vignette corners
            DrawRectangleGradientV(0, 0, W, 120, (Color){0,0,0,(unsigned char)(a*0.8f)}, (Color){0,0,0,0});
            DrawRectangleGradientV(0, H - 120, W, 120, (Color){0,0,0,0}, (Color){0,0,0,(unsigned char)(a*0.8f)});
        }

        const char* title = "ZomChess";
        float titleSize = 150.0f;
        Vector2 tsz = MeasureTextEx(gameFont, title, titleSize, 3.0f);
        float titleX = (W - tsz.x) / 2.0f;
        float titleY = H * 0.22f;

        // Blood-red jittery glow layers behind (zombie horror vibe)
        for (int i = 3; i >= 1; --i) {
            float jitterX = sinf(elapsed * 9.0f + i) * 2.5f * i;
            float jitterY = cosf(elapsed * 7.0f + i) * 2.0f * i;
            unsigned char glowA = (unsigned char)((a / 3) * (4 - i) / 3.0f);
            DrawTextEx(gameFont, title, (Vector2){titleX + jitterX, titleY + jitterY}, titleSize, 3.0f,
                       (Color){170, 20, 20, glowA});
        }
        // Main title — bright yellow-orange, high contrast against the dark background
        float pulse = 0.9f + 0.1f * sinf(elapsed * 2.5f);
        DrawTextEx(gameFont, title, (Vector2){titleX, titleY}, titleSize, 3.0f,
                   (Color){255, (unsigned char)(200*pulse), (unsigned char)(30*pulse), a});

        const char* tagline = "EVERY WRONG MOVE LEADS YOU TO DEATH!";
        float tagSize = 28.0f;
        Vector2 tagsz = MeasureTextEx(gameFont, tagline, tagSize, 1.0f);
        DrawTextEx(gameFont, tagline, (Vector2){(W - tagsz.x) / 2.0f, titleY + titleSize + 20}, tagSize, 1.0f,
                   (Color){190, 190, 190, a});

        const char* credit = "Created by: Phan Anh Luan";
        float credSize = 22.0f;
        Vector2 credsz = MeasureTextEx(gameFont, credit, credSize, 1.0f);
        DrawTextEx(gameFont, credit, (Vector2){(W - credsz.x) / 2.0f, H * 0.62f}, credSize, 1.0f,
                   (Color){220, 190, 90, a});

        const char* music = "Music: Kevin MacLeod (incompetech.com) - CC BY 4.0";
        float musSize = 15.0f;
        Vector2 mussz = MeasureTextEx(gameFont, music, musSize, 1.0f);
        DrawTextEx(gameFont, music, (Vector2){(W - mussz.x) / 2.0f, H * 0.62f + 32}, musSize, 1.0f,
                   (Color){120, 120, 120, a});

        DrawTextEx(gameFont, GameConstants::GAME_VERSION, (Vector2){(float)(W - 70), (float)(H - 30)}, 16.0f, 1.0f,
                   (Color){90, 90, 90, a});

        float blink = 0.5f + 0.5f * sinf(elapsed * 3.5f);
        const char* prompt = "Press any key to continue...";
        Vector2 psz = MeasureTextEx(gameFont, prompt, 22, 1);
        DrawTextEx(gameFont, prompt, (Vector2){(W - psz.x) / 2.0f, H * 0.82f},
                   22.0f, 1.0f, (Color){140, 140, 140, (unsigned char)(blink * a)});

        EndDrawing();
    }
}

// ── FileBrowser: directory navigation for .zom challenge files ───────────────
struct FileBrowser {
    enum class Mode { Open, Save };
    bool is_open = false;
    Mode mode = Mode::Open;
    char filename_buf[256] = "my_custom_challenge.zom";
    std::string current_dir;
    std::string selected_path;
    std::string error_msg;

    struct Entry { std::string name; bool is_dir; };
    std::vector<Entry> entries;

    void open(Mode m, const std::string& hint = "") {
        mode = m;
        selected_path.clear();
        error_msg.clear();
        std::string start_dir;
        std::error_code ec;
        if (!hint.empty()) {
            fs::path hp(hint);
            if (fs::is_regular_file(hp, ec)) {
                start_dir = hp.parent_path().string();
                std::string fn = hp.filename().string();
                strncpy(filename_buf, fn.c_str(), sizeof(filename_buf) - 1);
            } else {
                start_dir = fs::current_path(ec).string();
                if (m == Mode::Save && !hint.empty()) {
                    strncpy(filename_buf, hint.c_str(), sizeof(filename_buf) - 1);
                }
            }
        } else {
            start_dir = fs::current_path(ec).string();
        }
        if (start_dir.empty()) start_dir = ".";
        current_dir = start_dir;
        is_open = true;
        refresh();
    }

    void navigate(const std::string& dir) {
        current_dir = dir;
        selected_path.clear();
        error_msg.clear();
        refresh();
    }

    void refresh() {
        entries.clear();
        error_msg.clear();
        std::error_code ec;
        fs::path cur(current_dir);
        if (!fs::is_directory(cur, ec)) {
            error_msg = "Cannot open: " + current_dir;
            current_dir = fs::current_path(ec).string();
            cur = fs::path(current_dir);
            ec.clear();
        }
        fs::path parent = cur.parent_path();
        if (!parent.empty() && parent != cur) entries.push_back({"..", true});

        std::vector<Entry> dirs, files;
        fs::directory_iterator it(cur, ec);
        if (ec) { error_msg = "Read error"; return; }
        for (auto& e : it) {
            std::error_code ec2;
            std::string name = e.path().filename().string();
            if (name.empty() || name[0] == '.') continue;
            bool is_dir = e.is_directory(ec2);
            if (is_dir) {
                dirs.push_back({name, true});
            } else {
                bool is_zom = name.size() >= 4 && name.substr(name.size() - 4) == ".zom";
                if (is_zom || mode == Mode::Save) files.push_back({name, false});
            }
        }
        std::sort(dirs.begin(), dirs.end(), [](const Entry& a, const Entry& b){ return a.name < b.name; });
        std::sort(files.begin(), files.end(), [](const Entry& a, const Entry& b){ return a.name < b.name; });
        for (auto& d : dirs) entries.push_back(d);
        for (auto& f : files) entries.push_back(f);
    }
};

int main() {
    InitWindow(1400, 654, "ZomChess");
    SetTargetFPS(60);

    Font gameFont = LoadFontEx("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32, nullptr, 0);
    if (gameFont.texture.id == 0) {
        gameFont = LoadFontEx("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", 32, nullptr, 0);
    }
    if (gameFont.texture.id == 0) {
        gameFont = GetFontDefault();
    }
    SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);

    runSplashScreen(gameFont);

    #define DrawText(text, x, y, size, ...) DrawTextEx(gameFont, text, (Vector2){(float)(x),(float)(y)}, (float)(size), 1.0f, __VA_ARGS__)

    auto drawCenteredText = [&](const char* text, Rectangle box, float fontSize, Color color) {
        Vector2 sz = MeasureTextEx(gameFont, text, fontSize, 1.0f);
        float tx = box.x + (box.width - sz.x) / 2.0f;
        float ty = box.y + (box.height - sz.y) / 2.0f;
        DrawTextEx(gameFont, text, (Vector2){tx, ty}, fontSize, 1.0f, color);
    };

    GameState state; // starts at GameScene::MainMenu by default

    AudioManager& audio = AudioManager::getInstance();
    audio.loadMusicFromMemory("menu",    menu_theme_ogg,    menu_theme_ogg_len);
    audio.loadMusicFromMemory("battle",  battle_theme_ogg,  battle_theme_ogg_len);
    audio.loadMusicFromMemory("victory", victory_theme_ogg, victory_theme_ogg_len);
    audio.loadMusicFromMemory("defeat",  defeat_theme_ogg,  defeat_theme_ogg_len);
    audio.playMusic("menu");

    const float cellSize = 40.0f;
    const float boardOffset = 20.0f;
    const int VIEW_CELLS = 15;
    int viewX = 0, viewY = 0;
    auto centerViewOnHuman = [&]() {
        int maxVX = std::max(0, state.width - VIEW_CELLS);
        int maxVY = std::max(0, state.height - VIEW_CELLS);
        viewX = std::max(0, std::min(state.human.pos.x - VIEW_CELLS / 2, maxVX));
        viewY = std::max(0, std::min(state.human.pos.y - VIEW_CELLS / 2, maxVY));
    };
    const float scrollThickness = 12.0f;
    const float panelX = boardOffset + VIEW_CELLS * cellSize + 30;

    const float panelW = 720.0f; // wide enough for 3-per-row terrain/status legend
    const float colW = (panelW - 20.0f) / 3.0f; // 3 equal columns for weapon grid
    Rectangle endTurnBtn = { panelX,               boardOffset + 45, colW, 38 };
    Rectangle guideBtn   = { panelX + colW + 10,    boardOffset + 45, colW, 38 };
    Rectangle returnHubTopBtn = { panelX + (colW+10)*2, boardOffset + 45, colW, 38 };

    Rectangle moveBtn    = { panelX,               boardOffset + 115, colW, 36 };
    Rectangle knifeBtn   = { panelX + colW + 10,    boardOffset + 115, colW, 36 };
    Rectangle icePickBtn = { panelX + (colW+10)*2,  boardOffset + 115, colW, 36 };
    bool showGuide = false;

    Rectangle pistolBtn  = { panelX,               boardOffset + 160, colW, 36 };
    Rectangle shotgunBtn = { panelX + colW + 10,    boardOffset + 160, colW, 36 };
    Rectangle warpBoltBtn = { panelX + (colW+10)*2, boardOffset + 160, colW, 36 };

    Rectangle grenadeBtn = { panelX,               boardOffset + 205, colW, 36 };
    Rectangle molotovBtn = { panelX + colW + 10,    boardOffset + 205, colW, 36 };
    Rectangle mineBtn    = { panelX + (colW+10)*2,  boardOffset + 205, colW, 36 };

    Rectangle easyBtn   = { 60, 200, 260, 45 };
    Rectangle mediumBtn = { 60, 255, 260, 45 };
    Rectangle hardBtn   = { 60, 310, 260, 45 };
    Rectangle unfairBtn = { 60, 365, 260, 45 };
    Rectangle exportBtn = { 60, 440, 125, 38 };
    Rectangle importBtn = { 195, 440, 125, 38 };
    Rectangle startCustomBtn = { 60, 490, 260, 40 };
    Rectangle quitGameBtn = { 60, 560, 260, 40 };
    std::string ioMessage;
    float ioMessageTimer = 0.0f;
    bool hasImportedConfig = false;
    bool showImportWarnPopup = false;
    std::string importWarnMessage;
    std::string pendingImportPath;

    // ── Custom difficulty sliders (right column) ──
    float sliderX = 400.0f;
    float sliderW = 970.0f;
    Vector2 mouse = {0, 0};
    bool mouseDown = false;
    bool suppressNextClick = true;
    auto drawSlider = [&](const char* label, int* val, int minV, int maxV, float y, Color barColor) -> void {
        DrawText(TextFormat("%s: %d", label, *val), (int)sliderX, (int)y, 16, RAYWHITE);
        Rectangle track = { sliderX, y + 22, sliderW, 8 };
        DrawRectangleRounded(track, 0.5f, 4, (Color){40,40,40,255});
        float pct = (float)(*val - minV) / (float)(maxV - minV);
        Rectangle fill = { sliderX, y + 22, sliderW * pct, 8 };
        DrawRectangleRounded(fill, 0.5f, 4, barColor);
        float knobX = sliderX + sliderW * pct;
        DrawCircle((int)knobX, (int)(y + 26), 6.0f, RAYWHITE);
        if (mouseDown && CheckCollisionPointRec(mouse, (Rectangle){sliderX - 10, y, sliderW + 20, 36})) {
            float p = (mouse.x - sliderX) / sliderW;
            p = std::max(0.0f, std::min(1.0f, p));
            *val = minV + (int)std::round(p * (maxV - minV));
        }
    };

    Terrain editorSelectedTerrain = Terrain::Wall;
    bool editorPlacingZombie = false;
    bool editorEraseZombieMode = false;
    bool editorPlacingHuman = false;
    bool showConfirmEraseAllZombies = false;
    bool showConfirmResetMap = false;
    bool showConfirmResetTerrain = false;
    bool eraseAllZombiesJustOpened = false;
    bool resetMapJustOpened = false;
    bool resetTerrainJustOpened = false;
    ZombieType editorSelectedZombieType = ZombieType::Clever;   

    FileBrowser saveBrowser, loadBrowser;

    bool showConfirmExitGame = false;
    bool showConfirmReturnHub = false;
    bool shouldQuit = false;
    SetExitKey(KEY_NULL); // disable default ESC-to-close, we handle confirmation ourselves

    bool wasFocused = true;
    int minimizeRestorePending = 0; // 0 = idle, >0 = counting down to restore

    while (!shouldQuit) {
        bool closeRequested = WindowShouldClose();
        float dtSeconds = GetFrameTime();
        AudioManager::getInstance().updateMusic();

        bool isFocusedNow = IsWindowFocused();
        if (isFocusedNow && !wasFocused && minimizeRestorePending == 0) {
            minimizeRestorePending = 2;
        }
        wasFocused = isFocusedNow;

        if (minimizeRestorePending > 0) {
            minimizeRestorePending--;
            if (minimizeRestorePending == 1) {
                ToggleFullscreen();
            } else if (minimizeRestorePending == 0) {
                ToggleFullscreen();
            }
        }

        mouse = GetMousePosition();
        bool mouseDownRaw = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        if (suppressNextClick) {
            if (!mouseDownRaw) suppressNextClick = false;
        }
        bool mouseClicked = !suppressNextClick && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        mouseDown = !suppressNextClick && mouseDownRaw;

        if (closeRequested && !showConfirmExitGame && !showConfirmReturnHub) {
            if (state.current_scene == GameScene::Playing || state.current_scene == GameScene::MapEditor) {
                showConfirmExitGame = true;
            } else {
                shouldQuit = true;
                break;
            }
        }

        auto endTurnWithBanner = [&]() {
            state.start_zombie_phase();
            state.turn_banner_fx.type = FXType::Electricity;
            state.turn_banner_fx.timer = 1.0f;
            state.turn_banner_fx.max_duration = 1.0f;
            state.turn_banner_fx.banner_text = "TURN ENDED";
        };

        // ══════════════════════════════════════════════════════════════
        // MAIN MENU — quick play (left) + custom difficulty sliders (right)
        // ══════════════════════════════════════════════════════════════
        if (state.current_scene == GameScene::MainMenu) {
            if (ioMessageTimer > 0.0f) {
                ioMessageTimer -= dtSeconds;
                if (ioMessageTimer <= 0.0f) ioMessage.clear();
            }
            if (mouseClicked && !saveBrowser.is_open && !loadBrowser.is_open) {
                int chosen = -1;
                if (CheckCollisionPointRec(mouse, easyBtn))   chosen = 0;
                if (CheckCollisionPointRec(mouse, mediumBtn)) chosen = 1;
                if (CheckCollisionPointRec(mouse, hardBtn))   chosen = 2;
                if (CheckCollisionPointRec(mouse, unfairBtn)) chosen = 3;
                if (chosen >= 0) {
                    state.apply_quick_difficulty(chosen);
                    state.init_game();
                    centerViewOnHuman();
                    state.current_scene = GameScene::Playing;
                    AudioManager::getInstance().playMusic("battle");
                }
                if (CheckCollisionPointRec(mouse, exportBtn)) {
                    saveBrowser.open(FileBrowser::Mode::Save, "my_custom_challenge.zom");
                }
                if (CheckCollisionPointRec(mouse, importBtn)) {
                    loadBrowser.open(FileBrowser::Mode::Open, "my_custom_challenge.zom");
                }
                if (hasImportedConfig && CheckCollisionPointRec(mouse, startCustomBtn)) {
                    state.init_game();
                    centerViewOnHuman();
                    state.current_scene = GameScene::Playing;
                    AudioManager::getInstance().playMusic("battle");
                }
                if (CheckCollisionPointRec(mouse, quitGameBtn)) {
                    showConfirmExitGame = true;
                }
            }

            BeginDrawing();
            ClearBackground((Color){22, 23, 25, 255});
            DrawText("ZomChess", 60, 40, 48, RAYWHITE);

            // ── Music / SFX toggles ──
            {
                Vector2 musicPos = {60, 95}, sfxPos = {180, 95};
                bool musicClick = drawCheckbox(mouse, mouseClicked, musicPos.x, musicPos.y, state.music_enabled, "Music");
                bool sfxClick   = drawCheckbox(mouse, mouseClicked, sfxPos.x, sfxPos.y, state.sfx_enabled, "SFX");

                if (musicClick && !saveBrowser.is_open && !loadBrowser.is_open) {
                    state.music_enabled = !state.music_enabled;
                    if (state.music_enabled) AudioManager::getInstance().playMusic("menu");
                    else AudioManager::getInstance().stopMusic();
                }
                if (sfxClick && !saveBrowser.is_open && !loadBrowser.is_open) {
                    state.sfx_enabled = !state.sfx_enabled;
                    state.setSfxEnabled(state.sfx_enabled);
                }
            }

            // ── Left column: Quick Play ──
            DrawText("Quick Play", 60, 160, 22, (Color){130, 220, 255, 255});
            DrawRectangleRec(easyBtn, (Color){0,150,0,255});
            drawCenteredText("EASY", easyBtn, 20, WHITE);
            DrawRectangleRec(mediumBtn, (Color){150,150,0,255});
            drawCenteredText("MEDIUM", mediumBtn, 20, WHITE);
            DrawRectangleRec(hardBtn, (Color){200,100,0,255});
            drawCenteredText("HARD", hardBtn, 20, WHITE);
            DrawRectangleRec(unfairBtn, (Color){200,0,0,255});
            drawCenteredText("UNFAIR", unfairBtn, 20, WHITE);

            DrawText("Challenge Files", 60, 415, 18, (Color){230, 210, 100, 255});
            DrawRectangleRec(exportBtn, (Color){0,110,110,255});
            drawCenteredText("Export .zom", exportBtn, 15, WHITE);
            DrawRectangleRec(importBtn, (Color){110,90,0,255});
            drawCenteredText("Import .zom", importBtn, 15, WHITE);

            DrawRectangleRec(startCustomBtn, hasImportedConfig ? (Color){160,60,180,255} : (Color){60,60,60,255});
            drawCenteredText("Start Imported Game", startCustomBtn, 16, WHITE);

            DrawRectangleRec(quitGameBtn, (Color){140,20,20,255});
            drawCenteredText("Quit Game", quitGameBtn, 16, WHITE);

            if (!ioMessage.empty()) {
                DrawText(ioMessage.c_str(), 60, 540, 15, (Color){255, 220, 100, 255});
            }

            // ── Right column: Custom Difficulty (2 sub-columns, scrollable) ──
            DrawText("Custom Difficulty", (int)sliderX, 40, 26, (Color){255, 140, 220, 255});
            DrawLine((int)sliderX - 30, 30, (int)sliderX - 30, 620, (Color){70,70,70,255});

            static float customScroll = 0.0f;
            static float customScrollMaxCache = 0.0f;
            Rectangle scrollArea = { sliderX - 15, 68, sliderW + 30, 500 };
            if (CheckCollisionPointRec(mouse, scrollArea)) {
                customScroll -= GetMouseWheelMove() * 30.0f;
                customScroll = std::max(0.0f, std::min(customScroll, customScrollMaxCache));
            }

            float colAW = 330.0f; // left sub-column width (Map/Human/Weapons/Zombies)
            float colBX = sliderX + colAW + 40.0f; // right sub-column X (Terrain/Weather)
            float colBW = sliderW - colAW - 40.0f;

            BeginScissorMode((int)scrollArea.x, (int)scrollArea.y, (int)scrollArea.width, (int)scrollArea.height);
            float baseY = 68.0f - customScroll;

            // A small local slider drawer bound to colA width so labels/tracks fit
            auto drawSliderW = [&](const char* label, int* val, int minV, int maxV, float x, float w, float y, Color barColor) {
                DrawText(TextFormat("%s: %d", label, *val), (int)x, (int)y, 14, RAYWHITE);
                Rectangle track = { x, y + 20, w, 7 };
                DrawRectangleRounded(track, 0.5f, 4, (Color){40,40,40,255});
                float pct = (float)(*val - minV) / (float)(maxV - minV);
                Rectangle fill = { x, y + 20, w * pct, 7 };
                DrawRectangleRounded(fill, 0.5f, 4, barColor);
                float knobX = x + w * pct;
                DrawCircle((int)knobX, (int)(y + 23), 5.5f, RAYWHITE);
                if (mouseDown && CheckCollisionPointRec(mouse, (Rectangle){x - 10, y, w + 20, 32})) {
                    float p = (mouse.x - x) / w;
                    p = std::max(0.0f, std::min(1.0f, p));
                    *val = minV + (int)std::round(p * (maxV - minV));
                }
            };

            // ── COLUMN A: Map / Human / Weapons / Zombie counts ──
            float ay = baseY;
            DrawText("Map Size", (int)sliderX, (int)ay, 17, (Color){130,220,255,255}); ay += 24;
            static int savedManualMapW = state.active_config.map_width;
            static int savedManualMapH = state.active_config.map_height;

            if (state.active_config.custom_map_mode) {
                // Locked: map size always matches the custom_grid actual size
                state.active_config.map_width  = (int)state.active_config.custom_grid.size();
                state.active_config.map_height = state.active_config.custom_grid.empty() ? 0 : (int)state.active_config.custom_grid[0].size();

                DrawText(TextFormat("Map Width: %d (locked - edit in Map Editor)", state.active_config.map_width), (int)sliderX, (int)ay, 14, (Color){150,150,150,255});
                Rectangle trackW = { sliderX, ay + 20, colAW, 7 };
                DrawRectangleRounded(trackW, 0.5f, 4, (Color){30,30,30,255});
                ay += 42;

                DrawText(TextFormat("Map Height: %d (locked - edit in Map Editor)", state.active_config.map_height), (int)sliderX, (int)ay, 14, (Color){150,150,150,255});
                Rectangle trackH = { sliderX, ay + 20, colAW, 7 };
                DrawRectangleRounded(trackH, 0.5f, 4, (Color){30,30,30,255});
                ay += 42;
            } else {
                state.active_config.map_width  = savedManualMapW;
                state.active_config.map_height = savedManualMapH;
                drawSliderW("Map Width", &state.active_config.map_width,
                            GameConstants::Difficulty::SliderBounds::MAP_WIDTH_MIN,
                            GameConstants::Difficulty::SliderBounds::MAP_WIDTH_MAX, sliderX, colAW, ay, (Color){80,160,220,255}); ay += 42;
                drawSliderW("Map Height", &state.active_config.map_height,
                            GameConstants::Difficulty::SliderBounds::MAP_HEIGHT_MIN,
                            GameConstants::Difficulty::SliderBounds::MAP_HEIGHT_MAX, sliderX, colAW, ay, (Color){80,160,220,255}); ay += 42;
                savedManualMapW = state.active_config.map_width;
                savedManualMapH = state.active_config.map_height;
            }
            DrawText("Human Status", (int)sliderX, (int)ay, 17, (Color){130,220,255,255}); ay += 24;
            drawSliderW("Human HP", &state.active_config.human_hp,
                        GameConstants::Difficulty::SliderBounds::HUMAN_HP_MIN,
                        GameConstants::Difficulty::SliderBounds::HUMAN_HP_MAX, sliderX, colAW, ay, (Color){220,180,60,255}); ay += 42;
            drawSliderW("Initial Stamina", &state.active_config.initial_stamina,
                        GameConstants::Difficulty::SliderBounds::INITIAL_STAMINA_MIN,
                        GameConstants::Difficulty::SliderBounds::INITIAL_STAMINA_MAX, sliderX, colAW, ay, (Color){220,180,60,255}); ay += 42;
            drawSliderW("Turn Limit", &state.active_config.turn_limit,
                        GameConstants::Difficulty::SliderBounds::TURN_LIMIT_MIN,
                        GameConstants::Difficulty::SliderBounds::TURN_LIMIT_MAX, sliderX, colAW, ay, (Color){220,180,60,255}); ay += 50;

            DrawText("Weapons", (int)sliderX, (int)ay, 17, (Color){130,220,255,255}); ay += 24;
            drawSliderW("Pistol Ammo", &state.active_config.pistol_ammo,
                        GameConstants::Difficulty::SliderBounds::PISTOL_AMMO_MIN,
                        GameConstants::Difficulty::SliderBounds::PISTOL_AMMO_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 42;
            drawSliderW("Shotgun Ammo", &state.active_config.shotgun_ammo,
                        GameConstants::Difficulty::SliderBounds::SHOTGUN_AMMO_MIN,
                        GameConstants::Difficulty::SliderBounds::SHOTGUN_AMMO_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 42;
            drawSliderW("Warp Ammo", &state.active_config.warp_charges,
                        GameConstants::Difficulty::SliderBounds::WARP_CHARGES_MIN,
                        GameConstants::Difficulty::SliderBounds::WARP_CHARGES_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 42;
            drawSliderW("Grenades", &state.active_config.grenades,
                        GameConstants::Difficulty::SliderBounds::GRENADES_MIN,
                        GameConstants::Difficulty::SliderBounds::GRENADES_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 42;
            drawSliderW("Molotovs", &state.active_config.molotovs,
                        GameConstants::Difficulty::SliderBounds::MOLOTOVS_MIN,
                        GameConstants::Difficulty::SliderBounds::MOLOTOVS_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 42;
            drawSliderW("Mines", &state.active_config.mines,
                        GameConstants::Difficulty::SliderBounds::MINES_MIN,
                        GameConstants::Difficulty::SliderBounds::MINES_MAX, sliderX, colAW, ay, (Color){200,120,60,255}); ay += 50;

            int available = state.calculate_available_spawn_cells();
            int totalZoms = state.active_config.count_normal + state.active_config.count_fast +
                            state.active_config.count_exploding + state.active_config.count_vampire +
                            state.active_config.count_sick;
            bool overflow = !state.is_zombie_count_valid();

            // ── COLUMN B: Terrain ratios + Weather probabilities (multi-slider bars) ──
            float by = baseY;

            // Generic N-segment multi-slider: one bar, N-1 draggable internal knobs,
            // segments always sum to 100. `values` and `colors` must have `n` entries.
            // Returns nothing; mutates `values` in place. `activeKnobId` disambiguates
            // multiple multi-sliders sharing the same static drag-state variable.
            auto drawMultiSlider = [&](int* values[], Color colors[], const char* labels[], int n,
                                       float x, float w, float y, int barId) {
                static int activeKnob = -1;
                static int activeBarId = -1;

                float barH = 14.0f;
                Rectangle bar = { x, y, w, barH };
                DrawRectangleRec(bar, (Color){25,25,28,255});

                // Compute cumulative breakpoints p[0..n] where p[0]=0, p[n]=100
                std::vector<int> p(n + 1);
                p[0] = 0;
                for (int i = 0; i < n; ++i) p[i+1] = p[i] + *values[i];

                bool hovered = CheckCollisionPointRec(mouse, (Rectangle){x, y - 6, w, barH + 12});
                bool mouseDownHere = IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
                                      CheckCollisionPointRec(mouse, (Rectangle){x, y - 6, w, barH + 12});

                if (mouseDownHere && (activeBarId != barId || activeKnob == -1) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    float clickPct = (mouse.x - x) / w * 100.0f;
                    clickPct = std::max(0.0f, std::min(100.0f, clickPct));
                    int nearest = 1; float bestDist = 1e9f;
                    for (int i = 1; i < n; ++i) {
                        float d = fabsf(clickPct - p[i]);
                        if (d < bestDist) { bestDist = d; nearest = i; }
                    }
                    activeKnob = nearest;
                    activeBarId = barId;
                }
                if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { if (activeBarId == barId) activeKnob = -1; }

                if (activeBarId == barId && activeKnob >= 1 && activeKnob < n && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    float pct = (mouse.x - x) / w * 100.0f;
                    pct = std::max(0.0f, std::min(100.0f, pct));
                    int newVal = (int)std::round(pct);
                    newVal = std::max(p[activeKnob - 1] + 1, std::min(p[activeKnob + 1] - 1, newVal));
                    p[activeKnob] = newVal;
                }
                // Rebuild values from breakpoints
                for (int i = 0; i < n; ++i) *values[i] = p[i+1] - p[i];

                // Draw segments
                for (int i = 0; i < n; ++i) {
                    float sx = x + w * (p[i] / 100.0f);
                    float ex = x + w * (p[i+1] / 100.0f);
                    DrawRectangle((int)sx, (int)y, (int)std::max(1.0f, ex - sx), (int)barH, colors[i]);
                }
                DrawRectangleLinesEx(bar, 1, (Color){20,20,20,200});

                // Draw internal knobs
                for (int i = 1; i < n; ++i) {
                    float kx = x + w * (p[i] / 100.0f);
                    bool isActive = (activeBarId == barId && activeKnob == i);
                    Color kc = isActive ? (Color){255,215,25,255} : WHITE;
                    DrawRectangle((int)kx - 3, (int)y - 3, 6, (int)barH + 6, kc);
                }
            };

            bool terrainRatioLocked = state.active_config.custom_map_mode;
            DrawText("Terrain Ratios (sum = 100%)", (int)colBX, (int)by,
                     16, terrainRatioLocked ? (Color){110,110,60,255} : (Color){230,210,100,255});
            if (terrainRatioLocked) {
                DrawText("(locked - using Custom Map)", (int)colBX + 260, (int)by + 2, 13, (Color){200,200,80,255});
            }
            by += 26;
            {
                int* tv[5] = { &state.active_config.ratio_dirt, &state.active_config.ratio_wall,
                               &state.active_config.ratio_water, &state.active_config.ratio_forest,
                               &state.active_config.ratio_ice };
                Color tcFull[5] = { {105,60,35,255}, {60,62,66,255}, {35,75,115,255}, {34,110,48,255}, {160,210,240,255} };
                Color tc[5];
                for (int i = 0; i < 5; ++i) {
                    tc[i] = terrainRatioLocked
                        ? Color{ (unsigned char)(tcFull[i].r/3), (unsigned char)(tcFull[i].g/3), (unsigned char)(tcFull[i].b/3), 255 }
                        : tcFull[i];
                }
                const char* tl[5] = { "Dirt", "Wall", "Water", "Forest", "Ice" };

                if (terrainRatioLocked) {
                    // Draw a disabled, non-interactive bar
                    float barW = colBW;
                    float barH = 14.0f;
                    Rectangle disBar = { colBX, by, barW, barH };
                    DrawRectangleRec(disBar, (Color){25,25,28,255});
                    float pAcc = 0.0f;
                    for (int i = 0; i < 5; ++i) {
                        float segW = barW * (*tv[i] / 100.0f);
                        DrawRectangle((int)(colBX + pAcc), (int)by, (int)segW, (int)barH, tc[i]);
                        pAcc += segW;
                    }
                    DrawRectangleLinesEx(disBar, 1, (Color){20,20,20,200});
                } else {
                    drawMultiSlider(tv, tc, tl, 5, colBX, colBW, by, 1001);
                }
                by += 34;
                // Legend, 3-per-row
                float legW = colBW / 3.0f;
                Color legTextCol = terrainRatioLocked ? (Color){110,110,110,255} : RAYWHITE;
                for (int i = 0; i < 5; ++i) {
                    float lx = colBX + (i % 3) * legW;
                    float ly = by + (i / 3) * 22.0f;
                    DrawRectangle((int)lx, (int)ly + 2, 12, 12, tc[i]);
                    DrawText(TextFormat("%s: %d%%", tl[i], *tv[i]), (int)lx + 18, (int)ly, 13, legTextCol);
                }
                by += 22.0f * 2 + 12;
            }

            // ── Enable Environment Events checkbox ──
            bool enableEnvClick = drawCheckbox(mouse, mouseClicked, colBX, by, state.active_config.enable_environment, "Enable Environment Events");
            if (enableEnvClick) {
                state.active_config.enable_environment = !state.active_config.enable_environment;
            }
            by += 32;

            DrawText("Weather Probabilities (sum = 100%)", (int)colBX, (int)by,
                     16, state.active_config.enable_environment ? (Color){230,210,100,255} : (Color){90,85,60,255});
            by += 26;
            {
                int* wv[7] = { &state.active_config.env_prob_clear, &state.active_config.env_prob_wind,
                               &state.active_config.env_prob_rain, &state.active_config.env_prob_clouds,
                               &state.active_config.env_prob_lightning, &state.active_config.env_prob_heatwave,
                               &state.active_config.env_prob_blizzard };
                Color wcFull[7] = { {120,190,230,255}, {200,220,90,255}, {40,110,210,255}, {70,70,80,255},
                                    {255,230,40,255}, {235,110,30,255}, {200,240,255,255} };
                Color wc[7];
                bool envOn = state.active_config.enable_environment;
                for (int i = 0; i < 7; ++i) {
                    wc[i] = envOn ? wcFull[i] : (Color){ (unsigned char)(wcFull[i].r/3), (unsigned char)(wcFull[i].g/3), (unsigned char)(wcFull[i].b/3), 255 };
                }
                const char* wl[7] = { "Clear", "Wind", "Rain", "Dark Clouds", "Lightning", "Heatwave", "Blizzard" };
                if (envOn) {
                    drawMultiSlider(wv, wc, wl, 7, colBX, colBW, by, 1002);
                } else {
                    // Draw a disabled, non-interactive bar
                    Rectangle disBar = { colBX, by, colBW, 14.0f };
                    DrawRectangleRec(disBar, (Color){25,25,28,255});
                    float pAcc = 0.0f;
                    for (int i = 0; i < 7; ++i) {
                        float segW = colBW * (*wv[i] / 100.0f);
                        DrawRectangle((int)(colBX + pAcc), (int)by, (int)segW, 14, wc[i]);
                        pAcc += segW;
                    }
                    DrawRectangleLinesEx(disBar, 1, (Color){20,20,20,200});
                }
                by += 34;
                float legW = colBW / 3.0f;
                Color legTextCol = envOn ? RAYWHITE : (Color){110,110,110,255};
                for (int i = 0; i < 7; ++i) {
                    float lx = colBX + (i % 3) * legW;
                    float ly = by + (i / 3) * 22.0f;
                    DrawRectangle((int)lx, (int)ly + 2, 12, 12, wc[i]);
                    DrawText(TextFormat("%s: %d%%", wl[i], *wv[i]), (int)lx + 18, (int)ly, 13, legTextCol);
                }
                by += 22.0f * 3 + 16;
            }

            // ── Zombie Counts (moved here, below Weather) ──
            bool zombieCountsLocked = state.active_config.custom_map_mode && !state.active_config.custom_zombie_spawns.empty();

            DrawText("Zombie Counts", (int)colBX, (int)by, 17, (Color){255,100,100,255});
            if (zombieCountsLocked) {
                DrawText("(locked - set via Map Editor)", (int)colBX + 160, (int)by + 2, 13, (Color){200,200,80,255});
            }
            by += 24;

            if (zombieCountsLocked) {
                auto drawLockedSliderW = [&](const char* label, int val, int minV, int maxV, float x, float w, float y, Color barColor) {
                    DrawText(TextFormat("%s: %d", label, val), (int)x, (int)y, 14, (Color){150,150,150,255});
                    Rectangle track = { x, y + 20, w, 7 };
                    DrawRectangleRounded(track, 0.5f, 4, (Color){30,30,30,255});
                    float pct = (maxV > minV) ? (float)(val - minV) / (float)(maxV - minV) : 0.0f;
                    Rectangle fill = { x, y + 20, w * pct, 7 };
                    Color dim = { (unsigned char)(barColor.r/2), (unsigned char)(barColor.g/2), (unsigned char)(barColor.b/2), 255 };
                    DrawRectangleRounded(fill, 0.5f, 4, dim);
                };
                drawLockedSliderW("Clever Zombies", state.active_config.count_normal, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_CLEVER_MAX, colBX, colBW, by, (Color){45,175,90,255}); by += 42;
                drawLockedSliderW("Fast Sprinters", state.active_config.count_fast, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_FAST_MAX, colBX, colBW, by, (Color){55,168,255,255}); by += 42;
                drawLockedSliderW("Volatile Exploders", state.active_config.count_exploding, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_EXPLODING_MAX, colBX, colBW, by, (Color){230,140,20,255}); by += 42;
                drawLockedSliderW("Vampiric Draculas", state.active_config.count_vampire, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_VAMPIRE_MAX, colBX, colBW, by, (Color){170,50,170,255}); by += 42;
                drawLockedSliderW("Sick Carriers", state.active_config.count_sick, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_SICK_MAX, colBX, colBW, by, (Color){210,190,65,255}); by += 42;
            } else {
                drawSliderW("Clever Zombies", &state.active_config.count_normal, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_CLEVER_MAX, colBX, colBW, by, (Color){45,175,90,255}); by += 42;
                drawSliderW("Fast Sprinters", &state.active_config.count_fast, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_FAST_MAX, colBX, colBW, by, (Color){55,168,255,255}); by += 42;
                drawSliderW("Volatile Exploders", &state.active_config.count_exploding, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_EXPLODING_MAX, colBX, colBW, by, (Color){230,140,20,255}); by += 42;
                drawSliderW("Vampiric Draculas", &state.active_config.count_vampire, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_VAMPIRE_MAX, colBX, colBW, by, (Color){170,50,170,255}); by += 42;
                drawSliderW("Sick Carriers", &state.active_config.count_sick, 0,
                            GameConstants::Difficulty::SliderBounds::COUNT_SICK_MAX, colBX, colBW, by, (Color){210,190,65,255}); by += 42;
            }

            available = state.calculate_available_spawn_cells();
            totalZoms = state.active_config.count_normal + state.active_config.count_fast +
                        state.active_config.count_exploding + state.active_config.count_vampire +
                        state.active_config.count_sick;
            overflow = !state.is_zombie_count_valid();
            DrawText(TextFormat("Spawn tiles: %d | Zombies: %d", available, totalZoms),
                     (int)colBX, (int)by, 13, overflow ? (Color){255,80,80,255} : (Color){160,160,160,255});
            by += 30;

            // ── Spawn Shield checkbox (column A, below zombie counts) ──
            if (state.active_config.custom_map_mode) {
                state.active_config.spawn_shield = false;
                drawCheckbox(mouse, false, sliderX, ay, false, "Spawn Shield (disabled with Custom Map)");
            } else {
                bool spawnShieldClick = drawCheckbox(mouse, mouseClicked, sliderX, ay, state.active_config.spawn_shield, "Spawn Shield (safe 5x5 zone)");
                if (spawnShieldClick) {
                    state.active_config.spawn_shield = !state.active_config.spawn_shield;
                }
            }
            ay += 32;

            // ── Fixed Stamina checkbox (column A, below Spawn Shield) ──
            bool fixedStamClick = drawCheckbox(mouse, mouseClicked, sliderX, ay, state.active_config.fixed_stamina, "Fixed Stamina (no random roll)");
            if (fixedStamClick) {
                state.active_config.fixed_stamina = !state.active_config.fixed_stamina;
            }
            ay += 32;

            float contentBottom = std::max(ay, by) + 10.0f;
            EndScissorMode();

            // contentBottom was computed with baseY = 68 - customScroll already applied,
            // so convert back to an absolute content height independent of current scroll.
            float contentHeight = contentBottom + customScroll - 68.0f;
            float maxScroll = std::max(0.0f, contentHeight - scrollArea.height);
            customScroll = std::max(0.0f, std::min(customScroll, maxScroll));
            customScrollMaxCache = maxScroll;
            if (maxScroll > 0.0f) {
                float barH = scrollArea.height * (scrollArea.height / contentHeight);
                float barY = scrollArea.y + (customScroll / maxScroll) * (scrollArea.height - barH);
                DrawRectangle((int)(scrollArea.x + scrollArea.width - 6), (int)barY, 5, (int)barH, LIGHTGRAY);
            }

            // ── Fixed controls below the scroll area (always visible) ──
            float fixedY = scrollArea.y + scrollArea.height + 10.0f;

            // Custom Map checkbox + Open Editor button
            static int savedManualCount[5] = {-1,-1,-1,-1,-1}; // -1 = chưa lưu lần nào
            static bool customMapEverInitialized = false;
            auto syncCountsFromSpawns = [&]() {
                int c[5] = {0,0,0,0,0};
                for (const auto& zs : state.active_config.custom_zombie_spawns) {
                    switch (zs.type) {
                        case ZombieType::Clever:    c[0]++; break;
                        case ZombieType::Fast:      c[1]++; break;
                        case ZombieType::Exploding: c[2]++; break;
                        case ZombieType::Vampire:   c[3]++; break;
                        case ZombieType::Sick:      c[4]++; break;
                    }
                }
                state.active_config.count_normal    = c[0];
                state.active_config.count_fast      = c[1];
                state.active_config.count_exploding = c[2];
                state.active_config.count_vampire   = c[3];
                state.active_config.count_sick      = c[4];
            };

            bool customMapClick = drawCheckbox(mouse, mouseClicked, sliderX, fixedY, state.active_config.custom_map_mode, "Use Custom Map");
            if (customMapClick) {
                bool wasOn = state.active_config.custom_map_mode;
                state.active_config.custom_map_mode = !wasOn;
                if (!wasOn) {
                    // Turning ON custom map — only reinitialize from Main Menu sliders the FIRST time ever
                    if (!customMapEverInitialized) {
                        state.active_config.custom_grid.assign(state.active_config.map_width,
                            std::vector<Terrain>(state.active_config.map_height, Terrain::Dirt));
                        state.active_config.custom_zombie_spawns.clear();
                        state.active_config.custom_human_pos_set = false;
                        customMapEverInitialized = true;
                    }
                    if (!state.active_config.custom_zombie_spawns.empty()) {
                        syncCountsFromSpawns();
                    }
                } else {
                    // Turning OFF custom map — restore manual counts if we saved any
                    if (savedManualCount[0] >= 0) {
                        state.active_config.count_normal    = savedManualCount[0];
                        state.active_config.count_fast      = savedManualCount[1];
                        state.active_config.count_exploding = savedManualCount[2];
                        state.active_config.count_vampire   = savedManualCount[3];
                        state.active_config.count_sick      = savedManualCount[4];
                    }
                }
            }

            // Whenever the slider is unlocked (custom map off, or on but no manual zombie spawns yet),
            // keep tracking the player's manually-adjusted counts so we can restore them later.
            if (!state.active_config.custom_map_mode || state.active_config.custom_zombie_spawns.empty()) {
                savedManualCount[0] = state.active_config.count_normal;
                savedManualCount[1] = state.active_config.count_fast;
                savedManualCount[2] = state.active_config.count_exploding;
                savedManualCount[3] = state.active_config.count_vampire;
                savedManualCount[4] = state.active_config.count_sick;
            }
            if (state.active_config.custom_map_mode) {
                Rectangle editorBtn = { sliderX + 230, fixedY - 3, 200, 28 };
                DrawRectangleRec(editorBtn, (Color){140,90,20,255});
                drawCenteredText("Open Map Editor", editorBtn, 14, WHITE);
                if (mouseClicked && CheckCollisionPointRec(mouse, editorBtn)) {
                    state.current_scene = GameScene::MapEditor;
                }
            }

            Rectangle launchBtn = { sliderX, fixedY + 33, sliderW, 26 };
            DrawRectangleRec(launchBtn, overflow ? (Color){80,80,80,255} : (Color){15,110,15,255});
            drawCenteredText(overflow ? "TOO MANY ZOMBIES" : "LAUNCH CUSTOM GAME", launchBtn, 16, WHITE);
            if (!overflow && mouseClicked && CheckCollisionPointRec(mouse, launchBtn)) {
                state.init_game();
                centerViewOnHuman();
                state.current_scene = GameScene::Playing;
                AudioManager::getInstance().playMusic("battle");
            }

            // ── File Browser popups (Save / Load .zom) ──
            auto drawFileBrowser = [&](FileBrowser& fb, const char* title) {
                if (!fb.is_open) return;
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle box = { GetScreenWidth()/2.0f - 280, GetScreenHeight()/2.0f - 220, 560, 440 };
                DrawRectangleRec(box, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(box, 2, (Color){100, 100, 100, 255});
                DrawText(title, (int)box.x + 15, (int)box.y + 12, 18, (Color){230,210,100,255});

                Rectangle upBtn = { box.x + 15, box.y + 40, 50, 26 };
                DrawRectangleRec(upBtn, (Color){70,70,70,255});
                drawCenteredText("Up", upBtn, 14, WHITE);
                DrawText(fb.current_dir.c_str(), (int)(box.x + 75), (int)box.y + 46, 13, (Color){130,210,255,255});

                if (!fb.error_msg.empty()) {
                    DrawText(fb.error_msg.c_str(), (int)box.x + 15, (int)box.y + 70, 13, (Color){255,90,90,255});
                }

                float listY = box.y + 92;
                float listH = (fb.mode == FileBrowser::Mode::Save) ? 240.0f : 280.0f;
                Rectangle listBox = { box.x + 15, listY, box.width - 30, listH };
                DrawRectangleRec(listBox, (Color){18,18,20,255});

                static float browserScroll = 0.0f;
                if (CheckCollisionPointRec(mouse, listBox)) browserScroll -= GetMouseWheelMove() * 20.0f;
                float rowH = 22.0f;
                float contentH = fb.entries.size() * rowH;
                float maxScroll = std::max(0.0f, contentH - listBox.height);
                browserScroll = std::max(0.0f, std::min(browserScroll, maxScroll));

                BeginScissorMode((int)listBox.x, (int)listBox.y, (int)listBox.width, (int)listBox.height);
                for (size_t i = 0; i < fb.entries.size(); ++i) {
                    float ry = listBox.y + i * rowH - browserScroll;
                    if (ry < listBox.y - rowH || ry > listBox.y + listBox.height) continue;
                    Rectangle rowRect = { listBox.x, ry, listBox.width, rowH };
                    bool hovered = CheckCollisionPointRec(mouse, rowRect);
                    if (hovered) DrawRectangleRec(rowRect, (Color){50,50,55,255});
                    std::string label = (fb.entries[i].is_dir ? "[DIR] " : "       ") + fb.entries[i].name;
                    Color col = fb.entries[i].is_dir ? (Color){255,210,90,255} : RAYWHITE;
                    DrawText(label.c_str(), (int)listBox.x + 5, (int)ry + 3, 14, col);
                    if (mouseClicked && hovered) {
                        if (fb.entries[i].is_dir) {
                            if (fb.entries[i].name == "..") {
                                fb.navigate(fs::path(fb.current_dir).parent_path().string());
                            } else {
                                fb.navigate((fs::path(fb.current_dir) / fb.entries[i].name).string());
                            }
                        } else {
                            fb.selected_path = (fs::path(fb.current_dir) / fb.entries[i].name).string();
                            strncpy(fb.filename_buf, fb.entries[i].name.c_str(), sizeof(fb.filename_buf) - 1);
                        }
                    }
                }
                EndScissorMode();

                float belowListY = listY + listH + 10;
                if (fb.mode == FileBrowser::Mode::Save) {
                    DrawText("File name:", (int)box.x + 15, (int)belowListY, 14, RAYWHITE);
                    Rectangle nameBox = { box.x + 110, belowListY - 4, box.width - 125, 26 };
                    DrawRectangleRec(nameBox, (Color){20,20,22,255});
                    DrawRectangleLinesEx(nameBox, 1, (Color){90,90,90,255});
                    DrawText(fb.filename_buf, (int)nameBox.x + 5, (int)nameBox.y + 5, 14, RAYWHITE);
                    int ch = GetCharPressed();
                    while (ch > 0) {
                        size_t len = strlen(fb.filename_buf);
                        if (ch >= 32 && ch <= 125 && len < sizeof(fb.filename_buf) - 1) {
                            fb.filename_buf[len] = (char)ch;
                            fb.filename_buf[len + 1] = '\0';
                        }
                        ch = GetCharPressed();
                    }
                    if (IsKeyPressed(KEY_BACKSPACE)) {
                        size_t len = strlen(fb.filename_buf);
                        if (len > 0) fb.filename_buf[len - 1] = '\0';
                    }
                } else if (!fb.selected_path.empty()) {
                    DrawText(("Selected: " + fs::path(fb.selected_path).filename().string()).c_str(),
                             (int)box.x + 15, (int)belowListY, 14, (Color){100,255,140,255});
                }

                Rectangle confirmBtn = { box.x + box.width - 260, box.y + box.height - 45, 120, 34 };
                Rectangle cancelBtn  = { box.x + box.width - 130, box.y + box.height - 45, 120, 34 };
                bool isSave = (fb.mode == FileBrowser::Mode::Save);
                DrawRectangleRec(confirmBtn, isSave ? (Color){20,120,20,255} : (Color){20,90,150,255});
                drawCenteredText(isSave ? "Save" : "Open", confirmBtn, 15, WHITE);
                DrawRectangleRec(cancelBtn, (Color){70,70,70,255});
                drawCenteredText("Cancel", cancelBtn, 15, WHITE);

                if (mouseClicked && CheckCollisionPointRec(mouse, upBtn)) {
                    fb.navigate(fs::path(fb.current_dir).parent_path().string());
                }
                if (mouseClicked && CheckCollisionPointRec(mouse, cancelBtn)) {
                    fb.is_open = false;
                }
                if (mouseClicked && CheckCollisionPointRec(mouse, confirmBtn)) {
                    if (isSave) {
                        std::string fname = fb.filename_buf;
                        if (!fname.empty()) {
                            if (fname.size() < 4 || fname.substr(fname.size() - 4) != ".zom") fname += ".zom";
                            std::string path = (fs::path(fb.current_dir) / fname).string();
                            bool ok = state.export_challenge_file(path);
                            ioMessage = ok ? ("Exported to " + fname) : "Export failed!";
                            ioMessageTimer = 3.0f;
                            fb.is_open = false;
                        }
                    } else if (!fb.selected_path.empty()) {
                        bool ok = state.import_challenge_file(fb.selected_path);
                        ioMessage = ok ? ("Imported " + fs::path(fb.selected_path).filename().string()) : "Import failed!";
                        ioMessageTimer = 3.0f;
                        hasImportedConfig = ok;
                        fb.is_open = false;
                    }
                }
            };

            drawFileBrowser(saveBrowser, "Save Challenge File (.zom)");
            drawFileBrowser(loadBrowser, "Load Challenge File (.zom)");

            if (showConfirmExitGame) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 240, GetScreenHeight()/2.0f - 90, 480, 180 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
                DrawText("Are you sure you want to quit the game?", (int)popup.x + 20, (int)popup.y + 30, 16, RAYWHITE);

                Rectangle yesBtn = { popup.x + 40,  popup.y + 110, 180, 40 };
                Rectangle noBtn  = { popup.x + 260, popup.y + 110, 180, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
                drawCenteredText("Yes, Quit Game", yesBtn, 16, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 16, WHITE);

                if (mouseClicked && CheckCollisionPointRec(mouse, yesBtn)) {
                    shouldQuit = true;
                    showConfirmExitGame = false;
                } else if (mouseClicked && CheckCollisionPointRec(mouse, noBtn)) {
                    showConfirmExitGame = false;
                }
            }

            if (showImportWarnPopup) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 280, GetScreenHeight()/2.0f - 130, 560, 260 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){200, 160, 40, 255});
                DrawText("Import Compatibility Warning", (int)popup.x + 20, (int)popup.y + 15, 18, (Color){255, 210, 80, 255});

                // Multi-line message rendering
                {
                    float ly = popup.y + 50;
                    size_t start = 0;
                    while (start <= importWarnMessage.size()) {
                        size_t nl = importWarnMessage.find('\n', start);
                        std::string line = importWarnMessage.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
                        DrawText(line.c_str(), (int)popup.x + 20, (int)ly, 15, RAYWHITE);
                        ly += 22;
                        if (nl == std::string::npos) break;
                        start = nl + 1;
                    }
                }

                Rectangle yesBtn = { popup.x + 60,  popup.y + popup.height - 55, 200, 40 };
                Rectangle noBtn  = { popup.x + 300, popup.y + popup.height - 55, 200, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 100, 20, 255});
                drawCenteredText("Yes, Import Anyway", yesBtn, 15, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 15, WHITE);

                if (mouseClicked && CheckCollisionPointRec(mouse, yesBtn)) {
                    bool ok = state.import_challenge_file(pendingImportPath);
                    ioMessage = ok ? ("Imported " + fs::path(pendingImportPath).filename().string()) : "Import failed!";
                    ioMessageTimer = 3.0f;
                    hasImportedConfig = ok;
                    showImportWarnPopup = false;
                } else if (mouseClicked && CheckCollisionPointRec(mouse, noBtn)) {
                    showImportWarnPopup = false;
                }
            }

            EndDrawing();
            continue;
        }
        // ══════════════════════════════════════════════════════════════
        if (state.current_scene == GameScene::MapEditor) {
            int mw = state.active_config.map_width;
            int mh = state.active_config.map_height;

            static int editorViewX = 0;
            static int editorViewY = 0;
            int editorMaxViewX = std::max(0, mw - VIEW_CELLS);
            int editorMaxViewY = std::max(0, mh - VIEW_CELLS);
            editorViewX = std::max(0, std::min(editorViewX, editorMaxViewX));
            editorViewY = std::max(0, std::min(editorViewY, editorMaxViewY));

            float scrollThicknessEditor = 12.0f;
            float colTerrainX = boardOffset + VIEW_CELLS * cellSize + 6.0f + scrollThicknessEditor + 24.0f;
            float colZombieX  = colTerrainX + 210;
            float colHumanX   = colZombieX + 210;

            auto resizeCustomMap = [&](int newW, int newH) {
                newW = std::max(1, newW);
                newH = std::max(1, newH);
                auto oldGrid = state.active_config.custom_grid;
                int oldW = (int)oldGrid.size();
                int oldH = oldGrid.empty() ? 0 : (int)oldGrid[0].size();

                std::vector<std::vector<Terrain>> newGrid(newW, std::vector<Terrain>(newH, Terrain::Dirt));
                for (int x = 0; x < std::min(oldW, newW); ++x)
                    for (int y = 0; y < std::min(oldH, newH); ++y)
                        newGrid[x][y] = oldGrid[x][y];
                state.active_config.custom_grid = newGrid;
                state.active_config.map_width = newW;
                state.active_config.map_height = newH;

                auto& spawns = state.active_config.custom_zombie_spawns;
                spawns.erase(std::remove_if(spawns.begin(), spawns.end(),
                    [&](const ZombieSpawn& zs) { return zs.pos.x >= newW || zs.pos.y >= newH; }),
                    spawns.end());

                if (state.active_config.custom_human_pos_set &&
                    (state.active_config.custom_human_pos.x >= newW || state.active_config.custom_human_pos.y >= newH)) {
                    state.active_config.custom_human_pos_set = false;
                }

                editorViewX = std::max(0, std::min(editorViewX, std::max(0, newW - VIEW_CELLS)));
                editorViewY = std::max(0, std::min(editorViewY, std::max(0, newH - VIEW_CELLS)));
            };

            Rectangle mapWField    = { colTerrainX, 40, 150, 30 };
            Rectangle mapWMinus    = { colTerrainX + 155, 40, 26, 30 };
            Rectangle mapWPlus     = { colTerrainX + 185, 40, 26, 30 };
            Rectangle mapHField    = { colZombieX, 40, 150, 30 };
            Rectangle mapHMinus    = { colZombieX + 155, 40, 26, 30 };
            Rectangle mapHPlus     = { colZombieX + 185, 40, 26, 30 };

            const float btnH = 36.0f;
            const float btnStep = 40.0f;
            const float btnStartY = 130.0f;

            Rectangle brushDirt   = { colTerrainX, btnStartY + 0*btnStep, 200, btnH };
            Rectangle brushWall   = { colTerrainX, btnStartY + 1*btnStep, 200, btnH };
            Rectangle brushWater  = { colTerrainX, btnStartY + 2*btnStep, 200, btnH };
            Rectangle brushForest = { colTerrainX, btnStartY + 3*btnStep, 200, btnH };
            Rectangle brushIce    = { colTerrainX, btnStartY + 4*btnStep, 200, btnH };
            Rectangle brushResetTerrain = { colTerrainX, btnStartY + 5*btnStep, 200, btnH };

            Rectangle brushZClever   = { colZombieX, btnStartY + 0*btnStep, 200, btnH };
            Rectangle brushZFast     = { colZombieX, btnStartY + 1*btnStep, 200, btnH };
            Rectangle brushZExplode  = { colZombieX, btnStartY + 2*btnStep, 200, btnH };
            Rectangle brushZVampire  = { colZombieX, btnStartY + 3*btnStep, 200, btnH };
            Rectangle brushZSick     = { colZombieX, btnStartY + 4*btnStep, 200, btnH };
            Rectangle brushZErase    = { colZombieX, btnStartY + 5*btnStep, 200, btnH };
            Rectangle brushZEraseAll = { colZombieX, btnStartY + 6*btnStep, 200, btnH };

            Rectangle brushHuman       = { colHumanX, btnStartY + 0*btnStep, 200, btnH };
            Rectangle brushHumanUnset  = { colHumanX, btnStartY + 1*btnStep, 200, btnH };

            float belowColumnsY = btnStartY + 7 * btnStep + 60.0f;
            Rectangle resetMapBtn    = { colTerrainX, belowColumnsY,        260, 44 };
            Rectangle saveReturnBtn  = { colTerrainX, belowColumnsY + 60.0f, 260, 44 };

            bool anyPopupOpen = showConfirmEraseAllZombies || showConfirmResetMap || showConfirmResetTerrain;
            if (mouseClicked && !anyPopupOpen) {
                if (CheckCollisionPointRec(mouse, mapWMinus)) resizeCustomMap(mw - 1, mh);
                if (CheckCollisionPointRec(mouse, mapWPlus))  resizeCustomMap(mw + 1, mh);
                if (CheckCollisionPointRec(mouse, mapHMinus)) resizeCustomMap(mw, mh - 1);
                if (CheckCollisionPointRec(mouse, mapHPlus))  resizeCustomMap(mw, mh + 1);

                if (CheckCollisionPointRec(mouse, brushDirt))   { editorSelectedTerrain = Terrain::Dirt;   editorPlacingZombie = false; editorPlacingHuman = false; }
                if (CheckCollisionPointRec(mouse, brushWall))   { editorSelectedTerrain = Terrain::Wall;   editorPlacingZombie = false; editorPlacingHuman = false; }
                if (CheckCollisionPointRec(mouse, brushWater))  { editorSelectedTerrain = Terrain::Water;  editorPlacingZombie = false; editorPlacingHuman = false; }
                if (CheckCollisionPointRec(mouse, brushForest)) { editorSelectedTerrain = Terrain::Forest; editorPlacingZombie = false; editorPlacingHuman = false; }
                if (CheckCollisionPointRec(mouse, brushIce))    { editorSelectedTerrain = Terrain::Ice;    editorPlacingZombie = false; editorPlacingHuman = false; }
                if (CheckCollisionPointRec(mouse, brushResetTerrain)) {
                    showConfirmResetTerrain = true;
                    resetTerrainJustOpened = true;
                }

                if (CheckCollisionPointRec(mouse, brushZClever))  { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = false; editorSelectedZombieType = ZombieType::Clever; }
                if (CheckCollisionPointRec(mouse, brushZFast))    { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = false; editorSelectedZombieType = ZombieType::Fast; }
                if (CheckCollisionPointRec(mouse, brushZExplode)) { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = false; editorSelectedZombieType = ZombieType::Exploding; }
                if (CheckCollisionPointRec(mouse, brushZVampire)) { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = false; editorSelectedZombieType = ZombieType::Vampire; }
                if (CheckCollisionPointRec(mouse, brushZSick))    { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = false; editorSelectedZombieType = ZombieType::Sick; }
                if (CheckCollisionPointRec(mouse, brushZErase))   { editorPlacingZombie = true; editorPlacingHuman = false; editorEraseZombieMode = true; }

                if (CheckCollisionPointRec(mouse, brushHuman))    { editorPlacingHuman = true; editorPlacingZombie = false; }
                if (CheckCollisionPointRec(mouse, brushHumanUnset)) {
                    state.active_config.custom_human_pos_set = false;
                    editorPlacingHuman = false;
                }
                if (CheckCollisionPointRec(mouse, brushZEraseAll)) {
                    showConfirmEraseAllZombies = true;
                    eraseAllZombiesJustOpened = true;
                }

                if (CheckCollisionPointRec(mouse, resetMapBtn)) {
                    showConfirmResetMap = true;
                    resetMapJustOpened = true;
                }

                if (CheckCollisionPointRec(mouse, saveReturnBtn)) {
                    if (!state.active_config.custom_zombie_spawns.empty()) {
                        int cClever = 0, cFast = 0, cExplode = 0, cVampire = 0, cSick = 0;
                        for (const auto& zs : state.active_config.custom_zombie_spawns) {
                            switch (zs.type) {
                                case ZombieType::Clever:    cClever++; break;
                                case ZombieType::Fast:      cFast++; break;
                                case ZombieType::Exploding: cExplode++; break;
                                case ZombieType::Vampire:   cVampire++; break;
                                case ZombieType::Sick:      cSick++; break;
                            }
                        }
                        state.active_config.count_normal    = cClever;
                        state.active_config.count_fast      = cFast;
                        state.active_config.count_exploding = cExplode;
                        state.active_config.count_vampire   = cVampire;
                        state.active_config.count_sick      = cSick;
                    }
                    state.current_scene = GameScene::MainMenu;
                }
            }
            // Human placement: only on click, never on Wall tiles
            if (editorPlacingHuman && mouseClicked && !anyPopupOpen) {
                int lx = (int)((mouse.x - boardOffset) / cellSize);
                int ly = (int)((mouse.y - boardOffset) / cellSize);
                int tx = editorViewX + lx;
                int ty = editorViewY + ly;
                if (lx >= 0 && lx < VIEW_CELLS && ly >= 0 && ly < VIEW_CELLS &&
                    tx >= 0 && tx < mw && ty >= 0 && ty < mh &&
                    state.active_config.custom_grid[tx][ty] != Terrain::Wall) {
                    state.active_config.custom_human_pos = {tx, ty};
                    state.active_config.custom_human_pos_set = true;
                }
            }

            // Zombie placement: only on click (not held), and never on Wall tiles
            if (editorPlacingZombie && mouseClicked && !anyPopupOpen) {
                int lx = (int)((mouse.x - boardOffset) / cellSize);
                int ly = (int)((mouse.y - boardOffset) / cellSize);
                int tx = editorViewX + lx;
                int ty = editorViewY + ly;
                if (lx >= 0 && lx < VIEW_CELLS && ly >= 0 && ly < VIEW_CELLS &&
                    tx >= 0 && tx < mw && ty >= 0 && ty < mh) {
                    auto& spawns = state.active_config.custom_zombie_spawns;
                    spawns.erase(std::remove_if(spawns.begin(), spawns.end(),
                        [&](const ZombieSpawn& zs) { return zs.pos.x == tx && zs.pos.y == ty; }),
                        spawns.end());
                    if (!editorEraseZombieMode && state.active_config.custom_grid[tx][ty] != Terrain::Wall) {
                        spawns.push_back({Position{tx, ty}, editorSelectedZombieType});
                    }
                }
            }

            // Terrain painting: held drag, like before
            if (!editorPlacingZombie && !editorPlacingHuman && !anyPopupOpen && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int lx = (int)((mouse.x - boardOffset) / cellSize);
                int ly = (int)((mouse.y - boardOffset) / cellSize);
                int tx = editorViewX + lx;
                int ty = editorViewY + ly;
                if (lx >= 0 && lx < VIEW_CELLS && ly >= 0 && ly < VIEW_CELLS &&
                    tx >= 0 && tx < mw && ty >= 0 && ty < mh) {
                    if (state.active_config.custom_human_pos.x == tx && state.active_config.custom_human_pos.y == ty &&
                        editorSelectedTerrain == Terrain::Wall) {
                        // Prevent placing Wall directly on Human's spawn tile
                    } else {
                        state.active_config.custom_grid[tx][ty] = editorSelectedTerrain;
                    }
                }
            }

            BeginDrawing();
            ClearBackground((Color){22, 23, 25, 255});

            for (int lx = 0; lx < VIEW_CELLS; ++lx) {
                for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                    int x = editorViewX + lx;
                    int y = editorViewY + ly;
                    if (x >= mw || y >= mh) continue;
                    DrawRectangle((int)(lx * cellSize + boardOffset), (int)(ly * cellSize + boardOffset),
                                  (int)(cellSize - 2), (int)(cellSize - 2), terrainColor(state.active_config.custom_grid[x][y]));
                    if (state.active_config.custom_human_pos_set &&
                        x == state.active_config.custom_human_pos.x && y == state.active_config.custom_human_pos.y) {
                        DrawCircle((int)(lx * cellSize + boardOffset + cellSize/2), (int)(ly * cellSize + boardOffset + cellSize/2), 8.0f, WHITE);
                    }
                }
            }

            // Coordinate labels
            for (int lx = 0; lx < VIEW_CELLS; ++lx) {
                int mapX = editorViewX + lx;
                if (mapX >= mw) continue;
                DrawText(TextFormat("%d", mapX + 1), (int)(lx * cellSize + boardOffset + cellSize * 0.35f), (int)(boardOffset - 16), 12, (Color){180,190,205,255});
            }
            for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                int mapY = editorViewY + ly;
                if (mapY >= mh) continue;
                DrawText(TextFormat("%d", mapY + 1), (int)(boardOffset - 16), (int)(ly * cellSize + boardOffset + cellSize * 0.28f), 12, (Color){180,190,205,255});
            }

            for (const auto& zs : state.active_config.custom_zombie_spawns) {
                int zlx = zs.pos.x - editorViewX;
                int zly = zs.pos.y - editorViewY;
                if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
                float zx = zlx * cellSize + boardOffset + 3.0f;
                float zy = zly * cellSize + boardOffset + 3.0f;
                DrawRectangle((int)zx, (int)zy, (int)(cellSize - 6), (int)(cellSize - 6), zombieColor(zs.type));
            }

            // ── Horizontal scrollbar ──
            {
                static bool ehDragging = false;
                static float ehDragStartMouseX = 0.0f;
                static float ehDragStartPct = 0.0f;

                float trackW = VIEW_CELLS * cellSize;
                Rectangle hTrack = { boardOffset, boardOffset + VIEW_CELLS * cellSize + 6, trackW, scrollThicknessEditor };
                DrawRectangleRounded(hTrack, 0.5f, 4, (Color){40,40,40,255});
                if (editorMaxViewX > 0) {
                    float thumbW = std::max(30.0f, (float)VIEW_CELLS / mw * trackW);
                    float curPct = (float)editorViewX / editorMaxViewX;
                    float thumbX = hTrack.x + curPct * (trackW - thumbW);
                    Rectangle hThumb = { thumbX, hTrack.y, thumbW, scrollThicknessEditor };
                    bool hHover = CheckCollisionPointRec(mouse, hThumb);
                    DrawRectangleRounded(hThumb, 0.5f, 4, hHover ? (Color){170,170,170,255} : (Color){110,110,110,255});

                    if (mouseDown) {
                        if (!ehDragging && CheckCollisionPointRec(mouse, hThumb)) {
                            ehDragging = true;
                            ehDragStartMouseX = mouse.x;
                            ehDragStartPct = curPct;
                        }
                        if (ehDragging) {
                            float deltaPx = mouse.x - ehDragStartMouseX;
                            float deltaPct = deltaPx / (trackW - thumbW);
                            float newPct = std::max(0.0f, std::min(1.0f, ehDragStartPct + deltaPct));
                            editorViewX = (int)std::round(newPct * editorMaxViewX);
                        }
                    } else {
                        ehDragging = false;
                    }
                }
            }

            // ── Vertical scrollbar ──
            {
                static bool evDragging = false;
                static float evDragStartMouseY = 0.0f;
                static float evDragStartPct = 0.0f;

                float trackH = VIEW_CELLS * cellSize;
                Rectangle vTrack = { boardOffset + VIEW_CELLS * cellSize + 6, boardOffset, scrollThicknessEditor, trackH };
                DrawRectangleRounded(vTrack, 0.5f, 4, (Color){40,40,40,255});
                if (editorMaxViewY > 0) {
                    float thumbH = std::max(30.0f, (float)VIEW_CELLS / mh * trackH);
                    float curPct = (float)editorViewY / editorMaxViewY;
                    float thumbY = vTrack.y + curPct * (trackH - thumbH);
                    Rectangle vThumb = { vTrack.x, thumbY, scrollThicknessEditor, thumbH };
                    bool vHover = CheckCollisionPointRec(mouse, vThumb);
                    DrawRectangleRounded(vThumb, 0.5f, 4, vHover ? (Color){170,170,170,255} : (Color){110,110,110,255});

                    if (mouseDown) {
                        if (!evDragging && CheckCollisionPointRec(mouse, vThumb)) {
                            evDragging = true;
                            evDragStartMouseY = mouse.y;
                            evDragStartPct = curPct;
                        }
                        if (evDragging) {
                            float deltaPx = mouse.y - evDragStartMouseY;
                            float deltaPct = deltaPx / (trackH - thumbH);
                            float newPct = std::max(0.0f, std::min(1.0f, evDragStartPct + deltaPct));
                            editorViewY = (int)std::round(newPct * editorMaxViewY);
                        }
                    } else {
                        evDragging = false;
                    }
                }
            }

            // ── Map size row ──
            // ── Map size row ──
            DrawText("Map Size:", (int)colTerrainX, 15, 18, (Color){255, 140, 220, 255});

            DrawRectangleRec(mapWField, (Color){35,35,38,255});
            drawCenteredText(TextFormat("Width: %d", mw), mapWField, 15, WHITE);
            DrawRectangleRec(mapWMinus, DARKGRAY);
            drawCenteredText("-", mapWMinus, 16, WHITE);
            DrawRectangleRec(mapWPlus, DARKGRAY);
            drawCenteredText("+", mapWPlus, 16, WHITE);

            DrawRectangleRec(mapHField, (Color){35,35,38,255});
            drawCenteredText(TextFormat("Height: %d", mh), mapHField, 15, WHITE);
            DrawRectangleRec(mapHMinus, DARKGRAY);
            drawCenteredText("-", mapHMinus, 16, WHITE);
            DrawRectangleRec(mapHPlus, DARKGRAY);
            drawCenteredText("+", mapHPlus, 16, WHITE);

            DrawText("Shrinking removes terrain/zombies outside new bounds; Human position may unset.",
                     (int)mapWField.x, (int)(mapWField.y + mapWField.height + 6), 12, (Color){255,200,80,255});

            // ── Column 1: Terrain ──
            DrawText("Terrain:", (int)brushDirt.x, btnStartY - 24.0f, 18, (Color){255, 140, 220, 255});
            DrawRectangleRec(brushDirt, (!editorPlacingZombie && !editorPlacingHuman && editorSelectedTerrain == Terrain::Dirt) ? (Color){160,90,40,255} : DARKGRAY);
            drawCenteredText("Dirt", brushDirt, 16, WHITE);
            DrawRectangleRec(brushWall, (!editorPlacingZombie && !editorPlacingHuman && editorSelectedTerrain == Terrain::Wall) ? (Color){110,110,120,255} : DARKGRAY);
            drawCenteredText("Wall", brushWall, 16, WHITE);
            DrawRectangleRec(brushWater, (!editorPlacingZombie && !editorPlacingHuman && editorSelectedTerrain == Terrain::Water) ? (Color){60,120,180,255} : DARKGRAY);
            drawCenteredText("Water", brushWater, 16, WHITE);
            DrawRectangleRec(brushForest, (!editorPlacingZombie && !editorPlacingHuman && editorSelectedTerrain == Terrain::Forest) ? (Color){50,150,60,255} : DARKGRAY);
            drawCenteredText("Forest", brushForest, 16, WHITE);
            DrawRectangleRec(brushIce, (!editorPlacingZombie && !editorPlacingHuman && editorSelectedTerrain == Terrain::Ice) ? (Color){140,200,240,255} : DARKGRAY);
            drawCenteredText("Ice", brushIce, 16, WHITE);
            auto terrainNameStr = [](Terrain t) -> const char* {
                switch (t) {
                    case Terrain::Dirt:   return "Dirt";
                    case Terrain::Wall:   return "Wall";
                    case Terrain::Water:  return "Water";
                    case Terrain::Forest: return "Forest";
                    case Terrain::Ice:    return "Ice";
                    default: return "?";
                }
            };
            DrawRectangleRec(brushResetTerrain, (Color){140,20,20,255});
            drawCenteredText(TextFormat("Fill All: %s", terrainNameStr(editorSelectedTerrain)), brushResetTerrain, 14, WHITE);

            // ── Column 2: Zombies ──
            DrawText("Zombies:", (int)brushZClever.x, btnStartY - 24.0f, 18, (Color){255, 140, 220, 255});
            auto zBrushColor = [&](ZombieType t, bool erase) -> Color {
                if (editorPlacingZombie && ((erase && editorEraseZombieMode) || (!erase && !editorEraseZombieMode && editorSelectedZombieType == t)))
                    return (Color){200, 60, 60, 255};
                return DARKGRAY;
            };
            DrawRectangleRec(brushZClever, zBrushColor(ZombieType::Clever, false));
            drawCenteredText("Clever", brushZClever, 15, (Color){45,175,90,255});
            DrawRectangleRec(brushZFast, zBrushColor(ZombieType::Fast, false));
            drawCenteredText("Fast", brushZFast, 15, (Color){55,168,255,255});
            DrawRectangleRec(brushZExplode, zBrushColor(ZombieType::Exploding, false));
            drawCenteredText("Exploding", brushZExplode, 15, (Color){220,110,15,255});
            DrawRectangleRec(brushZVampire, zBrushColor(ZombieType::Vampire, false));
            drawCenteredText("Vampire", brushZVampire, 15, (Color){130,30,130,255});
            DrawRectangleRec(brushZSick, zBrushColor(ZombieType::Sick, false));
            drawCenteredText("Sick", brushZSick, 15, (Color){210,190,65,255});
            DrawRectangleRec(brushZErase, (editorPlacingZombie && editorEraseZombieMode) ? (Color){200,60,60,255} : DARKGRAY);
            drawCenteredText("Erase Zombie", brushZErase, 15, WHITE);
            DrawRectangleRec(brushZEraseAll, (Color){140,20,20,255});
            drawCenteredText("Erase All Zombies", brushZEraseAll, 15, WHITE);
            if (state.active_config.custom_zombie_spawns.empty()) {
                DrawText("(random spawn)", (int)brushZClever.x, (int)(brushZEraseAll.y + brushZEraseAll.height + 8), 13, LIGHTGRAY);
            }

            // ── Column 3: Human ──
            DrawText("Human:", (int)brushHuman.x, btnStartY - 24.0f, 18, (Color){255, 140, 220, 255});
            DrawRectangleRec(brushHuman, editorPlacingHuman ? (Color){200,60,60,255} : DARKGRAY);
            drawCenteredText("Place Human", brushHuman, 15, WHITE);
            DrawRectangleRec(brushHumanUnset, DARKGRAY);
            drawCenteredText("Unset Position", brushHumanUnset, 15, WHITE);
            if (!state.active_config.custom_human_pos_set) {
                DrawText("(random spawn)", (int)brushHuman.x, (int)(brushHumanUnset.y + brushHumanUnset.height + 8), 13, LIGHTGRAY);
            }

            DrawRectangleRec(saveReturnBtn, (Color){15,110,15,255});
            drawCenteredText("Save & Return", saveReturnBtn, 16, WHITE);
            DrawRectangleRec(resetMapBtn, (Color){140,20,20,255});
            drawCenteredText("Reset to Empty Map", resetMapBtn, 16, WHITE);

            if (showConfirmEraseAllZombies) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 220, GetScreenHeight()/2.0f - 80, 440, 160 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
                DrawText("Erase ALL placed zombies from this map?", (int)popup.x + 20, (int)popup.y + 25, 16, RAYWHITE);
                DrawText("This cannot be undone.", (int)popup.x + 20, (int)popup.y + 50, 14, (Color){255,200,80,255});

                Rectangle yesBtn = { popup.x + 40,  popup.y + 100, 170, 40 };
                Rectangle noBtn  = { popup.x + 230, popup.y + 100, 170, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
                drawCenteredText("Yes, Erase All", yesBtn, 15, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 15, WHITE);

                if (mouseClicked && !eraseAllZombiesJustOpened && CheckCollisionPointRec(mouse, yesBtn)) {
                    state.active_config.custom_zombie_spawns.clear();
                    showConfirmEraseAllZombies = false;
                } else if (mouseClicked && !eraseAllZombiesJustOpened && CheckCollisionPointRec(mouse, noBtn)) {
                    showConfirmEraseAllZombies = false;
                }
                eraseAllZombiesJustOpened = false;
            }

            if (showConfirmResetMap) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 220, GetScreenHeight()/2.0f - 80, 440, 160 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
                DrawText("Reset map to empty?", (int)popup.x + 20, (int)popup.y + 22, 16, RAYWHITE);
                DrawText("(All Dirt, no zombies, no Human)", (int)popup.x + 20, (int)popup.y + 44, 13, (Color){200,200,200,255});
                DrawText("This cannot be undone.", (int)popup.x + 20, (int)popup.y + 68, 14, (Color){255,200,80,255});

                Rectangle yesBtn = { popup.x + 40,  popup.y + 100, 170, 40 };
                Rectangle noBtn  = { popup.x + 230, popup.y + 100, 170, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
                drawCenteredText("Yes, Reset", yesBtn, 15, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 15, WHITE);

                if (mouseClicked && !resetMapJustOpened && CheckCollisionPointRec(mouse, yesBtn)) {
                    state.active_config.custom_grid.assign(mw, std::vector<Terrain>(mh, Terrain::Dirt));
                    state.active_config.custom_zombie_spawns.clear();
                    state.active_config.custom_human_pos_set = false;
                    showConfirmResetMap = false;
                } else if (mouseClicked && !resetMapJustOpened && CheckCollisionPointRec(mouse, noBtn)) {
                    showConfirmResetMap = false;
                }
                resetMapJustOpened = false;
            }

            if (showConfirmResetTerrain) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 220, GetScreenHeight()/2.0f - 80, 440, 160 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
                DrawText(TextFormat("Fill ALL terrain with %s?", terrainNameStr(editorSelectedTerrain)),
                         (int)popup.x + 20, (int)popup.y + 22, 16, RAYWHITE);
                DrawText("Zombies and Human position are kept.", (int)popup.x + 20, (int)popup.y + 44, 13, (Color){200,200,200,255});
                DrawText("This cannot be undone.", (int)popup.x + 20, (int)popup.y + 68, 14, (Color){255,200,80,255});

                Rectangle yesBtn = { popup.x + 40,  popup.y + 100, 170, 40 };
                Rectangle noBtn  = { popup.x + 230, popup.y + 100, 170, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
                drawCenteredText("Yes, Fill", yesBtn, 15, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 15, WHITE);

                if (mouseClicked && !resetTerrainJustOpened && CheckCollisionPointRec(mouse, yesBtn)) {
                    state.active_config.custom_grid.assign(mw, std::vector<Terrain>(mh, editorSelectedTerrain));
                    showConfirmResetTerrain = false;
                } else if (mouseClicked && !resetTerrainJustOpened && CheckCollisionPointRec(mouse, noBtn)) {
                    showConfirmResetTerrain = false;
                }
                resetTerrainJustOpened = false;
            }

            if (showConfirmExitGame) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
                Rectangle popup = { GetScreenWidth()/2.0f - 240, GetScreenHeight()/2.0f - 90, 480, 180 };
                DrawRectangleRec(popup, (Color){35, 36, 40, 255});
                DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
                DrawText("WARNING: All unsaved progress will be permanently lost!", (int)popup.x + 20, (int)popup.y + 20, 15, (Color){255,200,80,255});
                DrawText("Are you sure you want to quit the game?", (int)popup.x + 20, (int)popup.y + 45, 15, RAYWHITE);

                Rectangle yesBtn = { popup.x + 40,  popup.y + 110, 180, 40 };
                Rectangle noBtn  = { popup.x + 260, popup.y + 110, 180, 40 };
                DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
                drawCenteredText("Yes, Quit Game", yesBtn, 16, WHITE);
                DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
                drawCenteredText("Cancel", noBtn, 16, WHITE);

                if (mouseClicked && CheckCollisionPointRec(mouse, yesBtn)) {
                    shouldQuit = true;
                    showConfirmExitGame = false;
                } else if (mouseClicked && CheckCollisionPointRec(mouse, noBtn)) {
                    showConfirmExitGame = false;
                }
            }

            EndDrawing();
            continue;
        }

        bool onPanel = CheckCollisionPointRec(mouse, endTurnBtn) || CheckCollisionPointRec(mouse, moveBtn) ||
               CheckCollisionPointRec(mouse, knifeBtn)   || CheckCollisionPointRec(mouse, pistolBtn) ||
               CheckCollisionPointRec(mouse, shotgunBtn) || CheckCollisionPointRec(mouse, grenadeBtn) ||
               CheckCollisionPointRec(mouse, molotovBtn) || CheckCollisionPointRec(mouse, mineBtn) ||
               CheckCollisionPointRec(mouse, icePickBtn) || CheckCollisionPointRec(mouse, guideBtn) ||
               CheckCollisionPointRec(mouse, returnHubTopBtn) || CheckCollisionPointRec(mouse, warpBoltBtn);

        // ══════════════════════════════════════════════════════════════
        // INPUT HANDLING — mirrors main.cpp's event loop for Playing scene
        // ══════════════════════════════════════════════════════════════

        if (state.current_scene == GameScene::Playing &&
            !state.game_over && !state.game_won &&
            !showGuide && !showConfirmReturnHub && !showConfirmExitGame &&
            mouseClicked) {
            if (CheckCollisionPointRec(mouse, guideBtn)) {
                showGuide = true;
            } else if (CheckCollisionPointRec(mouse, returnHubTopBtn)) {
                showConfirmReturnHub = true;
            }
        }

        if (state.current_scene == GameScene::Playing &&
            !state.game_over && !state.game_won &&
            state.phase == TurnPhase::HumanTurn &&
            !state.human.is_paralyzed &&
            !showGuide && !showConfirmReturnHub && !showConfirmExitGame &&
            mouseClicked) {

            bool disabled_base = (state.human.stamina == 0);

            if (CheckCollisionPointRec(mouse, moveBtn) && !disabled_base) {
                state.input_mode = InputMode::MoveMode;
            } else if (CheckCollisionPointRec(mouse, knifeBtn) && !disabled_base) {
                state.input_mode = InputMode::TargetKnife;
            } else if (CheckCollisionPointRec(mouse, pistolBtn) && !disabled_base && state.human.pistol_ammo > 0) {
                state.input_mode = InputMode::TargetPistol;
            } else if (CheckCollisionPointRec(mouse, shotgunBtn) && !disabled_base && state.human.shotgun_ammo > 0) {
                state.input_mode = InputMode::TargetShotgun;
            } else if (CheckCollisionPointRec(mouse, grenadeBtn) && !disabled_base && state.human.grenades > 0) {
                state.input_mode = InputMode::TargetGrenade;
            } else if (CheckCollisionPointRec(mouse, molotovBtn) && !disabled_base && state.human.molotovs > 0) {
                state.input_mode = InputMode::TargetMolotov;
            } else if (CheckCollisionPointRec(mouse, warpBoltBtn) && !disabled_base && state.human.warp_ammo > 0) {
                state.input_mode = InputMode::TargetWarpBolt;
            } else if (CheckCollisionPointRec(mouse, mineBtn) && !disabled_base && state.human.mines > 0 &&
                       state.grid[state.human.pos.x][state.human.pos.y] != Terrain::Ice &&
                       !state.mine_grid[state.human.pos.x][state.human.pos.y]) {
                state.input_mode = InputMode::MoveMode;
                state.kills_this_turn = 0;
                state.pending_multikill_banner.clear();
                for (auto& z : state.zombies) if (z->hp > 0) z->kill_counted = false;
                state.mine_grid[state.human.pos.x][state.human.pos.y] = true;
                state.human.mines--; state.human.stamina--;
                AudioManager::getInstance().playSound("mine_plant");
                if (state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Fire) {
                    state.mine_grid[state.human.pos.x][state.human.pos.y] = false;
                    state.queue_explosion(state.human.pos.x, state.human.pos.y);
                }
            } else if (CheckCollisionPointRec(mouse, icePickBtn)) {
                int cost = state.human.is_frozen
                    ? GameConstants::Weapons::ICE_PICK_STAMINA_COST_FROZEN
                    : GameConstants::Weapons::ICE_PICK_STAMINA_COST_NORMAL;
                bool onIceNow = state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice;
                if (onIceNow && state.human.stamina >= cost) {
                    state.use_ice_pick();
                }
            } else if (CheckCollisionPointRec(mouse, endTurnBtn)) {
                endTurnWithBanner();
            }
        }

        // Board click — mirrors main.cpp's MouseButtonPressed handling for the board
        if (state.current_scene == GameScene::Playing &&
            !state.game_over && !state.game_won &&
            state.phase == TurnPhase::HumanTurn &&
            !state.human.is_paralyzed &&
            !showGuide && !showConfirmReturnHub && !showConfirmExitGame &&
            mouseClicked && !onPanel) {

            int lx = (int)((mouse.x - boardOffset) / cellSize);
            int ly = (int)((mouse.y - boardOffset) / cellSize);
            int tx = viewX + lx;
            int ty = viewY + ly;

            if (lx >= 0 && lx < VIEW_CELLS && ly >= 0 && ly < VIEW_CELLS &&
                tx >= 0 && tx < state.width && ty >= 0 && ty < state.height) {
                if (state.input_mode == InputMode::MoveMode) {
                    if (state.human.stamina == 0) {
                        endTurnWithBanner();
                    } else if (!state.human.is_frozen) {
                        int dx = std::abs(tx - state.human.pos.x);
                        int dy = std::abs(ty - state.human.pos.y);
                        if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                            if (state.grid[tx][ty] != Terrain::Wall) {
                                bool blocked = false;
                                for (const auto& z : state.zombies) {
                                    if (z->hp > 0 && z->pos == Position{tx, ty}) { blocked = true; break; }
                                }
                                if (!blocked) {
                                    int cost = (state.grid[tx][ty] == Terrain::Water) ? 2 : 1;
                                    if (state.human.stamina >= cost) {
                                        int move_dx = tx - state.human.pos.x;
                                        int move_dy = ty - state.human.pos.y;
                                        state.kills_this_turn = 0;
                                        state.pending_multikill_banner.clear();
                                        for (auto& z : state.zombies) if (z->hp > 0) z->kill_counted = false;
                                        state.human.pos = {tx, ty};
                                        state.human.stamina -= cost;
                                        AudioManager::getInstance().playSound("footstep");
                                        state.check_fire_interactions();
                                        state.check_mine_interactions();
                                        state.check_loot_pickup();
                                        if (state.human.hp > 0 &&
                                            state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice) {
                                            bool stun = false;
                                            state.try_ice_slide(true, 0, move_dx, move_dy, stun);
                                            if (stun) endTurnWithBanner();
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else if (state.input_mode == InputMode::TargetKnife) {
                    int dx = std::abs(tx - state.human.pos.x);
                    int dy = std::abs(ty - state.human.pos.y);
                    bool adjacent = (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0));
                    if (adjacent) {
                        state.handle_weapon_click(tx, ty, cellSize, boardOffset);
                    }
                } else if (state.input_mode == InputMode::TargetWarpBolt) {
                    int dx = std::abs(tx - state.human.pos.x);
                    int dy = std::abs(ty - state.human.pos.y);
                    bool adjacent = (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0));
                    if (adjacent) {
                        int vx = (tx > state.human.pos.x) - (tx < state.human.pos.x);
                        int vy = (ty > state.human.pos.y) - (ty < state.human.pos.y);
                        state.handle_warp_bolt(vx, vy);
                    }
                } else {
                    // Pistol, Shotgun, Grenade, Molotov: only accept clicks within the
                    // 8 adjacent tiles around Human (matches main.cpp's directional arrows)
                    int dx = std::abs(tx - state.human.pos.x);
                    int dy = std::abs(ty - state.human.pos.y);
                    bool adjacent = (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0));
                    if (adjacent) {
                        state.handle_weapon_click(tx, ty, cellSize, boardOffset);
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════════
        // PER-FRAME LOGIC UPDATE — mirrors main.cpp's main_loop() block
        // for GameScene::Playing (FX timers, phase updates)
        // ══════════════════════════════════════════════════════════════
        if (state.current_scene == GameScene::Playing) {
            if (state.turn_banner_fx.type != FXType::None) {
                state.turn_banner_fx.timer -= dtSeconds;
                if (state.turn_banner_fx.timer <= 0.0f) {
                    state.turn_banner_fx.type = FXType::None;
                    state.turn_banner_fx.banner_text = "";
                }
            }
            if (state.active_fx.type != FXType::None) {
                state.active_fx.timer -= dtSeconds;
                if (state.active_fx.timer <= 0.0f) state.active_fx.type = FXType::None;
            }
            if (state.active_fx.type == FXType::None && !state.explosion_queue.empty()) {
                auto ev = state.explosion_queue.front();
                state.explosion_queue.pop();
                state.execute_explosion_internal(ev.cx, ev.cy, ev.is_zombie_exploding);
            }
            for (auto it = state.attack_animations.begin(); it != state.attack_animations.end();) {
                it->timer -= dtSeconds;
                if (it->timer <= 0.0f) it = state.attack_animations.erase(it);
                else ++it;
            }
            for (auto it = state.floating_texts.begin(); it != state.floating_texts.end();) {
                it->timer -= dtSeconds;
                if (it->timer <= 0.0f) it = state.floating_texts.erase(it);
                else ++it;
            }
            for (auto& ld : state.loot_drops) ld.blink_timer += dtSeconds;

            for (auto it = state.wind_push_animations.begin(); it != state.wind_push_animations.end();) {
                it->timer += dtSeconds;
                if (it->timer >= it->duration) it = state.wind_push_animations.erase(it);
                else ++it;
            }

            if (state.multikill_banner_timer > 0.0f) {
                state.multikill_banner_timer -= dtSeconds;
                if (state.multikill_banner_timer <= 0.0f) {
                    state.multikill_banner_timer = 0.0f;
                    state.multikill_banner.clear();
                    state.pending_multikill_count = 0;
                }
            }
            if (!state.pending_multikill_banner.empty() &&
                state.active_fx.type == FXType::None &&
                state.attack_animations.empty() &&
                state.explosion_queue.empty()) {
                state.multikill_banner       = state.pending_multikill_banner;
                state.multikill_banner_timer = GameState::MULTIKILL_BANNER_DURATION;
                state.pending_multikill_banner.clear();
            }

            if (state.phase == TurnPhase::HumanTurn && !state.game_over && !state.game_won &&
                state.human.is_paralyzed) {
                state.add_log(state.tr("[SHOCK] Human is paralyzed and cannot act! Turn ends automatically.",
                                       "[SOC] Human bi te liet va khong the hanh dong! Tu dong ket thuc luot."),
                              ImVec4(0.45f, 0.9f, 1.0f, 1.0f));
                endTurnWithBanner();
            }

            if (!showConfirmReturnHub && !showConfirmExitGame) {
                state.update_zombie_logic(dtSeconds);
                state.update_environment_logic(dtSeconds);
            }
        }

        // ══════════════════════════════════════════════════════════════
        // RENDERING — Raylib equivalent of main.cpp's SFML/ImGui drawing
        // ══════════════════════════════════════════════════════════════
        BeginDrawing();
        ClearBackground((Color){22, 23, 25, 255});

        for (int lx = 0; lx < VIEW_CELLS; ++lx) {
            for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                int x = viewX + lx;
                int y = viewY + ly;
                if (x >= state.width || y >= state.height) continue;
                DrawRectangle((int)(lx * cellSize + boardOffset), (int)(ly * cellSize + boardOffset),
                              (int)(cellSize - 2.0f), (int)(cellSize - 2.0f), getTerrainDisplayColor(state, x, y));
                if (state.mine_grid[x][y]) {
                    DrawCircle((int)(lx * cellSize + boardOffset + cellSize / 2), (int)(ly * cellSize + boardOffset + cellSize / 2),
                               6.0f, (Color){230, 40, 40, 255});
                }
            }
        }

        // ── Column/row coordinate labels ──
        for (int lx = 0; lx < VIEW_CELLS; ++lx) {
            int mapX = viewX + lx;
            if (mapX >= state.width) continue;
            DrawText(TextFormat("%d", mapX + 1), (int)(lx * cellSize + boardOffset + cellSize * 0.35f), (int)(boardOffset - 16), 12, (Color){180,190,205,255});
        }
        for (int ly = 0; ly < VIEW_CELLS; ++ly) {
            int mapY = viewY + ly;
            if (mapY >= state.height) continue;
            DrawText(TextFormat("%d", mapY + 1), (int)(boardOffset - 16), (int)(ly * cellSize + boardOffset + cellSize * 0.28f), 12, (Color){180,190,205,255});
        }

        for (size_t li = 0; li < state.loot_drops.size(); ++li) {
            const auto& ld = state.loot_drops[li];
            float animGX, animGY;
            bool windAnim = getWindAnimGridPos(state, false, 0, true, li, false, 0, animGX, animGY);
            float gx = windAnim ? animGX : (float)ld.pos.x;
            float gy = windAnim ? animGY : (float)ld.pos.y;
            int llx = (int)std::floor(gx) - viewX, lly = (int)std::floor(gy) - viewY;
            if (llx < 0 || llx >= VIEW_CELLS || lly < 0 || lly >= VIEW_CELLS) continue;
            int lx = (int)((gx - viewX) * cellSize + boardOffset);
            int ly = (int)((gy - viewY) * cellSize + boardOffset);
            DrawRectangle(lx + 4, ly + 4, (int)(cellSize - 8), (int)(cellSize - 8), (Color){80, 60, 20, 200});
            DrawText("?", lx + (int)(cellSize / 2) - 5, ly + (int)(cellSize / 2) - 10, 20, (Color){255, 220, 60, 255});
        }

        for (size_t gi = 0; gi < state.active_grenades.size(); ++gi) {
            const auto& g = state.active_grenades[gi];
            if (!g.active) continue;
            float animGX, animGY;
            bool windAnim = getWindAnimGridPos(state, false, 0, false, 0, true, gi, animGX, animGY);
            float gx = windAnim ? animGX : (float)g.pos.x;
            float gy = windAnim ? animGY : (float)g.pos.y;
            int glx = (int)std::floor(gx) - viewX, gly = (int)std::floor(gy) - viewY;
            if (glx < 0 || glx >= VIEW_CELLS || gly < 0 || gly >= VIEW_CELLS) continue;
            DrawCircle((int)((gx - viewX) * cellSize + boardOffset + cellSize / 2), (int)((gy - viewY) * cellSize + boardOffset + cellSize / 2),
                       8.0f, (Color){50, 210, 50, 255});
        }

        for (size_t zi = 0; zi < state.zombies.size(); ++zi) {
            const auto& z = state.zombies[zi];
            if (z->hp <= 0) continue;
            Position drawPos = z->pos;
            if (state.ice_slide_animation.active && !state.ice_slide_animation.is_human &&
                state.ice_slide_animation.zombie_idx == zi &&
                state.ice_slide_animation.current_step < (int)state.ice_slide_animation.path.size()) {
                drawPos = state.ice_slide_animation.path[state.ice_slide_animation.current_step];
            }
            drawPos = getWarpDisplayPos(state, drawPos);
            float animGX, animGY;
            bool windAnim = getWindAnimGridPos(state, false, zi, false, 0, false, 0, animGX, animGY);
            int zlx, zly;
            float zx, zy;
            if (windAnim) {
                zlx = (int)std::floor(animGX) - viewX; zly = (int)std::floor(animGY) - viewY;
                zx = (animGX - viewX) * cellSize + boardOffset + 3.0f;
                zy = (animGY - viewY) * cellSize + boardOffset + 3.0f;
            } else {
                zlx = drawPos.x - viewX; zly = drawPos.y - viewY;
                zx = zlx * cellSize + boardOffset + 3.0f;
                zy = zly * cellSize + boardOffset + 3.0f;
            }
            if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
            DrawRectangle((int)zx, (int)zy, (int)(cellSize - 6.0f), (int)(cellSize - 6.0f), zombieColor(z->type));
            DrawText(TextFormat("%d", z->hp), (int)zx + 12, (int)zy + 10, 16, WHITE);

            // Active zombie indicator: bright pulsing yellow border on the zombie whose turn it is
            if (state.phase == TurnPhase::ZombieAnimating && zi == state.active_zombie_idx) {
                float pulse = 0.5f + 0.5f * sinf(GetTime() * 12.0f);
                unsigned char borderA = (unsigned char)(180 + 75 * pulse);
                DrawRectangleLinesEx((Rectangle){zx, zy, cellSize - 6.0f, cellSize - 6.0f}, 2.5f, (Color){255, 255, 60, borderA});
            }

            // Clever Zombie weapon-ammo indicator: black right-angle triangle, top-right corner
            if (z->type == ZombieType::Clever && z->hasWeaponAmmo()) {
                float triSize = std::max(5.0f, (cellSize - 6.0f) * 0.28f);
                float ttx = zx + (cellSize - 6.0f) - triSize;
                float tty = zy;
                Vector2 t1 = { ttx, tty };              // top-left
                Vector2 t2 = { ttx + triSize, tty + triSize }; // bottom-right
                Vector2 t3 = { ttx + triSize, tty };     // top-right
                DrawTriangle(t1, t2, t3, (Color){0, 0, 0, 220});
            }

            std::string tags;
            if (z->is_burning)   tags += "B";
            if (z->is_paralyzed) tags += "P";
            if (z->is_frozen)    tags += "F";
            if (!tags.empty()) {
                DrawText(tags.c_str(), (int)zx + 5, (int)zy + 1, 16, BLACK);
                DrawText(tags.c_str(), (int)zx + 4, (int)zy, 16, (Color){255, 60, 60, 255});
            }
        }

        {
            Position drawPos = state.human.pos;
            if (state.ice_slide_animation.active && state.ice_slide_animation.is_human &&
                state.ice_slide_animation.current_step < (int)state.ice_slide_animation.path.size()) {
                drawPos = state.ice_slide_animation.path[state.ice_slide_animation.current_step];
            }
            drawPos = getWarpDisplayPos(state, drawPos);
            float animGX, animGY;
            bool windAnim = getWindAnimGridPos(state, true, 0, false, 0, false, 0, animGX, animGY);
            int hlx, hly; float hx, hy;
            if (windAnim) {
                hlx = (int)std::floor(animGX) - viewX; hly = (int)std::floor(animGY) - viewY;
                hx = (animGX - viewX) * cellSize + boardOffset + 3.0f;
                hy = (animGY - viewY) * cellSize + boardOffset + 3.0f;
            } else {
                hlx = drawPos.x - viewX; hly = drawPos.y - viewY;
                hx = hlx * cellSize + boardOffset + 3.0f;
                hy = hly * cellSize + boardOffset + 3.0f;
            }
            if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) {
                float hw = cellSize - 6.0f;

                // Base square + colored border for visual distinction from zombies
                DrawRectangle((int)hx, (int)hy, (int)hw, (int)hw, (Color){235, 240, 245, 255});

                // Heartbeat-style pulsing border: sharp double-beat rhythm, faster/redder when HP is low
                {
                    float t = GetTime();
                    float hpRatio = (float)state.human.hp / std::max(1, state.active_config.human_hp);
                    float beatSpeed = 1.0f + (1.0f - std::clamp(hpRatio, 0.0f, 1.0f)) * 5.0f; // faster when hurt
                    float phase = fmodf(t * beatSpeed, 1.0f);
                    // Two quick pulses per cycle (lub-dub)
                    float beat = std::max(std::exp(-phase * 18.0f), std::exp(-fabsf(phase - 0.35f) * 18.0f));
                    float thickness = beat * 4.0f;
                    unsigned char r = 255;
                    unsigned char g = (unsigned char)(60 + (1.0f - beat) * 100);
                    unsigned char b = (unsigned char)(60 + (1.0f - beat) * 100);
                    DrawRectangleLinesEx((Rectangle){hx, hy, hw, hw}, thickness, (Color){r, g, b, 255});
                }

                // HP number centered (like zombies)
                std::string hpStr = TextFormat("%d", state.human.hp);
                Vector2 hpSz = MeasureTextEx(gameFont, hpStr.c_str(), 16, 1.0f);
                DrawTextEx(gameFont, hpStr.c_str(),
                           (Vector2){hx + hw/2.0f - hpSz.x/2.0f, hy + hw/2.0f - hpSz.y/2.0f},
                           16, 1.0f, (Color){20, 30, 40, 255});

                // Stamina dots centered at the bottom
                int safeStam = std::max(0, std::min(10, state.human.stamina));
                if (safeStam > 0) {
                    float dotGap = 5.0f;
                    float totalW = (safeStam - 1) * dotGap;
                    float startX = hx + hw/2.0f - totalW/2.0f;
                    for (int d = 0; d < safeStam; ++d) {
                        DrawCircle((int)(startX + d * dotGap), (int)(hy + hw - 6), 2.3f, (Color){60, 170, 255, 255});
                    }
                }

                std::string htags;
                if (state.human.is_burning)   htags += "B";
                if (state.human.is_paralyzed) htags += "P";
                if (state.human.is_frozen)    htags += "F";
                if (!htags.empty()) {
                    DrawText(htags.c_str(), (int)hx + 5, (int)hy + 1, 16, BLACK);
                    DrawText(htags.c_str(), (int)hx + 4, (int)hy, 16, (Color){255, 60, 60, 255});
                }
            }
        }

        if (state.dark_cloud_active && state.active_fx.type != FXType::DarkCloud) {
            DrawRectangle((int)boardOffset, (int)boardOffset,
                          (int)(VIEW_CELLS * cellSize), (int)(VIEW_CELLS * cellSize),
                          (Color){0, 0, 0, 254});

            // Cells lit by fire: fire tile + 8 neighbors + burning entities' tiles
            std::vector<std::vector<bool>> fireLit(state.width, std::vector<bool>(state.height, false));
            const int all8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
            for (int x = 0; x < state.width; ++x) {
                for (int y = 0; y < state.height; ++y) {
                    if (state.grid[x][y] != Terrain::Fire) continue;
                    fireLit[x][y] = true;
                    for (const auto& d : all8) {
                        int nx = x + d[0], ny = y + d[1];
                        if (nx >= 0 && nx < state.width && ny >= 0 && ny < state.height) fireLit[nx][ny] = true;
                    }
                }
            }
            for (const auto& z : state.zombies) if (z->hp > 0 && z->is_burning) fireLit[z->pos.x][z->pos.y] = true;
            if (state.human.hp > 0 && state.human.is_burning) fireLit[state.human.pos.x][state.human.pos.y] = true;

            for (int lx = 0; lx < VIEW_CELLS; ++lx) {
                for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                    int x = viewX + lx, y = viewY + ly;
                    if (x < 0 || x >= state.width || y < 0 || y >= state.height) continue;
                    if (!fireLit[x][y]) continue;
                    DrawRectangle((int)(lx * cellSize + boardOffset), (int)(ly * cellSize + boardOffset),
                                  (int)(cellSize - 2), (int)(cellSize - 2), getTerrainDisplayColor(state, x, y));
                    if (state.mine_grid[x][y]) {
                        DrawCircle((int)(lx * cellSize + boardOffset + cellSize/2), (int)(ly * cellSize + boardOffset + cellSize/2),
                                   6.0f, (Color){230,40,40,255});
                    }
                }
            }
            // Redraw zombies standing on lit cells
            for (size_t zi = 0; zi < state.zombies.size(); ++zi) {
                const auto& z = state.zombies[zi];
                if (z->hp <= 0) continue;
                if (!fireLit[z->pos.x][z->pos.y]) continue;
                int zlx = z->pos.x - viewX, zly = z->pos.y - viewY;
                if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
                float zx = zlx * cellSize + boardOffset + 3.0f;
                float zy = zly * cellSize + boardOffset + 3.0f;
                DrawRectangle((int)zx, (int)zy, (int)(cellSize - 6.0f), (int)(cellSize - 6.0f), zombieColor(z->type));
                DrawText(TextFormat("%d", z->hp), (int)zx + 12, (int)zy + 10, 16, WHITE);

                // Active zombie indicator: bright pulsing yellow border on the zombie whose turn it is
                if (state.phase == TurnPhase::ZombieAnimating && zi == state.active_zombie_idx) {
                    float pulse = 0.5f + 0.5f * sinf(GetTime() * 12.0f);
                    unsigned char borderA = (unsigned char)(180 + 75 * pulse);
                    DrawRectangleLinesEx((Rectangle){zx, zy, cellSize - 6.0f, cellSize - 6.0f}, 2.5f, (Color){255, 255, 60, borderA});
                }

                // Clever Zombie weapon-ammo indicator: black right-angle triangle, top-right corner
                if (z->type == ZombieType::Clever && z->hasWeaponAmmo()) {
                    float triSize = std::max(5.0f, (cellSize - 6.0f) * 0.28f);
                    float ttx = zx + (cellSize - 6.0f) - triSize;
                    float tty = zy;
                    Vector2 t1 = { ttx, tty };              // top-left
                    Vector2 t2 = { ttx + triSize, tty + triSize }; // bottom-right
                    Vector2 t3 = { ttx + triSize, tty };     // top-right
                    DrawTriangle(t1, t2, t3, (Color){0, 0, 0, 220});
                }
            }
            // Always redraw Human on top of the shroud, regardless of fire-lit status
            if (state.human.hp > 0) {
                int hlx = state.human.pos.x - viewX, hly = state.human.pos.y - viewY;
                if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) {
                    float hx = hlx * cellSize + boardOffset + 3.0f;
                    float hy = hly * cellSize + boardOffset + 3.0f;
                    float hw = cellSize - 6.0f;

                    DrawRectangle((int)hx, (int)hy, (int)hw, (int)hw, (Color){235, 240, 245, 255});
                    {
                        float t = GetTime();
                        float hpRatio = (float)state.human.hp / std::max(1, state.active_config.human_hp);
                        float beatSpeed = 1.0f + (1.0f - std::clamp(hpRatio, 0.0f, 1.0f)) * 5.0f;
                        float phase = fmodf(t * beatSpeed, 1.0f);
                        float beat = std::max(std::exp(-phase * 18.0f), std::exp(-fabsf(phase - 0.35f) * 18.0f));
                        float thickness = beat * 4.0f;
                        unsigned char g2 = (unsigned char)(60 + (1.0f - beat) * 100);
                        DrawRectangleLinesEx((Rectangle){hx, hy, hw, hw}, thickness, (Color){255, g2, g2, 255});
                    }

                    std::string hpStr = TextFormat("%d", state.human.hp);
                    Vector2 hpSz = MeasureTextEx(gameFont, hpStr.c_str(), 16, 1.0f);
                    DrawTextEx(gameFont, hpStr.c_str(),
                               (Vector2){hx + hw/2.0f - hpSz.x/2.0f, hy + hw/2.0f - hpSz.y/2.0f},
                               16, 1.0f, (Color){20, 30, 40, 255});

                    int safeStam = std::max(0, std::min(10, state.human.stamina));
                    if (safeStam > 0) {
                        float dotGap = 5.0f;
                        float totalW = (safeStam - 1) * dotGap;
                        float startX = hx + hw/2.0f - totalW/2.0f;
                        for (int d = 0; d < safeStam; ++d) {
                            DrawCircle((int)(startX + d * dotGap), (int)(hy + hw - 6), 2.3f, (Color){60, 170, 255, 255});
                        }
                    }
                }
            }
        }

        // ── Directional arrows around Human — always on top, never hidden by Dark Cloud ──
        if (state.phase == TurnPhase::HumanTurn && !state.human.is_paralyzed &&
            ((state.input_mode == InputMode::MoveMode && !state.human.is_frozen) ||
             state.input_mode == InputMode::TargetKnife ||
             state.input_mode == InputMode::TargetPistol ||
             state.input_mode == InputMode::TargetShotgun ||
             state.input_mode == InputMode::TargetGrenade ||
             state.input_mode == InputMode::TargetMolotov ||
             state.input_mode == InputMode::TargetWarpBolt)) {
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = state.human.pos.x + dx;
                    int ny = state.human.pos.y + dy;
                    if (nx < 0 || nx >= state.width || ny < 0 || ny >= state.height) continue;

                    if (state.input_mode == InputMode::MoveMode) {
                        if (state.grid[nx][ny] == Terrain::Wall) continue;
                        bool blocked = false;
                        for (const auto& z : state.zombies) {
                            if (z->hp > 0 && z->pos == Position{nx, ny}) { blocked = true; break; }
                        }
                        if (blocked) continue;
                        int cost = (state.grid[nx][ny] == Terrain::Water) ? 2 : 1;
                        if (state.human.stamina < cost) continue;
                    }
                    if (state.input_mode == InputMode::TargetKnife) {
                        bool hasZ = false;
                        for (const auto& z : state.zombies) {
                            if (z->hp > 0 && z->pos == Position{nx, ny}) { hasZ = true; break; }
                        }
                        if (!hasZ || state.human.stamina < 1) continue;
                    }

                    int ovx = nx - viewX, ovy = ny - viewY;
                    if (ovx < 0 || ovx >= VIEW_CELLS || ovy < 0 || ovy >= VIEW_CELLS) continue;

                    float humanCX = (state.human.pos.x - viewX) * cellSize + boardOffset + cellSize / 2.0f;
                    float humanCY = (state.human.pos.y - viewY) * cellSize + boardOffset + cellSize / 2.0f;
                    float targetCX = ovx * cellSize + boardOffset + cellSize / 2.0f;
                    float targetCY = ovy * cellSize + boardOffset + cellSize / 2.0f;
                    // Pull arrow position toward Human (35% of the way from target to human)
                    const float PULL_FACTOR = 0.2f;
                    float acx = targetCX + (humanCX - targetCX) * PULL_FACTOR;
                    float acy = targetCY + (humanCY - targetCY) * PULL_FACTOR;
                    float angle = atan2f((float)dy, (float)dx);
                    float pulse = 1.0f + 0.15f * sinf(GetTime() * 15.0f);
                    float size = (state.input_mode == InputMode::TargetKnife ? 7.0f : 10.0f) * pulse;

                    Color arrowColor;
                    if (state.input_mode == InputMode::MoveMode) arrowColor = (Color){255, 220, 50, 230};
                    else if (state.input_mode == InputMode::TargetKnife) arrowColor = (Color){200, 100, 255, 250};
                    else if (state.input_mode == InputMode::TargetPistol || state.input_mode == InputMode::TargetShotgun) arrowColor = (Color){255, 60, 60, 230};
                    else if (state.input_mode == InputMode::TargetWarpBolt) arrowColor = (Color){150, 40, 255, 240};
                    else arrowColor = (Color){60, 255, 60, 230};

                    Vector2 p1 = { acx + cosf(angle) * size,        acy + sinf(angle) * size };
                    Vector2 p2 = { acx + cosf(angle - 2.0f) * size, acy + sinf(angle - 2.0f) * size };
                    Vector2 p3 = { acx + cosf(angle + 2.0f) * size, acy + sinf(angle + 2.0f) * size };
                    DrawTriangle(p1, p2, p3, arrowColor);
                }
            }
        }


        if (state.active_fx.type != FXType::None) {
            float progress = state.active_fx.timer / state.active_fx.max_duration;
            unsigned char alpha = (unsigned char)(progress * 255);
            Vector2 viewShift = { (float)(viewX * cellSize), (float)(viewY * cellSize) };

            auto toScreen = [&](sf::Vector2f p) -> Vector2 {
                return { p.x - viewShift.x, p.y - viewShift.y };
            };

            if (state.active_fx.type == FXType::Pistol) {
                Vector2 s = toScreen(state.active_fx.start_p);
                Vector2 e = toScreen(state.active_fx.end_p);
                DrawLineEx(s, e, 3.0f, (Color){255, 60, 60, alpha});
            } else if (state.active_fx.type == FXType::Knife) {
                Vector2 e = toScreen(state.active_fx.end_p);
                float t = 1.0f - progress;
                Vector2 slashStart = { e.x - 15 + 30 * t, e.y - 15 - 10 * t };
                Vector2 slashEnd   = { e.x + 15 - 10 * t, e.y + 15 + 30 * t };
                DrawLineEx(slashStart, slashEnd, 3.0f, (Color){230, 230, 230, alpha});
            } else if (state.active_fx.type == FXType::GrenadeFly) {
                Vector2 s = toScreen(state.active_fx.start_p);
                Vector2 e = toScreen(state.active_fx.end_p);
                float t = 1.0f - progress;
                Vector2 pos = { s.x + (e.x - s.x) * t, s.y + (e.y - s.y) * t };
                DrawCircle((int)pos.x, (int)pos.y, 6.0f, (Color){100, 255, 100, 230});
            } else if (state.active_fx.type == FXType::Molotov) {
                Vector2 s = toScreen(state.active_fx.start_p);
                Vector2 e = toScreen(state.active_fx.end_p);
                float t = 1.0f - progress;
                Vector2 pos = { s.x + (e.x - s.x) * t, s.y + (e.y - s.y) * t };
                DrawCircle((int)pos.x, (int)pos.y, 6.0f, (Color){255, 120, 0, 230});
            } else if (state.active_fx.type == FXType::Shotgun) {
                float intensity = sqrtf(progress);
                Color blastColor = (Color){255, 130, 30, (unsigned char)(intensity * 180)};
                for (const auto& p : state.active_fx.blast_cells) {
                    int bx = p.x - viewX, by = p.y - viewY;
                    if (bx < 0 || bx >= VIEW_CELLS || by < 0 || by >= VIEW_CELLS) continue;
                    DrawRectangle((int)(bx * cellSize + boardOffset + 1), (int)(by * cellSize + boardOffset + 1),
                                  (int)(cellSize - 2), (int)(cellSize - 2), blastColor);
                }
            } else if (state.active_fx.type == FXType::Explosion) {
                // Flickering/blinking effect: alternates between bright flash and dim
                float flash = 0.2f + 0.8f * std::abs(sinf(state.active_fx.timer * 35.0f));
                float intensity = progress * flash;
                Color blastColor = (Color){255, 50, 10, (unsigned char)(intensity * 240)};
                for (const auto& p : state.active_fx.blast_cells) {
                    int bx = p.x - viewX, by = p.y - viewY;
                    if (bx < 0 || bx >= VIEW_CELLS || by < 0 || by >= VIEW_CELLS) continue;
                    DrawRectangle((int)(bx * cellSize + boardOffset + 1), (int)(by * cellSize + boardOffset + 1),
                                  (int)(cellSize - 2), (int)(cellSize - 2), blastColor);
                }
            } else if (state.active_fx.type == FXType::Lightning) {
                for (const auto& p : state.active_fx.blast_cells) {
                    int bx = p.x - viewX, by = p.y - viewY;
                    if (bx < 0 || bx >= VIEW_CELLS || by < 0 || by >= VIEW_CELLS) continue;
                    DrawRectangle((int)(bx * cellSize + boardOffset + 2), (int)(by * cellSize + boardOffset + 2),
                                  (int)(cellSize - 4), (int)(cellSize - 4), (Color){80, 220, 255, (unsigned char)(progress * 130)});
                }
                int clx = state.active_fx.cx - viewX, cly = state.active_fx.cy - viewY;
                if (clx >= 0 && clx < VIEW_CELLS && cly >= 0 && cly < VIEW_CELLS) {
                    float targetX = clx * cellSize + boardOffset + cellSize / 2.0f;
                    float targetY = cly * cellSize + boardOffset + cellSize / 2.0f;
                    float startY = boardOffset;

                    // Deterministic noise from the fixed seed — shape never changes during the strike
                    unsigned int seed = (unsigned int)state.active_fx.lightning_seed;
                    auto noiseAt = [&](int a, int b) -> float {
                        unsigned int h = seed * 2654435761u + a * 73856093u + b * 19349663u;
                        h = (h ^ (h >> 13)) * 1274126177u;
                        h ^= (h >> 16);
                        return ((float)(h % 1000) / 1000.0f) * 2.0f - 1.0f; // -1..1
                    };

                    // Fade: bright flash at strike start, fading out with progress
                    unsigned char boltA = (unsigned char)(alpha);

                    // Main zigzag trunk: sharp broken segments, fixed shape
                    int segments = 6;
                    std::vector<Vector2> trunk;
                    trunk.push_back((Vector2){targetX, startY});
                    for (int i = 1; i < segments; ++i) {
                        float py = startY + (targetY - startY) * ((float)i / segments);
                        float maxDrift = 26.0f * (1.0f - (float)i / segments * 0.55f); // narrows toward strike
                        float nx = noiseAt(i, 0);
                        trunk.push_back((Vector2){targetX + nx * maxDrift, py});
                    }
                    trunk.push_back((Vector2){targetX, targetY}); // exact sharp tip at cell center

                    // Draw trunk: thick+branching near top, sharp thin near bottom
                    for (size_t i = 0; i + 1 < trunk.size(); ++i) {
                        float segT = (float)i / (trunk.size() - 1);
                        float thickness = 8.0f * (1.0f - segT) + 1.2f;
                        DrawLineEx(trunk[i], trunk[i+1], thickness + 3.0f, (Color){255, 225, 80, (unsigned char)(boltA * 0.45f)});
                        DrawLineEx(trunk[i], trunk[i+1], thickness, (Color){255, 250, 190, boltA});
                    }

                    // Root-like branches near the top (first 2-3 segments only)
                    int branchSegs = std::min(3, segments - 1);
                    for (int i = 0; i < branchSegs; ++i) {
                        Vector2 base = trunk[i];
                        float by2 = base.y + (trunk[i+1].y - base.y) * 0.5f;
                        for (int side = -1; side <= 1; side += 2) {
                            float bn = noiseAt(i + 10, side) ;
                            float branchLen = 16.0f - i * 3.0f;
                            Vector2 tip = { base.x + side * (10.0f + std::abs(bn) * 14.0f), by2 + branchLen * 0.5f };
                            float bthick = 2.5f - i * 0.5f;
                            DrawLineEx(base, tip, bthick + 2.0f, (Color){255, 220, 70, (unsigned char)(boltA * 0.3f)});
                            DrawLineEx(base, tip, bthick, (Color){255, 245, 170, (unsigned char)(boltA * 0.8f)});
                        }
                    }

                    // Bright core flash at the exact strike point, strongest at start, fading with progress
                    float flashIntensity = progress; // progress already goes 1 -> 0 over lifetime
                    DrawCircle((int)targetX, (int)targetY, 5.0f, (Color){255, 255, 225, (unsigned char)(255 * flashIntensity)});
                    DrawCircle((int)targetX, (int)targetY, 11.0f, (Color){255, 225, 90, (unsigned char)(140 * flashIntensity)});
                }
            } else if (state.active_fx.type == FXType::Rain) {
                float boardDim = VIEW_CELLS * cellSize;
                float t = GetTime();
                for (int i = 0; i < 100; ++i) {
                    float rx = boardOffset + fmodf(i * 47.0f + t * 320.0f, boardDim);
                    float ry = boardOffset + fmodf(i * 73.0f + t * 480.0f, boardDim);
                    DrawLineEx((Vector2){rx, ry}, (Vector2){rx - 8, ry + 18}, 1.5f, (Color){110, 170, 255, alpha});
                }
            } else if (state.active_fx.type == FXType::Wind) {
                float boardDim = VIEW_CELLS * cellSize;
                float t = GetTime();
                for (int i = 0; i < 25; ++i) {
                    float perp = fmodf(i * 37.0f, boardDim);
                    float sweep = fmodf(i * 71.0f + t * 750.0f, boardDim);
                    float sx, sy;
                    if (std::abs(state.active_fx.dx) > 0.5f) {
                        sx = boardOffset + (state.active_fx.dx > 0 ? sweep : (boardDim - sweep));
                        sy = boardOffset + perp;
                    } else {
                        sx = boardOffset + perp;
                        sy = boardOffset + (state.active_fx.dy > 0 ? sweep : (boardDim - sweep));
                    }
                    float ex = sx + state.active_fx.dx * 100.0f;
                    float ey = sy + state.active_fx.dy * 100.0f;
                    DrawLineEx((Vector2){sx, sy}, (Vector2){ex, ey}, 1.5f, (Color){180, 220, 255, alpha});
                }
            } else if (state.active_fx.type == FXType::DarkCloud) {
                DrawRectangle((int)boardOffset, (int)boardOffset, (int)(VIEW_CELLS * cellSize), (int)(VIEW_CELLS * cellSize),
                              (Color){0, 0, 0, (unsigned char)((1.0f - progress) * 200)});
            } else if (state.active_fx.type == FXType::Heatwave) {
                float intensity = 0.5f + 0.3f * sinf(GetTime() * 4.0f);
                DrawRectangle((int)boardOffset, (int)boardOffset, (int)(VIEW_CELLS * cellSize), (int)(VIEW_CELLS * cellSize),
                              (Color){255, 220, 100, (unsigned char)(intensity * 100)});
            } else if (state.active_fx.type == FXType::Blizzard) {
                float intensity = 0.4f + 0.2f * sinf(GetTime() * 3.0f);
                DrawRectangle((int)boardOffset, (int)boardOffset, (int)(VIEW_CELLS * cellSize), (int)(VIEW_CELLS * cellSize),
                              (Color){220, 240, 255, (unsigned char)(intensity * 70)});

                // Falling snowflakes, deterministic per-flake drift using index-based noise
                float boardDim = VIEW_CELLS * cellSize;
                float t = GetTime();
                int flakeCount = 60;
                for (int i = 0; i < flakeCount; ++i) {
                    float seedX = fmodf(i * 53.7f, boardDim);
                    float fallSpeed = 40.0f + (i % 5) * 15.0f;
                    float sway = sinf(t * 1.5f + i) * 8.0f;
                    float fx = boardOffset + seedX + sway;
                    float fy = boardOffset + fmodf(i * 91.3f + t * fallSpeed, boardDim);
                    float radius = 1.5f + (i % 3) * 0.8f;
                    unsigned char flakeA = (unsigned char)(alpha * (0.5f + 0.5f * ((i % 4) / 4.0f)));
                    DrawCircle((int)fx, (int)fy, radius, (Color){255, 255, 255, flakeA});
                }
            } else if (state.active_fx.type == FXType::WarpBolt) {
                float t = 1.0f - progress; // 0 -> 1 over time

                const float FLY_END = 0.3f;
                const float DARK_PEAK = 0.5f;
                const float DARK_END = 0.7f;

                if (t < FLY_END) {
                    // Phase 1: bullet flies from origin to dest
                    float flyT = t / FLY_END;
                    Vector2 s = { state.active_fx.start_p.x - viewX * cellSize, state.active_fx.start_p.y - viewY * cellSize };
                    Vector2 e = { state.active_fx.end_p.x   - viewX * cellSize, state.active_fx.end_p.y   - viewY * cellSize };
                    Vector2 pos = { s.x + (e.x - s.x) * flyT, s.y + (e.y - s.y) * flyT };
                    DrawCircle((int)pos.x, (int)pos.y, 6.0f, (Color){180, 80, 255, 255});
                    DrawCircle((int)pos.x, (int)pos.y, 10.0f, (Color){180, 80, 255, 100});
                } else {
                    // Phase 2/3: both cells darken then brighten
                    float blackness;
                    if (t < DARK_PEAK) blackness = (t - FLY_END) / (DARK_PEAK - FLY_END);
                    else if (t < DARK_END) blackness = 1.0f;
                    else blackness = std::max(0.0f, 1.0f - (t - DARK_END) / (1.0f - DARK_END));

                    for (const auto& p : state.active_fx.blast_cells) {
                        int bx = p.x - viewX, by = p.y - viewY;
                        if (bx < 0 || bx >= VIEW_CELLS || by < 0 || by >= VIEW_CELLS) continue;
                        unsigned char blackA = (unsigned char)(blackness * 255);
                        DrawRectangle((int)(bx * cellSize + boardOffset), (int)(by * cellSize + boardOffset),
                                      (int)(cellSize - 2), (int)(cellSize - 2), (Color){5, 0, 15, blackA});
                        float swirl = GetTime() * 6.0f + (p.x + p.y);
                        for (int i = 0; i < 4; ++i) {
                            float ang = swirl + i * (PI / 2.0f);
                            float r = 14.0f * blackness;
                            float px = bx * cellSize + boardOffset + cellSize/2.0f + cosf(ang) * r;
                            float py = by * cellSize + boardOffset + cellSize/2.0f + sinf(ang) * r;
                            DrawCircle((int)px, (int)py, 3.0f, (Color){150, 60, 255, blackA});
                        }
                    }
                }
            }
        }

        // Attack animations (bite/scratch) — mirrors main.cpp's attack_animations loop
        for (const auto& fx : state.attack_animations) {
            float progress = 1.0f - (fx.timer / fx.max_duration);
            unsigned char alpha = (unsigned char)((1.0f - progress) * 255);
            int clx = fx.cx - viewX, cly = fx.cy - viewY;
            if (clx < 0 || clx >= VIEW_CELLS || cly < 0 || cly >= VIEW_CELLS) continue;
            float cx = clx * cellSize + boardOffset + cellSize / 2.0f;
            float cy = cly * cellSize + boardOffset + cellSize / 2.0f;
            if (fx.type == FXType::Bite) {
                DrawCircle((int)cx, (int)cy, 10.0f * (1.0f - progress), (Color){255, 50, 50, alpha});
            } else if (fx.type == FXType::Scratch) {
                for (int i = -1; i <= 1; ++i) {
                    float off = i * 6.0f;
                    DrawLineEx((Vector2){cx - 10 + off, cy - 12 + off}, (Vector2){cx + 10 + off, cy + 12 + off}, 2.0f, (Color){220, 220, 220, alpha});
                }
            }
        }

        // Floating damage/heal numbers
        for (const auto& ft : state.floating_texts) {
            float progress = 1.0f - (ft.timer / ft.max_duration);
            unsigned char alpha = (unsigned char)((1.0f - progress) * 255);
            int flx = ft.pos.x - viewX, fly = ft.pos.y - viewY;
            if (flx < 0 || flx >= VIEW_CELLS || fly < 0 || fly >= VIEW_CELLS) continue;
            float cx = flx * cellSize + boardOffset + cellSize / 2.0f;
            float cy = fly * cellSize + boardOffset + cellSize / 2.0f;
            Color col = ft.amount > 0 ? (Color){50, 255, 50, alpha} : (Color){255, 50, 50, alpha};
            DrawText(TextFormat("%s%d", ft.amount > 0 ? "+" : "", ft.amount), (int)cx - 8, (int)(cy - 15 - progress * 25.0f), 16, col);
        }

        // ── Turn banner ("YOUR TURN" / custom text) ──
        if (state.turn_banner_fx.type != FXType::None) {
            float bannerCX = boardOffset + VIEW_CELLS * cellSize * 0.5f;
            float bannerCY = boardOffset + VIEW_CELLS * cellSize * 0.5f;
            std::string bannerStr = state.turn_banner_fx.banner_text.empty() ? "YOUR TURN" : state.turn_banner_fx.banner_text;
            float fontSize = state.turn_banner_fx.banner_text.empty() ? 56.0f : 36.0f;
            Vector2 sz = MeasureTextEx(gameFont, bannerStr.c_str(), fontSize, 1.0f);
            unsigned char a = 255;
            if (state.turn_banner_fx.banner_text.empty()) {
                float p = state.turn_banner_fx.timer / state.turn_banner_fx.max_duration;
                a = (unsigned char)(255 * p);
            }
            DrawTextEx(gameFont, bannerStr.c_str(), (Vector2){bannerCX - sz.x/2, bannerCY - sz.y/2}, fontSize, 1.0f, (Color){255, 245, 120, a});
        }

        // ── Multikill banner ──
        if (!state.multikill_banner.empty() && state.multikill_banner_timer > 0.0f) {
            float ratio = state.multikill_banner_timer / GameState::MULTIKILL_BANNER_DURATION;
            float fade = (ratio > 0.85f) ? (1.0f - ratio) / 0.15f
                       : (ratio < 0.25f) ? ratio / 0.25f
                       : 1.0f;
            unsigned char a = (unsigned char)(255 * fade);
            int k = state.pending_multikill_count;
            Color textCol;
            if      (k >= 9) textCol = (Color){255, 60, 255, a};
            else if (k >= 7) textCol = (Color){255, 80, 40, a};
            else if (k >= 5) textCol = (Color){255, 160, 20, a};
            else if (k >= 3) textCol = (Color){255, 240, 40, a};
            else             textCol = (Color){200, 255, 100, a};

            float bannerCX = boardOffset + VIEW_CELLS * cellSize * 0.5f;
            float bannerCY = boardOffset + VIEW_CELLS * cellSize * 0.5f - 22.0f;
            Vector2 sz = MeasureTextEx(gameFont, state.multikill_banner.c_str(), 44.0f, 1.0f);
            DrawTextEx(gameFont, state.multikill_banner.c_str(), (Vector2){bannerCX - sz.x/2 + 2, bannerCY - sz.y/2 + 2}, 44.0f, 1.0f, (Color){0, 0, 0, (unsigned char)(a * 0.6f)});
            DrawTextEx(gameFont, state.multikill_banner.c_str(), (Vector2){bannerCX - sz.x/2, bannerCY - sz.y/2}, 44.0f, 1.0f, textCol);

            std::string sub = TextFormat("%d kills in one action", k);
            Vector2 subSz = MeasureTextEx(gameFont, sub.c_str(), 18.0f, 1.0f);
            DrawTextEx(gameFont, sub.c_str(), (Vector2){bannerCX - subSz.x/2, bannerCY + 22 - subSz.y/2}, 18.0f, 1.0f, (Color){255, 210, 60, a});
        }

        // ── Horizontal scrollbar (below board) ──
        {
            static bool hDragging = false;
            static float hDragStartMouseX = 0.0f;
            static float hDragStartPct = 0.0f;

            int maxViewX = std::max(0, state.width - VIEW_CELLS);
            float trackW = VIEW_CELLS * cellSize;
            Rectangle hTrack = { boardOffset, boardOffset + VIEW_CELLS * cellSize + 6, trackW, scrollThickness };
            DrawRectangleRounded(hTrack, 0.5f, 4, (Color){40,40,40,255});
            if (maxViewX > 0) {
                float thumbW = std::max(30.0f, (float)VIEW_CELLS / state.width * trackW);
                float curPct = (float)viewX / maxViewX;
                float thumbX = hTrack.x + curPct * (trackW - thumbW);
                Rectangle hThumb = { thumbX, hTrack.y, thumbW, scrollThickness };
                bool hHover = CheckCollisionPointRec(mouse, hThumb);
                DrawRectangleRounded(hThumb, 0.5f, 4, hHover ? (Color){170,170,170,255} : (Color){110,110,110,255});

                if (mouseDown) {
                    if (!hDragging && CheckCollisionPointRec(mouse, hThumb)) {
                        hDragging = true;
                        hDragStartMouseX = mouse.x;
                        hDragStartPct = curPct;
                    }
                    if (hDragging) {
                        float deltaPx = mouse.x - hDragStartMouseX;
                        float deltaPct = deltaPx / (trackW - thumbW);
                        float newPct = std::max(0.0f, std::min(1.0f, hDragStartPct + deltaPct));
                        viewX = (int)std::round(newPct * maxViewX);
                    }
                } else {
                    hDragging = false;
                }
            }
        }

        // ── Vertical scrollbar (right of board) ──
        {
            static bool vDragging = false;
            static float vDragStartMouseY = 0.0f;
            static float vDragStartPct = 0.0f;

            int maxViewY = std::max(0, state.height - VIEW_CELLS);
            float trackH = VIEW_CELLS * cellSize;
            Rectangle vTrack = { boardOffset + VIEW_CELLS * cellSize + 6, boardOffset, scrollThickness, trackH };
            DrawRectangleRounded(vTrack, 0.5f, 4, (Color){40,40,40,255});
            if (maxViewY > 0) {
                float thumbH = std::max(30.0f, (float)VIEW_CELLS / state.height * trackH);
                float curPct = (float)viewY / maxViewY;
                float thumbY = vTrack.y + curPct * (trackH - thumbH);
                Rectangle vThumb = { vTrack.x, thumbY, scrollThickness, thumbH };
                bool vHover = CheckCollisionPointRec(mouse, vThumb);
                DrawRectangleRounded(vThumb, 0.5f, 4, vHover ? (Color){170,170,170,255} : (Color){110,110,110,255});

                if (mouseDown) {
                    if (!vDragging && CheckCollisionPointRec(mouse, vThumb)) {
                        vDragging = true;
                        vDragStartMouseY = mouse.y;
                        vDragStartPct = curPct;
                    }
                    if (vDragging) {
                        float deltaPx = mouse.y - vDragStartMouseY;
                        float deltaPct = deltaPx / (trackH - thumbH);
                        float newPct = std::max(0.0f, std::min(1.0f, vDragStartPct + deltaPct));
                        viewY = (int)std::round(newPct * maxViewY);
                    }
                } else {
                    vDragging = false;
                }
            }
        }

        DrawText(TextFormat("TURN %d/%d | ST %d | HP %d | Pos [%d,%d] | Env: %s",
                             state.current_turn, state.turn_limit, state.human.stamina, state.human.hp,
                             state.human.pos.x + 1, state.human.pos.y + 1, state.last_environment_event.c_str()),
                 (int)panelX, (int)boardOffset, 18, (Color){130, 220, 255, 255});

        // ── Music / SFX toggles (in-game) ──
        {
            float musicX = panelX + panelW - 190, sfxX = panelX + panelW - 80;
            float checkY = boardOffset - 4;
            bool musicClickG = drawCheckbox(mouse, mouseClicked, musicX, checkY, state.music_enabled, "Music", 13);
            bool sfxClickG   = drawCheckbox(mouse, mouseClicked, sfxX, checkY, state.sfx_enabled, "SFX", 13);

            if (musicClickG &&
                !showGuide && !showConfirmReturnHub && !showConfirmExitGame && !state.game_over && !state.game_won) {
                state.music_enabled = !state.music_enabled;
                if (state.music_enabled) AudioManager::getInstance().playMusic("battle");
                else AudioManager::getInstance().stopMusic();
            }
            if (sfxClickG &&
                !showGuide && !showConfirmReturnHub && !showConfirmExitGame && !state.game_over && !state.game_won) {
                state.sfx_enabled = !state.sfx_enabled;
                state.setSfxEnabled(state.sfx_enabled);
            }
        }

        if (state.game_over) DrawText("GAME OVER", 20, (int)(state.height * cellSize + boardOffset + 50), 24, RED);
        if (state.game_won)  DrawText("YOU WON",   20, (int)(state.height * cellSize + boardOffset + 50), 24, GREEN);

        DrawLine((int)panelX, (int)(boardOffset + 32), (int)(panelX + panelW), (int)(boardOffset + 32), (Color){70,70,70,255});

        // ── Top row: End Turn / Game Guide / Return Hub ──
        bool humanTurnUI = (state.phase == TurnPhase::HumanTurn && !state.game_over && !state.game_won);
        DrawRectangleRec(endTurnBtn, humanTurnUI ? DARKGREEN : GRAY);
        drawCenteredText("END TURN", endTurnBtn, 18, WHITE);

        DrawRectangleRec(guideBtn, (Color){160, 130, 10, 255});
        drawCenteredText("GUIDE", guideBtn, 18, WHITE);

        DrawRectangleRec(returnHubTopBtn, (Color){160, 20, 20, 255});
        drawCenteredText("Return Hub", returnHubTopBtn, 17, WHITE);

        DrawLine((int)panelX, (int)(boardOffset + 88), (int)(panelX + panelW), (int)(boardOffset + 88), (Color){70,70,70,255});

        // ── Actions label (own line, not overlapped by the separator) ──
        DrawText("Actions", (int)panelX, (int)(boardOffset + 92), 16, (Color){255, 140, 220, 255});

        // Unified weapon-button coloring: bright base, orange when active (selected),
        // dim/dark when unusable (disabled), so the player instantly reads availability.
        auto weaponBtnColor = [&](bool isActive, bool isDisabled) -> Color {
            if (isActive) return (Color){230, 140, 20, 255};       // orange = selected
            if (isDisabled) return (Color){45, 45, 48, 255};        // dark = unusable
            return (Color){70, 90, 100, 255};                       // bright neutral = usable
        };

        bool humanTurnNow = (state.phase == TurnPhase::HumanTurn && !state.game_over && !state.game_won && !state.human.is_paralyzed);
        int icePickCost = state.human.is_frozen
            ? GameConstants::Weapons::ICE_PICK_STAMINA_COST_FROZEN
            : GameConstants::Weapons::ICE_PICK_STAMINA_COST_NORMAL;
        bool onIceUI = state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice;
        bool noAdjZombie = true;
        for (const auto& z : state.zombies) {
            if (z->hp > 0 && std::abs(z->pos.x - state.human.pos.x) <= 1 && std::abs(z->pos.y - state.human.pos.y) <= 1) { noAdjZombie = false; break; }
        }
        bool staminaZero = (state.human.stamina == 0);

        bool moveDisabled    = !humanTurnNow || staminaZero;
        bool knifeDisabled   = !humanTurnNow || staminaZero || noAdjZombie;
        bool icePickDisabled = !humanTurnNow || !onIceUI || state.human.stamina < icePickCost;
        bool pistolDisabled  = !humanTurnNow || staminaZero || state.human.pistol_ammo <= 0;
        bool shotgunDisabled = !humanTurnNow || staminaZero || state.human.shotgun_ammo <= 0;
        bool warpBoltDisabled = !humanTurnNow || staminaZero || state.human.warp_ammo <= 0;
        bool grenadeDisabled = !humanTurnNow || staminaZero || state.human.grenades <= 0;
        bool molotovDisabled = !humanTurnNow || staminaZero || state.human.molotovs <= 0;
        bool mineDisabled    = !humanTurnNow || staminaZero || state.human.mines <= 0 ||
                                state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice ||
                                state.mine_grid[state.human.pos.x][state.human.pos.y];

        DrawRectangleRec(moveBtn, weaponBtnColor(state.input_mode == InputMode::MoveMode, moveDisabled));
        drawCenteredText("Move", moveBtn, 17, WHITE);

        DrawRectangleRec(knifeBtn, weaponBtnColor(state.input_mode == InputMode::TargetKnife, knifeDisabled));
        drawCenteredText("Knife", knifeBtn, 17, WHITE);

        DrawRectangleRec(icePickBtn, weaponBtnColor(false, icePickDisabled));
        drawCenteredText(TextFormat("Ice Pick [-%d ST]", icePickCost), icePickBtn, 16, WHITE);

        DrawRectangleRec(pistolBtn, weaponBtnColor(state.input_mode == InputMode::TargetPistol, pistolDisabled));
        drawCenteredText(TextFormat("Pistol (%d)", state.human.pistol_ammo), pistolBtn, 17, WHITE);

        DrawRectangleRec(shotgunBtn, weaponBtnColor(state.input_mode == InputMode::TargetShotgun, shotgunDisabled));
        drawCenteredText(TextFormat("Shotgun (%d)", state.human.shotgun_ammo), shotgunBtn, 17, WHITE);

        DrawRectangleRec(warpBoltBtn, weaponBtnColor(state.input_mode == InputMode::TargetWarpBolt, warpBoltDisabled));
        drawCenteredText(TextFormat("Warp Bolt (%d)", state.human.warp_ammo), warpBoltBtn, 15, WHITE);

        DrawRectangleRec(grenadeBtn, weaponBtnColor(state.input_mode == InputMode::TargetGrenade, grenadeDisabled));
        drawCenteredText(TextFormat("Grenade (%d)", state.human.grenades), grenadeBtn, 16, WHITE);

        DrawRectangleRec(molotovBtn, weaponBtnColor(state.input_mode == InputMode::TargetMolotov, molotovDisabled));
        drawCenteredText(TextFormat("Molotov (%d)", state.human.molotovs), molotovBtn, 16, WHITE);

        DrawRectangleRec(mineBtn, weaponBtnColor(false, mineDisabled));
        drawCenteredText(TextFormat("Mine (%d)", state.human.mines), mineBtn, 17, WHITE);

        DrawLine((int)panelX, (int)(boardOffset + 255), (int)(panelX + panelW), (int)(boardOffset + 255), (Color){70,70,70,255});

        // ── Two-column section: Zombies (left) | Terrain + Status (right) ──
        float sectionY = boardOffset + 265;
        float colLeftW = panelW * 0.55f; // wide enough for 10 zombies per row
        float colRightX = panelX + colLeftW + 20.0f;
        float colRightW = panelW - colLeftW - 20.0f;

        // Left column: zombie type legend with live counts (replaces the scrollable #index list)
        int aliveCount = 0;
        for (const auto& z : state.zombies) if (z->hp > 0) aliveCount++;
        DrawText(TextFormat("Zombies: %d", aliveCount), (int)panelX, (int)sectionY, 16, (Color){255,100,100,255});

        {
            int cvClever = 0, cvFast = 0, cvExploding = 0, cvVampire = 0, cvSick = 0;
            for (const auto& z : state.zombies) {
                if (z->hp <= 0) continue;
                switch (z->type) {
                    case ZombieType::Clever:    cvClever++; break;
                    case ZombieType::Fast:      cvFast++; break;
                    case ZombieType::Exploding: cvExploding++; break;
                    case ZombieType::Vampire:   cvVampire++; break;
                    case ZombieType::Sick:      cvSick++; break;
                }
            }
            struct LegendRow { const char* label; Color color; int count; };
            LegendRow rows[5] = {
                { "Clever",   zombieColor(ZombieType::Clever),    cvClever },
                { "Fast",     zombieColor(ZombieType::Fast),      cvFast },
                { "Exploder", zombieColor(ZombieType::Exploding), cvExploding },
                { "Vampire",  zombieColor(ZombieType::Vampire),   cvVampire },
                { "Sick",     zombieColor(ZombieType::Sick),      cvSick },
            };
            float itemW = colLeftW / 3.0f;
            float ly = sectionY + 25;
            for (int i = 0; i < 5; ++i) {
                float lx = panelX + (i % 3) * itemW;
                float ry = ly + (i / 3) * 22.0f;
                DrawRectangle((int)lx, (int)ry, 14, 14, rows[i].color);
                DrawText(TextFormat("%s: %d", rows[i].label, rows[i].count), (int)lx + 20, (int)(ry - 2), 14, RAYWHITE);
            }
        }

        // Right column: Terrain + Status legend, 3-per-row grid (matches original layout)
        DrawText("Terrain", (int)colRightX, (int)sectionY, 16, (Color){230, 210, 100, 255});
        {
            float tX = colRightX;
            float tY = sectionY + 24;
            float itemW = colRightW / 3.0f;
            auto drawTerrainItem = [&](const char* label, Color c) {
                DrawRectangle((int)tX, (int)tY, 14, 14, c);
                DrawText(label, (int)tX + 20, (int)(tY - 2), 14, RAYWHITE);
                tX += itemW;
            };
            drawTerrainItem("Dirt", (Color){105,60,35,255});
            drawTerrainItem("Wall", (Color){60,62,66,255});
            drawTerrainItem("Water", (Color){35,75,115,255});
            tX = colRightX; tY += 24;
            drawTerrainItem("Forest", (Color){34,110,48,255});
            drawTerrainItem("Ice", (Color){160,210,240,255});
            drawTerrainItem("Fire", (Color){220,100,20,255});
        }
        DrawText("Status", (int)colRightX, (int)(sectionY + 62), 16, (Color){230, 210, 100, 255});
        {
            float itemW = colRightW / 3.0f;
            DrawText("B=Burned", (int)colRightX, (int)(sectionY + 84), 14, (Color){255,60,60,255});
            DrawText("F=Frozen", (int)(colRightX + itemW), (int)(sectionY + 84), 14, (Color){160,210,255,255});
            DrawText("P=Paralyzed", (int)(colRightX + itemW * 2.0f), (int)(sectionY + 84), 14, (Color){240,220,60,255});
        }

        float belowColumnsY = sectionY + 105; // matches zlistBox bottom (95) plus margin
        DrawLine((int)panelX, (int)belowColumnsY, (int)(panelX + panelW), (int)belowColumnsY, (Color){70,70,70,255});

        // ── Combat log panel — fills remaining space exactly to window bottom ──
        {
            Rectangle logBox = { panelX, belowColumnsY + 10,
                                  panelW, (boardOffset + VIEW_CELLS * cellSize) - (belowColumnsY + 10) };
            DrawRectangleRec(logBox, (Color){15,15,15,230});

            static float logScroll = 0.0f;
            float lineH = 18.0f;
            float contentH = state.logs.size() * lineH;
            float maxScroll = contentH - logBox.height;
            if (maxScroll < 0) maxScroll = 0;

            if (CheckCollisionPointRec(mouse, logBox)) {
                logScroll -= GetMouseWheelMove() * 20.0f;
            }
            static float prevContentH = 0.0f;
            if (contentH > prevContentH && logScroll >= prevContentH - logBox.height - 5.0f) {
                logScroll = maxScroll;
            }
            prevContentH = contentH;

            if (logScroll < 0) logScroll = 0;
            if (logScroll > maxScroll) logScroll = maxScroll;

            BeginScissorMode((int)logBox.x, (int)logBox.y, (int)logBox.width, (int)logBox.height);
            float ly = logBox.y + 5 - logScroll;
            for (size_t i = 0; i < state.logs.size(); ++i) {
                Color c = (Color){
                    (unsigned char)(state.logs[i].color.x * 255),
                    (unsigned char)(state.logs[i].color.y * 255),
                    (unsigned char)(state.logs[i].color.z * 255),
                    255
                };
                DrawText(state.logs[i].text.c_str(), (int)logBox.x + 5, (int)ly, 13, c);
                ly += lineH;
            }
            EndScissorMode();

            if (maxScroll > 0) {
                float barH = logBox.height * (logBox.height / contentH);
                float barY = logBox.y + (logScroll / maxScroll) * (logBox.height - barH);
                DrawRectangle((int)(logBox.x + logBox.width - 6), (int)barY, 5, (int)barH, LIGHTGRAY);
            }
        }

        // ── End Game popup modal — drawn last so it overlays everything ──
        if (state.game_over || state.game_won) {
            Rectangle popup = { 1400/2.0f - 220, 654/2.0f - 100, 440, 200 };
            DrawRectangle(0, 0, 1400, 654, (Color){0, 0, 0, 150}); // dim background
            DrawRectangleRec(popup, (Color){35, 36, 40, 255});
            DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});

            const char* msg = state.game_over ? "OPERATION FAILED! YOU DIED."
                                               : "MISSION ACCOMPLISHED! SECTOR CLEAN.";
            DrawText(msg, (int)popup.x + 20, (int)popup.y + 30, 18, RAYWHITE);

            static bool endMusicPlayed = false;
            if (!endMusicPlayed) {
                AudioManager::getInstance().playMusic(state.game_won ? "victory" : "defeat");
                endMusicPlayed = true;
            }

            Rectangle returnBtn = { popup.x + 120, popup.y + 120, 200, 40 };
            DrawRectangleRec(returnBtn, (Color){120,20,20,255});
            drawCenteredText("Return to HUB", returnBtn, 16, WHITE);
            if (mouseClicked && CheckCollisionPointRec(mouse, returnBtn)) {
                state.current_scene = GameScene::MainMenu;
                AudioManager::getInstance().playMusic("menu");
                endMusicPlayed = false;
            }
        }

        // ── Game Guide popup (scrollable) ──
        if (showGuide) {
            Rectangle gpopup = { 1400/2.0f - 350, 654/2.0f - 280, 700, 560 };
            DrawRectangle(0, 0, 1400, 654, (Color){0, 0, 0, 160});
            DrawRectangleRec(gpopup, (Color){28, 29, 32, 255});
            DrawRectangleLinesEx(gpopup, 2, (Color){110, 110, 110, 255});
            DrawText("ZOMCHESS — GAME GUIDE", (int)gpopup.x + 20, (int)gpopup.y + 15, 24, (Color){240, 230, 90, 255});

            static const char* guideLines[] = {
                "TURN STRUCTURE",
                "- Human acts first, then Zombies, then Environment.",
                "- Win: eliminate all zombies. Lose: HP = 0 or turn limit exceeded.",
                "",
                "STAMINA",
                "- Rolled 1-6 each turn. Move = 1 (Water = 2). Each weapon use = 1.",
                "",
                "WEAPONS",
                "- Knife: melee, 1 dmg. Pistol: ranged, accuracy drops with distance.",
                "- Shotgun: 3-tile line, hits all. Grenade: 1-turn fuse, AoE blast.",
                "- Molotov: throws fire 1-6 tiles. Mine: placed trap, 1-tile blast.",
                "",
                "ZOMBIE TYPES",
                "- Clever: 1 move/turn, picks up loot & uses weapons.",
                "- Fast: 2 moves/turn. Exploding: detonates on death.",
                "- Vampire: heals on hit. Sick: infects, reduces stamina next turn.",
                "",
                "TERRAIN",
                "- Dirt: normal. Wall: blocks movement & shots.",
                "- Water: costs 2 stamina. Forest: blocks line-of-sight.",
                "- Fire: damages on entry. Ice: may cause sliding.",
                "",
                "ENVIRONMENT EVENTS",
                "- Wind, Rain, Lightning, Heatwave, Blizzard reshape the battlefield.",
                "",
                "TIP: Start with EASY to learn the mechanics!",
            };
            int lineCount = sizeof(guideLines) / sizeof(guideLines[0]);

            Rectangle guideBox = { gpopup.x + 15, gpopup.y + 50, gpopup.width - 30, gpopup.height - 110 };
            DrawRectangleRec(guideBox, (Color){15, 15, 18, 200});

            static float guideScroll = 0.0f;
            float lineH = 20.0f;
            float contentH = lineCount * lineH;
            float maxScroll = contentH - guideBox.height;
            if (maxScroll < 0) maxScroll = 0;
            if (CheckCollisionPointRec(mouse, guideBox)) {
                guideScroll -= GetMouseWheelMove() * 20.0f;
            }
            if (guideScroll < 0) guideScroll = 0;
            if (guideScroll > maxScroll) guideScroll = maxScroll;

            BeginScissorMode((int)guideBox.x, (int)guideBox.y, (int)guideBox.width, (int)guideBox.height);
            float gy = guideBox.y + 8 - guideScroll;
            for (int i = 0; i < lineCount; ++i) {
                Color lc = RAYWHITE;
                std::string s = guideLines[i];
                if (!s.empty() && s.find(':') == std::string::npos && s[0] != '-') lc = (Color){130, 220, 180, 255};
                DrawText(guideLines[i], (int)guideBox.x + 10, (int)gy, 15, lc);
                gy += lineH;
            }
            EndScissorMode();

            if (maxScroll > 0) {
                float barH = guideBox.height * (guideBox.height / contentH);
                float barY = guideBox.y + (guideScroll / maxScroll) * (guideBox.height - barH);
                DrawRectangle((int)(guideBox.x + guideBox.width - 6), (int)barY, 5, (int)barH, LIGHTGRAY);
            }

            Rectangle closeBtn = { gpopup.x + gpopup.width/2 - 80, gpopup.y + gpopup.height - 45, 160, 34 };
            DrawRectangleRec(closeBtn, (Color){140, 30, 30, 255});
            drawCenteredText("Close", closeBtn, 16, WHITE);
            if (mouseClicked && CheckCollisionPointRec(mouse, closeBtn)) {
                showGuide = false;
            }
        }

        // ── Confirm: Exit Game popup ──
        if (showConfirmExitGame) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
            Rectangle popup = { GetScreenWidth()/2.0f - 240, GetScreenHeight()/2.0f - 90, 480, 180 };
            DrawRectangleRec(popup, (Color){35, 36, 40, 255});
            DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
            DrawText("WARNING: All unsaved progress will be permanently lost!", (int)popup.x + 20, (int)popup.y + 20, 15, (Color){255,200,80,255});
            DrawText("Are you sure you want to quit the game?", (int)popup.x + 20, (int)popup.y + 45, 15, RAYWHITE);

            Rectangle yesBtn = { popup.x + 40,  popup.y + 110, 180, 40 };
            Rectangle noBtn  = { popup.x + 260, popup.y + 110, 180, 40 };
            DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
            drawCenteredText("Yes, Quit Game", yesBtn, 16, WHITE);
            DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
            drawCenteredText("Cancel", noBtn, 16, WHITE);

            if (mouseClicked && CheckCollisionPointRec(mouse, yesBtn)) {
                shouldQuit = true;
                showConfirmExitGame = false;
            } else if (mouseClicked && CheckCollisionPointRec(mouse, noBtn)) {
                showConfirmExitGame = false;
            }
        }

        // ── Confirm: Return to Hub popup ──
        if (showConfirmReturnHub) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
            Rectangle popup = { GetScreenWidth()/2.0f - 240, GetScreenHeight()/2.0f - 90, 480, 180 };
            DrawRectangleRec(popup, (Color){35, 36, 40, 255});
            DrawRectangleLinesEx(popup, 2, (Color){100, 100, 100, 255});
            DrawText("WARNING: All progress in the current match will be lost!", (int)popup.x + 20, (int)popup.y + 20, 15, (Color){255,200,80,255});
            DrawText("Are you sure you want to return to the Main Menu?", (int)popup.x + 20, (int)popup.y + 45, 15, RAYWHITE);

            Rectangle yesBtn = { popup.x + 40,  popup.y + 110, 180, 40 };
            Rectangle noBtn  = { popup.x + 260, popup.y + 110, 180, 40 };
            DrawRectangleRec(yesBtn, (Color){160, 20, 20, 255});
            drawCenteredText("Yes, Exit Match", yesBtn, 16, WHITE);
            DrawRectangleRec(noBtn, (Color){60, 60, 60, 255});
            drawCenteredText("Cancel", noBtn, 16, WHITE);

            if (mouseClicked && CheckCollisionPointRec(mouse, yesBtn)) {
                AudioManager::getInstance().stopMusic();
                state.current_scene = GameScene::MainMenu;
                AudioManager::getInstance().playMusic("menu");
                showConfirmReturnHub = false;
            } else if (mouseClicked && CheckCollisionPointRec(mouse, noBtn)) {
                showConfirmReturnHub = false;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
