#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "GameState.h"
#include "UI.h"
#include "AudioManager.h"
#include "SplashScreen.h"
#include <cmath>

// Shorthand helper for sound effects
static inline void sfx(const std::string& name) {
    AudioManager::getInstance().playSound(name);
}

int main() {
    enum class Lang { EN, VI };
    static Lang ui_lang = Lang::EN;
    auto tr = [&](const char* en, const char* vi) { return (ui_lang == Lang::VI) ? vi : en; };
    sf::RenderWindow window(sf::VideoMode(1400, 658), "ZomChess Game", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    SetupTacticalImGuiTheme();

    sf::Font boardFont;
    bool hasFont = false;
    const char* fontCandidates[] = {
        "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    };
    for (const char* fp : fontCandidates) {
        if (boardFont.loadFromFile(fp)) { hasFont = true; break; }
    }

    // ── Splash Screen ──────────────────────────────────────────────────
    {
        SplashScreen splash;
        splash.init(window.getSize().x, window.getSize().y);
        sf::Clock splashClock;

        while (window.isOpen()) {
            sf::Event ev;
            while (window.pollEvent(ev)) {
                if (ev.type == sf::Event::Closed) { window.close(); return 0; }
                splash.handle_event(ev);
            }

            float dt = splashClock.restart().asSeconds();
            if (!splash.update(dt)) break; // splash finished

            window.clear(sf::Color(10, 11, 14));
            if (hasFont) splash.draw(window, boardFont);
            window.display();
        }
    }
    if (!window.isOpen()) return 0;
    // ──────────────────────────────────────────────────────────────────

    GameState state;
    state.initAudio();
    state.playBackgroundMusic("menu");
    
    sf::Clock deltaClock;
    sf::Clock animationClock; 

    float cellSize = 40.0f;
    float boardOffset = 20.0f;
    const int VIEW_CELLS = 15;
    int viewX = 0;
    int viewY = 0;
    bool view_initialized = false;

    while (window.isOpen()) {
        static bool show_guide_popup = false;
        static bool show_confirm_return_hub = false;
        static bool show_confirm_exit_game = false;
        static float panelWidthCache = 380.0f;
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed) {
                if (state.current_scene == GameScene::Playing || state.current_scene == GameScene::MapEditor) {
                    // Close any open dialog first to avoid ImGui popup stack corruption
                    show_confirm_return_hub = false;
                    show_confirm_exit_game = true;
                } else {
                    window.close();
                }
            }

            if (state.current_scene == GameScene::Playing && !ImGui::GetIO().WantCaptureMouse && event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !state.game_over && !state.game_won && state.phase == TurnPhase::HumanTurn && !state.human.is_paralyzed) {
                    int lx = (event.mouseButton.x - boardOffset) / cellSize;
                    int ly = (event.mouseButton.y - boardOffset) / cellSize;
                    int padX = (state.width < VIEW_CELLS) ? (VIEW_CELLS - state.width) / 2 : 0;
                    int padY = (state.height < VIEW_CELLS) ? (VIEW_CELLS - state.height) / 2 : 0;
                    int tx = viewX + (lx - padX);
                    int ty = viewY + (ly - padY);

                    if (lx >= 0 && lx < VIEW_CELLS && ly >= 0 && ly < VIEW_CELLS &&
                        tx >= 0 && tx < state.width && ty >= 0 && ty < state.height) {
                        if (state.input_mode == InputMode::MoveMode) {
                            // Check if human is frozen
                            if (!state.human.is_frozen) {
                                int dx = std::abs(tx - state.human.pos.x);
                                int dy = std::abs(ty - state.human.pos.y);
                                if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                                    if (state.grid[tx][ty] != Terrain::Wall && state.grid[tx][ty] != Terrain::Wall) {
                                        bool blocked = false;
                                        for (const auto& z : state.zombies) {
                                            if (z->hp > 0 && z->pos == Position{tx, ty}) { blocked = true; break; }
                                        }
                                        if (!blocked) {
                                            int cost = (state.grid[tx][ty] == Terrain::Water) ? 2 : 1;
                                            if (state.human.stamina >= cost) {
                                                int move_dx = tx - state.human.pos.x;
                                                int move_dy = ty - state.human.pos.y;
                                                state.human.pos = {tx, ty};
                                                state.human.stamina -= cost;
                                                sfx("footstep");
                                                state.check_fire_interactions();
                                                state.check_mine_interactions();
                                                state.check_loot_pickup();
                                                // Ice slide check — try_ice_slide also calls check_fire/mine internally
                                                if (state.human.hp > 0 && state.grid[state.human.pos.x][state.human.pos.y] == Terrain::Ice) {
                                                    state.try_ice_slide(true, 0, move_dx, move_dy);
                                                }
                                                // Auto-end turn if stunned (set by try_ice_slide)
                                                if (state.human.is_stunned) {
                                                    state.human.is_stunned = false;
                                                    state.start_zombie_phase();
                                                }
                                            } else {
                                                if (state.human.stamina == 0) {
                                                    state.add_log(tr("[SYSTEM] Zero stamina! End turn!", "[HE THONG] Het stamina! Ket thuc luot!"), ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                                                    state.turn_banner_fx.type = FXType::Electricity;
                                                    state.turn_banner_fx.timer = 1.0f;
                                                    state.turn_banner_fx.max_duration = 1.0f;
                                                    state.turn_banner_fx.banner_text = (ui_lang == Lang::VI) ? "HET STAMINA! KET THUC LUOT!" : "ZERO STAMINA! END TURN!";
                                                    state.start_zombie_phase();
                                                } else {
                                                    state.add_log(tr("[SYSTEM] Not enough stamina!", "[HE THONG] Khong du stamina!"), ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                                                    state.turn_banner_fx.type = FXType::Electricity;
                                                    state.turn_banner_fx.timer = 1.0f;
                                                    state.turn_banner_fx.max_duration = 1.0f;
                                                    state.turn_banner_fx.banner_text = (ui_lang == Lang::VI) ? "KHONG DU STAMINA!" : "NOT ENOUGH STAMINA!";
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                // Human is frozen — costs 2 stamina to break free
                                if (state.human.stamina >= 2) {
                                    state.human.stamina -= 2;
                                    state.human.is_frozen = false;
                                    state.add_log(tr("[SYSTEM] You break free from the ice! (-2 stamina)",
                                                   "[HE THONG] Ban tu giai bang! (-2 stamina)"),
                                                ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                                    // Don't end turn — player can now act with remaining stamina
                                } else {
                                    state.add_log(tr("[SYSTEM] Frozen solid! Need 2 stamina to break free. Turn skipped.",
                                                   "[HE THONG] Bi dong cung! Can 2 stamina de giai bang. Bo qua luot."),
                                                ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                                    state.start_zombie_phase();
                                }
                            }
                        } else {
                            state.handle_weapon_click(tx, ty, cellSize, boardOffset);
                        }
                    }
                }
            }

            if (state.current_scene == GameScene::MapEditor && !ImGui::GetIO().WantCaptureMouse) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || event.type == sf::Event::MouseButtonPressed) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    int tx = (mousePos.x - boardOffset) / cellSize;
                    int ty = (mousePos.y - boardOffset) / cellSize;

                    if (tx >= 0 && tx < state.active_config.map_width && ty >= 0 && ty < state.active_config.map_height) {
                        state.active_config.custom_grid[tx][ty] = state.editor_selected_terrain;
                        if (state.editor_selected_terrain == Terrain::Dirt && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                            state.active_config.custom_human_pos = {tx, ty};
                        }
                    }
                }
            }
        }

        sf::Time dtTime = deltaClock.restart();
        float dtSeconds = dtTime.asSeconds();
        ImGui::SFML::Update(window, dtTime);

        if (state.current_scene == GameScene::Playing) {
            state.use_vietnamese = (ui_lang == Lang::VI);
            if (state.human.stamina == 0 && state.input_mode != InputMode::MoveMode) {
                state.input_mode = InputMode::MoveMode;
            }
            cellSize = 40.0f;
            boardOffset = 20.0f;
            int maxViewX = std::max(0, state.width - VIEW_CELLS);
            int maxViewY = std::max(0, state.height - VIEW_CELLS);
            if (!view_initialized) {
                viewX = std::max(0, std::min(state.human.pos.x - VIEW_CELLS / 2, maxViewX));
                viewY = std::max(0, std::min(state.human.pos.y - VIEW_CELLS / 2, maxViewY));
                view_initialized = true;
            } else {
                viewX = std::max(0, std::min(viewX, maxViewX));
                viewY = std::max(0, std::min(viewY, maxViewY));
            }
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
            // Update loot blink timers
            for (auto& ld : state.loot_drops) {
                ld.blink_timer += dtSeconds;
            }
            state.update_zombie_logic(dtSeconds);
            state.update_environment_logic(dtSeconds);
        }

        window.clear(sf::Color(22, 23, 25));
        state.use_vietnamese = (ui_lang == Lang::VI);

        if (state.current_scene == GameScene::MainMenu) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(1400, 658));
            ImGui::Begin("ZomChess Tactical System Hub", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.9f, 1), "%s", tr("QUICK PLAY", "CHOI NHANH"));
            ImGui::SameLine();
            ImGui::Text("%s", tr("Language:", "Ngon ngu:"));
            ImGui::SameLine();
            if (ImGui::RadioButton("EN", ui_lang == Lang::EN)) ui_lang = Lang::EN;
            ImGui::SameLine();
            if (ImGui::RadioButton("VI", ui_lang == Lang::VI)) ui_lang = Lang::VI;
            ImGui::SameLine();
            if (ImGui::Button(tr("EASY", "DE"), ImVec2(120, 30))) { state.apply_quick_difficulty(0); state.init_game(); state.current_scene = GameScene::Playing; state.playBackgroundMusic("battle"); view_initialized = false; }
            ImGui::SameLine();
            if (ImGui::Button(tr("MEDIUM", "TRUNG BINH"), ImVec2(120, 30))) { state.apply_quick_difficulty(1); state.init_game(); state.current_scene = GameScene::Playing; state.playBackgroundMusic("battle"); view_initialized = false; }
            ImGui::SameLine();
            if (ImGui::Button(tr("HARD", "KHO"), ImVec2(120, 30))) { state.apply_quick_difficulty(2); state.init_game(); state.current_scene = GameScene::Playing; state.playBackgroundMusic("battle"); view_initialized = false; }
            ImGui::SameLine();
            if (ImGui::Button(tr("UNFAIR", "SIEU KHO"), ImVec2(120, 30))) { state.apply_quick_difficulty(3); state.init_game(); state.current_scene = GameScene::Playing; state.playBackgroundMusic("battle"); view_initialized = false; }

            // Audio toggles
            ImGui::SameLine(); ImGui::Dummy(ImVec2(20, 1)); ImGui::SameLine();
            {
                bool music_on = state.music_enabled;
                if (ImGui::Checkbox(tr("Music", "Nhac nen"), &music_on)) {
                    state.music_enabled = music_on;
                    if (music_on) state.playBackgroundMusic("menu");
                    else state.stopBackgroundMusic();
                }
                ImGui::SameLine();
                bool sfx_on = state.sfx_enabled;
                if (ImGui::Checkbox(tr("SFX", "Am thanh"), &sfx_on)) {
                    state.setSfxEnabled(sfx_on);
                }
                ImGui::SameLine(); ImGui::Dummy(ImVec2(12, 1)); ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.35f, 0.55f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.50f, 0.75f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.28f, 0.45f, 1.0f));
                if (ImGui::Button(tr("Game Guide", "Huong Dan"), ImVec2(130, 30))) show_guide_popup = true;
                ImGui::PopStyleColor(3);
            }

            ImGui::Separator(); ImGui::Spacing();
            ImGui::Columns(2, "menu_split", false); ImGui::SetColumnWidth(0, 690);
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "--- BATTLEFIELD DIMENSIONS ---");
            
            auto draw_legend_item = [&](const char* label, ImVec4 color, int ratio) {
                ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImDrawList* d = ImGui::GetWindowDrawList();
                d->AddRectFilled(ImVec2(cursor.x, cursor.y + 3.0f), ImVec2(cursor.x + 12.0f, cursor.y + 15.0f), ImGui::ColorConvertFloat4ToU32(color));
                d->AddRect(ImVec2(cursor.x, cursor.y + 3.0f), ImVec2(cursor.x + 12.0f, cursor.y + 15.0f), ImGui::ColorConvertFloat4ToU32(ImVec4(1,1,1,0.5f)));
                ImGui::Dummy(ImVec2(15.0f, 18.0f));
                ImGui::SameLine();
                ImGui::Text("%s: %d%%", label, ratio);
            };

            auto draw_elegant_slider = [&](const char* label, int* val, int min_val, int max_val, ImVec4 bar_color, ImVec4 text_color = ImVec4(1, 1, 1, 1)) {
                ImGui::TextColored(text_color, "%s: %d", label, *val);
                float width = ImGui::GetContentRegionAvail().x - 10.0f;
                if (width < 100.0f) width = 360.0f;
                float height = 12.0f;
                
                ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                char id_buf[128];
                snprintf(id_buf, sizeof(id_buf), "##ele_slider_%s", label);
                ImGui::InvisibleButton(id_buf, ImVec2(width, height + 8.0f));
                
                bool active = ImGui::IsItemActive();
                bool hovered = ImGui::IsItemHovered();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                
                if (active) {
                    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                    float clicked_pct = (mouse_pos.x - cursor_pos.x) / width;
                    clicked_pct = std::max(0.0f, std::min(1.0f, clicked_pct));
                    *val = min_val + std::round(clicked_pct * (max_val - min_val));
                    *val = std::max(min_val, std::min(max_val, *val));
                }
                
                float pct = static_cast<float>(*val - min_val) / (max_val - min_val);
                float x0 = cursor_pos.x;
                float x_knob = x0 + pct * width;
                float y_center = cursor_pos.y + 4.0f + height / 2.0f;
                
                ImU32 col_bg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.16f, 0.18f, 1.0f));
                ImU32 col_bar = ImGui::ColorConvertFloat4ToU32(bar_color);
                ImU32 col_knob = ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                if (active) col_knob = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
                else if (hovered) col_knob = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                
                draw_list->AddRectFilled(
                    ImVec2(x0, y_center - 2.5f),
                    ImVec2(x0 + width, y_center + 2.5f),
                    col_bg,
                    2.0f
                );
                draw_list->AddRectFilled(
                    ImVec2(x0, y_center - 2.5f),
                    ImVec2(x_knob, y_center + 2.5f),
                    col_bar,
                    2.0f
                );
                
                draw_list->AddCircleFilled(
                    ImVec2(x_knob, y_center),
                    5.0f,
                    col_knob
                );
                draw_list->AddCircle(
                    ImVec2(x_knob, y_center),
                    5.0f,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f)),
                    0,
                    1.0f
                );
                
                ImGui::Spacing();
            };

            int prev_w = state.active_config.map_width; int prev_h = state.active_config.map_height;
            draw_elegant_slider("Map Width (Horizontal)", &state.active_config.map_width, 10, 25, ImVec4(0.2f, 0.7f, 0.9f, 1.0f));
            draw_elegant_slider("Map Height (Vertical)", &state.active_config.map_height, 10, 16, ImVec4(0.2f, 0.7f, 0.9f, 1.0f));
            if (state.active_config.map_width != prev_w || state.active_config.map_height != prev_h) {
                state.active_config.custom_grid.assign(state.active_config.map_width, std::vector<Terrain>(state.active_config.map_height, Terrain::Dirt));
                state.active_config.custom_human_pos = {1, 1};
            }

            ImGui::Checkbox("ACTIVATE INITIAL SPAWN SHIELD", &state.active_config.spawn_shield);
            ImGui::Checkbox("Use Custom Map Design Mode", &state.active_config.custom_map_mode);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", tr("--- PROCEDURAL GENERATION RATIOS (SUM = 100%) ---", "--- TI LE SINH BAN CO (TONG = 100%) ---"));

            int temp_dirt = state.active_config.ratio_dirt;
            int temp_wall = state.active_config.ratio_wall;
            int temp_water = state.active_config.ratio_water;
            int temp_forest = state.active_config.ratio_forest;
            int temp_ice = state.active_config.ratio_ice;

            int total_r = temp_dirt + temp_wall + temp_water + temp_forest + temp_ice;
            if (total_r != 100) {
                state.active_config.ratio_dirt = 55;
                state.active_config.ratio_wall = 10;
                state.active_config.ratio_water = 10;
                state.active_config.ratio_forest = 15;
                state.active_config.ratio_ice = 10;
                temp_dirt = 55; temp_wall = 10; temp_water = 10; temp_forest = 15; temp_ice = 10;
            }

            int p1 = temp_dirt;
            int p2 = p1 + temp_wall;
            int p3 = p2 + temp_water;
            int p4 = p3 + temp_forest;

            float bar_width = ImGui::GetContentRegionAvail().x - 10.0f;
            if (bar_width < 100.0f) bar_width = 360.0f;
            float bar_height = 20.0f;

            ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##multi_slider_button", ImVec2(bar_width, bar_height + 8.0f));
            
            bool active = ImGui::IsItemActive();
            bool hovered = ImGui::IsItemHovered();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            static int active_knob = -1;
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;

            if (active) {
                float clicked_pct = (mouse_pos.x - cursor_pos.x) / bar_width * 100.0f;
                clicked_pct = std::max(0.0f, std::min(100.0f, clicked_pct));

                if (ImGui::IsMouseClicked(0)) {
                    float d1 = std::abs(clicked_pct - p1);
                    float d2 = std::abs(clicked_pct - p2);
                    float d3 = std::abs(clicked_pct - p3);
                    float d4 = std::abs(clicked_pct - p4);
                    if (d1 <= d2 && d1 <= d3 && d1 <= d4) active_knob = 0;
                    else if (d2 <= d1 && d2 <= d3 && d2 <= d4) active_knob = 1;
                    else if (d3 <= d1 && d3 <= d2 && d3 <= d4) active_knob = 2;
                    else active_knob = 3;
                }

                // MIN_KNOB_DISTANCE = 1 keeps knobs adjacent but never overlapping
                if (active_knob == 0) {
                    p1 = std::round(clicked_pct);
                    p1 = std::max(0, std::min(p2 - 1, p1));
                } else if (active_knob == 1) {
                    p2 = std::round(clicked_pct);
                    p2 = std::max(p1 + 1, std::min(p3 - 1, p2));
                } else if (active_knob == 2) {
                    p3 = std::round(clicked_pct);
                    p3 = std::max(p2 + 1, std::min(p4 - 1, p3));
                } else if (active_knob == 3) {
                    p4 = std::round(clicked_pct);
                    p4 = std::max(p3 + 1, std::min(100, p4));
                }
            } else {
                active_knob = -1;
            }

            state.active_config.ratio_dirt = p1;
            state.active_config.ratio_wall = p2 - p1;
            state.active_config.ratio_water = p3 - p2;
            state.active_config.ratio_forest = p4 - p3;
            state.active_config.ratio_ice = 100 - p4;

            temp_dirt = state.active_config.ratio_dirt;
            temp_wall = state.active_config.ratio_wall;
            temp_water = state.active_config.ratio_water;
            temp_forest = state.active_config.ratio_forest;
            temp_ice = state.active_config.ratio_ice;

            float x0 = cursor_pos.x;
            float x1 = x0 + p1 * (bar_width / 100.f);
            float x2 = x0 + p2 * (bar_width / 100.f);
            float x3 = x0 + p3 * (bar_width / 100.f);
            float x4 = x0 + p4 * (bar_width / 100.f);
            float x5 = x0 + bar_width;

            float y_top = cursor_pos.y + 4.0f;
            float y_bot = y_top + bar_height;

            ImU32 col_dirt = ImGui::ColorConvertFloat4ToU32(ImVec4(105/255.f, 60/255.f, 35/255.f, 1.f));
            ImU32 col_wall = ImGui::ColorConvertFloat4ToU32(ImVec4(60/255.f, 62/255.f, 66/255.f, 1.f));
            ImU32 col_water = ImGui::ColorConvertFloat4ToU32(ImVec4(35/255.f, 75/255.f, 115/255.f, 1.f));
            ImU32 col_forest = ImGui::ColorConvertFloat4ToU32(ImVec4(34/255.f, 110/255.f, 48/255.f, 1.f));
            ImU32 col_ice = ImGui::ColorConvertFloat4ToU32(ImVec4(160/255.f, 210/255.f, 240/255.f, 1.f));

            draw_list->AddRectFilled(ImVec2(x0, y_top), ImVec2(x1, y_bot), col_dirt);
            draw_list->AddRectFilled(ImVec2(x1, y_top), ImVec2(x2, y_bot), col_wall);
            draw_list->AddRectFilled(ImVec2(x2, y_top), ImVec2(x3, y_bot), col_water);
            draw_list->AddRectFilled(ImVec2(x3, y_top), ImVec2(x4, y_bot), col_forest);
            draw_list->AddRectFilled(ImVec2(x4, y_top), ImVec2(x5, y_bot), col_ice);

            auto draw_knob = [&](float x_center, int knob_idx) {
                bool knob_hovered = hovered && std::abs(mouse_pos.x - x_center) < 10.0f;
                bool knob_active = (active_knob == knob_idx);
                ImU32 knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                if (knob_active) knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
                else if (knob_hovered) knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                float knob_half_w = 3.0f;
                draw_list->AddRectFilled(
                    ImVec2(x_center - knob_half_w, y_top - 2.0f),
                    ImVec2(x_center + knob_half_w, y_bot + 2.0f),
                    knob_color,
                    1.5f
                );
                draw_list->AddRect(
                    ImVec2(x_center - knob_half_w, y_top - 2.0f),
                    ImVec2(x_center + knob_half_w, y_bot + 2.0f),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f)),
                    1.5f
                );
            };

            draw_knob(x1, 0);
            draw_knob(x2, 1);
            draw_knob(x3, 2);
            draw_knob(x4, 3);

            draw_list->AddRect(ImVec2(x0, y_top), ImVec2(x5, y_bot), ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.6f)), 0.0f, 0, 1.5f);

            ImGui::Spacing();

            draw_legend_item(tr("Dirt", "Dat"), ImVec4(105/255.f, 60/255.f, 35/255.f, 1.f), temp_dirt); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Wall", "Tuong"), ImVec4(60/255.f, 62/255.f, 66/255.f, 1.f), temp_wall); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Water", "Nuoc"), ImVec4(35/255.f, 75/255.f, 115/255.f, 1.f), temp_water); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Forest", "Rung"), ImVec4(34/255.f, 110/255.f, 48/255.f, 1.f), temp_forest); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Ice", "Bang"), ImVec4(160/255.f, 210/255.f, 240/255.f, 1.f), temp_ice);

            if (state.active_config.custom_map_mode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.1f, 1));
                if (ImGui::Button("⚙️ OPEN MAP VISUAL EDITOR", ImVec2(400, 35))) state.current_scene = GameScene::MapEditor;
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "--- HUMAN OPERATIVE STATS ---");
            draw_elegant_slider("Vital HP", &state.active_config.human_hp, 1, 20, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Turn 1 Stamina", &state.active_config.initial_stamina, 1, 6, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Pistol Ammo", &state.active_config.pistol_ammo, 0, 50, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Shotgun Shells", &state.active_config.shotgun_ammo, 0, 30, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Grenades", &state.active_config.grenades, 0, 10, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Claymore Mines", &state.active_config.mines, 0, 10, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Molotov Bombs", &state.active_config.molotovs, 0, 10, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Operation Max Turns", &state.active_config.turn_limit, 10, 200, ImVec4(0.9f, 0.8f, 0.4f, 1.0f));

            ImGui::NextColumn();

            ImGui::Checkbox("Enable Environment Events", &state.active_config.enable_environment);
            if (state.active_config.enable_environment) {
                ImGui::Indent(15.0f);
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", tr("ENVIRONMENT EVENT PROBABILITIES (SUM = 100%)", "XAC SUAT BIEN CO MOI TRUONG (TONG = 100%)"));
                
                int temp_clear = state.active_config.env_prob_clear;
                int temp_wind = state.active_config.env_prob_wind;
                int temp_rain = state.active_config.env_prob_rain;
                int temp_clouds = state.active_config.env_prob_clouds;
                int temp_light = state.active_config.env_prob_lightning;
                int temp_heatwave = state.active_config.env_prob_heatwave;
                int temp_blizzard = state.active_config.env_prob_blizzard;

                int total_env = temp_clear + temp_wind + temp_rain + temp_clouds + temp_light + temp_heatwave + temp_blizzard;
                if (total_env != 100) {
                    state.active_config.env_prob_clear = 50;
                    state.active_config.env_prob_wind = 14;
                    state.active_config.env_prob_rain = 12;
                    state.active_config.env_prob_clouds = 4;
                    state.active_config.env_prob_lightning = 8;
                    state.active_config.env_prob_heatwave = 6;
                    state.active_config.env_prob_blizzard = 6;
                    temp_clear = 50; temp_wind = 14; temp_rain = 12; temp_clouds = 4; temp_light = 8; temp_heatwave = 6; temp_blizzard = 6;
                }

                int ep1 = temp_clear;
                int ep2 = ep1 + temp_wind;
                int ep3 = ep2 + temp_rain;
                int ep4 = ep3 + temp_clouds;
                int ep5 = ep4 + temp_light;
                int ep6 = ep5 + temp_heatwave;

                float ebar_width = ImGui::GetContentRegionAvail().x - 10.0f;
                if (ebar_width < 100.0f) ebar_width = 360.0f;
                float ebar_height = 20.0f;

                ImVec2 ecursor_pos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##multi_env_slider_button", ImVec2(ebar_width, ebar_height + 8.0f));
                
                bool eactive = ImGui::IsItemActive();
                bool ehovered = ImGui::IsItemHovered();
                ImDrawList* edraw_list = ImGui::GetWindowDrawList();

                static int eactive_knob = -1;
                ImVec2 emouse_pos = ImGui::GetIO().MousePos;

                if (eactive) {
                    float eclicked_pct = (emouse_pos.x - ecursor_pos.x) / ebar_width * 100.0f;
                    eclicked_pct = std::max(0.0f, std::min(100.0f, eclicked_pct));

                    if (ImGui::IsMouseClicked(0)) {
                        float ed1 = std::abs(eclicked_pct - ep1);
                        float ed2 = std::abs(eclicked_pct - ep2);
                        float ed3 = std::abs(eclicked_pct - ep3);
                        float ed4 = std::abs(eclicked_pct - ep4);
                        float ed5 = std::abs(eclicked_pct - ep5);
                        float ed6 = std::abs(eclicked_pct - ep6);
                        if (ed1 <= ed2 && ed1 <= ed3 && ed1 <= ed4 && ed1 <= ed5 && ed1 <= ed6) eactive_knob = 0;
                        else if (ed2 <= ed1 && ed2 <= ed3 && ed2 <= ed4 && ed2 <= ed5 && ed2 <= ed6) eactive_knob = 1;
                        else if (ed3 <= ed1 && ed3 <= ed2 && ed3 <= ed4 && ed3 <= ed5 && ed3 <= ed6) eactive_knob = 2;
                        else if (ed4 <= ed1 && ed4 <= ed2 && ed4 <= ed3 && ed4 <= ed5 && ed4 <= ed6) eactive_knob = 3;
                        else if (ed5 <= ed1 && ed5 <= ed2 && ed5 <= ed3 && ed5 <= ed4 && ed5 <= ed6) eactive_knob = 4;
                        else eactive_knob = 5;
                    }

                    const int MIN_KNOB_DISTANCE = 1;  // Minimum distance between knobs to prevent overlap

                    if (eactive_knob == 0) {
                        ep1 = std::round(eclicked_pct);
                        ep1 = std::max(0, std::min(ep2 - MIN_KNOB_DISTANCE, ep1));
                    } else if (eactive_knob == 1) {
                        ep2 = std::round(eclicked_pct);
                        ep2 = std::max(ep1 + MIN_KNOB_DISTANCE, std::min(ep3 - MIN_KNOB_DISTANCE, ep2));
                    } else if (eactive_knob == 2) {
                        ep3 = std::round(eclicked_pct);
                        ep3 = std::max(ep2 + MIN_KNOB_DISTANCE, std::min(ep4 - MIN_KNOB_DISTANCE, ep3));
                    } else if (eactive_knob == 3) {
                        ep4 = std::round(eclicked_pct);
                        ep4 = std::max(ep3 + MIN_KNOB_DISTANCE, std::min(ep5 - MIN_KNOB_DISTANCE, ep4));
                    } else if (eactive_knob == 4) {
                        ep5 = std::round(eclicked_pct);
                        ep5 = std::max(ep4 + MIN_KNOB_DISTANCE, std::min(ep6 - MIN_KNOB_DISTANCE, ep5));
                    } else if (eactive_knob == 5) {
                        ep6 = std::round(eclicked_pct);
                        ep6 = std::max(ep5 + MIN_KNOB_DISTANCE, std::min(100, ep6));
                    }
                } else {
                    eactive_knob = -1;
                }

                state.active_config.env_prob_clear = ep1;
                state.active_config.env_prob_wind = ep2 - ep1;
                state.active_config.env_prob_rain = ep3 - ep2;
                state.active_config.env_prob_clouds = ep4 - ep3;
                state.active_config.env_prob_lightning = ep5 - ep4;
                state.active_config.env_prob_heatwave = ep6 - ep5;
                state.active_config.env_prob_blizzard = 100 - ep6;

                temp_clear = state.active_config.env_prob_clear;
                temp_wind = state.active_config.env_prob_wind;
                temp_rain = state.active_config.env_prob_rain;
                temp_clouds = state.active_config.env_prob_clouds;
                temp_light = state.active_config.env_prob_lightning;
                temp_heatwave = state.active_config.env_prob_heatwave;
                temp_blizzard = state.active_config.env_prob_blizzard;

                float ex0 = ecursor_pos.x;
                float ex1 = ex0 + ep1 * (ebar_width / 100.f);
                float ex2 = ex0 + ep2 * (ebar_width / 100.f);
                float ex3 = ex0 + ep3 * (ebar_width / 100.f);
                float ex4 = ex0 + ep4 * (ebar_width / 100.f);
                float ex5 = ex0 + ep5 * (ebar_width / 100.f);
                float ex6 = ex0 + ep6 * (ebar_width / 100.f);
                float ex7 = ex0 + ebar_width;

                float ey_top = ecursor_pos.y + 4.0f;
                float ey_bot = ey_top + ebar_height;

                ImU32 col_clear = ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.7f, 0.9f, 1.0f));
                ImU32 col_wind = ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.85f, 0.95f, 1.0f));
                ImU32 col_rain = ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.55f, 0.85f, 1.0f));
                ImU32 col_clouds = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
                ImU32 col_light = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
                ImU32 col_heatwave = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
                ImU32 col_blizzard = ImGui::ColorConvertFloat4ToU32(ImVec4(0.6f, 0.8f, 1.0f, 1.0f));

                edraw_list->AddRectFilled(ImVec2(ex0, ey_top), ImVec2(ex1, ey_bot), col_clear);
                edraw_list->AddRectFilled(ImVec2(ex1, ey_top), ImVec2(ex2, ey_bot), col_wind);
                edraw_list->AddRectFilled(ImVec2(ex2, ey_top), ImVec2(ex3, ey_bot), col_rain);
                edraw_list->AddRectFilled(ImVec2(ex3, ey_top), ImVec2(ex4, ey_bot), col_clouds);
                edraw_list->AddRectFilled(ImVec2(ex4, ey_top), ImVec2(ex5, ey_bot), col_light);
                edraw_list->AddRectFilled(ImVec2(ex5, ey_top), ImVec2(ex6, ey_bot), col_heatwave);
                edraw_list->AddRectFilled(ImVec2(ex6, ey_top), ImVec2(ex7, ey_bot), col_blizzard);

                auto draw_eknob = [&](float x_center, int knob_idx) {
                    bool knob_hovered = ehovered && std::abs(emouse_pos.x - x_center) < 10.0f;
                    bool knob_active = (eactive_knob == knob_idx);
                    ImU32 knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
                    if (knob_active) knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
                    else if (knob_hovered) knob_color = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                    float knob_half_w = 3.0f;
                    edraw_list->AddRectFilled(
                        ImVec2(x_center - knob_half_w, ey_top - 2.0f),
                        ImVec2(x_center + knob_half_w, ey_bot + 2.0f),
                        knob_color,
                        1.5f
                    );
                    edraw_list->AddRect(
                        ImVec2(x_center - knob_half_w, ey_top - 2.0f),
                        ImVec2(x_center + knob_half_w, ey_bot + 2.0f),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f)),
                        1.5f
                    );
                };

                draw_eknob(ex1, 0);
                draw_eknob(ex2, 1);
                draw_eknob(ex3, 2);
                draw_eknob(ex4, 3);
                draw_eknob(ex5, 4);
                draw_eknob(ex6, 5);

                edraw_list->AddRect(ImVec2(ex0, ey_top), ImVec2(ex7, ey_bot), ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.6f)), 0.0f, 0, 1.5f);

                ImGui::Spacing();

                // Draw legend items in three rows to avoid overflow
                draw_legend_item(tr("Clear", "Troi quang"), ImVec4(0.5f, 0.7f, 0.9f, 1.0f), temp_clear); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Wind", "Gio lon"), ImVec4(0.7f, 0.85f, 0.95f, 1.0f), temp_wind); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Rain", "Mua to"), ImVec4(0.35f, 0.55f, 0.85f, 1.0f), temp_rain); 
                
                ImGui::Spacing();
                
                draw_legend_item(tr("Clouds", "May mu"), ImVec4(0.3f, 0.3f, 0.35f, 1.0f), temp_clouds); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Lightning", "Sam set"), ImVec4(1.0f, 0.85f, 0.2f, 1.0f), temp_light); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Heatwave", "Nang nong"), ImVec4(1.0f, 0.5f, 0.2f, 1.0f), temp_heatwave);
                
                ImGui::Spacing();
                
                draw_legend_item(tr("Blizzard", "Bang gia"), ImVec4(0.6f, 0.8f, 1.0f, 1.0f), temp_blizzard);
                
                ImGui::Unindent(15.0f);
                ImGui::Spacing();
            }
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "--- HOSTILE THREAT QUANTITIES ---");
            draw_elegant_slider("Normal Zombies", &state.active_config.count_normal, 0, 20, ImVec4(0.2f, 0.8f, 0.4f, 1.0f), ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
            draw_elegant_slider("Fast Sprinters", &state.active_config.count_fast, 0, 15, ImVec4(0.35f, 0.75f, 1.0f, 1.0f), ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
            draw_elegant_slider("Volatile Exploders", &state.active_config.count_exploding, 0, 15, ImVec4(0.9f, 0.5f, 0.1f, 1.0f), ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
            draw_elegant_slider("Vampiric Draculas", &state.active_config.count_vampire, 0, 10, ImVec4(0.7f, 0.2f, 0.7f, 1.0f), ImVec4(0.7f, 0.2f, 0.7f, 1.0f));
            draw_elegant_slider("Sick Carriers", &state.active_config.count_sick, 0, 15, ImVec4(0.85f, 0.78f, 0.3f, 1.0f), ImVec4(0.85f, 0.78f, 0.3f, 1.0f));

            int available_slots = state.calculate_available_spawn_cells();
            int total_zoms = state.active_config.count_normal + state.active_config.count_fast + state.active_config.count_exploding + state.active_config.count_vampire + state.active_config.count_sick;
            
            ImGui::Spacing(); ImGui::Text("Current Map Available Spawn Tiles: %d", available_slots); ImGui::Text("Total Zombies Requested: %d", total_zoms);
            bool overflow_error = !state.is_zombie_count_valid();
            if (overflow_error) ImGui::TextColored(ImVec4(1, 0.1f, 0.1f, 1), "⚠️ WARNING: TOO MANY ZOMBIES!");

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::TextColored(ImVec4(0, 1, 0.5f, 1), "--- IMPORT / EXPORT DATA HUB ---");
            ImGui::InputText("Save path", state.export_filename, IM_ARRAYSIZE(state.export_filename));
            if (ImGui::Button("Save Configuration & Map File", ImVec2(400, 30))) state.export_challenge_file(state.export_filename);
            ImGui::InputText("Load path", state.import_filename, IM_ARRAYSIZE(state.import_filename));
            if (ImGui::Button("Load External Shared Challenge", ImVec2(400, 30))) state.import_challenge_file(state.import_filename);

            ImGui::Columns(1); ImGui::Separator(); ImGui::Spacing();

            if (overflow_error) ImGui::TextColored(ImVec4(1,0,0,1), "Fix error to run simulation.");
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.6f, 0.15f, 1));
                float full_btn_w = ImGui::GetContentRegionAvail().x - 10.0f;
                if (ImGui::Button("LAUNCH CUSTOM STRATEGY COMBAT", ImVec2(full_btn_w, 45))) { state.init_game(); state.current_scene = GameScene::Playing; state.playBackgroundMusic("battle"); view_initialized = false; }
                ImGui::PopStyleColor();
            }

            // ── Game Guide popup (accessible from menu hub) ────────────────
            if (show_guide_popup) ImGui::OpenPopup(tr("Game Guide##menu", "Cam Nang Tro Choi##menu"));
            if (ImGui::BeginPopupModal(tr("Game Guide##menu", "Cam Nang Tro Choi##menu"), &show_guide_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::SetNextWindowSizeConstraints(ImVec2(700, 0), ImVec2(700, 580));
                ImGui::BeginChild("##guide_scroll_menu", ImVec2(680, 520), false, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "%s", tr("ZOMCHESS — GAME WIKI", "ZOMCHESS — CAM NANG TRO CHOI"));
                ImGui::Spacing();
                ImGui::TextWrapped("%s", tr(
                    "Open this guide in-game for the full interactive reference. Below is a quick overview.",
                    "Mo huong dan nay trong game de xem tai lieu day du. Duoi day la tom tat nhanh."
                ));
                ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Turn Structure", "Cau Truc Luot"));
                ImGui::BulletText("%s", tr("Human acts first, then Zombies, then Environment.", "Human hanh dong truoc, sau do Zombie, roi Moi Truong."));
                ImGui::BulletText("%s", tr("Win: eliminate all zombies. Lose: HP = 0 or turn limit exceeded.", "Thang: diet het zombie. Thua: HP = 0 hoac het luot."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Stamina", "The Luc"));
                ImGui::BulletText("%s", tr("Rolled 1-6 each turn. Move = 1 (Water = 2). Each weapon use = 1.", "Tung 1-6 moi luot. Di chuyen = 1 (Nuoc = 2). Moi vu khi = 1."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Weapons", "Vu Khi"));
                ImGui::BulletText("%s", tr("Knife: melee, 1 dmg. Pistol: ranged, accuracy drops with distance.", "Dao: can chien, 1 sat thuong. Sung luc: xa, do chinh xac giam theo khoang cach."));
                ImGui::BulletText("%s", tr("Shotgun: 3-tile line, hits all. Grenade: 1-turn fuse, AoE blast.", "Sung hoa mai: 3 o thang hang, ban tat ca. Luu dan: no sau 1 luot, sat thuong vung."));
                ImGui::BulletText("%s", tr("Molotov: throws fire 1-6 tiles. Mine: placed trap, 1-tile blast.", "Bom xang: nem lua 1-6 o. Min: bay dat san, no 1 o."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Zombie Types", "Loai Zombie"));
                ImGui::BulletText("%s", tr("Normal: 1 move/turn. Fast: 2 moves/turn. Exploding: detonates on death.", "Binh thuong: 1 buoc/luot. Nhanh: 2 buoc/luot. No: phat no khi chet."));
                ImGui::BulletText("%s", tr("Vampire: heals on hit. Sick: infects — reduces your stamina next turn.", "Ma ca rong: hoi phuc khi tan cong. Benh: lay nhiem — giam the luc luot sau."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Terrain", "Dia Hinh"));
                ImGui::BulletText("%s", tr("Dirt: normal. Wall: blocks movement & shots. Water: costs 2 stamina.", "Dat: binh thuong. Tuong: chan di chuyen & dan. Nuoc: ton 2 the luc."));
                ImGui::BulletText("%s", tr("Forest: blocks line-of-sight. Fire: damages on entry. Ice: may slide.", "Rung: chan tam nhin. Lua: gay sat thuong khi buoc vao. Bang: co the truot."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Environment Events", "Bien Co Moi Truong"));
                ImGui::BulletText("%s", tr("Wind, Rain, Lightning, Heatwave, Blizzard — each reshapes the battlefield.", "Gio, Mua, Set, Nang Nong, Bao Tuyet — moi su kien thay doi chien truong."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "%s", tr("Tip: Start with EASY to learn the mechanics!", "Meo: Bat dau voi DE de hoc co che tro choi!"));
                ImGui::EndChild();
                ImGui::Separator();
                if (ImGui::Button(tr("Close", "Dong"), ImVec2(160, 34))) {
                    show_guide_popup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }
        else if (state.current_scene == GameScene::MapEditor) {
            int mw = state.active_config.map_width; int mh = state.active_config.map_height;
            for (int x = 0; x < mw; ++x) {
                for (int y = 0; y < mh; ++y) {
                    sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    cell.setPosition(x * cellSize + boardOffset, y * cellSize + boardOffset);
                    Terrain t = state.active_config.custom_grid[x][y];
                    if (t == Terrain::Wall) cell.setFillColor(sf::Color(60, 62, 66));
                    else if (t == Terrain::Water) cell.setFillColor(sf::Color(35, 75, 115));
                    else if (t == Terrain::Forest) cell.setFillColor(sf::Color(34, 110, 48));
                    else if (t == Terrain::Ice) cell.setFillColor(sf::Color(160, 210, 240));
                    else cell.setFillColor(sf::Color(105, 60, 35));
                    window.draw(cell);

                    if (x == state.active_config.custom_human_pos.x && y == state.active_config.custom_human_pos.y) {
                        sf::CircleShape marker(8.0f); marker.setFillColor(sf::Color::White);
                        marker.setPosition(x * cellSize + boardOffset + cellSize/2 - 8.0f, y * cellSize + boardOffset + cellSize/2 - 8.0f);
                        window.draw(marker);
                    }
                }
            }

            ImGui::SetNextWindowPos(ImVec2(mw * cellSize + boardOffset + 30.0f, 20.0f));
            ImGui::SetNextWindowSize(ImVec2(600, 618));
            ImGui::Begin("To set Human's initial position, choose Dirt then Shift+Click", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "Brush Selection Tools:");
            if (ImGui::RadioButton("Plain Dirt Floor", state.editor_selected_terrain == Terrain::Dirt)) state.editor_selected_terrain = Terrain::Dirt;
            if (ImGui::RadioButton("Reinforced Wall", state.editor_selected_terrain == Terrain::Wall)) state.editor_selected_terrain = Terrain::Wall;
            if (ImGui::RadioButton("Deep Water Hazard", state.editor_selected_terrain == Terrain::Water)) state.editor_selected_terrain = Terrain::Water;
            if (ImGui::RadioButton(state.tr("Dense Forest", "Rung Ram Cung").c_str(), state.editor_selected_terrain == Terrain::Forest)) state.editor_selected_terrain = Terrain::Forest;
            if (ImGui::RadioButton(state.tr("Ice Terrain", "Dia Hinh Bang").c_str(), state.editor_selected_terrain == Terrain::Ice)) state.editor_selected_terrain = Terrain::Ice;

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            if (ImGui::Button("SAVE DESIGN & RETURN TO ENGINE", ImVec2(360, 50))) state.current_scene = GameScene::MainMenu;
            ImGui::End();
        }
        else if (state.current_scene == GameScene::Playing) {
            float timeSec = animationClock.getElapsedTime().asSeconds();

            int padX = (state.width < VIEW_CELLS) ? (VIEW_CELLS - state.width) / 2 : 0;
            int padY = (state.height < VIEW_CELLS) ? (VIEW_CELLS - state.height) / 2 : 0;
            for (int lx = 0; lx < VIEW_CELLS; ++lx) {
                for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                    int x = viewX + (lx - padX);
                    int y = viewY + (ly - padY);
                    bool in_map = (x >= 0 && x < state.width && y >= 0 && y < state.height);
                    sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    cell.setPosition(lx * cellSize + boardOffset, ly * cellSize + boardOffset);

                    if (!in_map || state.grid[x][y] == Terrain::Wall) {
                        cell.setFillColor(sf::Color(60, 62, 66));
                    }
                    else if (state.grid[x][y] == Terrain::Water) {
                        // Check for terrain transitions
                        sf::Color finalColor(35, 75, 115);
                        for (const auto& trans : state.terrain_transitions) {
                            if (trans.pos.x == x && trans.pos.y == y && trans.to_terrain == Terrain::Water) {
                                float progress = trans.timer / trans.max_duration;
                                sf::Color fromColor, toColor;
                                if (trans.from_terrain == Terrain::Ice) {
                                    fromColor = sf::Color(160, 210, 240);
                                    toColor = sf::Color(35, 75, 115);
                                } else if (trans.from_terrain == Terrain::Dirt) {
                                    fromColor = sf::Color(105, 60, 35);
                                    toColor = sf::Color(35, 75, 115);
                                }
                                finalColor = sf::Color(
                                    static_cast<sf::Uint8>(fromColor.r + progress * (toColor.r - fromColor.r)),
                                    static_cast<sf::Uint8>(fromColor.g + progress * (toColor.g - fromColor.g)),
                                    static_cast<sf::Uint8>(fromColor.b + progress * (toColor.b - fromColor.b))
                                );
                                break;
                            }
                        }
                        cell.setFillColor(finalColor);
                    }
                    else if (state.grid[x][y] == Terrain::Fire) {
                        int pulse = static_cast<int>(25.0f * std::sin(timeSec * 12.0f));
                        cell.setFillColor(sf::Color(220 + pulse, 100 + pulse / 2, 20));
                    }
                    else if (state.grid[x][y] == Terrain::Forest) {
                        // Check for terrain transitions
                        sf::Color finalColor(34, 110, 48);
                        for (const auto& trans : state.terrain_transitions) {
                            if (trans.pos.x == x && trans.pos.y == y && trans.to_terrain == Terrain::Forest) {
                                float progress = trans.timer / trans.max_duration;
                                sf::Color fromColor(105, 60, 35);  // Dirt
                                sf::Color toColor(34, 110, 48);    // Forest
                                finalColor = sf::Color(
                                    static_cast<sf::Uint8>(fromColor.r + progress * (toColor.r - fromColor.r)),
                                    static_cast<sf::Uint8>(fromColor.g + progress * (toColor.g - fromColor.g)),
                                    static_cast<sf::Uint8>(fromColor.b + progress * (toColor.b - fromColor.b))
                                );
                                break;
                            }
                        }
                        cell.setFillColor(finalColor);
                    }
                    else if (state.grid[x][y] == Terrain::Ice) {
                        // Check for terrain transitions
                        sf::Color finalColor(160, 210, 240);
                        for (const auto& trans : state.terrain_transitions) {
                            if (trans.pos.x == x && trans.pos.y == y && trans.to_terrain == Terrain::Ice) {
                                float progress = trans.timer / trans.max_duration;
                                sf::Color fromColor(35, 75, 115);  // Water
                                sf::Color toColor(160, 210, 240);  // Ice
                                finalColor = sf::Color(
                                    static_cast<sf::Uint8>(fromColor.r + progress * (toColor.r - fromColor.r)),
                                    static_cast<sf::Uint8>(fromColor.g + progress * (toColor.g - fromColor.g)),
                                    static_cast<sf::Uint8>(fromColor.b + progress * (toColor.b - fromColor.b))
                                );
                                break;
                            }
                        }
                        // Shimmering ice: light blue with subtle pulse
                        int pulse = static_cast<int>(15.0f * std::sin(timeSec * 3.0f + x * 0.7f + y * 0.5f));
                        finalColor.r = std::min(255, static_cast<int>(finalColor.r) + pulse);
                        finalColor.g = std::min(255, static_cast<int>(finalColor.g) + pulse/2);
                        cell.setFillColor(finalColor);
                    }
                    else {
                        // Dirt - check for transitions
                        sf::Color finalColor(105, 60, 35);
                        for (const auto& trans : state.terrain_transitions) {
                            if (trans.pos.x == x && trans.pos.y == y && trans.to_terrain == Terrain::Dirt) {
                                float progress = trans.timer / trans.max_duration;
                                sf::Color fromColor, toColor(105, 60, 35);  // Dirt
                                if (trans.from_terrain == Terrain::Water) {
                                    fromColor = sf::Color(35, 75, 115);
                                } else if (trans.from_terrain == Terrain::Forest) {
                                    fromColor = sf::Color(34, 110, 48);
                                }
                                finalColor = sf::Color(
                                    static_cast<sf::Uint8>(fromColor.r + progress * (toColor.r - fromColor.r)),
                                    static_cast<sf::Uint8>(fromColor.g + progress * (toColor.g - fromColor.g)),
                                    static_cast<sf::Uint8>(fromColor.b + progress * (toColor.b - fromColor.b))
                                );
                                break;
                            }
                        }
                        cell.setFillColor(finalColor);
                    }
                    
                    window.draw(cell);

                    if (in_map && state.mine_grid[x][y]) {
                        sf::CircleShape mine(6.0f); mine.setFillColor(sf::Color(230, 40, 40)); mine.setOrigin(6.0f, 6.0f);
                        mine.setPosition(lx * cellSize + boardOffset + cellSize/2, ly * cellSize + boardOffset + cellSize/2); window.draw(mine);
                    }
                }
            }
            if (hasFont) {
                for (int lx = 0; lx < VIEW_CELLS; ++lx) {
                    sf::Text tx;
                    tx.setFont(boardFont);
                    tx.setCharacterSize(12);
                    tx.setString(std::to_string(viewX + (lx - padX) + 1));
                    tx.setFillColor(sf::Color(180, 190, 205));
                    tx.setPosition(lx * cellSize + boardOffset + cellSize * 0.35f, boardOffset - 16.0f);
                    window.draw(tx);
                }
                for (int ly = 0; ly < VIEW_CELLS; ++ly) {
                    sf::Text ty;
                    ty.setFont(boardFont);
                    ty.setCharacterSize(12);
                    ty.setString(std::to_string(viewY + (ly - padY) + 1));
                    ty.setFillColor(sf::Color(180, 190, 205));
                    ty.setPosition(boardOffset - 16.0f, ly * cellSize + boardOffset + cellSize * 0.28f);
                    window.draw(ty);
                }
            }

            for (const auto& g : state.active_grenades) {
                if (g.active) {
                    sf::CircleShape gren(8.0f, 4); gren.setFillColor(sf::Color(50, 210, 50)); gren.setOrigin(8.0f, 8.0f);
                    int glx = g.pos.x - viewX + padX;
                    int gly = g.pos.y - viewY + padY;
                    if (glx >= 0 && glx < VIEW_CELLS && gly >= 0 && gly < VIEW_CELLS) {
                        gren.setPosition(glx * cellSize + boardOffset + cellSize/2, gly * cellSize + boardOffset + cellSize/2); window.draw(gren);
                    }
                }
            }

            // Draw loot drops as blinking "?" icons
            if (hasFont) {
                for (const auto& ld : state.loot_drops) {
                    int llx = ld.pos.x - viewX + padX;
                    int lly = ld.pos.y - viewY + padY;
                    if (llx < 0 || llx >= VIEW_CELLS || lly < 0 || lly >= VIEW_CELLS) continue;

                    // Blinking background
                    float blink = std::sin(ld.blink_timer * 4.0f) * 0.5f + 0.5f; // 0..1
                    sf::Uint8 bgAlpha = static_cast<sf::Uint8>(120 + blink * 80);
                    sf::RectangleShape lootBg(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
                    lootBg.setPosition(llx * cellSize + boardOffset + 4.0f, lly * cellSize + boardOffset + 4.0f);
                    lootBg.setFillColor(sf::Color(80, 60, 20, bgAlpha));
                    lootBg.setOutlineThickness(1.5f);
                    lootBg.setOutlineColor(sf::Color(220, 180, 60, bgAlpha));
                    window.draw(lootBg);

                    // "?" text
                    sf::Uint8 txtAlpha = static_cast<sf::Uint8>(180 + blink * 75);
                    sf::Text qMark;
                    qMark.setFont(boardFont);
                    qMark.setString("?");
                    qMark.setCharacterSize(static_cast<unsigned int>(cellSize * 0.55f));
                    qMark.setFillColor(sf::Color(255, 220, 60, txtAlpha));
                    sf::FloatRect qBounds = qMark.getLocalBounds();
                    qMark.setOrigin(qBounds.left + qBounds.width / 2.0f, qBounds.top + qBounds.height / 2.0f);
                    qMark.setPosition(llx * cellSize + boardOffset + cellSize / 2.0f,
                                      lly * cellSize + boardOffset + cellSize / 2.0f);
                    window.draw(qMark);
                }
            }

            for (size_t i = 0; i < state.zombies.size(); ++i) {
                const auto& z = state.zombies[i];
                int zlx = z->pos.x - viewX + padX;
                int zly = z->pos.y - viewY + padY;
                if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
                if (z->hp <= 0) {
                    // Nếu có loot ở đây thì không vẽ ô đen (loot "?" đã được vẽ ở layer trước)
                    bool has_loot = false;
                    for (const auto& ld : state.loot_drops) {
                        if (ld.pos == z->pos) { has_loot = true; break; }
                    }
                    if (!has_loot) {
                        sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
                        deadZ.setFillColor(sf::Color(15, 15, 15, 160));
                        deadZ.setPosition(zlx * cellSize + boardOffset + 4.0f, zly * cellSize + boardOffset + 4.0f);
                        window.draw(deadZ);
                    }
                    continue;
                }
                
                float drawX = zlx * cellSize + boardOffset + 3.0f;
                float drawY = zly * cellSize + boardOffset + 3.0f;
                
                // Handle ice slide animation for this zombie
                if (state.ice_slide_animation.active && !state.ice_slide_animation.is_human && state.ice_slide_animation.zombie_idx == i) {
                    if (state.ice_slide_animation.current_step < state.ice_slide_animation.path.size()) {
                        Position slide_pos = state.ice_slide_animation.path[state.ice_slide_animation.current_step];
                        int slide_lx = slide_pos.x - viewX + padX;
                        int slide_ly = slide_pos.y - viewY + padY;
                        if (slide_lx >= 0 && slide_lx < VIEW_CELLS && slide_ly >= 0 && slide_ly < VIEW_CELLS) {
                            drawX = slide_lx * cellSize + boardOffset + 3.0f;
                            drawY = slide_ly * cellSize + boardOffset + 3.0f;
                        }
                    }
                }
                
                if (state.active_fx.type == FXType::Wind) {
                    bool was_pushed = false;
                    for (auto p : state.active_fx.blast_cells) {
                        if (p.x == z->pos.x && p.y == z->pos.y) {
                            was_pushed = true;
                            break;
                        }
                    }
                    if (was_pushed) {
                        float progress = 1.0f - (state.active_fx.timer / state.active_fx.max_duration);
                        drawX += (progress - 1.0f) * state.active_fx.dx * cellSize;
                        drawY += (progress - 1.0f) * state.active_fx.dy * cellSize;
                    }
                }

                sf::RectangleShape zVisual(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
                zVisual.setPosition(drawX, drawY);
                
                if (z->type == ZombieType::Fast) zVisual.setFillColor(sf::Color(55, 168, 255));
                else if (z->type == ZombieType::Exploding) zVisual.setFillColor(sf::Color(220, 110, 15));
                else if (z->type == ZombieType::Vampire) zVisual.setFillColor(sf::Color(130, 30, 130));
                else if (z->type == ZombieType::Sick) zVisual.setFillColor(sf::Color(210, 190, 65));
                else zVisual.setFillColor(sf::Color(45, 175, 90));
                window.draw(zVisual);

                // Segmented HP bar at bottom of zombie tile
                int safe_max_hp = std::max(1, z->max_hp);
                float barW = cellSize - 8.0f;
                float segmentGap = 1.0f;
                float segmentW = (barW - (safe_max_hp - 1) * segmentGap) / static_cast<float>(safe_max_hp);
                float baseX = drawX + 1.0f;
                float baseY = drawY + cellSize - 10.0f;
                for (int hpSeg = 0; hpSeg < safe_max_hp; ++hpSeg) {
                    sf::RectangleShape seg(sf::Vector2f(std::max(1.0f, segmentW), 3.0f));
                    seg.setPosition(baseX + hpSeg * (segmentW + segmentGap), baseY);
                    if (hpSeg < z->hp) seg.setFillColor(sf::Color(185, 36, 36));
                    else seg.setFillColor(sf::Color(55, 55, 55));
                    window.draw(seg);
                }

                if (hasFont) {
                    sf::Text zIdStr; zIdStr.setFont(boardFont); zIdStr.setString(std::to_string(i + 1)); zIdStr.setCharacterSize(14);
                    sf::FloatRect textRect = zIdStr.getLocalBounds(); zIdStr.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
                    zIdStr.setFillColor(sf::Color::White);
                    zIdStr.setPosition(drawX + (cellSize - 6.0f)/2.0f, drawY + (cellSize - 6.0f)/2.0f); window.draw(zIdStr);

                    // Blinking status tags on zombie icon
                    bool blinkOn = std::sin(timeSec * 10.0f) > 0.0f;
                    if (blinkOn && (z->is_burning || z->is_paralyzed || z->is_stunned || z->is_frozen)) {
                        if (z->is_burning) {
                            sf::Text burnTxt;
                            burnTxt.setFont(boardFont);
                            burnTxt.setCharacterSize(11);
                            burnTxt.setFillColor(sf::Color(230, 40, 40));
                            burnTxt.setString("B");
                            burnTxt.setPosition(drawX + 3.0f, drawY - 1.0f);
                            window.draw(burnTxt);
                        }
                        if (z->is_paralyzed) {
                            sf::Text paraTxt;
                            paraTxt.setFont(boardFont);
                            paraTxt.setCharacterSize(11);
                            paraTxt.setFillColor(sf::Color(242, 214, 61));
                            paraTxt.setString("P");
                            paraTxt.setPosition(drawX + cellSize - 17.0f, drawY - 1.0f);
                            window.draw(paraTxt);
                        }
                        if (z->is_stunned) {
                            sf::Text stunTxt;
                            stunTxt.setFont(boardFont);
                            stunTxt.setCharacterSize(11);
                            stunTxt.setFillColor(sf::Color(100, 220, 255));
                            stunTxt.setString("S");
                            stunTxt.setPosition(drawX + (cellSize - 6.0f) / 2.0f - 4.0f, drawY - 1.0f);
                            window.draw(stunTxt);
                        }
                        if (z->is_frozen) {
                            sf::Text frozTxt;
                            frozTxt.setFont(boardFont);
                            frozTxt.setCharacterSize(11);
                            frozTxt.setFillColor(sf::Color(160, 230, 255));
                            frozTxt.setString("F");
                            frozTxt.setPosition(drawX + (cellSize - 6.0f) / 2.0f + 5.0f, drawY - 1.0f);
                            window.draw(frozTxt);
                        }
                    }
                }
            }

            int hlx = state.human.pos.x - viewX + padX;
            int hly = state.human.pos.y - viewY + padY;
            
            float drawX = hlx * cellSize + boardOffset + 3.0f;
            float drawY = hly * cellSize + boardOffset + 3.0f;
            
            // Handle ice slide animation for human
            if (state.ice_slide_animation.active && state.ice_slide_animation.is_human) {
                if (state.ice_slide_animation.current_step < state.ice_slide_animation.path.size()) {
                    Position slide_pos = state.ice_slide_animation.path[state.ice_slide_animation.current_step];
                    int slide_lx = slide_pos.x - viewX + padX;
                    int slide_ly = slide_pos.y - viewY + padY;
                    if (slide_lx >= 0 && slide_lx < VIEW_CELLS && slide_ly >= 0 && slide_ly < VIEW_CELLS) {
                        drawX = slide_lx * cellSize + boardOffset + 3.0f;
                        drawY = slide_ly * cellSize + boardOffset + 3.0f;
                    }
                }
            }
            
            if (state.active_fx.type == FXType::Wind) {
                bool was_pushed = false;
                for (auto p : state.active_fx.blast_cells) {
                    if (p.x == state.human.pos.x && p.y == state.human.pos.y) {
                        was_pushed = true;
                        break;
                    }
                }
                if (was_pushed) {
                    float progress = 1.0f - (state.active_fx.timer / state.active_fx.max_duration);
                    drawX += (progress - 1.0f) * state.active_fx.dx * cellSize;
                    drawY += (progress - 1.0f) * state.active_fx.dy * cellSize;
                }
            }

            float hCx = drawX + (cellSize - 6.0f) * 0.5f;
            float hCy = drawY + (cellSize - 6.0f) * 0.5f;

            sf::RectangleShape hSquare(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
            hSquare.setPosition(drawX, drawY);
            hSquare.setFillColor(sf::Color::White);
            hSquare.setOutlineThickness(0.0f);
            if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(hSquare);

            sf::ConvexShape hTriangle(3);
            hTriangle.setPoint(0, sf::Vector2f(0.0f, -5.0f));
            hTriangle.setPoint(1, sf::Vector2f(-4.0f, 3.0f));
            hTriangle.setPoint(2, sf::Vector2f(4.0f, 3.0f));
            hTriangle.setPosition(hCx, hCy);
            hTriangle.setFillColor(sf::Color(100, 100, 100));
            if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(hTriangle);

            // Human HP segmented bar at bottom
            int humanMaxHp = std::max(1, state.active_config.human_hp);
            float hBarW = cellSize - 8.0f;
            float hGap = 1.0f;
            float hSegW = (hBarW - (humanMaxHp - 1) * hGap) / static_cast<float>(humanMaxHp);
            float hBaseX = hlx * cellSize + boardOffset + 4.0f;
            float hBaseY = hly * cellSize + boardOffset + cellSize - 7.0f;
            for (int hpSeg = 0; hpSeg < humanMaxHp; ++hpSeg) {
                sf::RectangleShape seg(sf::Vector2f(std::max(1.0f, hSegW), 3.0f));
                seg.setPosition(hBaseX + hpSeg * (hSegW + hGap), hBaseY);
                if (hpSeg < state.human.hp) seg.setFillColor(sf::Color(70, 205, 90));
                else seg.setFillColor(sf::Color(55, 55, 55));
                if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(seg);
            }

            // Human stamina dots above HP bar
            int safeStam = std::max(0, state.human.stamina);
            int maxDots = 14;
            int drawnDots = std::min(maxDots, safeStam);
            for (int d = 0; d < drawnDots; ++d) {
                sf::CircleShape dot(2.7f);
                dot.setFillColor(sf::Color(90, 205, 255));
                dot.setPosition(hlx * cellSize + boardOffset + 5.0f + d * 5.0f,
                                hly * cellSize + boardOffset + cellSize - 15.0f);
                if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(dot);
            }

            if (hasFont) {
                bool blinkOn = std::sin(timeSec * 10.0f) > 0.0f;
                if (blinkOn && (state.human.is_burning || state.human.is_paralyzed || state.human.is_stunned || state.human.is_frozen)) {
                    if (state.human.is_burning) {
                        sf::Text burnTxt;
                        burnTxt.setFont(boardFont);
                        burnTxt.setCharacterSize(11);
                        burnTxt.setFillColor(sf::Color(230, 40, 40));
                        burnTxt.setString("B");
                        burnTxt.setPosition(hlx * cellSize + boardOffset + 6.0f, hly * cellSize + boardOffset + 2.0f);
                        if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(burnTxt);
                    }
                    if (state.human.is_paralyzed) {
                        sf::Text paraTxt;
                        paraTxt.setFont(boardFont);
                        paraTxt.setCharacterSize(11);
                        paraTxt.setFillColor(sf::Color(242, 214, 61));
                        paraTxt.setString("P");
                        paraTxt.setPosition(hlx * cellSize + boardOffset + cellSize - 14.0f, hly * cellSize + boardOffset + 2.0f);
                        if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(paraTxt);
                    }
                    if (state.human.is_stunned) {
                        sf::Text stunTxt;
                        stunTxt.setFont(boardFont);
                        stunTxt.setCharacterSize(11);
                        stunTxt.setFillColor(sf::Color(100, 220, 255));
                        stunTxt.setString("S");
                        stunTxt.setPosition(hlx * cellSize + boardOffset + cellSize / 2.0f - 4.0f, hly * cellSize + boardOffset + 2.0f);
                        if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(stunTxt);
                    }
                    if (state.human.is_frozen) {
                        sf::Text frozTxt;
                        frozTxt.setFont(boardFont);
                        frozTxt.setCharacterSize(11);
                        frozTxt.setFillColor(sf::Color(160, 230, 255));
                        frozTxt.setString("F");
                        frozTxt.setPosition(hlx * cellSize + boardOffset + cellSize / 2.0f + 4.0f, hly * cellSize + boardOffset + 2.0f);
                        if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(frozTxt);
                    }
                }
            }

            if (state.dark_cloud_active && state.active_fx.type != FXType::DarkCloud) {
                sf::RectangleShape shroud(sf::Vector2f(VIEW_CELLS * cellSize, VIEW_CELLS * cellSize));
                shroud.setFillColor(sf::Color(0, 0, 0, 254));
                shroud.setPosition(boardOffset, boardOffset);
                window.draw(shroud);
                sf::RectangleShape humanSpot(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
                humanSpot.setFillColor(sf::Color(225, 235, 245));
                humanSpot.setPosition(hlx * cellSize + boardOffset + 3.0f, hly * cellSize + boardOffset + 3.0f);
                if (hlx >= 0 && hlx < VIEW_CELLS && hly >= 0 && hly < VIEW_CELLS) window.draw(humanSpot);
            }

            if (state.turn_banner_fx.type != FXType::None) {
                float p = state.turn_banner_fx.timer / state.turn_banner_fx.max_duration;
                sf::Uint8 a = static_cast<sf::Uint8>(255 * p);
                if (!state.turn_banner_fx.banner_text.empty()) {
                    a = 255;
                }
                if (hasFont) {
                    sf::Text msg;
                    msg.setFont(boardFont);
                    msg.setCharacterSize(state.turn_banner_fx.banner_text.empty() ? 70 : 45);
                    std::string banner_str = state.turn_banner_fx.banner_text.empty() ? "YOUR TURN" : state.turn_banner_fx.banner_text;
                    msg.setString(banner_str);
                    msg.setFillColor(sf::Color(255, 245, 120, a));
                    sf::FloatRect r = msg.getLocalBounds();
                    msg.setOrigin(r.left + r.width/2.0f, r.top + r.height/2.0f);
                    msg.setPosition(boardOffset + VIEW_CELLS * cellSize * 0.5f, boardOffset + VIEW_CELLS * cellSize * 0.5f);
                    window.draw(msg);
                }
            }

            // Handle FX
            if (state.active_fx.type != FXType::None) {
                float progress = state.active_fx.timer / state.active_fx.max_duration;
                sf::Uint8 alpha = static_cast<sf::Uint8>(progress * 255);
                if (state.active_fx.type == FXType::Pistol) {
                    sf::Vertex bLine[] = { sf::Vertex(state.active_fx.start_p, sf::Color(255, 255, 100, alpha)), sf::Vertex(state.active_fx.end_p, sf::Color(255, 60, 60, alpha)) };
                    window.draw(bLine, 2, sf::Lines);
                } else if (state.active_fx.type == FXType::Molotov) {
                    float t = 1.0f - progress;
                    sf::Vector2f current_pos = state.active_fx.start_p + (state.active_fx.end_p - state.active_fx.start_p) * t;
                    sf::CircleShape proj(6.0f, 6); 
                    proj.setFillColor(sf::Color(255, 120, 0, 230)); 
                    proj.setOutlineColor(sf::Color(200, 30, 0, 230));
                    proj.setOutlineThickness(1.5f);
                    proj.setOrigin(6.0f, 6.0f);
                    proj.setPosition(current_pos);
                    window.draw(proj);
                } else if (state.active_fx.type == FXType::GrenadeFly) {
                    float t = 1.0f - progress;
                    sf::Vector2f current_pos = state.active_fx.start_p + (state.active_fx.end_p - state.active_fx.start_p) * t;
                    sf::CircleShape proj(6.0f, 8); 
                    proj.setFillColor(sf::Color(100, 255, 100, 220)); 
                    proj.setOutlineColor(sf::Color(30, 150, 30, 220));
                    proj.setOutlineThickness(1.5f);
                    proj.setOrigin(6.0f, 6.0f);
                    proj.setPosition(current_pos);
                    window.draw(proj);
                } else if (state.active_fx.type == FXType::Shotgun || state.active_fx.type == FXType::Explosion) {
                    float intensity = std::sqrt(progress);
                    if (state.active_fx.type == FXType::Explosion) {
                        // High-frequency blinking/flashing effect for explosions
                        float flash = 0.2f + 0.8f * std::abs(std::sin(state.active_fx.timer * 35.0f));
                        intensity = progress * flash;
                    }
                    for (auto p : state.active_fx.blast_cells) {
                        sf::RectangleShape blastZone(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                        blastZone.setFillColor(state.active_fx.type == FXType::Shotgun ? sf::Color(255,130,30, intensity*180) : sf::Color(255,50,10, intensity*240));
                        blastZone.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset); window.draw(blastZone);
                    }
                } else if (state.active_fx.type == FXType::Wind) {
                    sf::Color windColor(180, 220, 255, alpha);
                    float board_dim = VIEW_CELLS * cellSize;
                    for (int i = 0; i < 35; ++i) {
                        float perp_offset = std::fmod(i * 37.0f, board_dim);
                        float sweep = std::fmod(i * 71.0f + timeSec * 750.0f, board_dim);
                        
                        float sx, sy;
                        if (std::abs(state.active_fx.dx) > 0.5f) { // Horizontal wind
                            sx = boardOffset + (state.active_fx.dx > 0 ? sweep : (board_dim - sweep));
                            sy = boardOffset + perp_offset + std::sin(timeSec * 5.0f + i) * 10.0f;
                        } else { // Vertical wind
                            sx = boardOffset + perp_offset + std::cos(timeSec * 5.0f + i) * 10.0f;
                            sy = boardOffset + (state.active_fx.dy > 0 ? sweep : (board_dim - sweep));
                        }
                        
                        float ex = sx + state.active_fx.dx * 120.0f;
                        float ey = sy + state.active_fx.dy * 120.0f;
                        
                        sf::Vertex gust[] = {
                            sf::Vertex(sf::Vector2f(sx, sy), windColor),
                            sf::Vertex(sf::Vector2f(ex, ey), sf::Color(180, 220, 255, 0))
                        };
                        window.draw(gust, 2, sf::Lines);
                    }
                    for (auto p : state.active_fx.blast_cells) {
                        int plx = p.x - viewX + padX;
                        int ply = p.y - viewY + padY;
                        if (plx >= 0 && plx < VIEW_CELLS && ply >= 0 && ply < VIEW_CELLS) {
                            sf::RectangleShape pushed(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                            pushed.setFillColor(sf::Color(200, 235, 255, progress * 90));
                            pushed.setPosition(plx * cellSize + boardOffset, ply * cellSize + boardOffset);
                            window.draw(pushed);
                        }
                    }
                } else if (state.active_fx.type == FXType::Rain || state.active_fx.type == FXType::DarkCloud) {
                    if (state.active_fx.type == FXType::Rain) {
                        float board_dim = VIEW_CELLS * cellSize;
                        sf::Color rainColor(110, 170, 255, alpha);
                        for (int i = 0; i < 150; ++i) {
                            float x = boardOffset + std::fmod(i * 47.0f + timeSec * 320.0f, board_dim);
                            float y = boardOffset + std::fmod(i * 73.0f + timeSec * 480.0f, board_dim);
                            sf::Vertex drop[] = {
                                sf::Vertex(sf::Vector2f(x, y), rainColor),
                                sf::Vertex(sf::Vector2f(x - 8.0f, y + 18.0f), sf::Color(110, 170, 255, 0))
                            };
                            window.draw(drop, 2, sf::Lines);
                        }
                    } else {
                        sf::RectangleShape cloud(sf::Vector2f(state.width * cellSize, state.height * cellSize));
                        cloud.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>((1.0f - progress) * 254)));
                        cloud.setPosition(boardOffset, boardOffset);
                        window.draw(cloud);
                    }
                } else if (state.active_fx.type == FXType::Heatwave) {
                    // Golden/yellow shimmer overlay for heatwave
                    float intensity = 0.5f + 0.3f * std::sin(timeSec * 4.0f);  // Pulsing effect
                    sf::RectangleShape overlay(sf::Vector2f(state.width * cellSize, state.height * cellSize));
                    overlay.setFillColor(sf::Color(255, 220, 100, static_cast<sf::Uint8>(intensity * 100)));
                    overlay.setPosition(boardOffset, boardOffset);
                    window.draw(overlay);
                } else if (state.active_fx.type == FXType::Blizzard) {
                    // Icy white shimmer overlay for blizzard
                    float intensity = 0.5f + 0.3f * std::sin(timeSec * 4.0f);  // Pulsing effect
                    sf::RectangleShape overlay(sf::Vector2f(state.width * cellSize, state.height * cellSize));
                    overlay.setFillColor(sf::Color(220, 240, 255, static_cast<sf::Uint8>(intensity * 100)));
                    overlay.setPosition(boardOffset, boardOffset);
                    window.draw(overlay);
                } else if (state.active_fx.type == FXType::Lightning) {
                    sf::Vector2f target = state.getCellCenter(state.active_fx.cx, state.active_fx.cy, cellSize, boardOffset);
                    float startY = boardOffset;
                    float endY = target.y + cellSize / 2.0f;
                    
                    std::vector<sf::Vector2f> points;
                    points.push_back(sf::Vector2f(target.x, startY));
                    
                    int segments = 8;
                    float segmentH = (endY - startY) / segments;
                    
                    int seed_val = static_cast<int>(timeSec * 15.0f);
                    auto get_noise = [&](int step) -> float {
                        int h = seed_val * 73 + step * 31;
                        h = (h ^ 61) ^ (h >> 16);
                        h += (h << 3);
                        h ^= (h >> 4);
                        h *= 0x27d4eb2d;
                        h ^= (h >> 15);
                        float val = (float)(std::abs(h) % 100) / 100.0f;
                        return val * 2.0f - 1.0f;
                    };
                    
                    for (int i = 1; i < segments; ++i) {
                        float py = startY + i * segmentH;
                        float max_drift = 35.0f * (1.0f - (float)i / segments * 0.5f);
                        float px = target.x + get_noise(i) * max_drift;
                        points.push_back(sf::Vector2f(px, py));
                    }
                    points.push_back(target);
                    
                    auto draw_bolt = [&](sf::Color color, float offset_range) {
                        for (float dx_offset = -offset_range; dx_offset <= offset_range; dx_offset += 1.5f) {
                            std::vector<sf::Vertex> vertices;
                            for (const auto& p : points) {
                                float alpha_mult = (color.a * alpha) / 255.f;
                                sf::Color c = color;
                                c.a = static_cast<sf::Uint8>(alpha_mult);
                                vertices.push_back(sf::Vertex(sf::Vector2f(p.x + dx_offset, p.y), c));
                            }
                            window.draw(vertices.data(), vertices.size(), sf::LineStrip);
                        }
                    };
                    
                    draw_bolt(sf::Color(80, 200, 255, 120), 4.0f);
                    draw_bolt(sf::Color(255, 255, 180, 180), 2.0f);
                    draw_bolt(sf::Color(255, 255, 255, 255), 0.0f);
                    
                    for (auto p : state.active_fx.blast_cells) {
                        sf::RectangleShape electric(sf::Vector2f(cellSize - 4.0f, cellSize - 4.0f));
                        electric.setFillColor(sf::Color(80, 220, 255, progress * 130));
                        electric.setPosition(p.x * cellSize + boardOffset + 2.0f, p.y * cellSize + boardOffset + 2.0f);
                        window.draw(electric);
                    }
                }
            }

            // Render Attack Animations (Bite/Scratch)
            for (const auto& fx : state.attack_animations) {
                float progress = 1.0f - (fx.timer / fx.max_duration);
                sf::Uint8 alpha = static_cast<sf::Uint8>((1.0f - progress) * 255);
                sf::Vector2f center = state.getCellCenter(fx.cx, fx.cy, cellSize, boardOffset);

                if (fx.type == FXType::Bite) {
                    float dist = (progress < 0.5f) ? (15.0f * (1.0f - progress * 2.0f)) : 0.0f;
                    sf::Color teethCol(255, 50, 50, alpha);
                    sf::Vertex top[] = { sf::Vertex(sf::Vector2f(center.x - 8, center.y - dist - 4), teethCol), sf::Vertex(sf::Vector2f(center.x, center.y - dist + 6), teethCol), sf::Vertex(sf::Vector2f(center.x + 8, center.y - dist - 4), teethCol) };
                    sf::Vertex bot[] = { sf::Vertex(sf::Vector2f(center.x - 8, center.y + dist + 4), teethCol), sf::Vertex(sf::Vector2f(center.x, center.y + dist - 6), teethCol), sf::Vertex(sf::Vector2f(center.x + 8, center.y + dist + 4), teethCol) };
                    window.draw(top, 3, sf::LineStrip);
                    window.draw(bot, 3, sf::LineStrip);
                } else if (fx.type == FXType::Scratch) {
                    sf::Color clawCol(220, 220, 220, alpha);
                    for (int i = -1; i <= 1; ++i) {
                        float off = i * 6.0f;
                        sf::Vertex line[] = { sf::Vertex(sf::Vector2f(center.x - 10 + off, center.y - 12 + off), clawCol), sf::Vertex(sf::Vector2f(center.x + 10 + off, center.y + 12 + off), clawCol) };
                        window.draw(line, 2, sf::Lines);
                    }
                }
            }

            // Render Floating Text HP Updates
            if (hasFont) {
                for (const auto& ft : state.floating_texts) {
                    float progress = 1.0f - (ft.timer / ft.max_duration);
                    sf::Uint8 alpha = static_cast<sf::Uint8>((1.0f - progress) * 255);
                    sf::Text fTxt;
                    fTxt.setFont(boardFont);
                    fTxt.setString((ft.amount > 0 ? "+" : "") + std::to_string(ft.amount));
                    fTxt.setCharacterSize(16);
                    fTxt.setFillColor(sf::Color(ft.amount > 0 ? 50 : 255, ft.amount > 0 ? 255 : 50, 50, alpha));
                    
                    sf::Vector2f center = state.getCellCenter(ft.pos.x, ft.pos.y, cellSize, boardOffset);
                    fTxt.setPosition(center.x - 8, center.y - 15 - (progress * 25.0f));
                    window.draw(fTxt);
                }
            }

            float scroll_thickness = 12.0f;
            float panelX = boardOffset + VIEW_CELLS * cellSize + 6.0f + scroll_thickness + 12.0f;
            float panelY = boardOffset;
            float panelW2 = 1400.0f - panelX - 20.0f;
            float panelH = VIEW_CELLS * cellSize + 6.0f + scroll_thickness;
            ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(panelW2, panelH), ImGuiCond_Always);
            ImGui::Begin("Tactical Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            ImGui::TextColored(ImVec4(0.3f,0.95f,1,1), "%s %d/%d | ST %d | HP %d | %s [%d,%d] | %s: %s",
                               tr("TURN","LUOT"), state.current_turn, state.turn_limit, state.human.stamina, state.human.hp,
                               tr("Pos", "Vi tri"), state.human.pos.x + 1, state.human.pos.y + 1,
                               tr("Env", "MT"), state.last_environment_event.c_str());
            if (state.human.is_burning) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.45f, 0.1f, 1), "BURNING"); }
            if (state.human.is_paralyzed) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.45f, 0.9f, 1.0f, 1), "PARALYZED"); }
            if (state.human.is_frozen) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1), "FROZEN"); }
            if (state.dark_cloud_active) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1), "DARK CLOUD"); }
            // Audio toggles moved to left side to avoid text cutoff
            ImGui::SameLine(); ImGui::Dummy(ImVec2(10, 1)); ImGui::SameLine();
            {
                bool music_on = state.music_enabled;
                if (ImGui::Checkbox(tr("Music##ig", "Nhac##ig"), &music_on)) {
                    state.music_enabled = music_on;
                    if (music_on) state.playBackgroundMusic("battle");
                    else state.stopBackgroundMusic();
                }
                ImGui::SameLine();
                bool sfx_on = state.sfx_enabled;
                if (ImGui::Checkbox(tr("SFX##ig", "Am thanh##ig"), &sfx_on)) {
                    state.setSfxEnabled(sfx_on);
                }
            }
            ImGui::Separator();

            bool human_turn = (state.phase == TurnPhase::HumanTurn);
            if (!human_turn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.35f, 0.35f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.35f, 0.35f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.35f, 0.35f, 0.8f));
                ImGui::BeginDisabled();
            }
            if (ImGui::Button(tr("END TURN", "KET THUC LUOT"), ImVec2(200, 35)) && human_turn) {
                state.start_zombie_phase();
            }
            if (!human_turn) {
                ImGui::EndDisabled();
                ImGui::PopStyleColor(3);
            }
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(40, 1));
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.6f, 0.0f, 1.0f));
            if (ImGui::Button(tr("GAME GUIDE", "HUONG DAN"), ImVec2(150, 35))) show_guide_popup = true;
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.14f, 0.14f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.58f, 0.08f, 0.08f, 1.0f));
            if (ImGui::Button(tr("Return Hub", "Ve Hub"), ImVec2(150, 35))) show_confirm_return_hub = true;
            ImGui::PopStyleColor(3);

            ImGui::TextColored(ImVec4(1, 0.55f, 0.9f, 1), "%s", tr("Weapons:", "Vu khi:"));
            
            auto weapon_button = [&](const char* label, InputMode mode) {
                bool disabled = state.human.is_paralyzed || state.human.stamina == 0 || state.phase != TurnPhase::HumanTurn;
                if (disabled) { ImGui::BeginDisabled(); }
                if (state.input_mode == mode) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1));
                else ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
                if (ImGui::Button(label) && !disabled) state.input_mode = mode;
                ImGui::PopStyleColor();
                if (disabled) { ImGui::EndDisabled(); }
            };

            weapon_button("Knife", InputMode::TargetKnife); ImGui::SameLine();
            weapon_button(("Pistol (" + std::to_string(state.human.pistol_ammo) + ")").c_str(), InputMode::TargetPistol); ImGui::SameLine();
            weapon_button(("Shotgun (" + std::to_string(state.human.shotgun_ammo) + ")").c_str(), InputMode::TargetShotgun);
            
            weapon_button(("Grenade (" + std::to_string(state.human.grenades) + ")").c_str(), InputMode::TargetGrenade); ImGui::SameLine();
            weapon_button(("Molotov (" + std::to_string(state.human.molotovs) + ")").c_str(), InputMode::TargetMolotov); ImGui::SameLine();
            
            bool mine_disabled = state.human.is_paralyzed || state.human.stamina == 0 || state.phase != TurnPhase::HumanTurn || state.human.mines == 0;
            if (mine_disabled) { ImGui::BeginDisabled(); }
            if (ImGui::Button(("Mine (" + std::to_string(state.human.mines) + ")").c_str())) {
                if (!mine_disabled) {
                    state.mine_grid[state.human.pos.x][state.human.pos.y] = true; 
                    state.human.mines--; state.human.stamina--;
                }
            }
            if (mine_disabled) { ImGui::EndDisabled(); }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s: %zu", tr("Hostiles", "Zombie"), state.zombies.size());
            ImGui::Text("%s:", tr("Legend", "Chu giai"));
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.2f, 0.75f, 0.3f, 1.0f), "Normal");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Fast");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f), "Exploder");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.7f, 0.2f, 0.7f, 1.0f), "Vampire");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.85f, 0.78f, 0.3f, 1.0f), "Sick");
            ImGui::Text("%s:", tr("Status", "Trang thai"));
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.95f, 0.2f, 0.2f, 1.0f), "B = Burned");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.25f, 1.0f), "P = Paralyzed");
            ImGui::Text("%s:", tr("Zombie IDs", "So thu tu Zombie"));
            float start_x = ImGui::GetCursorPosX();
            for (size_t i = 0; i < state.zombies.size(); ++i) {
                int col = i % 10;
                if (col > 0) ImGui::SameLine();
                ImGui::SetCursorPosX(start_x + col * 42.0f);
                
                const auto& z = state.zombies[i];
                ImVec4 idColor = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
                if (z->hp > 0) {
                    if (z->type == ZombieType::Fast) idColor = ImVec4(0.35f, 0.75f, 1.0f, 1.0f);
                    else if (z->type == ZombieType::Exploding) idColor = ImVec4(0.9f, 0.5f, 0.1f, 1.0f);
                    else if (z->type == ZombieType::Vampire) idColor = ImVec4(0.7f, 0.2f, 0.7f, 1.0f);
                    else if (z->type == ZombieType::Sick) idColor = ImVec4(0.85f, 0.78f, 0.3f, 1.0f);
                    else idColor = ImVec4(0.2f, 0.8f, 0.4f, 1.0f);
                }
                char id_buf[32];
                snprintf(id_buf, sizeof(id_buf), "#%2zu", i + 1);
                ImGui::TextColored(idColor, "%s", id_buf);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0.85f, 0.25f, 1), "=== %s ===", tr("LIVE RADIO LOGS", "NHAT KY VO TUYEN"));
            ImGui::BeginChild("LiveLogBox", ImVec2(0, ImGui::GetContentRegionAvail().y - 5.0f), true);
            for (const auto& log : state.logs) {
                ImGui::PushTextWrapPos(0.0f);
                ImGui::TextColored(log.color, "%s", log.text.c_str());
                ImGui::PopTextWrapPos();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            ImGui::SetNextWindowPos(ImVec2(boardOffset, boardOffset + VIEW_CELLS * cellSize + 6.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(VIEW_CELLS * cellSize, scroll_thickness), ImGuiCond_Always);
            ImGui::Begin("HScroll", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
            {
                ImVec2 h_start = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##hscroll_btn", ImVec2(VIEW_CELLS * cellSize, scroll_thickness));
                bool h_active = ImGui::IsItemActive();
                bool h_hovered = ImGui::IsItemHovered();
                ImDrawList* h_draw = ImGui::GetWindowDrawList();
                
                int maxHX = std::max(0, state.width - VIEW_CELLS);
                float h_track_w = VIEW_CELLS * cellSize;
                float h_thumb_w = maxHX > 0 ? ((float)VIEW_CELLS / (float)state.width) * h_track_w : h_track_w;
                h_thumb_w = std::max(30.0f, h_thumb_w);
                float h_max_travel = h_track_w - h_thumb_w;
                
                float h_thumb_x = maxHX > 0 ? ((float)viewX / (float)maxHX) * h_max_travel : 0.0f;
                
                if (h_active && maxHX > 0) {
                    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                    float pct = (mouse_pos.x - h_start.x - h_thumb_w * 0.5f) / h_max_travel;
                    pct = std::max(0.0f, std::min(1.0f, pct));
                    viewX = std::round(pct * maxHX);
                }
                
                h_draw->AddRectFilled(h_start, ImVec2(h_start.x + h_track_w, h_start.y + scroll_thickness), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.4f)), 6.0f);
                
                ImVec4 thumb_col = ImVec4(0.5f, 0.5f, 0.5f, 0.6f);
                if (h_active) thumb_col = ImVec4(0.8f, 0.8f, 0.8f, 0.9f);
                else if (h_hovered) thumb_col = ImVec4(0.65f, 0.65f, 0.65f, 0.8f);
                
                h_draw->AddRectFilled(
                    ImVec2(h_start.x + h_thumb_x, h_start.y),
                    ImVec2(h_start.x + h_thumb_x + h_thumb_w, h_start.y + scroll_thickness),
                    ImGui::ColorConvertFloat4ToU32(thumb_col),
                    6.0f
                );
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(boardOffset + VIEW_CELLS * cellSize + 6.0f, boardOffset), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(scroll_thickness, VIEW_CELLS * cellSize), ImGuiCond_Always);
            ImGui::Begin("VScroll", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
            {
                ImVec2 v_start = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##vscroll_btn", ImVec2(scroll_thickness, VIEW_CELLS * cellSize));
                bool v_active = ImGui::IsItemActive();
                bool v_hovered = ImGui::IsItemHovered();
                ImDrawList* v_draw = ImGui::GetWindowDrawList();
                
                int maxVY = std::max(0, state.height - VIEW_CELLS);
                float v_track_h = VIEW_CELLS * cellSize;
                float v_thumb_h = maxVY > 0 ? ((float)VIEW_CELLS / (float)state.height) * v_track_h : v_track_h;
                v_thumb_h = std::max(30.0f, v_thumb_h);
                float v_max_travel = v_track_h - v_thumb_h;
                
                float v_thumb_y = maxVY > 0 ? ((float)viewY / (float)maxVY) * v_max_travel : 0.0f;
                
                if (v_active && maxVY > 0) {
                    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                    float pct = (mouse_pos.y - v_start.y - v_thumb_h * 0.5f) / v_max_travel;
                    pct = std::max(0.0f, std::min(1.0f, pct));
                    viewY = std::round(pct * maxVY);
                }
                
                v_draw->AddRectFilled(v_start, ImVec2(v_start.x + scroll_thickness, v_start.y + v_track_h), ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.4f)), 6.0f);
                
                ImVec4 thumb_col = ImVec4(0.5f, 0.5f, 0.5f, 0.6f);
                if (v_active) thumb_col = ImVec4(0.8f, 0.8f, 0.8f, 0.9f);
                else if (v_hovered) thumb_col = ImVec4(0.65f, 0.65f, 0.65f, 0.8f);
                
                v_draw->AddRectFilled(
                    ImVec2(v_start.x, v_start.y + v_thumb_y),
                    ImVec2(v_start.x + scroll_thickness, v_start.y + v_thumb_y + v_thumb_h),
                    ImGui::ColorConvertFloat4ToU32(thumb_col),
                    6.0f
                );
            }
            ImGui::End();

            ImGui::PopStyleVar(3);

            if (show_confirm_exit_game) ImGui::OpenPopup(tr("Exit ZomChess?", "Thoat Game ZomChess?"));
            if (ImGui::BeginPopupModal(tr("Exit ZomChess?", "Thoat Game ZomChess?"), &show_confirm_exit_game, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", tr("WARNING: All unsaved progress will be permanently lost!", "CANH BAO: Tat ca tien trinh chua luu se bi mat vinh vien!"));
                ImGui::Text("%s", tr("Are you sure you want to quit the game?", "Ban co chac chan muon thoat khoi tro choi?"));
                ImGui::Separator();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.14f, 0.14f, 1.0f));
                if (ImGui::Button(tr("Yes, Quit Game", "Co, Thoat Game"), ImVec2(180, 30))) {
                    window.close();
                    show_confirm_exit_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                
                if (ImGui::Button(tr("Cancel", "Huy"), ImVec2(180, 30))) {
                    show_confirm_exit_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (show_confirm_return_hub) ImGui::OpenPopup(tr("Exit Current Match?", "Thoat Tran Dau Hien Tai?"));
            if (ImGui::BeginPopupModal(tr("Exit Current Match?", "Thoat Tran Dau Hien Tai?"), &show_confirm_return_hub, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%s", tr("WARNING: All progress in the current match will be permanently lost!", "CANH BAO: Tat ca tien trinh tran dau hien tai se bi mat vinh vien!"));
                ImGui::Text("%s", tr("Are you sure you want to return to the Main Menu?", "Ban co chac chan muon quay lai Menu Chinh?"));
                ImGui::Separator();
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.14f, 0.14f, 1.0f));
                if (ImGui::Button(tr("Yes, Exit Match", "Co, Thoat Tran"), ImVec2(180, 30))) {
                    state.stopBackgroundMusic();
                    state.current_scene = GameScene::MainMenu;
                    state.playBackgroundMusic("menu");
                    show_confirm_return_hub = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                
                if (ImGui::Button(tr("Cancel", "Huy"), ImVec2(180, 30))) {
                    show_confirm_return_hub = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (show_guide_popup) ImGui::OpenPopup(tr("Game Guide", "Cam Nang Tro Choi"));
            if (ImGui::BeginPopupModal(tr("Game Guide", "Cam Nang Tro Choi"), &show_guide_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::SetNextWindowSizeConstraints(ImVec2(700, 0), ImVec2(700, 580));
                ImGui::BeginChild("##guide_scroll", ImVec2(680, 520), false, ImGuiWindowFlags_HorizontalScrollbar);

                // ── STORY ──────────────────────────────────────────────────────────────
                ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "%s", tr("ZOMCHESS — GAME WIKI", "ZOMCHESS — CAM NANG TRO CHOI"));
                ImGui::Spacing();
                if (ImGui::CollapsingHeader(tr("Story", "Boi Canh"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::TextWrapped("%s", tr(
                        "Year 2047. A rogue bioweapon experiment in the Arctic Circle shattered containment. The undead plague spread faster than any evacuation could handle. You are the last field operative of ECHO Squad — cut off, alone, and surrounded. Your only objective: eliminate every hostile before the extraction window closes. The battlefield is alive — fire spreads, storms roll in, and the dead keep coming.",
                        "Nam 2047. Mot thi nghiem vu khi sinh hoc bi mat o Bac Cuc mat kiem soat. Dich benh zombie lan rong truoc khi bat ky cuoc di tan nao co the thuc hien. Ban la chien binh cuoi cung cua Doi ECHO — bi cat dut lien lac, don doc, va bi bao vay. Muc tieu duy nhat: tieu diet moi ke thu truoc khi cua so thoat hiem dong lai. Chien truong song dong — lua lan, bao to ap den, va nhung ke chet van tiep tuc tien."
                    ));
                }
                ImGui::Spacing();

                // ── BASIC GAMEPLAY ─────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Basic Gameplay & Controls", "Huong Dan Co Ban & Dieu Khien"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Turn Structure", "Cau Truc Luot"));
                    ImGui::BulletText("%s", tr("Each round: Human acts first, then all Zombies act, then the Environment phase.", "Moi vong: Human hanh dong truoc, sau do tat ca Zombie hanh dong, cuoi cung la pha Moi Truong."));
                    ImGui::BulletText("%s", tr("Win: eliminate all zombies. Lose: HP reaches 0 or turn limit exceeded.", "Thang: tieu diet het zombie. Thua: HP ve 0 hoac het gioi han luot."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Stamina System", "He Thong The Luc"));
                    ImGui::BulletText("%s", tr("At the start of each Human turn, stamina is randomly rolled 1-6 (uniform).", "Dau moi luot Human, the luc duoc tung ngau nhien 1-6 (dong deu)."));
                    ImGui::BulletText("%s", tr("Moving to an adjacent tile costs 1 stamina (2 if the tile is Water).", "Di chuyen sang o ke ben ton 1 the luc (2 neu o do la Nuoc)."));
                    ImGui::BulletText("%s", tr("Every weapon action costs 1 stamina. Running out ends your turn automatically.", "Moi hanh dong vu khi ton 1 the luc. Het the luc tu dong ket thuc luot."));
                    ImGui::BulletText("%s", tr("If Paralyzed, stamina is set to 0 for that turn.", "Neu bi Te Liet, the luc ve 0 trong luot do."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Controls", "Dieu Khien"));
                    ImGui::BulletText("%s", tr("Left-click an adjacent tile (8-directional) to move.", "Click trai vao o ke ben (8 huong) de di chuyen."));
                    ImGui::BulletText("%s", tr("Select a weapon button, then left-click a target tile or direction.", "Chon nut vu khi, sau do click trai vao o muc tieu hoac huong ban."));
                    ImGui::BulletText("%s", tr("END TURN button: manually end your turn and pass to zombies.", "Nut KET THUC LUOT: ket thuc luot thu cong va chuyen sang zombie."));
                    ImGui::BulletText("%s", tr("RETURN HUB: return to main menu (confirmation required).", "RETURN HUB: quay ve menu chinh (can xac nhan)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("In-Game Panel", "Bang Thong Tin In-Game"));
                    ImGui::BulletText("%s", tr("Top bar: current turn / turn limit, HP bar, stamina bar.", "Thanh tren: luot hien tai / gioi han luot, thanh HP, thanh the luc."));
                    ImGui::BulletText("%s", tr("Weapon buttons show remaining ammo/count. Greyed out = unavailable.", "Cac nut vu khi hien so dan/so luong con lai. Mo nhat = khong the dung."));
                    ImGui::BulletText("%s", tr("Info list about the hostiles, dead ones are greyed out.", "Bieu tuong trang thai (B/P/F/S) hien ben canh HP khi kich hoat."));
                    ImGui::BulletText("%s", tr("Combat log on the right shows all events in real time.", "Nhat ky chien dau ben phai hien tat ca su kien theo thoi gian thuc."));
                    ImGui::BulletText("%s", tr("Music and SFX toggles are in the top-right of the panel.", "Nut bat/tat nhac va am thanh o goc tren phai cua bang."));
                }
                ImGui::Spacing();

                // ── ZOMBIE TYPES ───────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Zombie Types", "Cac Loai Zombie"))) {
                    ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.45f, 1.0f), "%s", tr("Normal Zombie  [HP: 2]", "Zombie Thuong  [HP: 2]"));
                    ImGui::BulletText("%s", tr("Moves 1 tile per turn (8-directional, weighted toward Human).", "Di chuyen 1 o moi luot (8 huong, uu tien huong Human)."));
                    ImGui::BulletText("%s", tr("Orthogonal attack: Bite -2 HP. Diagonal attack: Scratch -1 HP.", "Tan cong ngang/doc: Can -2 HP. Tan cong cheo: Cao -1 HP."));
                    ImGui::BulletText("%s", tr("In Water: Bite deals only -1 HP; Scratch deals 0 HP.", "Trong Nuoc: Can chi gay -1 HP; Cao gay 0 HP."));
                    //ImGui::BulletText("%s", tr("AI uses exponential weighting (lambda=1.2) — mostly advances, rarely retreats.", "AI dung trong so mu (lambda=1.2) — chu yeu tien, hiem khi lui."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", tr("Fast Sprinter  [HP: 2]", "Zombie Nhanh  [HP: 2]"));
                    ImGui::BulletText("%s", tr("Takes 2 move+attack actions per turn instead of 1.", "Co 2 hanh dong di chuyen+tan cong moi luot thay vi 1."));
                    ImGui::BulletText("%s", tr("In Water: capped to 1 action per turn (same as Normal).", "Trong Nuoc: bi gioi han con 1 hanh dong moi luot (giong Thuong)."));
                    ImGui::BulletText("%s", tr("When Frozen: thaws after only 1 turn (others need 2 turns).", "Khi bi Dong Bang: tu giai sau 1 luot (loai khac can 2 luot)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.1f, 1.0f), "%s", tr("Exploding Zombie  [HP: 3]", "Zombie No  [HP: 3]"));
                    ImGui::BulletText("%s", tr("On death (any cause): triggers explosion radius 1 — center 3 dmg, ring 2 dmg.", "Khi chet (bat ky nguyen nhan): kich no ban kinh 1 — tam 3 sat thuong, vong ngoai 2 sat thuong."));
                    //ImGui::BulletText("%s", tr("Chain reaction: if another Exploder is in blast range, it also detonates.", "Phan ung day chuyen: neu Zombie No khac trong vung no, no cung phat no."));
                    //ImGui::BulletText("%s", tr("Explosion radius reduced by 1 if standing on Water or Ice.", "Ban kinh no giam 1 neu dang dung tren Nuoc hoac Bang."));
                    ImGui::BulletText("%s", tr("Blast destroys Walls, melts Ice into Water, and destroys loot drops.", "Vu no pha Tuong, tan Bang thanh Nuoc, va pha huy loot drop."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.75f, 0.2f, 0.75f, 1.0f), "%s", tr("Vampire Zombie  [HP: 4]", "Zombie Hut Mau  [HP: 4]"));
                    ImGui::BulletText("%s", tr("After each successful attack, heals +1 HP (no cap shown, but max_hp is 4).", "Sau moi don tan cong thanh cong, hoi phuc +1 HP."));
                    ImGui::BulletText("%s", tr("Same attack pattern as Normal (Bite/Scratch). Highest base HP of all zombies.", "Cung kieu tan cong nhu Thuong (Can/Cao). HP co ban cao nhat."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.9f, 0.2f, 1.0f), "%s", tr("Sick Zombie  [HP: 2]", "Zombie Benh  [HP: 2]"));
                    ImGui::BulletText("%s", tr("On orthogonal Bite: halves remaining turns AND inflicts -1 stamina next turn.", "Khi Can ngang/doc: giam nua so luot con lai VA tru -1 stamina luot sau."));
                    ImGui::BulletText("%s", tr("Diagonal Scratch does NOT trigger infection — only direct Bite does.", "Cao cheo KHONG gay nhiem — chi Can truc tiep moi gay nhiem."));
                    //ImGui::BulletText("%s", tr("Multiple bites stack: each halves the remaining turns again.", "Nhieu lan can chong chong: moi lan lai giam nua so luot con lai."));
                }
                ImGui::Spacing();

                // ── WEAPONS ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Weapons", "Vu Khi"))) {
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.7f, 1.0f), "%s", tr("Knife  [Cost: 1 stamina, unlimited]", "Dao  [Chi phi: 1 the luc, khong gioi han]"));
                    ImGui::BulletText("%s", tr("Melee: click an adjacent tile (range 1, 8-directional). Deals -1 HP.", "Cong cu: click o ke ben (tam 1, 8 huong). Gay -1 HP."));
                    ImGui::BulletText("%s", tr("No ammo limit. Reliable for finishing low-HP zombies.", "Khong gioi han dan. Dang tin cay de ket lieu zombie it HP."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.35f, 1.0f), "%s", tr("Pistol  [Cost: 1 stamina + 1 ammo]", "Sung Luc  [Chi phi: 1 the luc + 1 dan]"));
                    ImGui::BulletText("%s", tr("Fires in 8 directions, range up to 20 tiles. Deals -1 HP on hit.", "Ban theo 8 huong, tam toi da 20 o. Gay -1 HP khi trung."));
                    ImGui::BulletText("%s", tr("Accuracy: exponential decay with distance. Point-blank ~100%%, long range ~10%%.", "Do chinh xac: giam mu theo khoang cach (lambda=4.32). Gan ~100%%, xa ~10%%."));
                    ImGui::BulletText("%s", tr("Bullet stops at first zombie hit or Wall. Passes through empty tiles.", "Dan dung lai o zombie dau tien trung hoac Tuong. Xuyen qua o trong."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", tr("Shotgun  [Cost: 1 stamina + 1 shell]", "Sung Hoa Mai  [Chi phi: 1 the luc + 1 vien]"));
                    ImGui::BulletText("%s", tr("Straight shot: expanding cone 3 steps deep (9 cells). Diagonal: triangular area (10 cells).", "Ban thang: hinh non mo rong 3 buoc (toi da 9 o). Cheo: vung tam giac."));
                    ImGui::BulletText("%s", tr("Each zombie in cone takes -1 HP, then pushed back 1 tile in fire direction.", "Moi zombie trong vung chiu -1 HP, sau do bi day 1 o theo huong ban."));
                    ImGui::BulletText("%s", tr("If pushed into Wall or another zombie: +1 collision damage to both.", "Neu bi day vao Tuong hoac zombie khac: +1 sat thuong va cham cho ca hai."));
                    //ImGui::BulletText("%s", tr("Recoil: Human pushed 1 tile backward. If blocked by Wall: Wall destroyed, Human -1 HP.", "Giat lui: Human bi day 1 o ra sau. Neu bi chan boi Tuong: Tuong bi pha, Human -1 HP."));
                    ImGui::BulletText("%s", tr("Shotgun blast destroys loot drops in its cone area.", "Dan shotgun pha huy loot drop trong vung ban."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", tr("Grenade  [Cost: 1 stamina + 1 grenade]", "Luu Dan  [Chi phi: 1 the luc + 1 qua]"));
                    ImGui::BulletText("%s", tr("Thrown in 8 directions. Travels 1-6 tiles randomly (stops before Walls).", "Nem theo 8 huong. Bay 1-6 o ngau nhien (dung truoc Tuong)."));
                    ImGui::BulletText("%s", tr("Detonates after 1 zombie phase. Blast radius 2: center -3 HP, ring -2 HP, outer -1 HP.", "No sau 1 pha zombie. Ban kinh 2: tam -3 HP, vong giua -2 HP, ngoai -1 HP."));
                    ImGui::BulletText("%s", tr("Entities at ring/outer are pushed outward; if blocked, +1 impact damage.", "Thuc the o vong giua/ngoai bi day ra; neu bi chan, +1 sat thuong va cham."));
                    ImGui::BulletText("%s", tr("Wind can blow a live grenade 1 tile in wind direction before it detonates.", "Gio co the thoi luu dan dang bay 1 o theo huong gio truoc khi no."));
                    ImGui::BulletText("%s", tr("Thrown into own feet (all walls ahead): explodes immediately under Human.", "Nem vao chan minh (tat ca phia truoc la Tuong): no ngay duoi chan Human."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.1f, 1.0f), "%s", tr("Molotov  [Cost: 1 stamina + 1 molotov]", "Bom Xang  [Chi phi: 1 the luc + 1 chai]"));
                    ImGui::BulletText("%s", tr("Thrown in 8 directions. Travels 1-6 tiles randomly (stops before Walls).", "Nem theo 8 huong. Bay 1-6 o ngau nhien (dung truoc Tuong)."));
                    ImGui::BulletText("%s", tr("Lands on Water: fizzles out — no fire, no heat transfer.", "Roi xuong Nuoc: tat ngay — khong co lua, khong truyen nhiet."));
                    ImGui::BulletText("%s", tr("Lands on Dirt/Forest: creates Fire cell (lasts 2 turns). Melts adjacent Ice (8-dir).", "Roi xuong Dat/Rung: tao o Lua (ton 2 luot). Tan Bang ke canh (8 huong)."));
                    ImGui::BulletText("%s", tr("Lands on Frozen entity: unfreezes it, no fire spread.", "Roi trung thuc the Dong Bang: giai bang, khong lan lua."));
                    ImGui::BulletText("%s", tr("Lands on Ice: melts that cell + all adjacent Ice (8-dir) into Water.", "Roi xuong Bang: tan o do + tat ca Bang ke canh (8 huong) thanh Nuoc."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "%s", tr("Mine  [Cost: 0 stamina, placed on current tile]", "Min  [Chi phi: 0 the luc, dat tai o hien tai]"));
                    ImGui::BulletText("%s", tr("Placed invisibly on Human's current tile. Triggers when any entity steps on it.", "Dat an tren o Human dang dung. Kich hoat khi bat ky thuc the buoc len."));
                    ImGui::BulletText("%s", tr("Explosion radius 1: same damage as grenade (center 3, ring 2, outer 1).", "Ban kinh no 1: sat thuong giong luu dan (tam 3, vong 2, ngoai 1)."));
                }
                ImGui::Spacing();

                // ── TERRAIN ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Terrain Types", "Cac Loai Dia Hinh"))) {
                    ImGui::TextColored(ImVec4(0.75f, 0.6f, 0.4f, 1.0f), "%s", tr("Dirt", "Dat"));
                    ImGui::BulletText("%s", tr("Default terrain. No movement penalty. Molotov/lightning can ignite it.", "Dia hinh mac dinh. Khong phat sinh them chi phi. Molotov/set co the dot chay."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.25f, 0.55f, 1.0f, 1.0f), "%s", tr("Water", "Nuoc"));
                    ImGui::BulletText("%s", tr("Human movement costs 2 stamina. Zombies (except Sprinter) unaffected.", "Di chuyen cua Human ton 2 the luc. Zombie (tru Sprinter) khong bi anh huong."));
                    ImGui::BulletText("%s", tr("Sprinter capped to 1 action/turn in Water.", "Sprinter bi gioi han con 1 hanh dong/luot trong Nuoc."));
                    ImGui::BulletText("%s", tr("Extinguishes Burning status on any entity that enters.", "Dap tat trang thai Chay cua bat ky thuc the buoc vao."));
                    ImGui::BulletText("%s", tr("Zombie attacks in Water: Bite -1 HP (not 2), Scratch 0 HP.", "Zombie tan cong trong Nuoc: Can -1 HP (khong phai 2), Cao 0 HP."));
                    ImGui::BulletText("%s", tr("Molotov landing here fizzles. Explosion radius reduced by 1 here.", "Molotov roi vao day se tat. Ban kinh no giam 1 tai day."));
                    ImGui::BulletText("%s", tr("Loot drops on Water cannot be picked up until the tile changes.", "Loot drop tren Nuoc khong the nhat cho den khi o thay doi."));
                    ImGui::BulletText("%s", tr("Heatwave: isolated water cells may evaporate to Dirt.", "Nang Nong: o nuoc co lap co the boc hoi thanh Dat."));
                    ImGui::BulletText("%s", tr("Blizzard: water clusters may freeze into Ice.", "Bao Tuyet: cum nuoc co the dong bang thanh Bang."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "%s", tr("Wall", "Tuong"));
                    ImGui::BulletText("%s", tr("Impassable for all entities. Blocks projectiles (pistol, shotgun).", "Khong the di qua voi moi thuc the. Chan dan (sung luc, hoa mai)."));
                    ImGui::BulletText("%s", tr("Shotgun recoil into Wall: Wall destroyed, Human -1 HP.", "Giat lui shotgun vao Tuong: Tuong bi pha, Human -1 HP."));
                    ImGui::BulletText("%s", tr("Explosion destroys adjacent Walls.", "Vu no pha Tuong ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.25f, 0.75f, 0.25f, 1.0f), "%s", tr("Forest", "Rung"));
                    ImGui::BulletText("%s", tr("Normal movement. Flammable: adjacent Fire spreads here each fire-spread event.", "Di chuyen binh thuong. De chay: Lua ke canh lan vao day moi su kien lan lua."));
                    ImGui::BulletText("%s", tr("Lightning on Forest with no entity: ignites to Fire. With entity: suppressed.", "Set danh Rung khong co thuc the: bat lua. Co thuc the: bi uc che."));
                    ImGui::BulletText("%s", tr("Heatwave: 15%% chance each Forest cell dies (becomes Dirt).", "Nang Nong: 15%% kha nang moi o Rung chet (thanh Dat)."));
                    ImGui::BulletText("%s", tr("Rain: 12%% chance Forest spreads to adjacent Dirt.", "Mua: 12%% kha nang Rung lan sang Dat ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.1f, 1.0f), "%s", tr("Fire", "Lua"));
                    ImGui::BulletText("%s", tr("Lasts 2 turns, then reverts to Dirt.", "Ton tai 2 luot, sau do chuyen lai thanh Dat."));
                    ImGui::BulletText("%s", tr("Entity entering Fire: gains Burning status.", "Thuc the buoc vao Lua: nhan trang thai Chay."));
                    ImGui::BulletText("%s", tr("Entity already Burning entering Fire: -1 HP immediately.", "Thuc the dang Chay buoc vao Lua: -1 HP ngay lap tuc."));
                    ImGui::BulletText("%s", tr("Spreads to adjacent Forest (4-dir) at: end of Human turn, end of all Zombie turns, end of Environment phase.", "Lan sang Rung ke canh (4 huong) vao: cuoi luot Human, cuoi tat ca luot Zombie, cuoi pha Moi Truong."));
                    ImGui::BulletText("%s", tr("Melts adjacent Ice (4-dir) into Water each spread event.", "Tan Bang ke canh (4 huong) thanh Nuoc moi su kien lan."));
                    ImGui::BulletText("%s", tr("Loot drops on Fire are destroyed immediately.", "Loot drop tren Lua bi pha huy ngay lap tuc."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", tr("Ice", "Bang"));
                    ImGui::BulletText("%s", tr("Normal movement cost. Entity standing on Water when it freezes gets Frozen status.", "Chi phi di chuyen binh thuong. Thuc the dang dung tren Nuoc khi dong bang nhan trang thai Dong Bang."));
                    ImGui::BulletText("%s", tr("Ice slide: if >= 4 consecutive Ice tiles ahead in move direction, 50%% chance to slide.", "Truot bang: neu >= 4 o Bang lien tiep phia truoc theo huong di, 50%% kha nang truot."));
                    ImGui::BulletText("%s", tr("Slide ends at first non-Ice tile or map edge. Collision with Wall/entity: Stunned.", "Truot dung o o khong phai Bang dau tien hoac bien ban do. Va cham voi Tuong/thuc the: bi Choang."));
                    ImGui::BulletText("%s", tr("Adjacent to Fire (4-dir): melts to Water each fire-spread event.", "Ke canh Lua (4 huong): tan thanh Nuoc moi su kien lan lua."));
                    ImGui::BulletText("%s", tr("Loot drops on Ice cannot be picked up until the tile changes.", "Loot drop tren Bang khong the nhat cho den khi o thay doi."));
                    ImGui::BulletText("%s", tr("Explosion, lightning, or molotov heat melts Ice to Water and unfreezes entities.", "Vu no, set, hoac nhiet molotov tan Bang thanh Nuoc va giai bang thuc the."));
                }
                ImGui::Spacing();

                // ── WEATHER ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Weather Events", "Cac Su Kien Thoi Tiet"))) {
                    ImGui::TextWrapped("%s", tr("One weather event occurs each turn (after all zombies act). Probabilities are configurable in Hub.", "Mot su kien thoi tiet xay ra moi luot (sau khi tat ca zombie hanh dong). Xac suat co the chinh trong Hub."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f, 0.95f, 1.0f, 1.0f), "%s", tr("Clear Skies", "Troi Quang"));
                    ImGui::BulletText("%s", tr("Nothing happens. Default ~50%% probability.", "Khong co gi xay ra. Xac suat mac dinh ~50%%."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.65f, 0.85f, 1.0f, 1.0f), "%s", tr("Windstorm", "Bao Gio"));
                    ImGui::BulletText("%s", tr("Blows in a random cardinal/diagonal direction. Pushes all non-frozen entities 1 tile.", "Thoi theo huong ngau nhien. Day tat ca thuc the khong bi Dong Bang 1 o."));
                    ImGui::BulletText("%s", tr("Entity must have open space both behind and ahead to be pushed (no pinning).", "Thuc the phai co khoang trong ca phia sau lan phia truoc moi bi day (khong bi kem)."));
                    ImGui::BulletText("%s", tr("Also blows live grenades and loot drops 1 tile in wind direction.", "Cung thoi luu dan dang bay va loot drop 1 o theo huong gio."));
                    ImGui::BulletText("%s", tr("Frozen entities are immune to wind push.", "Thuc the Dong Bang mien nhiem voi gio."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s", tr("Heavy Rain", "Mua To"));
                    ImGui::BulletText("%s", tr("Extinguishes all Burning entities (Human and Zombies).", "Dap tat trang thai Chay cua tat ca thuc the (Human va Zombie)."));
                    ImGui::BulletText("%s", tr("30%% chance each Dirt adjacent to Water floods to Water.", "30%% kha nang moi o Dat ke canh Nuoc bi ngap thanh Nuoc."));
                    ImGui::BulletText("%s", tr("10%% chance isolated Dirt cells flood to Water.", "10%% kha nang o Dat co lap bi ngap thanh Nuoc."));
                    ImGui::BulletText("%s", tr("12%% chance each Forest spreads to adjacent Dirt.", "12%% kha nang moi o Rung lan sang Dat ke canh."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.7f, 1.0f), "%s", tr("Dark Clouds", "May Den"));
                    ImGui::BulletText("%s", tr("Activates Dark Cloud overlay. Reduces pistol effective range (visual effect).", "Kich hoat lop phu May Den. Giam tam nhin hieu qua cua sung luc (hieu ung hinh anh)."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.35f, 1.0f), "%s", tr("Lightning Strike", "Set Danh"));
                    ImGui::BulletText("%s", tr("Picks a random cell weighted by: Water x4, entity-occupied x2, default x1.", "Chon o ngau nhien theo trong so: Nuoc x4, co thuc the x2, mac dinh x1."));
                    ImGui::BulletText("%s", tr("Direct strike: -1 HP to entity on that cell. Unfreezes Frozen entity on direct hit.", "Danh truc tiep: -1 HP cho thuc the tren o do. Giai bang thuc the Dong Bang bi danh truc tiep."));
                    ImGui::BulletText("%s", tr("Electricity spreads through connected Water/Ice cluster: all occupants Paralyzed.", "Dien lan qua cum Nuoc/Bang lien ket: tat ca thuc the trong cum bi Te Liet."));
                    ImGui::BulletText("%s", tr("Forest struck with no entity: ignites to Fire. With entity: fire suppressed.", "Rung bi set khong co thuc the: bat lua. Co thuc the: lua bi uc che."));
                    ImGui::BulletText("%s", tr("Loot drop on the struck cell is destroyed.", "Loot drop tren o bi set bi pha huy."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", tr("Heatwave", "Nang Nong Gay Gat"));
                    ImGui::BulletText("%s", tr("All Ice melts to Water. Entities on melted Ice are unfrozen.", "Tat ca Bang tan thanh Nuoc. Thuc the tren Bang tan duoc giai bang."));
                    ImGui::BulletText("%s", tr("Isolated Water cells evaporate to Dirt (probability decreases with water neighbors).", "O Nuoc co lap boc hoi thanh Dat (xac suat giam khi co nhieu Nuoc ke canh)."));
                    ImGui::BulletText("%s", tr("15%% chance each Forest cell dies (becomes Dirt) from drought.", "15%% kha nang moi o Rung chet (thanh Dat) vi han han."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", tr("Blizzard", "Bao Tuyet"));
                    ImGui::BulletText("%s", tr("25%% chance each Water cell seeds a freeze. Entire connected Water cluster freezes.", "25%% kha nang moi o Nuoc khoi dong dong bang. Toan bo cum Nuoc lien ket dong bang."));
                    ImGui::BulletText("%s", tr("Entities standing on newly frozen cells gain Frozen status.", "Thuc the dang dung tren o vua dong bang nhan trang thai Dong Bang."));
                }
                ImGui::Spacing();

                // ── STATUS EFFECTS ────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Status Effects", "Trang Thai Hieu Ung"))) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.1f, 1.0f), "%s", tr("[B] Burning", "[B] Chay"));
                    ImGui::BulletText("%s", tr("Gained by: entering Fire tile, proximity to fire source, or molotov hit.", "Nhan duoc khi: buoc vao o Lua, gan nguon lua, hoac bi molotov danh trung."));
                    ImGui::BulletText("%s", tr("Effect: at start of next zombie phase, entity takes -1 HP and Burning is cleared.", "Hieu ung: dau pha zombie tiep theo, thuc the chiu -1 HP va Chay bi xoa."));
                    ImGui::BulletText("%s", tr("Entering Fire while already Burning: -1 HP immediately.", "Buoc vao Lua khi dang Chay: -1 HP ngay lap tuc."));
                    ImGui::BulletText("%s", tr("Cured by: stepping into Water, or Heavy Rain event.", "Chua bang: buoc vao Nuoc, hoac su kien Mua To."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.1f, 1.0f), "%s", tr("[P] Paralyzed", "[P] Te Liet"));
                    ImGui::BulletText("%s", tr("Human: stamina set to 0 for that turn (cannot act).", "Human: the luc ve 0 trong luot do (khong the hanh dong)."));
                    ImGui::BulletText("%s", tr("Zombie: loses its entire action for that turn.", "Zombie: mat toan bo hanh dong trong luot do."));
                    ImGui::BulletText("%s", tr("Caused by: electricity spreading through conductive Water/Ice cluster.", "Nguyen nhan: dien lan qua cum Nuoc/Bang dan dien."));
                    ImGui::BulletText("%s", tr("Only the direct lightning strike cell unfreezes Frozen entities — conductive spread does NOT.", "Chi o bi set danh truc tiep moi giai bang — lan dien KHONG giai bang."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "%s", tr("[F] Frozen", "[F] Dong Bang"));
                    ImGui::BulletText("%s", tr("Gained by: standing on Water when Blizzard freezes it.", "Nhan duoc khi: dang dung tren Nuoc khi Bao Tuyet dong bang no."));
                    ImGui::BulletText("%s", tr("Human: costs 2 stamina to break free (click any tile). If < 2 stamina, turn is skipped.", "Human: ton 2 the luc de tu giai (click bat ky o). Neu < 2 the luc, bo qua luot."));
                    ImGui::BulletText("%s", tr("Zombie: loses action each turn. Normal/Vampire/Sick/Exploding thaw after 2 turns. Sprinter after 1 turn.", "Zombie: mat hanh dong moi luot. Thuong/Hut Mau/Benh/No giai sau 2 luot. Sprinter sau 1 luot."));
                    ImGui::BulletText("%s", tr("Frozen entities cannot be pushed by Wind.", "Thuc the Dong Bang mien nhiem voi Gio."));
                    ImGui::BulletText("%s", tr("Cleared by: ice melting (any cause), explosion blast, lightning direct hit, molotov heat.", "Giai bang: bang tan (bat ky nguyen nhan), vu no, set danh truc tiep, nhiet molotov."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "%s", tr("[S] Stunned", "[S] Choang"));
                    ImGui::BulletText("%s", tr("Caused by: ice slide collision with Wall or entity at end of slide.", "Nguyen nhan: truot bang va cham voi Tuong hoac thuc the cuoi duong truot."));
                    ImGui::BulletText("%s", tr("Effect: entity loses all remaining actions for that turn.", "Hieu ung: thuc the mat tat ca hanh dong con lai trong luot do."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.82f, 0.93f, 0.24f, 1.0f), "%s", tr("[I] Infected (Sick Zombie bite)", "[I] Nhiem Doc (Zombie Benh can)"));
                    ImGui::BulletText("%s", tr("Halves remaining turns immediately (e.g. 10 left -> 5 left).", "Giam nua so luot con lai ngay lap tuc (vd: con 10 -> con 5)."));
                    ImGui::BulletText("%s", tr("Also inflicts -1 stamina penalty at the start of the next Human turn.", "Cung gay phat -1 the luc dau luot Human tiep theo."));
                    ImGui::BulletText("%s", tr("Multiple bites stack: each halves the remaining turns again.", "Nhieu lan can chong chong: moi lan lai giam nua so luot con lai."));
                }
                ImGui::Spacing();

                // ── PHYSICAL MECHANISMS ────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Physical Mechanisms", "Co Che Vat Ly"))) {

                    // Shotgun Recoil
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", tr("Shotgun Recoil", "Giat Lui Sung Hoa Mai"));
                    ImGui::BulletText("%s", tr(
                        "Firing the shotgun pushes the Human 1 tile in the opposite direction of the shot if there is obstacle right before the shooting direction.",
                        "Ban sung hoa mai day lui Human 1 o theo huong nguoc lai voi huong ban neu co vat can ngay truoc huong ban."));
                    ImGui::BulletText("%s", tr(
                        "If the tile behind is free (no wall, no zombie), Human slides back safely.",
                        "Neu o phia sau trong (khong tuong, khong zombie), Human truot lui an toan."));
                    //ImGui::BulletText("%s", tr(
                    //    "If blocked by a wall: the wall is destroyed AND Human loses 1 HP (impact damage).",
                    //    "Neu bi chan boi tuong: tuong bi pha huy VA Human mat 1 HP (sat thuong va dap)."));
                    ImGui::BulletText("%s", tr(
                        "If blocked by a zombie: the zombie takes 1 impact damage, Human stays in place.",
                        "Neu bi chan boi zombie: zombie nhan 1 sat thuong va dap, Human dung yen."));
                    ImGui::Spacing();

                    // Explosion Push
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", tr("Explosion Knockback", "Day Lui Do Vu No"));
                    ImGui::BulletText("%s", tr(
                        "Entities within 1 tile of an explosion center are pushed 1 tile away from the blast.",
                        "Thuc the trong vong 1 o tu tam vu no bi day lui 1 o ra xa tam no."));
                    ImGui::BulletText("%s", tr(
                        "If the push destination is blocked (wall or entity), the pushed entity takes +1 bonus impact damage.",
                        "Neu dich den bi chan (tuong hoac thuc the khac), thuc the bi day nhan them +1 sat thuong va dap."));
                    ImGui::BulletText("%s", tr(
                        "The blocking entity also takes 1 impact damage from the collision.",
                        "Thuc the dang chan cung nhan 1 sat thuong va dap tu su va cham."));
                    ImGui::BulletText("%s", tr(
                        "Walls adjacent to the blast center are destroyed by the explosion.",
                        "Tuong ke sat tam vu no bi pha huy boi vu no."));
                    ImGui::Spacing();

                    // Zombie-Zombie Collision
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", tr("Entity Collision", "Va Cham Thuc The"));
                    ImGui::BulletText("%s", tr(
                        "When a zombie is pushed into another zombie (or the Human), both take 1 collision damage.",
                        "Khi mot zombie bi day vao zombie khac (hoac Human), ca hai nhan 1 sat thuong va cham."));
                    ImGui::BulletText("%s", tr(
                        "Collision damage is applied after the push resolves — an Exploding Zombie can chain-detonate.",
                        "Sat thuong va cham duoc ap dung sau khi day lui hoan tat — Zombie No co the kich no day chuyen."));
                    ImGui::Spacing();

                    // Line-of-Sight Blocking
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", tr("Cover & Line-of-Sight", "Che Chan & Tam Nhin"));
                    ImGui::BulletText("%s", tr(
                        "Walls block all ranged attacks (pistol, shotgun cone, grenade throw path).",
                        "Tuong chan tat ca tan cong tam xa (sung luc, vung shotgun, duong nem luu dan)."));
                    //ImGui::BulletText("%s", tr(
                    //    "Forest tiles block line-of-sight: pistol shots cannot pass through Forest.",
                    //    "O Rung chan tam nhin: dan sung luc khong the xuyen qua Rung."));
                    ImGui::BulletText("%s", tr(
                        "Shotgun fires in a straight 3-tile line; the first wall hit stops the blast and is destroyed.",
                        "Sung hoa mai ban thang 3 o; tuong dau tien bi trung se dung lai va bi pha huy."));
                    ImGui::BulletText("%s", tr(
                        "Grenades are thrown in a straight line and can be blocked mid-air by walls.",
                        "Luu dan duoc nem theo duong thang va co the bi chan giua chung boi tuong."));
                    ImGui::Spacing();

                    // Ice Slide
                    ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "%s", tr("Ice Slide", "Truot Bang"));
                    ImGui::BulletText("%s", tr(
                        "Trigger: entering an Ice tile when 4+ consecutive Ice cells lie ahead in the movement direction.",
                        "Kich hoat: buoc vao o Bang khi co 4+ o Bang lien tiep phia truoc theo huong di chuyen."));
                    ImGui::BulletText("%s", tr(
                        "Base slide chance: 50%%. Each additional step has a 25%% lower probability (exponential decay).",
                        "Xac suat truot co ban: 50%%. Moi buoc them co xac suat thap hon 25%% (giam mu)."));
                    ImGui::BulletText("%s", tr(
                        "The entity slides until: the ice ends, a wall/entity blocks the path, or the probability roll fails.",
                        "Thuc the truot cho den khi: het bang, tuong/thuc the chan duong, hoac xac suat that bai."));
                    ImGui::BulletText("%s", tr(
                        "Slamming into an obstacle at the end of a slide inflicts STUN: the entity loses all remaining actions that turn.",
                        "Truot vao vat can o cuoi duong truot gay CHOANG: thuc the mat toan bo hanh dong con lai trong luot do."));
                    ImGui::BulletText("%s", tr(
                        "Applies to both Human and Zombies. Fire/Mine interactions are checked at the final slide position.",
                        "Ap dung cho ca Human va Zombie. Tuong tac Lua/Min duoc kiem tra tai vi tri truot cuoi cung."));
                    ImGui::Spacing();

                    // Electricity Conduction
                    ImGui::TextColored(ImVec4(0.45f, 0.9f, 1.0f, 1.0f), "%s", tr("Electricity Conduction", "Lan Truyen Dien"));
                    ImGui::BulletText("%s", tr(
                        "Lightning strikes a weighted-random cell (Water/Ice cells and cells with entities are 2-4x more likely).",
                        "Set danh vao o ngau nhien co trong so (o Nuoc/Bang va o co thuc the co xac suat cao hon 2-4 lan)."));
                    ImGui::BulletText("%s", tr(
                        "Conductive cells: Water, Ice, and any cell occupied by a living entity.",
                        "O dan dien: Nuoc, Bang, va bat ky o nao co thuc the dang song."));
                    ImGui::BulletText("%s", tr(
                        "Electricity spreads via BFS through all connected conductive cells (4-directional adjacency).",
                        "Dien lan truyen qua BFS qua tat ca o dan dien lien ket (ke canh 4 huong)."));
                    ImGui::BulletText("%s", tr(
                        "All entities in the conductive cluster are PARALYZED for their next action (cannot act for 1 turn).",
                        "Tat ca thuc the trong cum dan dien bi TE LIET cho hanh dong tiep theo (khong the hanh dong 1 luot)."));
                    ImGui::BulletText("%s", tr(
                        "Direct strike cell: entity takes 1 HP damage AND is unfrozen if frozen. Spread cells: paralysis only.",
                        "O bi set danh truc tiep: thuc the mat 1 HP VA duoc giai bang neu dang bi dong. O lan truyen: chi te liet."));
                    ImGui::Spacing();

                    // Fire Spread
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.1f, 1.0f), "%s", tr("Fire Spread", "Lan Lua"));
                    ImGui::BulletText("%s", tr(
                        "Fire spreads to adjacent Forest tiles (4-directional) at the end of each phase: after Human turn, after Zombie phase, and after Environment phase.",
                        "Lua lan sang o Rung ke canh (4 huong) vao cuoi moi pha: sau luot Human, sau pha Zombie, va sau pha Moi Truong."));
                    ImGui::BulletText("%s", tr(
                        "Fire does NOT spread to Dirt, Water, Wall, or Ice directly — only Forest ignites.",
                        "Lua KHONG lan sang Dat, Nuoc, Tuong, hoac Bang truc tiep — chi Rung moi bat lua."));
                    ImGui::BulletText("%s", tr(
                        "Each Fire cell has a duration counter (default 2 turns). When it expires, the cell becomes Dirt.",
                        "Moi o Lua co bo dem thoi gian (mac dinh 2 luot). Khi het, o do chuyen thanh Dat."));
                    ImGui::BulletText("%s", tr(
                        "Entities entering or standing on a Fire cell take 1 HP damage per interaction check.",
                        "Thuc the buoc vao hoac dung tren o Lua nhan 1 sat thuong moi lan kiem tra tuong tac."));
                    ImGui::BulletText("%s", tr(
                        "Human standing in water while burning is extinguished automatically.",
                        "Human dang chay khi dung trong nuoc se tu dong tat lua."));
                    ImGui::Spacing();

                    // Heat Transfer (Ice Melting)
                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.15f, 1.0f), "%s", tr("Heat Transfer (Ice Melting)", "Truyen Nhiet (Tan Bang)"));
                    ImGui::BulletText("%s", tr(
                        "Any heat source (Fire cell, Molotov landing, Heatwave event) melts all 8 adjacent Ice tiles immediately.",
                        "Bat ky nguon nhiet nao (o Lua, Molotov roi xuong, su kien Nang Nong) lam tan tat ca 8 o Bang ke canh ngay lap tuc."));
                    ImGui::BulletText("%s", tr(
                        "Melted Ice becomes Water. Entities frozen on that cell are automatically unfrozen.",
                        "Bang tan chuyen thanh Nuoc. Thuc the bi dong tren o do duoc tu dong giai bang."));
                    ImGui::BulletText("%s", tr(
                        "Fire spread also melts adjacent Ice (4-directional) during the propagation step.",
                        "Lan lua cung lam tan Bang ke canh (4 huong) trong buoc lan truyen."));
                    ImGui::BulletText("%s", tr(
                        "Heatwave: all isolated Water cells (no adjacent Water neighbor) evaporate to Dirt (100%% chance). Forest cells have a 15%% drought chance.",
                        "Nang Nong: tat ca o Nuoc bi co lap (khong co o Nuoc ke canh) boc hoi thanh Dat (100%%). O Rung co 15%% kha nang han han."));
                    ImGui::Spacing();

                    // Blizzard Freezing
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", tr("Blizzard & Freezing", "Bao Tuyet & Dong Cung"));
                    ImGui::BulletText("%s", tr(
                        "Blizzard: each Water cell has a 25%% chance to freeze. Freezing is flood-filled — the entire connected Water body freezes together.",
                        "Bao Tuyet: moi o Nuoc co 25%% kha nang dong bang. Dong bang lan theo flood-fill — toan bo vung Nuoc lien ket dong cung nhau."));
                    ImGui::BulletText("%s", tr(
                        "Entities standing on a cell that freezes become FROZEN: they cannot move until they spend 2 stamina to break free.",
                        "Thuc the dang dung tren o bi dong bang tro nen BI DONG CUNG: khong the di chuyen cho den khi tieu 2 the luc de tu giai."));
                    ImGui::BulletText("%s", tr(
                        "Frozen Zombies skip their movement each turn (frozen_turns counts down). They are still damaged normally.",
                        "Zombie bi dong cung bo qua di chuyen moi luot (frozen_turns dem nguoc). Chung van nhan sat thuong binh thuong."));
                    ImGui::BulletText("%s", tr(
                        "Ice cells are conductive — a frozen entity on Ice can be paralyzed by a subsequent lightning strike.",
                        "O Bang dan dien — thuc the bi dong cung tren Bang co the bi te liet boi tia set tiep theo."));
                }
                ImGui::Spacing();

                // ── LOOT DROPS ────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Loot Drops", "Loot Drop"))) {
                    ImGui::TextWrapped("%s", tr("Every zombie leaves a '?' loot drop on death. Walk onto it to collect.", "Moi zombie de lai loot drop '?' khi chet. Di chuyen vao o do de nhat."));
                    ImGui::BulletText("%s", tr("75%% Junk (useless). 25%% useful items split by rarity.", "75%% Rac (vo dung). 25%% do huu ich phan chia theo do hiem."));
                    ImGui::BulletText("%s", tr("Items: Health Potion +2 HP, Stamina Potion (restore to 6), Pistol Ammo +3, Shotgun Shell +1, Grenade +1, Molotov +1, Mine +1.", "Do: Binh Mau +2 HP, Binh The Luc (phuc hoi ve 6), Dan Luc +3, Dan Hoa Mai +1, Luu Dan +1, Bom Xang +1, Min +1."));
                    ImGui::BulletText("%s", tr("Cannot pick up on Water or Ice tiles. Pickup resumes when tile changes to Dirt/Forest.", "Khong the nhat tren o Nuoc hoac Bang. Co the nhat khi o chuyen thanh Dat/Rung."));
                    ImGui::BulletText("%s", tr("Destroyed by: explosion blast, shotgun cone, lightning direct hit, tile becoming Fire, or Wind blowing it into Fire.", "Bi pha huy boi: vu no, vung shotgun, set danh truc tiep, o chuyen thanh Lua, hoac Gio thoi vao Lua."));
                    ImGui::BulletText("%s", tr("Wind blows loot drops 1 tile (same as grenades). Frozen loot is NOT immune to wind.", "Gio thoi loot drop 1 o (giong luu dan). Loot KHONG mien nhiem voi gio."));
                }
                ImGui::Spacing();

                // ── SETUP & SHARING ───────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Game Setup & .zom Files", "Thiet Lap & File .zom"))) {
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Quick Play", "Choi Nhanh"));
                    ImGui::BulletText("%s", tr("4 presets: Easy / Medium / Hard / Unfair. Each sets HP, stamina, ammo, zombie counts, terrain ratios, and weather probabilities.", "4 che do: De / Trung Binh / Kho / Bat Cong. Moi che do thiet lap HP, the luc, dan, so luong zombie, ti le dia hinh va xac suat thoi tiet."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr("Custom Setup", "Thiet Lap Tuy Chinh"));
                    ImGui::BulletText("%s", tr("Adjust map size, all terrain ratios, weather probabilities, zombie counts, and Human stats.", "Chinh kich thuoc ban do (toi da 25x25), ti le dia hinh, xac suat thoi tiet, so luong zombie va chi so Human."));
                    ImGui::BulletText("%s", tr("Map Editor: paint terrain tile-by-tile and place Human spawn manually.", "Trinh chinh sua ban do: to mau dia hinh tung o va dat vi tri xuat hien Human thu cong."));
                    ImGui::BulletText("%s", tr("Spawn Shield: protects a 5x5 area around Human spawn from zombie spawns.", "Khien Xuat Hien: bao ve vung 5x5 quanh vi tri xuat hien Human khoi zombie."));
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", tr(".zom Challenge Files", "File Thach Thuc .zom"));
                    ImGui::BulletText("%s", tr("Export: saves all current settings (map, stats, weather, terrain) to a .zom text file.", "Xuat: luu tat ca cai dat hien tai (ban do, chi so, thoi tiet, dia hinh) vao file .zom."));
                    ImGui::BulletText("%s", tr("Import: loads a .zom file to recreate the exact same challenge.", "Nhap: tai file .zom de tai tao chinh xac thach thuc do."));
                    ImGui::BulletText("%s", tr("Share .zom files with friends to compete on identical maps.", "Chia se file .zom voi ban be de thi dau tren ban do giong het nhau."));
                }
                ImGui::Spacing();

                // ── CREDITS ───────────────────────────────────────────────────────────
                if (ImGui::CollapsingHeader(tr("Credits", "Tin Chi"))) {
                    ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "ZomChess v1.0");
                    ImGui::BulletText("%s", tr("Design & Programming: Phan Anh Luan + AIs", "Thiet ke & Lap trinh: Phan Anh Luan + AI"));
                    ImGui::BulletText("%s", tr("Music: 'Ancient Rite', 'Discovery Hit', 'Impending Boom', 'The Ice Giants' — licensed for use.", "Nhac nen: 'Ancient Rite', 'Discovery Hit', 'Impending Boom', 'The Ice Giants' — duoc cap phep su dung."));
                    ImGui::BulletText("%s", tr("Built with: C++, SFML, Dear ImGui, ImGui-SFML.", "Xay dung bang: C++, SFML, Dear ImGui, ImGui-SFML."));
                    ImGui::BulletText("%s", tr("Sound effects synthesized procedurally in-engine.", "Hieu ung am thanh duoc tong hop thu tuc trong engine."));
                }

                ImGui::EndChild();
                ImGui::Separator();
                if (ImGui::Button(tr("Close", "Dong"), ImVec2(160, 34))) {
                    show_guide_popup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (state.game_over || state.game_won) {
                ImGui::OpenPopup("End Game");
                // Chuyển nhạc ngay khi kết quả xác định (chỉ chuyển 1 lần nhờ logic trong playMusic)
                if (state.game_won)  state.playBackgroundMusic("victory");
                else                 state.playBackgroundMusic("defeat");

                if (ImGui::BeginPopupModal("End Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text(state.game_over ? "OPERATION FAILED! YOU DIED." : "MISSION ACCOMPLISHED! DISPATCH SECTOR CLEAN.");
                    if (ImGui::Button(tr("Return to HUB", "Tro Ve Tru So"), ImVec2(180, 30))) {
                        ImGui::CloseCurrentPopup();
                        state.stopBackgroundMusic();
                        state.current_scene = GameScene::MainMenu;
                        state.playBackgroundMusic("menu");
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::End();
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
