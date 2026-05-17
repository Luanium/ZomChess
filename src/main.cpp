#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "GameState.h"
#include "UI.h"
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode(1400, 750), "ZomChess Tactical Engine v2.1");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    SetupTacticalImGuiTheme();

    sf::Font boardFont;
    bool hasFont = boardFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");

    GameState state;
    sf::Clock deltaClock;
    sf::Clock animationClock; // Tính toán hiệu ứng lửa nhấp nháy

    float cellSize = 40.0f;
    float boardOffset = 20.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed)
                window.close();

            if (state.current_scene == GameScene::Playing && !ImGui::GetIO().WantCaptureMouse && event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !state.game_over && !state.game_won && state.phase == TurnPhase::HumanTurn) {
                    int tx = (event.mouseButton.x - boardOffset) / cellSize;
                    int ty = (event.mouseButton.y - boardOffset) / cellSize;

                    if (tx >= 0 && tx < state.width && ty >= 0 && ty < state.height) {
                        if (state.input_mode == InputMode::MoveMode) {
                            int dx = std::abs(tx - state.human.pos.x);
                            int dy = std::abs(ty - state.human.pos.y);
                            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                                if (state.grid[tx][ty] != Terrain::Wall && state.grid[tx][ty] != Terrain::Obstacle) {
                                    bool blocked = false;
                                    for (const auto& z : state.zombies) {
                                        if (z->hp > 0 && z->pos == Position{tx, ty}) { blocked = true; break; }
                                    }
                                    if (!blocked) {
                                        int cost = (state.grid[tx][ty] == Terrain::Water) ? 2 : 1;
                                        if (state.human.stamina >= cost) {
                                            state.human.pos = {tx, ty};
                                            state.human.stamina -= cost;
                                            
                                            // Kiểm tra lây lan lửa sau khi di chuyển
                                            state.check_fire_interactions();
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

                    if (tx > 0 && tx < state.active_config.map_width - 1 && ty > 0 && ty < state.active_config.map_height - 1) {
                        state.active_config.custom_grid[tx][ty] = state.editor_selected_terrain;
                        if (state.editor_selected_terrain == Terrain::Dirt && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                            state.active_config.custom_human_pos = {tx, ty};
                        }
                    }
                }
            }
        }

        // --- Cấu trúc ImGui Update nguyên bản gốc ---
        sf::Time dt = deltaClock.restart();
        float dtSeconds = dt.asSeconds();
        ImGui::SFML::Update(window, dt); 

        if (state.current_scene == GameScene::Playing) {
            if (state.active_fx.type != FXType::None) {
                state.active_fx.timer -= dtSeconds;
                if (state.active_fx.timer <= 0.0f) state.active_fx.type = FXType::None;
            }
            state.update_zombie_logic(dtSeconds);
        }

        window.clear(sf::Color(22, 23, 25));

        if (state.current_scene == GameScene::MainMenu) {
            ImGui::SetNextWindowPos(ImVec2(80, 40));
            ImGui::SetNextWindowSize(ImVec2(1240, 670));
            ImGui::Begin("ZomChess Tactical System Hub", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0, 1, 0.8f, 1), "⚡ QUICK PLAY - SELECT DIFFICULTY AND LAUNCH IMMEDIATELY:");
            ImGui::SameLine();
            if (ImGui::Button("EASY (Private)", ImVec2(150, 30))) { state.apply_quick_difficulty(0); state.init_game(); state.current_scene = GameScene::Playing; }
            ImGui::SameLine();
            if (ImGui::Button("MEDIUM (Colonel)", ImVec2(150, 30))) { state.apply_quick_difficulty(1); state.init_game(); state.current_scene = GameScene::Playing; }
            ImGui::SameLine();
            if (ImGui::Button("HARD (Nightmare)", ImVec2(150, 30))) { state.apply_quick_difficulty(2); state.init_game(); state.current_scene = GameScene::Playing; }

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
            ImGui::Checkbox("Use Custom Map Design Mode", &state.active_config.custom_map_mode);

            if (state.active_config.custom_map_mode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.1f, 1));
                if (ImGui::Button("⚙️ OPEN MAP VISUAL EDITOR", ImVec2(400, 35))) {
                    int w = state.active_config.map_width; int h = state.active_config.map_height;
                    for (int x = 0; x < w; ++x) { state.active_config.custom_grid[x][0] = Terrain::Wall; state.active_config.custom_grid[x][h-1] = Terrain::Wall; }
                    for (int y = 0; y < h; ++y) { state.active_config.custom_grid[0][y] = Terrain::Wall; state.active_config.custom_grid[w-1][y] = Terrain::Wall; }
                    state.current_scene = GameScene::MapEditor;
                }
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "--- HUMAN OPERATIVE STATS ---");
            ImGui::SliderInt("Vital HP", &state.active_config.human_hp, 1, 20);
            ImGui::SliderInt("Turn 1 Stamina", &state.active_config.initial_stamina, 2, 12);
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

            int available_slots = state.calculate_available_spawn_cells();
            int total_zoms = state.active_config.count_normal + state.active_config.count_fast + state.active_config.count_exploding + state.active_config.count_vampire;
            
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
                if (ImGui::Button("LAUNCH CUSTOM STRATEGY COMBAT", ImVec2(1200, 45))) { state.init_game(); state.current_scene = GameScene::Playing; }
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
                    else if (t == Terrain::Obstacle) cell.setFillColor(sf::Color(40, 41, 43));
                    else if (t == Terrain::Water) cell.setFillColor(sf::Color(35, 75, 115));
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
            if (ImGui::RadioButton("Solid Block Obstacle", state.editor_selected_terrain == Terrain::Obstacle)) state.editor_selected_terrain = Terrain::Obstacle;
            if (ImGui::RadioButton("Deep Water Hazard", state.editor_selected_terrain == Terrain::Water)) state.editor_selected_terrain = Terrain::Water;
            if (ImGui::RadioButton("Reinforced Interior Wall", state.editor_selected_terrain == Terrain::Wall)) state.editor_selected_terrain = Terrain::Wall;

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            if (ImGui::Button("SAVE DESIGN & RETURN TO ENGINE", ImVec2(360, 50))) state.current_scene = GameScene::MainMenu;
            ImGui::End();
        }
        else if (state.current_scene == GameScene::Playing) {
            float timeSec = animationClock.getElapsedTime().asSeconds();

            for (int x = 0; x < state.width; ++x) {
                for (int y = 0; y < state.height; ++y) {
                    sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    cell.setPosition(x * cellSize + boardOffset, y * cellSize + boardOffset);

                    if (state.grid[x][y] == Terrain::Wall) cell.setFillColor(sf::Color(60, 62, 66));
                    else if (state.grid[x][y] == Terrain::Obstacle) cell.setFillColor(sf::Color(40, 41, 43));
                    else if (state.grid[x][y] == Terrain::Water) cell.setFillColor(sf::Color(35, 75, 115));
                    else if (state.grid[x][y] == Terrain::Fire) {
                        // Animation ngọn lửa chớp tắt bập bùng
                        int pulse = static_cast<int>(25.0f * std::sin(timeSec * 12.0f));
                        cell.setFillColor(sf::Color(220 + pulse, 100 + pulse / 2, 20));
                    }
                    else cell.setFillColor(sf::Color(105, 60, 35));
                    
                    window.draw(cell);

                    if (state.mine_grid[x][y]) {
                        sf::CircleShape mine(6.0f); mine.setFillColor(sf::Color(230, 40, 40)); mine.setOrigin(6.0f, 6.0f);
                        mine.setPosition(x * cellSize + boardOffset + cellSize/2, y * cellSize + boardOffset + cellSize/2); window.draw(mine);
                    }
                }
            }

            if (state.grenade_box.active) {
                sf::CircleShape gren(8.0f, 4); gren.setFillColor(sf::Color(50, 210, 50)); gren.setOrigin(8.0f, 8.0f);
                gren.setPosition(state.grenade_box.pos.x * cellSize + boardOffset + cellSize/2, state.grenade_box.pos.y * cellSize + boardOffset + cellSize/2); window.draw(gren);
            }

            // Vẽ Zombie
            for (size_t i = 0; i < state.zombies.size(); ++i) {
                const auto& z = state.zombies[i];
                if (z->hp <= 0) {
                    sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f)); deadZ.setFillColor(sf::Color(15, 15, 15, 160));
                    deadZ.setPosition(z->pos.x * cellSize + boardOffset + 4.0f, z->pos.y * cellSize + boardOffset + 4.0f); window.draw(deadZ); continue;
                }
                sf::RectangleShape zVisual(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
                zVisual.setPosition(z->pos.x * cellSize + boardOffset + 3.0f, z->pos.y * cellSize + boardOffset + 3.0f);
                
                // MÀU SẮC ZOMBIE (Bảo tồn chuẩn xác theo code cũ)
                if (z->type == ZombieType::Fast) zVisual.setFillColor(sf::Color(135, 200, 45));
                else if (z->type == ZombieType::Exploding) zVisual.setFillColor(sf::Color(220, 110, 15));
                else if (z->type == ZombieType::Vampire) zVisual.setFillColor(sf::Color(130, 30, 130));
                else zVisual.setFillColor(sf::Color(40, 140, 55));
                window.draw(zVisual);

                if (hasFont) {
                    sf::Text zIdStr; zIdStr.setFont(boardFont); zIdStr.setString(std::to_string(i + 1)); zIdStr.setCharacterSize(14);
                    sf::FloatRect textRect = zIdStr.getLocalBounds(); zIdStr.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
                    zIdStr.setPosition(z->pos.x * cellSize + boardOffset + cellSize/2.0f, z->pos.y * cellSize + boardOffset + cellSize/2.0f); window.draw(zIdStr);
                    
                    // Ký hiệu lửa nhấp nháy cho Zombie
                    if (z->is_burning && std::fmod(timeSec, 0.6f) < 0.3f) {
                        sf::Text fTxt("[F]", boardFont, 14); fTxt.setFillColor(sf::Color(255, 140, 0));
                        fTxt.setPosition(z->pos.x * cellSize + boardOffset + 2, z->pos.y * cellSize + boardOffset - 12);
                        window.draw(fTxt);
                    }
                }
            }

            // Vẽ Human player
            sf::CircleShape hVisual(cellSize / 2.5f); hVisual.setFillColor(sf::Color::White);
            hVisual.setPosition(state.human.pos.x * cellSize + boardOffset + 4.0f, state.human.pos.y * cellSize + boardOffset + 4.0f);
            window.draw(hVisual);

            // Ký hiệu lửa nhấp nháy cho Human
            if (state.human.is_burning && hasFont && std::fmod(timeSec, 0.6f) < 0.3f) {
                sf::Text hfTxt("[F]", boardFont, 14); hfTxt.setFillColor(sf::Color::Red);
                hfTxt.setPosition(state.human.pos.x * cellSize + boardOffset + 2, state.human.pos.y * cellSize + boardOffset - 12);
                window.draw(hfTxt);
            }

            // Xử lý FX đạn bắn bay (Bao gồm cả FX tia Molotov)
            if (state.active_fx.type != FXType::None) {
                float progress = state.active_fx.timer / state.active_fx.max_duration;
                sf::Uint8 alpha = static_cast<sf::Uint8>(progress * 255);
                if (state.active_fx.type == FXType::Pistol || state.active_fx.type == FXType::Molotov) {
                    sf::Color beamColor = (state.active_fx.type == FXType::Molotov) ? sf::Color(255, 120, 0, alpha) : sf::Color(255, 255, 100, alpha);
                    sf::Vertex bLine[] = { sf::Vertex(state.active_fx.start_p, beamColor), sf::Vertex(state.active_fx.end_p, sf::Color(255, 60, 60, alpha)) };
                    window.draw(bLine, 2, sf::Lines);
                } else if (state.active_fx.type == FXType::Shotgun || state.active_fx.type == FXType::Explosion) {
                    for (auto p : state.active_fx.blast_cells) {
                        sf::RectangleShape blastZone(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                        blastZone.setFillColor(state.active_fx.type == FXType::Shotgun ? sf::Color(255,130,30, progress*120) : sf::Color(255,50,10, progress*160));
                        blastZone.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset); window.draw(blastZone);
                    }
                }
            }

            ImGui::SetNextWindowPos(ImVec2(state.width * cellSize + boardOffset + 20.0f, 20.0f));
            ImGui::SetNextWindowSize(ImVec2(1380 - (state.width * cellSize + boardOffset), 700));
            ImGui::Begin("Tactical Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            ImGui::TextColored(ImVec4(0,1,1,1), "🎯 TURN: %d/%d", state.current_turn, state.turn_limit);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "🔋 STAMINA: %d | ❤️ HEALTH: %d", state.human.stamina, state.human.hp);
            if (state.human.is_burning) ImGui::TextColored(ImVec4(1, 0.4f, 0, 1), "⚠️ WARNING: You are BURNING!");
            ImGui::Text("Position: [%d, %d]", state.human.pos.x, state.human.pos.y);
            ImGui::Separator();

            if (ImGui::Button("SURRENDER & RETURN HUB", ImVec2(210, 35))) state.current_scene = GameScene::MainMenu;
            ImGui::SameLine();
            if (ImGui::Button("END PLAYER TURN", ImVec2(210, 35))) { 
                if (state.phase == TurnPhase::HumanTurn) state.start_zombie_phase(); 
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.4f, 1, 1), "WEAPONRY SELECTOR:");
            
            auto weapon_button = [&](const char* label, InputMode mode) {
                if (state.input_mode == mode) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1));
                else ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
                if (ImGui::Button(label)) state.input_mode = mode;
                ImGui::PopStyleColor();
            };

            weapon_button("Knife [1]", InputMode::TargetKnife); ImGui::SameLine();
            weapon_button(("Pistol (" + std::to_string(state.human.pistol_ammo) + ")").c_str(), InputMode::TargetPistol); ImGui::SameLine();
            weapon_button(("Shotgun (" + std::to_string(state.human.shotgun_ammo) + ")").c_str(), InputMode::TargetShotgun); ImGui::SameLine();
            weapon_button(("Grenade (" + std::to_string(state.human.grenades) + ")").c_str(), InputMode::TargetGrenade); ImGui::SameLine();
            
            // Nút bấm Molotov mới
            weapon_button(("Molotov (" + std::to_string(state.human.molotovs) + ")").c_str(), InputMode::TargetMolotov);
            
            if (ImGui::Button(("Plant Mine (" + std::to_string(state.human.mines) + ")").c_str())) {
                if (state.human.stamina >= 1 && state.human.mines > 0 && state.phase == TurnPhase::HumanTurn) {
                    state.mine_grid[state.human.pos.x][state.human.pos.y] = true; 
                    state.human.mines--; state.human.stamina--;
                }
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "💀 HOSTILE RADAR DETECTIONS (%zu Active Targets):", state.zombies.size());
            
            ImGui::BeginChild("ZombieStatusBox", ImVec2(0, 180), true, ImGuiWindowFlags_HorizontalScrollbar);
            if (ImGui::BeginTable("ZombieTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("Type Class", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                ImGui::TableSetupColumn("Vital HP", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Coordinates", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < state.zombies.size(); ++i) {
                    const auto& z = state.zombies[i];
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0); ImGui::Text("#%zu", i + 1);

                    ImGui::TableSetColumnIndex(1);
                    if (z->hp <= 0) ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "[DECEASED]");
                    else {
                        if (z->type == ZombieType::Fast) ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.2f, 1), "Sprinter");
                        else if (z->type == ZombieType::Exploding) ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1), "Exploder");
                        else if (z->type == ZombieType::Vampire) ImGui::TextColored(ImVec4(0.7f, 0.2f, 0.7f, 1), "Vampire");
                        else ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.3f, 1), "Normal");
                    }

                    ImGui::TableSetColumnIndex(2);
                    if (z->hp <= 0) ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "0 HP");
                    else {
                        std::string hp_bar = ""; for(int h=0; h<z->hp; ++h) hp_bar += "I";
                        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s (%d)", hp_bar.c_str(), z->hp);
                    }

                    ImGui::TableSetColumnIndex(3);
                    if (z->hp <= 0) ImGui::Text("-");
                    else ImGui::Text("[%d, %d] %s", z->pos.x, z->pos.y, z->is_burning ? "(FIRE)" : "");
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 0.75f, 0, 1), "=== LIVE RADAR LOGS ===");
            ImGui::BeginChild("LiveLogBox", ImVec2(0, 200), true);
            for (const auto& log : state.logs) ImGui::TextColored(log.color, "%s", log.text.c_str());
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

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
