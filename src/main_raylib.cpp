#include "raylib.h"
#include "GameState.h"
#include "RaylibAudioManagerReal.h"
#include "embedded/menu_theme.h"
#include "embedded/battle_theme.h"
#include "embedded/victory_theme.h"
#include "embedded/defeat_theme.h"
#include <cmath>
#include <string>

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

int main() {
    InitWindow(1400, 665, "ZomChess (Raylib)");
    SetTargetFPS(60);

    Font gameFont = LoadFontEx("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32, nullptr, 0);
    if (gameFont.texture.id == 0) {
        gameFont = LoadFontEx("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", 32, nullptr, 0);
    }
    if (gameFont.texture.id == 0) {
        gameFont = GetFontDefault();
    }
    SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);

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
    std::string ioMessage;
    float ioMessageTimer = 0.0f;
    bool hasImportedConfig = false;

    // ── Custom difficulty sliders (right column) ──
    float sliderX = 400.0f;
    float sliderW = 560.0f;
    Vector2 mouse = {0, 0};
    auto drawSlider = [&](const char* label, int* val, int minV, int maxV, float y, Color barColor) -> void {
        DrawText(TextFormat("%s: %d", label, *val), (int)sliderX, (int)y, 16, RAYWHITE);
        Rectangle track = { sliderX, y + 22, sliderW, 14 };
        DrawRectangleRounded(track, 0.5f, 4, (Color){40,40,40,255});
        float pct = (float)(*val - minV) / (float)(maxV - minV);
        Rectangle fill = { sliderX, y + 22, sliderW * pct, 14 };
        DrawRectangleRounded(fill, 0.5f, 4, barColor);
        float knobX = sliderX + sliderW * pct;
        DrawCircle((int)knobX, (int)(y + 29), 9.0f, RAYWHITE);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, (Rectangle){sliderX - 10, y, sliderW + 20, 36})) {
            float p = (mouse.x - sliderX) / sliderW;
            p = std::max(0.0f, std::min(1.0f, p));
            *val = minV + (int)std::round(p * (maxV - minV));
        }
    };

    Terrain editorSelectedTerrain = Terrain::Wall;

    while (!WindowShouldClose()) {
        float dtSeconds = GetFrameTime();
        AudioManager::getInstance().updateMusic();
        mouse = GetMousePosition();
        bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

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
            if (mouseClicked) {
                int chosen = -1;
                if (CheckCollisionPointRec(mouse, easyBtn))   chosen = 0;
                if (CheckCollisionPointRec(mouse, mediumBtn)) chosen = 1;
                if (CheckCollisionPointRec(mouse, hardBtn))   chosen = 2;
                if (CheckCollisionPointRec(mouse, unfairBtn)) chosen = 3;
                if (chosen >= 0) {
                    state.apply_quick_difficulty(chosen);
                    state.init_game();
                    state.current_scene = GameScene::Playing;
                    AudioManager::getInstance().playMusic("battle");
                }
                if (CheckCollisionPointRec(mouse, exportBtn)) {
                    bool ok = state.export_challenge_file("my_custom_challenge.zom");
                    ioMessage = ok ? "Exported to my_custom_challenge.zom" : "Export failed!";
                    ioMessageTimer = 3.0f;
                }
                if (CheckCollisionPointRec(mouse, importBtn)) {
                    bool ok = state.import_challenge_file("my_custom_challenge.zom");
                    ioMessage = ok ? "Imported my_custom_challenge.zom" : "Import failed! (file not found?)";
                    ioMessageTimer = 3.0f;
                    hasImportedConfig = ok;
                }
                if (hasImportedConfig && CheckCollisionPointRec(mouse, startCustomBtn)) {
                    state.init_game();
                    state.current_scene = GameScene::Playing;
                    AudioManager::getInstance().playMusic("battle");
                }
            }

            BeginDrawing();
            ClearBackground((Color){22, 23, 25, 255});
            DrawText("ZomChess", 60, 40, 48, RAYWHITE);

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

            if (!ioMessage.empty()) {
                DrawText(ioMessage.c_str(), 60, 540, 15, (Color){255, 220, 100, 255});
            }

            // ── Right column: Custom Difficulty Sliders ──
            DrawText("Custom Difficulty", (int)sliderX, 40, 26, (Color){255, 140, 220, 255});
            DrawLine((int)sliderX - 30, 30, (int)sliderX - 30, 620, (Color){70,70,70,255});

            drawSlider("Map Width", &state.active_config.map_width,
                       GameConstants::Difficulty::SliderBounds::MAP_WIDTH_MIN,
                       GameConstants::Difficulty::SliderBounds::MAP_WIDTH_MAX, 90, (Color){80,160,220,255});
            drawSlider("Map Height", &state.active_config.map_height,
                       GameConstants::Difficulty::SliderBounds::MAP_HEIGHT_MIN,
                       GameConstants::Difficulty::SliderBounds::MAP_HEIGHT_MAX, 140, (Color){80,160,220,255});

            drawSlider("Human HP", &state.active_config.human_hp,
                       GameConstants::Difficulty::SliderBounds::HUMAN_HP_MIN,
                       GameConstants::Difficulty::SliderBounds::HUMAN_HP_MAX, 200, (Color){220,180,60,255});
            drawSlider("Initial Stamina", &state.active_config.initial_stamina,
                       GameConstants::Difficulty::SliderBounds::INITIAL_STAMINA_MIN,
                       GameConstants::Difficulty::SliderBounds::INITIAL_STAMINA_MAX, 250, (Color){220,180,60,255});
            drawSlider("Turn Limit", &state.active_config.turn_limit,
                       GameConstants::Difficulty::SliderBounds::TURN_LIMIT_MIN,
                       GameConstants::Difficulty::SliderBounds::TURN_LIMIT_MAX, 300, (Color){220,180,60,255});

            drawSlider("Pistol Ammo", &state.active_config.pistol_ammo,
                       GameConstants::Difficulty::SliderBounds::PISTOL_AMMO_MIN,
                       GameConstants::Difficulty::SliderBounds::PISTOL_AMMO_MAX, 350, (Color){200,120,60,255});
            drawSlider("Shotgun Ammo", &state.active_config.shotgun_ammo,
                       GameConstants::Difficulty::SliderBounds::SHOTGUN_AMMO_MIN,
                       GameConstants::Difficulty::SliderBounds::SHOTGUN_AMMO_MAX, 400, (Color){200,120,60,255});

            DrawText("Zombie Counts", (int)sliderX, 460, 20, (Color){255, 100, 100, 255});
            drawSlider("Clever Zombies", &state.active_config.count_normal, 0,
                       GameConstants::Difficulty::SliderBounds::COUNT_CLEVER_MAX, 490, (Color){45,175,90,255});
            drawSlider("Fast Sprinters", &state.active_config.count_fast, 0,
                       GameConstants::Difficulty::SliderBounds::COUNT_FAST_MAX, 540, (Color){55,168,255,255});

            int available = state.calculate_available_spawn_cells();
            int totalZoms = state.active_config.count_normal + state.active_config.count_fast +
                            state.active_config.count_exploding + state.active_config.count_vampire +
                            state.active_config.count_sick;
            bool overflow = !state.is_zombie_count_valid();
            DrawText(TextFormat("Spawn tiles: %d | Zombies: %d", available, totalZoms),
                     (int)sliderX, 585, 14, overflow ? (Color){255,80,80,255} : (Color){160,160,160,255});

            // ── Custom Map checkbox + Open Editor button ──
            Rectangle customMapCheck = { sliderX, 607, 18, 18 };
            DrawRectangleRec(customMapCheck, (Color){40,40,40,255});
            if (state.active_config.custom_map_mode) {
                DrawRectangle((int)customMapCheck.x + 3, (int)customMapCheck.y + 3, 12, 12, (Color){0,200,100,255});
            }
            DrawText("Use Custom Map", (int)sliderX + 26, 606, 15, RAYWHITE);
            if (mouseClicked && CheckCollisionPointRec(mouse, customMapCheck)) {
                state.active_config.custom_map_mode = !state.active_config.custom_map_mode;
                if (state.active_config.custom_map_mode) {
                    state.active_config.custom_grid.assign(state.active_config.map_width,
                        std::vector<Terrain>(state.active_config.map_height, Terrain::Dirt));
                    state.active_config.custom_human_pos = {1, 1};
                }
            }
            if (state.active_config.custom_map_mode) {
                Rectangle editorBtn = { sliderX + 220, 602, 200, 28 };
                DrawRectangleRec(editorBtn, (Color){140,90,20,255});
                drawCenteredText("Open Map Editor", editorBtn, 14, WHITE);
                if (mouseClicked && CheckCollisionPointRec(mouse, editorBtn)) {
                    state.current_scene = GameScene::MapEditor;
                }
            }

            Rectangle launchBtn = { sliderX, 635, sliderW, 26 };
            DrawRectangleRec(launchBtn, overflow ? (Color){80,80,80,255} : (Color){15,110,15,255});
            drawCenteredText(overflow ? "TOO MANY ZOMBIES" : "LAUNCH CUSTOM GAME", launchBtn, 16, WHITE);
            if (!overflow && mouseClicked && CheckCollisionPointRec(mouse, launchBtn)) {
                state.init_game();
                state.current_scene = GameScene::Playing;
                AudioManager::getInstance().playMusic("battle");
            }

            EndDrawing();
            continue;
        }

        // ══════════════════════════════════════════════════════════════
        // MAP EDITOR — paint terrain tile-by-tile, set Human spawn
        // ══════════════════════════════════════════════════════════════
        if (state.current_scene == GameScene::MapEditor) {
            int mw = state.active_config.map_width;
            int mh = state.active_config.map_height;

            Rectangle brushDirt   = { boardOffset + mw * cellSize + 30, 40,  200, 36 };
            Rectangle brushWall   = { boardOffset + mw * cellSize + 30, 80,  200, 36 };
            Rectangle brushWater  = { boardOffset + mw * cellSize + 30, 120, 200, 36 };
            Rectangle brushForest = { boardOffset + mw * cellSize + 30, 160, 200, 36 };
            Rectangle brushIce    = { boardOffset + mw * cellSize + 30, 200, 200, 36 };
            Rectangle saveReturnBtn = { boardOffset + mw * cellSize + 30, 260, 200, 44 };

            if (mouseClicked) {
                if (CheckCollisionPointRec(mouse, brushDirt))   editorSelectedTerrain = Terrain::Dirt;
                if (CheckCollisionPointRec(mouse, brushWall))   editorSelectedTerrain = Terrain::Wall;
                if (CheckCollisionPointRec(mouse, brushWater))  editorSelectedTerrain = Terrain::Water;
                if (CheckCollisionPointRec(mouse, brushForest)) editorSelectedTerrain = Terrain::Forest;
                if (CheckCollisionPointRec(mouse, brushIce))    editorSelectedTerrain = Terrain::Ice;
                if (CheckCollisionPointRec(mouse, saveReturnBtn)) {
                    state.current_scene = GameScene::MainMenu;
                }
            }
            // Paint while holding left mouse button, like the original (Selectable-style painting)
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int tx = (int)((mouse.x - boardOffset) / cellSize);
                int ty = (int)((mouse.y - boardOffset) / cellSize);
                if (tx >= 0 && tx < mw && ty >= 0 && ty < mh) {
                    state.active_config.custom_grid[tx][ty] = editorSelectedTerrain;
                    if (editorSelectedTerrain == Terrain::Dirt && IsKeyDown(KEY_LEFT_SHIFT)) {
                        state.active_config.custom_human_pos = {tx, ty};
                    }
                }
            }

            BeginDrawing();
            ClearBackground((Color){22, 23, 25, 255});

            for (int x = 0; x < mw; ++x) {
                for (int y = 0; y < mh; ++y) {
                    DrawRectangle((int)(x * cellSize + boardOffset), (int)(y * cellSize + boardOffset),
                                  (int)(cellSize - 2), (int)(cellSize - 2), terrainColor(state.active_config.custom_grid[x][y]));
                    if (x == state.active_config.custom_human_pos.x && y == state.active_config.custom_human_pos.y) {
                        DrawCircle((int)(x * cellSize + boardOffset + cellSize/2), (int)(y * cellSize + boardOffset + cellSize/2), 8.0f, WHITE);
                    }
                }
            }

            DrawText("Brush:", (int)brushDirt.x, 15, 18, (Color){255, 140, 220, 255});
            DrawRectangleRec(brushDirt, editorSelectedTerrain == Terrain::Dirt ? (Color){160,90,40,255} : DARKGRAY);
            drawCenteredText("Dirt", brushDirt, 16, WHITE);
            DrawRectangleRec(brushWall, editorSelectedTerrain == Terrain::Wall ? (Color){110,110,120,255} : DARKGRAY);
            drawCenteredText("Wall", brushWall, 16, WHITE);
            DrawRectangleRec(brushWater, editorSelectedTerrain == Terrain::Water ? (Color){60,120,180,255} : DARKGRAY);
            drawCenteredText("Water", brushWater, 16, WHITE);
            DrawRectangleRec(brushForest, editorSelectedTerrain == Terrain::Forest ? (Color){50,150,60,255} : DARKGRAY);
            drawCenteredText("Forest", brushForest, 16, WHITE);
            DrawRectangleRec(brushIce, editorSelectedTerrain == Terrain::Ice ? (Color){140,200,240,255} : DARKGRAY);
            drawCenteredText("Ice", brushIce, 16, WHITE);

            DrawText("Shift+Click Dirt = set Human spawn", (int)brushDirt.x, 240, 13, LIGHTGRAY);

            DrawRectangleRec(saveReturnBtn, (Color){15,110,15,255});
            drawCenteredText("Save & Return", saveReturnBtn, 16, WHITE);

            EndDrawing();
            continue;
        }

        bool onPanel = CheckCollisionPointRec(mouse, endTurnBtn) || CheckCollisionPointRec(mouse, moveBtn) ||
                       CheckCollisionPointRec(mouse, knifeBtn)   || CheckCollisionPointRec(mouse, pistolBtn) ||
                       CheckCollisionPointRec(mouse, shotgunBtn) || CheckCollisionPointRec(mouse, grenadeBtn) ||
                       CheckCollisionPointRec(mouse, molotovBtn) || CheckCollisionPointRec(mouse, mineBtn) ||
                       CheckCollisionPointRec(mouse, icePickBtn) || CheckCollisionPointRec(mouse, guideBtn) ||
                       CheckCollisionPointRec(mouse, returnHubTopBtn);

        // ══════════════════════════════════════════════════════════════
        // INPUT HANDLING — mirrors main.cpp's event loop for Playing scene
        // ══════════════════════════════════════════════════════════════

        if (state.current_scene == GameScene::Playing &&
            !state.game_over && !state.game_won &&
            state.phase == TurnPhase::HumanTurn &&
            !state.human.is_paralyzed &&
            !showGuide &&
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
            } else if (CheckCollisionPointRec(mouse, guideBtn)) {
                showGuide = true;
            } else if (CheckCollisionPointRec(mouse, endTurnBtn)) {
                endTurnWithBanner();
            } else if (CheckCollisionPointRec(mouse, returnHubTopBtn)) {
                state.current_scene = GameScene::MainMenu;
                AudioManager::getInstance().playMusic("menu");
            }
        }

        // Board click — mirrors main.cpp's MouseButtonPressed handling for the board
        if (state.current_scene == GameScene::Playing &&
            !state.game_over && !state.game_won &&
            state.phase == TurnPhase::HumanTurn &&
            !state.human.is_paralyzed &&
            !showGuide &&
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
                } else {
                    // Pistol, Shotgun, Grenade, Molotov all delegate to handle_weapon_click
                    state.handle_weapon_click(tx, ty, cellSize, boardOffset);
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

            state.update_zombie_logic(dtSeconds);
            state.update_environment_logic(dtSeconds);
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
                              (int)(cellSize - 2.0f), (int)(cellSize - 2.0f), terrainColor(state.grid[x][y]));
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

        for (const auto& ld : state.loot_drops) {
            int llx = ld.pos.x - viewX, lly = ld.pos.y - viewY;
            if (llx < 0 || llx >= VIEW_CELLS || lly < 0 || lly >= VIEW_CELLS) continue;
            int lx = (int)(llx * cellSize + boardOffset);
            int ly = (int)(lly * cellSize + boardOffset);
            DrawRectangle(lx + 4, ly + 4, (int)(cellSize - 8), (int)(cellSize - 8), (Color){80, 60, 20, 200});
            DrawText("?", lx + (int)(cellSize / 2) - 5, ly + (int)(cellSize / 2) - 10, 20, (Color){255, 220, 60, 255});
        }

        for (const auto& g : state.active_grenades) {
            if (!g.active) continue;
            int glx = g.pos.x - viewX, gly = g.pos.y - viewY;
            if (glx < 0 || glx >= VIEW_CELLS || gly < 0 || gly >= VIEW_CELLS) continue;
            DrawCircle((int)(glx * cellSize + boardOffset + cellSize / 2), (int)(gly * cellSize + boardOffset + cellSize / 2),
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
            int zlx = drawPos.x - viewX, zly = drawPos.y - viewY;
            if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
            float zx = zlx * cellSize + boardOffset + 3.0f;
            float zy = zly * cellSize + boardOffset + 3.0f;
            DrawRectangle((int)zx, (int)zy, (int)(cellSize - 6.0f), (int)(cellSize - 6.0f), zombieColor(z->type));
            DrawText(TextFormat("%d", z->hp), (int)zx + 12, (int)zy + 10, 16, WHITE);

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
            int hlx = drawPos.x - viewX, hly = drawPos.y - viewY;
            if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) {
                float hx = hlx * cellSize + boardOffset + 3.0f;
                float hy = hly * cellSize + boardOffset + 3.0f;
                DrawRectangle((int)hx, (int)hy, (int)(cellSize - 6.0f), (int)(cellSize - 6.0f), WHITE);

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

        // ── Active FX animations (mirrors main.cpp's FX drawing block) ──
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
            } else if (state.active_fx.type == FXType::Shotgun || state.active_fx.type == FXType::Explosion) {
                float intensity = sqrtf(progress);
                Color blastColor = (state.active_fx.type == FXType::Shotgun)
                    ? (Color){255, 130, 30, (unsigned char)(intensity * 180)}
                    : (Color){255, 50, 10, (unsigned char)(intensity * 240)};
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
                    float tx = clx * cellSize + boardOffset + cellSize / 2.0f;
                    float ty = cly * cellSize + boardOffset + cellSize / 2.0f;
                    DrawLineEx((Vector2){tx, boardOffset}, (Vector2){tx, ty + cellSize / 2.0f}, 3.0f, (Color){255, 255, 255, alpha});
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
                float intensity = 0.5f + 0.3f * sinf(GetTime() * 4.0f);
                DrawRectangle((int)boardOffset, (int)boardOffset, (int)(VIEW_CELLS * cellSize), (int)(VIEW_CELLS * cellSize),
                              (Color){220, 240, 255, (unsigned char)(intensity * 100)});
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
            int maxViewX = std::max(0, state.width - VIEW_CELLS);
            float trackW = VIEW_CELLS * cellSize;
            Rectangle hTrack = { boardOffset, boardOffset + VIEW_CELLS * cellSize + 6, trackW, scrollThickness };
            DrawRectangleRounded(hTrack, 0.5f, 4, (Color){40,40,40,255});
            if (maxViewX > 0) {
                float thumbW = std::max(30.0f, (float)VIEW_CELLS / state.width * trackW);
                float thumbX = hTrack.x + ((float)viewX / maxViewX) * (trackW - thumbW);
                Rectangle hThumb = { thumbX, hTrack.y, thumbW, scrollThickness };
                bool hHover = CheckCollisionPointRec(mouse, hThumb);
                DrawRectangleRounded(hThumb, 0.5f, 4, hHover ? (Color){170,170,170,255} : (Color){110,110,110,255});
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, hTrack)) {
                    float pct = (mouse.x - hTrack.x - thumbW * 0.5f) / (trackW - thumbW);
                    pct = std::max(0.0f, std::min(1.0f, pct));
                    viewX = (int)std::round(pct * maxViewX);
                }
            }
        }

        // ── Vertical scrollbar (right of board) ──
        {
            int maxViewY = std::max(0, state.height - VIEW_CELLS);
            float trackH = VIEW_CELLS * cellSize;
            Rectangle vTrack = { boardOffset + VIEW_CELLS * cellSize + 6, boardOffset, scrollThickness, trackH };
            DrawRectangleRounded(vTrack, 0.5f, 4, (Color){40,40,40,255});
            if (maxViewY > 0) {
                float thumbH = std::max(30.0f, (float)VIEW_CELLS / state.height * trackH);
                float thumbY = vTrack.y + ((float)viewY / maxViewY) * (trackH - thumbH);
                Rectangle vThumb = { vTrack.x, thumbY, scrollThickness, thumbH };
                bool vHover = CheckCollisionPointRec(mouse, vThumb);
                DrawRectangleRounded(vThumb, 0.5f, 4, vHover ? (Color){170,170,170,255} : (Color){110,110,110,255});
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, vTrack)) {
                    float pct = (mouse.y - vTrack.y - thumbH * 0.5f) / (trackH - thumbH);
                    pct = std::max(0.0f, std::min(1.0f, pct));
                    viewY = (int)std::round(pct * maxViewY);
                }
            }
        }

        DrawText(TextFormat("TURN %d/%d | ST %d | HP %d | Pos [%d,%d] | Env: %s",
                             state.current_turn, state.turn_limit, state.human.stamina, state.human.hp,
                             state.human.pos.x + 1, state.human.pos.y + 1, state.last_environment_event.c_str()),
                 (int)panelX, (int)boardOffset, 18, (Color){130, 220, 255, 255});


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

        DrawRectangleRec(moveBtn, (state.input_mode == InputMode::MoveMode) ? (Color){220,140,20,255} : DARKGRAY);
        drawCenteredText("Move", moveBtn, 17, WHITE);

        DrawRectangleRec(knifeBtn, (state.input_mode == InputMode::TargetKnife) ? (Color){150,0,150,255} : DARKGRAY);
        drawCenteredText("Knife", knifeBtn, 17, WHITE);

        int icePickCost = state.human.is_frozen
            ? GameConstants::Weapons::ICE_PICK_STAMINA_COST_FROZEN
            : GameConstants::Weapons::ICE_PICK_STAMINA_COST_NORMAL;
        bool onIceUI = state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice;
        DrawRectangleRec(icePickBtn, onIceUI ? (Color){0,150,200,255} : DARKGRAY);
        drawCenteredText(TextFormat("Ice Pick [-%d ST]", icePickCost), icePickBtn, 16, WHITE);

        DrawRectangleRec(pistolBtn, (state.input_mode == InputMode::TargetPistol) ? (Color){200,0,0,255} : DARKGRAY);
        drawCenteredText(TextFormat("Pistol (%d)", state.human.pistol_ammo), pistolBtn, 17, WHITE);

        DrawRectangleRec(shotgunBtn, (state.input_mode == InputMode::TargetShotgun) ? (Color){200,80,0,255} : DARKGRAY);
        drawCenteredText(TextFormat("Shotgun (%d)", state.human.shotgun_ammo), shotgunBtn, 17, WHITE);

        DrawRectangleRec(grenadeBtn, (state.input_mode == InputMode::TargetGrenade) ? (Color){0,200,0,255} : DARKGRAY);
        drawCenteredText(TextFormat("Grenade (%d)", state.human.grenades), grenadeBtn, 16, WHITE);

        DrawRectangleRec(molotovBtn, (state.input_mode == InputMode::TargetMolotov) ? (Color){220,90,10,255} : DARKGRAY);
        drawCenteredText(TextFormat("Molotov (%d)", state.human.molotovs), molotovBtn, 16, WHITE);

        DrawRectangleRec(mineBtn, DARKGRAY);
        drawCenteredText(TextFormat("Mine (%d)", state.human.mines), mineBtn, 17, WHITE);

        DrawLine((int)panelX, (int)(boardOffset + 255), (int)(panelX + panelW), (int)(boardOffset + 255), (Color){70,70,70,255});

        // ── Two-column section: Zombies (left) | Terrain + Status (right) ──
        float sectionY = boardOffset + 265;
        float colLeftW = panelW * 0.55f; // wide enough for 10 zombies per row
        float colRightX = panelX + colLeftW + 20.0f;
        float colRightW = panelW - colLeftW - 20.0f;

        // Left column: zombie list
        int aliveCount = 0;
        for (const auto& z : state.zombies) if (z->hp > 0) aliveCount++;
        DrawText(TextFormat("Zombies: %d", aliveCount), (int)panelX, (int)sectionY, 16, (Color){255,100,100,255});

        Rectangle zlistBox = { panelX, sectionY + 25, colLeftW, 70 };
        DrawRectangleRec(zlistBox, (Color){15,15,15,180});
        {
            static float zlistScroll = 0.0f;
            if (CheckCollisionPointRec(mouse, zlistBox)) {
                zlistScroll -= GetMouseWheelMove() * 20.0f;
            }
            int perRow = (int)(zlistBox.width / 40.0f);
            if (perRow < 1) perRow = 1;
            int rows = ((int)state.zombies.size() + perRow - 1) / perRow;
            float contentH = rows * 20.0f;
            float maxScroll = contentH - zlistBox.height;
            if (maxScroll < 0) maxScroll = 0;
            if (zlistScroll < 0) zlistScroll = 0;
            if (zlistScroll > maxScroll) zlistScroll = maxScroll;

            BeginScissorMode((int)zlistBox.x, (int)zlistBox.y, (int)zlistBox.width, (int)zlistBox.height);
            for (size_t i = 0; i < state.zombies.size(); ++i) {
                Color idc = state.zombies[i]->hp > 0 ? zombieColor(state.zombies[i]->type) : GRAY;
                float ix = zlistBox.x + 5 + (i % perRow) * 40.0f;
                float iy = zlistBox.y + 3 + (i / perRow) * 20.0f - zlistScroll;
                DrawText(TextFormat("#%zu", i + 1), (int)ix, (int)iy, 14, idc);
            }
            EndScissorMode();

            if (maxScroll > 0) {
                float barH = zlistBox.height * (zlistBox.height / contentH);
                float barY = zlistBox.y + (zlistScroll / maxScroll) * (zlistBox.height - barH);
                DrawRectangle((int)(zlistBox.x + zlistBox.width - 6), (int)barY, 5, (int)barH, LIGHTGRAY);
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
                                  panelW, (boardOffset + state.height * cellSize) - (belowColumnsY + 10) };
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
            Rectangle popup = { 1400/2.0f - 220, 665/2.0f - 100, 440, 200 };
            DrawRectangle(0, 0, 1400, 665, (Color){0, 0, 0, 150}); // dim background
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
            Rectangle gpopup = { 1400/2.0f - 350, 665/2.0f - 280, 700, 560 };
            DrawRectangle(0, 0, 1400, 665, (Color){0, 0, 0, 160});
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

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
