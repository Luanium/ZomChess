#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "GameState.h"
#include "UI.h"
#include <cmath>

int main() {
    enum class Lang { EN, VI };
    static Lang ui_lang = Lang::EN;
    auto tr = [&](const char* en, const char* vi) { return (ui_lang == Lang::VI) ? vi : en; };
    sf::RenderWindow window(sf::VideoMode(1400, 750), "ZomChess Tactical Engine v2.1", sf::Style::Titlebar | sf::Style::Close);
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
                if (state.turn_banner_fx.timer <= 0.0f) state.turn_banner_fx.type = FXType::None;
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
            ImGui::SetNextWindowPos(ImVec2(80, 40));
            ImGui::SetNextWindowSize(ImVec2(1240, 670));
            ImGui::Begin("ZomChess Tactical System Hub", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.9f, 1), "%s", tr("QUICK PLAY", "CHOI NHANH"));
            ImGui::SameLine();
            ImGui::Text("%s", tr("Language:", "Ngon ngu:"));
            ImGui::SameLine();
            if (ImGui::RadioButton("EN", ui_lang == Lang::EN)) ui_lang = Lang::EN;
            ImGui::SameLine();
            if (ImGui::RadioButton("VI", ui_lang == Lang::VI)) ui_lang = Lang::VI;
            ImGui::SameLine();
            if (ImGui::Button(tr("EASY", "DE"), ImVec2(150, 30))) { state.apply_quick_difficulty(0); state.init_game(); state.current_scene = GameScene::Playing; view_initialized = false; }
            ImGui::SameLine();
            if (ImGui::Button(tr("MEDIUM", "TRUNG BINH"), ImVec2(150, 30))) { state.apply_quick_difficulty(1); state.init_game(); state.current_scene = GameScene::Playing; view_initialized = false; }
            ImGui::SameLine();
            if (ImGui::Button(tr("HARD", "KHO"), ImVec2(150, 30))) { state.apply_quick_difficulty(2); state.init_game(); state.current_scene = GameScene::Playing; view_initialized = false; }

            ImGui::Separator(); ImGui::Spacing();
            ImGui::Columns(2, "menu_split", false); ImGui::SetColumnWidth(0, 600);
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "--- BATTLEFIELD DIMENSIONS ---");
            
            int prev_w = state.active_config.map_width; int prev_h = state.active_config.map_height;
            ImGui::SliderInt("Map Width (Horizontal)", &state.active_config.map_width, 10, 25);
            ImGui::SliderInt("Map Height (Vertical)", &state.active_config.map_height, 10, 16);
            if (state.active_config.map_width != prev_w || state.active_config.map_height != prev_h) {
                state.active_config.custom_grid.assign(state.active_config.map_width, std::vector<Terrain>(state.active_config.map_height, Terrain::Dirt));
                state.active_config.custom_human_pos = {1, 1};
            }

            ImGui::Checkbox("ACTIVATE INITIAL SPAWN SHIELD", &state.active_config.spawn_shield);
            ImGui::Checkbox("Enable Environment Events", &state.active_config.enable_environment);
            ImGui::Checkbox("Use Custom Map Design Mode", &state.active_config.custom_map_mode);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", tr("--- PROCEDURAL GENERATION RATIOS (SUM = 100%) ---", "--- TI LE SINH BAN CO (TONG = 100%) ---"));

            auto adjust_ratios = [](int& target, int& o1, int& o2, int& o3, int new_val) {
                int diff = new_val - target;
                if (diff == 0) return;
                target = new_val;
                
                // Distribute diff to o1, o2, o3
                for (int step = 0; step < std::abs(diff); ++step) {
                    int sign = (diff > 0) ? -1 : 1;
                    if (sign == -1) {
                        // Find the largest of o1, o2, o3 that is > 0 to decrement
                        if (o1 >= o2 && o1 >= o3 && o1 > 0) o1 += sign;
                        else if (o2 >= o1 && o2 >= o3 && o2 > 0) o2 += sign;
                        else if (o3 > 0) o3 += sign;
                    } else {
                        // Increment o1, o2, o3 sequentially
                        if (o1 <= o2 && o1 <= o3) o1 += sign;
                        else if (o2 <= o1 && o2 <= o3) o2 += sign;
                        else o3 += sign;
                    }
                }
                
                // Clamp all
                target = std::clamp(target, 0, 100);
                o1 = std::clamp(o1, 0, 100);
                o2 = std::clamp(o2, 0, 100);
                o3 = std::clamp(o3, 0, 100);
                
                int current_sum = target + o1 + o2 + o3;
                if (current_sum != 100) {
                    o1 += (100 - current_sum);
                    o1 = std::clamp(o1, 0, 100);
                }
            };

            int temp_dirt = state.active_config.ratio_dirt;
            int temp_wall = state.active_config.ratio_wall;
            int temp_water = state.active_config.ratio_water;
            int temp_grass = state.active_config.ratio_grass;

            int total_r = temp_dirt + temp_wall + temp_water + temp_grass;
            if (total_r != 100) {
                state.active_config.ratio_dirt = 60;
                state.active_config.ratio_wall = 10;
                state.active_config.ratio_water = 10;
                state.active_config.ratio_grass = 20;
                temp_dirt = 60; temp_wall = 10; temp_water = 10; temp_grass = 20;
            }

            if (ImGui::SliderInt("Dirt Ratio %", &temp_dirt, 0, 100)) {
                adjust_ratios(state.active_config.ratio_dirt, state.active_config.ratio_wall, state.active_config.ratio_water, state.active_config.ratio_grass, temp_dirt);
            }
            if (ImGui::SliderInt("Wall Ratio %", &temp_wall, 0, 100)) {
                adjust_ratios(state.active_config.ratio_wall, state.active_config.ratio_dirt, state.active_config.ratio_water, state.active_config.ratio_grass, temp_wall);
            }
            if (ImGui::SliderInt("Water Ratio %", &temp_water, 0, 100)) {
                adjust_ratios(state.active_config.ratio_water, state.active_config.ratio_dirt, state.active_config.ratio_wall, state.active_config.ratio_grass, temp_water);
            }
            if (ImGui::SliderInt("Grass Ratio %", &temp_grass, 0, 100)) {
                adjust_ratios(state.active_config.ratio_grass, state.active_config.ratio_dirt, state.active_config.ratio_wall, state.active_config.ratio_water, temp_grass);
            }

            if (state.active_config.custom_map_mode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.1f, 1));
                if (ImGui::Button("⚙️ OPEN MAP VISUAL EDITOR", ImVec2(400, 35))) state.current_scene = GameScene::MapEditor;
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "--- HUMAN OPERATIVE STATS ---");
            ImGui::SliderInt("Vital HP", &state.active_config.human_hp, 1, 20);
            ImGui::SliderInt("Turn 1 Stamina", &state.active_config.initial_stamina, 1, 6);
            ImGui::SliderInt("Pistol Ammo", &state.active_config.pistol_ammo, 0, 50);
            ImGui::SliderInt("Shotgun Shells", &state.active_config.shotgun_ammo, 0, 30);
            ImGui::SliderInt("Grenades", &state.active_config.grenades, 0, 10);
            ImGui::SliderInt("Claymore Mines", &state.active_config.mines, 0, 10);
            ImGui::SliderInt("Molotov Bombs", &state.active_config.molotovs, 0, 10);
            ImGui::SliderInt("Operation Max Turns", &state.active_config.turn_limit, 10, 200);

            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "--- HOSTILE THREAT QUANTITIES ---");
            ImGui::SliderInt("Normal Zombies", &state.active_config.count_normal, 0, 20);
            ImGui::SliderInt("Fast Sprinters", &state.active_config.count_fast, 0, 15);
            ImGui::SliderInt("Volatile Exploders", &state.active_config.count_exploding, 0, 15);
            ImGui::SliderInt("Vampiric Draculas", &state.active_config.count_vampire, 0, 10);
            ImGui::SliderInt("Sick Carriers", &state.active_config.count_sick, 0, 15);

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
                if (ImGui::Button("LAUNCH CUSTOM STRATEGY COMBAT", ImVec2(1200, 45))) { state.init_game(); state.current_scene = GameScene::Playing; view_initialized = false; }
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
                    else if (t == Terrain::Grass) cell.setFillColor(sf::Color(55, 125, 35));
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
            ImGui::SetNextWindowSize(ImVec2(400, 650));
            ImGui::Begin("Map Blueprint Architect", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "Brush Selection Tools:");
            if (ImGui::RadioButton("Plain Dirt Floor", state.editor_selected_terrain == Terrain::Dirt)) state.editor_selected_terrain = Terrain::Dirt;
            if (ImGui::RadioButton("Reinforced Wall", state.editor_selected_terrain == Terrain::Wall)) state.editor_selected_terrain = Terrain::Wall;
            if (ImGui::RadioButton("Deep Water Hazard", state.editor_selected_terrain == Terrain::Water)) state.editor_selected_terrain = Terrain::Water;
            if (ImGui::RadioButton("Tall Lush Grass", state.editor_selected_terrain == Terrain::Grass)) state.editor_selected_terrain = Terrain::Grass;

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
                    else if (state.grid[x][y] == Terrain::Grass) {
                        cell.setFillColor(sf::Color(55, 125, 35));
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
                    for (int i = -4; i <= 20; ++i) {
                        float sx = boardOffset + (i * 38.0f * state.active_fx.dx) + std::sin(timeSec * 8.0f + i) * 18.0f;
                        float sy = boardOffset + (i * 30.0f * state.active_fx.dy) + std::cos(timeSec * 7.0f + i) * 18.0f;
                        sf::Vertex gust[] = {
                            sf::Vertex(sf::Vector2f(sx, sy), windColor),
                            sf::Vertex(sf::Vector2f(sx + state.active_fx.dx * state.width * cellSize, sy + state.active_fx.dy * state.height * cellSize), sf::Color(180, 220, 255, 0))
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
                        sf::Color rainColor(110, 170, 255, alpha);
                        for (int i = 0; i < 90; ++i) {
                            float x = boardOffset + std::fmod(i * 37.0f + timeSec * 260.0f, state.width * cellSize);
                            float y = boardOffset + std::fmod(i * 53.0f + timeSec * 420.0f, state.height * cellSize);
                            sf::Vertex drop[] = { sf::Vertex(sf::Vector2f(x, y), rainColor), sf::Vertex(sf::Vector2f(x - 7, y + 16), sf::Color(110, 170, 255, 0)) };
                            window.draw(drop, 2, sf::Lines);
                        }
                    } else {
                        sf::RectangleShape cloud(sf::Vector2f(state.width * cellSize, state.height * cellSize));
                        cloud.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>((1.0f - progress) * 254)));
                        cloud.setPosition(boardOffset, boardOffset);
                        window.draw(cloud);
                    }
                } else if (state.active_fx.type == FXType::Lightning) {
                    sf::Vector2f center = state.getCellCenter(state.active_fx.cx, state.active_fx.cy, cellSize, boardOffset);
                    sf::Vertex bolt[] = {
                        sf::Vertex(sf::Vector2f(center.x - 10, boardOffset), sf::Color(255, 255, 160, alpha)),
                        sf::Vertex(sf::Vector2f(center.x + 6, center.y - 10), sf::Color(160, 230, 255, alpha)),
                        sf::Vertex(sf::Vector2f(center.x - 4, center.y + 10), sf::Color(255, 255, 255, alpha)),
                        sf::Vertex(sf::Vector2f(center.x + 10, center.y + cellSize / 2), sf::Color(160, 230, 255, 0))
                    };
                    window.draw(bolt, 4, sf::LineStrip);
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

            float panelX = boardOffset + VIEW_CELLS * cellSize + 28.0f;
            float panelY = boardOffset;
            float panelW2 = 1400.0f - panelX - 20.0f;
            float panelH = VIEW_CELLS * cellSize;
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
                bool disabled = state.human.is_paralyzed || state.human.stamina == 0;
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
            if (ImGui::Button(("Mine (" + std::to_string(state.human.mines) + ")").c_str())) {
                if (state.human.stamina >= 1 && state.human.mines > 0 && state.phase == TurnPhase::HumanTurn && !state.human.is_paralyzed) {
                    state.mine_grid[state.human.pos.x][state.human.pos.y] = true; 
                    state.human.mines--; state.human.stamina--;
                }
            }

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
            for (size_t i = 0; i < state.zombies.size(); ++i) {
                if (i % 10 != 0) ImGui::SameLine();
                const auto& z = state.zombies[i];
                ImVec4 idColor = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
                if (z->hp > 0) {
                    if (z->type == ZombieType::Fast) idColor = ImVec4(0.35f, 0.75f, 1.0f, 1.0f);
                    else if (z->type == ZombieType::Exploding) idColor = ImVec4(0.9f, 0.5f, 0.1f, 1.0f);
                    else if (z->type == ZombieType::Vampire) idColor = ImVec4(0.7f, 0.2f, 0.7f, 1.0f);
                    else if (z->type == ZombieType::Sick) idColor = ImVec4(0.85f, 0.78f, 0.3f, 1.0f);
                    else idColor = ImVec4(0.2f, 0.8f, 0.4f, 1.0f);
                }
                ImGui::TextColored(idColor, "#%zu", i + 1);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0.85f, 0.25f, 1), "=== %s ===", tr("LIVE RADIO LOGS", "NHAT KY VO TUYEN"));
            ImGui::BeginChild("LiveLogBox", ImVec2(0, 200), true);
            for (const auto& log : state.logs) ImGui::TextColored(log.color, "%s", log.text.c_str());
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

            ImGui::SetNextWindowPos(ImVec2(boardOffset, boardOffset + VIEW_CELLS * cellSize + 6.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(VIEW_CELLS * cellSize, 24.0f), ImGuiCond_Always);
            ImGui::Begin("HScroll", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            int maxHX = std::max(0, state.width - VIEW_CELLS);
            ImGui::SliderInt("##hscroll", &viewX, 0, maxHX);
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(boardOffset + VIEW_CELLS * cellSize + 2.0f, boardOffset), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(24.0f, VIEW_CELLS * cellSize), ImGuiCond_Always);
            ImGui::Begin("VScroll", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            int maxVY = std::max(0, state.height - VIEW_CELLS);
            ImGui::VSliderInt("##vscroll", ImVec2(16.0f, VIEW_CELLS * cellSize - 8.0f), &viewY, 0, maxVY);
            ImGui::End();

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
                    state.current_scene = GameScene::MainMenu;
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
                ImGui::OpenPopup("EndGameModal");
                if (ImGui::BeginPopupModal("EndGameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text(state.game_over ? "OPERATION FAILED! YOU DIED." : "MISSION ACCOMPLISHED! DISPATCH SECTOR CLEAN.");
                    if (ImGui::Button("Return to HQ", ImVec2(180, 30))) { ImGui::CloseCurrentPopup(); state.current_scene = GameScene::MainMenu; }
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
