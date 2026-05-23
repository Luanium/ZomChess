#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "GameState.h"
#include "UI.h"
#include "AudioManager.h"
#include <cmath>

int main() {
    enum class Lang { EN, VI };
    static Lang ui_lang = Lang::EN;
    auto tr = [&](const char* en, const char* vi) { return (ui_lang == Lang::VI) ? vi : en; };
    sf::RenderWindow window(sf::VideoMode(1400, 658), "ZomChess Tactical Engine v2.1", sf::Style::Titlebar | sf::Style::Close);
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
                                            state.human.pos = {tx, ty};
                                            state.human.stamina -= cost;
                                            state.check_fire_interactions();
                                            state.check_mine_interactions();
                                        } else {
                                            if (state.human.stamina == 0) {
                                                state.add_log(tr("[SYSTEM] Out of stamina! End turn!", "[HE THONG] Het stamina! Ket thuc luot!"), ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                                                state.turn_banner_fx.type = FXType::Electricity;
                                                state.turn_banner_fx.timer = 1.0f;
                                                state.turn_banner_fx.max_duration = 1.0f;
                                                state.turn_banner_fx.banner_text = (ui_lang == Lang::VI) ? "HET STAMINA! KET THUC LUOT!" : "OUT OF STAMINA! END TURN!";
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

            int total_r = temp_dirt + temp_wall + temp_water + temp_forest;
            if (total_r != 100) {
                state.active_config.ratio_dirt = 60;
                state.active_config.ratio_wall = 10;
                state.active_config.ratio_water = 10;
                state.active_config.ratio_forest = 20;
                temp_dirt = 60; temp_wall = 10; temp_water = 10; temp_forest = 20;
            }

            int p1 = temp_dirt;
            int p2 = p1 + temp_wall;
            int p3 = p2 + temp_water;

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
                    if (d1 <= d2 && d1 <= d3) active_knob = 0;
                    else if (d2 <= d1 && d2 <= d3) active_knob = 1;
                    else active_knob = 2;
                }

                if (active_knob == 0) {
                    p1 = std::round(clicked_pct);
                    p1 = std::max(0, std::min(p2, p1));
                } else if (active_knob == 1) {
                    p2 = std::round(clicked_pct);
                    p2 = std::max(p1, std::min(p3, p2));
                } else if (active_knob == 2) {
                    p3 = std::round(clicked_pct);
                    p3 = std::max(p2, std::min(100, p3));
                }
            } else {
                active_knob = -1;
            }

            state.active_config.ratio_dirt = p1;
            state.active_config.ratio_wall = p2 - p1;
            state.active_config.ratio_water = p3 - p2;
            state.active_config.ratio_forest = 100 - p3;

            temp_dirt = state.active_config.ratio_dirt;
            temp_wall = state.active_config.ratio_wall;
            temp_water = state.active_config.ratio_water;
            temp_forest = state.active_config.ratio_forest;

            float x0 = cursor_pos.x;
            float x1 = x0 + p1 * (bar_width / 100.f);
            float x2 = x0 + p2 * (bar_width / 100.f);
            float x3 = x0 + p3 * (bar_width / 100.f);
            float x4 = x0 + bar_width;

            float y_top = cursor_pos.y + 4.0f;
            float y_bot = y_top + bar_height;

            ImU32 col_dirt = ImGui::ColorConvertFloat4ToU32(ImVec4(105/255.f, 60/255.f, 35/255.f, 1.f));
            ImU32 col_wall = ImGui::ColorConvertFloat4ToU32(ImVec4(60/255.f, 62/255.f, 66/255.f, 1.f));
            ImU32 col_water = ImGui::ColorConvertFloat4ToU32(ImVec4(35/255.f, 75/255.f, 115/255.f, 1.f));
            ImU32 col_forest = ImGui::ColorConvertFloat4ToU32(ImVec4(34/255.f, 110/255.f, 48/255.f, 1.f));

            draw_list->AddRectFilled(ImVec2(x0, y_top), ImVec2(x1, y_bot), col_dirt);
            draw_list->AddRectFilled(ImVec2(x1, y_top), ImVec2(x2, y_bot), col_wall);
            draw_list->AddRectFilled(ImVec2(x2, y_top), ImVec2(x3, y_bot), col_water);
            draw_list->AddRectFilled(ImVec2(x3, y_top), ImVec2(x4, y_bot), col_forest);

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

            draw_list->AddRect(ImVec2(x0, y_top), ImVec2(x4, y_bot), ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.6f)), 0.0f, 0, 1.5f);

            ImGui::Spacing();

            draw_legend_item(tr("Dirt", "Dat"), ImVec4(105/255.f, 60/255.f, 35/255.f, 1.f), temp_dirt); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Wall", "Tuong"), ImVec4(60/255.f, 62/255.f, 66/255.f, 1.f), temp_wall); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Water", "Nuoc"), ImVec4(35/255.f, 75/255.f, 115/255.f, 1.f), temp_water); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
            draw_legend_item(tr("Forest", "Rung"), ImVec4(34/255.f, 110/255.f, 48/255.f, 1.f), temp_forest);

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

                int total_env = temp_clear + temp_wind + temp_rain + temp_clouds + temp_light;
                if (total_env != 100) {
                    state.active_config.env_prob_clear = 58;
                    state.active_config.env_prob_wind = 16;
                    state.active_config.env_prob_rain = 14;
                    state.active_config.env_prob_clouds = 4;
                    state.active_config.env_prob_lightning = 8;
                    temp_clear = 58; temp_wind = 16; temp_rain = 14; temp_clouds = 4; temp_light = 8;
                }

                int ep1 = temp_clear;
                int ep2 = ep1 + temp_wind;
                int ep3 = ep2 + temp_rain;
                int ep4 = ep3 + temp_clouds;

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
                        if (ed1 <= ed2 && ed1 <= ed3 && ed1 <= ed4) eactive_knob = 0;
                        else if (ed2 <= ed1 && ed2 <= ed3 && ed2 <= ed4) eactive_knob = 1;
                        else if (ed3 <= ed1 && ed3 <= ed2 && ed3 <= ed4) eactive_knob = 2;
                        else eactive_knob = 3;
                    }

                    if (eactive_knob == 0) {
                        ep1 = std::round(eclicked_pct);
                        ep1 = std::max(0, std::min(ep2, ep1));
                    } else if (eactive_knob == 1) {
                        ep2 = std::round(eclicked_pct);
                        ep2 = std::max(ep1, std::min(ep3, ep2));
                    } else if (eactive_knob == 2) {
                        ep3 = std::round(eclicked_pct);
                        ep3 = std::max(ep2, std::min(ep4, ep3));
                    } else if (eactive_knob == 3) {
                        ep4 = std::round(eclicked_pct);
                        ep4 = std::max(ep3, std::min(100, ep4));
                    }
                } else {
                    eactive_knob = -1;
                }

                state.active_config.env_prob_clear = ep1;
                state.active_config.env_prob_wind = ep2 - ep1;
                state.active_config.env_prob_rain = ep3 - ep2;
                state.active_config.env_prob_clouds = ep4 - ep3;
                state.active_config.env_prob_lightning = 100 - ep4;

                temp_clear = state.active_config.env_prob_clear;
                temp_wind = state.active_config.env_prob_wind;
                temp_rain = state.active_config.env_prob_rain;
                temp_clouds = state.active_config.env_prob_clouds;
                temp_light = state.active_config.env_prob_lightning;

                float ex0 = ecursor_pos.x;
                float ex1 = ex0 + ep1 * (ebar_width / 100.f);
                float ex2 = ex0 + ep2 * (ebar_width / 100.f);
                float ex3 = ex0 + ep3 * (ebar_width / 100.f);
                float ex4 = ex0 + ep4 * (ebar_width / 100.f);
                float ex5 = ex0 + ebar_width;

                float ey_top = ecursor_pos.y + 4.0f;
                float ey_bot = ey_top + ebar_height;

                ImU32 col_clear = ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.7f, 0.9f, 1.0f));
                ImU32 col_wind = ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.85f, 0.95f, 1.0f));
                ImU32 col_rain = ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.55f, 0.85f, 1.0f));
                ImU32 col_clouds = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
                ImU32 col_light = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.2f, 1.0f));

                edraw_list->AddRectFilled(ImVec2(ex0, ey_top), ImVec2(ex1, ey_bot), col_clear);
                edraw_list->AddRectFilled(ImVec2(ex1, ey_top), ImVec2(ex2, ey_bot), col_wind);
                edraw_list->AddRectFilled(ImVec2(ex2, ey_top), ImVec2(ex3, ey_bot), col_rain);
                edraw_list->AddRectFilled(ImVec2(ex3, ey_top), ImVec2(ex4, ey_bot), col_clouds);
                edraw_list->AddRectFilled(ImVec2(ex4, ey_top), ImVec2(ex5, ey_bot), col_light);

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

                edraw_list->AddRect(ImVec2(ex0, ey_top), ImVec2(ex5, ey_bot), ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.6f)), 0.0f, 0, 1.5f);

                ImGui::Spacing();

                // Draw legend items in two rows if needed to avoid overflow
                draw_legend_item(tr("Clear", "Troi quang"), ImVec4(0.5f, 0.7f, 0.9f, 1.0f), temp_clear); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Wind", "Gio lon"), ImVec4(0.7f, 0.85f, 0.95f, 1.0f), temp_wind); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Rain", "Mua to"), ImVec4(0.35f, 0.55f, 0.85f, 1.0f), temp_rain); 
                
                ImGui::Spacing();
                
                draw_legend_item(tr("Clouds", "May mu"), ImVec4(0.3f, 0.3f, 0.35f, 1.0f), temp_clouds); ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 1.0f)); ImGui::SameLine();
                draw_legend_item(tr("Lightning", "Sam set"), ImVec4(1.0f, 0.85f, 0.2f, 1.0f), temp_light);
                
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
            ImGui::SetNextWindowSize(ImVec2(400, 618));
            ImGui::Begin("Map Blueprint Architect", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "Brush Selection Tools:");
            if (ImGui::RadioButton("Plain Dirt Floor", state.editor_selected_terrain == Terrain::Dirt)) state.editor_selected_terrain = Terrain::Dirt;
            if (ImGui::RadioButton("Reinforced Wall", state.editor_selected_terrain == Terrain::Wall)) state.editor_selected_terrain = Terrain::Wall;
            if (ImGui::RadioButton("Deep Water Hazard", state.editor_selected_terrain == Terrain::Water)) state.editor_selected_terrain = Terrain::Water;
            if (ImGui::RadioButton(state.tr("Dense Forest", "Rung Ram Cung").c_str(), state.editor_selected_terrain == Terrain::Forest)) state.editor_selected_terrain = Terrain::Forest;

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
                        bool is_transitioning = false;
                        if (state.active_fx.type == FXType::Rain) {
                            for (auto p : state.active_fx.blast_cells) {
                                if (p.x == x && p.y == y) {
                                    is_transitioning = true;
                                    break;
                                }
                            }
                        }
                        if (is_transitioning) {
                            float progress = 1.0f - (state.active_fx.timer / state.active_fx.max_duration);
                            sf::Color dirtColor(105, 60, 35);
                            sf::Color waterColor(35, 75, 115);
                            sf::Color lerpColor(
                                static_cast<sf::Uint8>(dirtColor.r + progress * (waterColor.r - dirtColor.r)),
                                static_cast<sf::Uint8>(dirtColor.g + progress * (waterColor.g - dirtColor.g)),
                                static_cast<sf::Uint8>(dirtColor.b + progress * (waterColor.b - dirtColor.b))
                            );
                            cell.setFillColor(lerpColor);
                        } else {
                            cell.setFillColor(sf::Color(35, 75, 115));
                        }
                    }
                    else if (state.grid[x][y] == Terrain::Fire) {
                        int pulse = static_cast<int>(25.0f * std::sin(timeSec * 12.0f));
                        cell.setFillColor(sf::Color(220 + pulse, 100 + pulse / 2, 20));
                    }
                    else if (state.grid[x][y] == Terrain::Forest) {
                        cell.setFillColor(sf::Color(34, 110, 48));
                    }
                    else {
                        bool was_extinguished = false;
                        if (state.active_fx.type == FXType::Rain) {
                            for (auto p : state.active_fx.extinguished_cells) {
                                if (p.x == x && p.y == y) {
                                    was_extinguished = true;
                                    break;
                                }
                            }
                        }
                        if (was_extinguished) {
                            float progress = 1.0f - (state.active_fx.timer / state.active_fx.max_duration);
                            int pulse = static_cast<int>(25.0f * std::sin(timeSec * 12.0f));
                            sf::Color fireColor(220 + pulse, 100 + pulse / 2, 20);
                            sf::Color dirtColor(105, 60, 35);
                            sf::Color lerpColor(
                                static_cast<sf::Uint8>(fireColor.r + progress * (dirtColor.r - fireColor.r)),
                                static_cast<sf::Uint8>(fireColor.g + progress * (dirtColor.g - fireColor.g)),
                                static_cast<sf::Uint8>(fireColor.b + progress * (dirtColor.b - fireColor.b))
                            );
                            cell.setFillColor(lerpColor);
                        } else {
                            cell.setFillColor(sf::Color(105, 60, 35));
                        }
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

            for (size_t i = 0; i < state.zombies.size(); ++i) {
                const auto& z = state.zombies[i];
                int zlx = z->pos.x - viewX + padX;
                int zly = z->pos.y - viewY + padY;
                if (zlx < 0 || zlx >= VIEW_CELLS || zly < 0 || zly >= VIEW_CELLS) continue;
                if (z->hp <= 0) {
                    sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f)); deadZ.setFillColor(sf::Color(15, 15, 15, 160));
                    deadZ.setPosition(zlx * cellSize + boardOffset + 4.0f, zly * cellSize + boardOffset + 4.0f); window.draw(deadZ); continue;
                }
                
                float drawX = zlx * cellSize + boardOffset + 3.0f;
                float drawY = zly * cellSize + boardOffset + 3.0f;
                
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
                    if (blinkOn && (z->is_burning || z->is_paralyzed)) {
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
                    }
                }
            }

            int hlx = state.human.pos.x - viewX + padX;
            int hly = state.human.pos.y - viewY + padY;
            
            float drawX = hlx * cellSize + boardOffset + 3.0f;
            float drawY = hly * cellSize + boardOffset + 3.0f;
            
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
                if (blinkOn && (state.human.is_burning || state.human.is_paralyzed)) {
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
            if (state.dark_cloud_active) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1), "DARK CLOUD"); }
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
                ImGui::TextColored(ImVec4(0.95f, 0.9f, 0.35f, 1.0f), "%s", tr("Quick Reference", "Tra Cuu Nhanh"));
                if (ImGui::CollapsingHeader(tr("Basic Controls", "Dieu Khien Co Ban"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::BulletText("%s", tr("Left click an adjacent tile to move.", "Click trai o ke ben de di chuyen."));
                    ImGui::BulletText("%s", tr("Select a weapon, then click target direction/cell.", "Chon vu khi, sau do click huong/o muc tieu."));
                    ImGui::BulletText("%s", tr("Press END TURN to pass action to zombies.", "Nhan KET THUC LUOT de chuyen sang zombie."));
                }
                if (ImGui::CollapsingHeader(tr("Core Mechanics", "Co Che Chinh"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::BulletText("%s", tr("Human stamina is 1..6 each turn (0 when paralyzed).", "The luc Human moi luot la 1..6 (0 neu bi te liet)."));
                    ImGui::BulletText("%s", tr("Game over when HP <= 0 or turn limit exceeded.", "Thua khi HP <= 0 hoac qua gioi han luot."));
                    ImGui::BulletText("%s", tr("Exploders chain-detonate when killed by blast.", "Zombie no se kich no day chuyen khi chet vi no."));
                }
                if (ImGui::CollapsingHeader(tr("Terrain", "Dia Hinh"))) {
                    ImGui::BulletText("%s", tr("Dirt: normal movement.", "Dat: di chuyen binh thuong."));
                    ImGui::BulletText("%s", tr("Water: human move costs more stamina.", "Nuoc: Human ton nhieu the luc hon."));
                    ImGui::BulletText("%s", tr("Wall: blocks movement and projectiles.", "Tuong: chan di chuyen va dan."));
                    ImGui::BulletText("%s", tr("Fire: causes burn status and burn damage.", "Lua: gay trang thai chay va sat thuong dot."));
                }
                if (ImGui::CollapsingHeader(tr("Zombie Types", "Cac Loai Zombie"))) {
                    ImGui::BulletText("%s", tr("Normal: standard movement and attack.", "Thuong: di chuyen va tan cong co ban."));
                    ImGui::BulletText("%s", tr("Fast: takes 2 actions per turn.", "Nhanh: co 2 hanh dong moi luot."));
                    ImGui::BulletText("%s", tr("Exploding: detonates on death.", "No: phat no khi chet."));
                    ImGui::BulletText("%s", tr("Vampire: heals after attacking.", "Hut mau: hoi phuc sau khi tan cong."));
                    ImGui::BulletText("%s", tr("Sick: halves remaining turns after hit.", "Benh tat: giam nua so luot con lai sau khi trung don."));
                }
                if (ImGui::CollapsingHeader(tr("Effects & Status", "Hieu Ung & Trang Thai"))) {
                    ImGui::BulletText("%s", tr("B = Burned (red), takes damage over time.", "B = Chay (do), mat mau theo thoi gian."));
                    ImGui::BulletText("%s", tr("P = Paralyzed (yellow), loses action/turn stamina.", "P = Te liet (vang), mat hanh dong/the luc luot."));
                    ImGui::BulletText("%s", tr("HP bars update per damage/heal in real-time.", "Thanh HP cap nhat theo sat thuong/hoi phuc theo thoi gian thuc."));
                }
                if (ImGui::CollapsingHeader(tr("Environment", "Moi Truong"))) {
                    ImGui::BulletText("%s", tr("Can be toggled ON/OFF in Hub.", "Co the bat/tat trong Hub."));
                    ImGui::BulletText("%s", tr("Possible events: Wind, Rain, Dark Cloud, Lightning, or Clear Sky.", "Su kien co the xay ra: Gio, Mua, May den, Set, hoac Troi quang."));
                }
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
