#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "GameState.h"
#include "UI.h"
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode(1400, 740), "ZomChess C++20 Refined Tactical Edition");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    SetupTacticalImGuiTheme();

    sf::Font boardFont;
    bool hasFont = boardFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");

    GameState state;
    sf::Clock deltaClock;

    float cellSize = 42.0f;
    float boardOffset = 20.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed)
                window.close();

            if (!ImGui::GetIO().WantCaptureMouse && event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !state.game_over && !state.game_won && state.phase == TurnPhase::HumanTurn) {
                    int mx = event.mouseButton.x;
                    int my = event.mouseButton.y;

                    int tx = (mx - boardOffset) / cellSize;
                    int ty = (my - boardOffset) / cellSize;

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
                                            state.add_log("Human moved to cell (" + std::to_string(tx) + ", " + std::to_string(ty) + ").");
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
        }

        sf::Time dt = deltaClock.restart();
        float dtSeconds = dt.asSeconds();
        ImGui::SFML::Update(window, dt); 

        if (state.active_fx.type != FXType::None) {
            state.active_fx.timer -= dtSeconds;
            if (state.active_fx.timer <= 0.0f) {
                state.active_fx.type = FXType::None;
            }
        }
        state.update_zombie_logic(dtSeconds);

        window.clear(sf::Color(25, 26, 28));

        // Draw Board Grid
        for (int x = 0; x < state.width; ++x) {
            for (int y = 0; y < state.height; ++y) {
                sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                cell.setPosition(x * cellSize + boardOffset, y * cellSize + boardOffset);

                if (state.grid[x][y] == Terrain::Wall) cell.setFillColor(sf::Color(60, 62, 66));
                else if (state.grid[x][y] == Terrain::Obstacle) cell.setFillColor(sf::Color(40, 41, 43));
                else if (state.grid[x][y] == Terrain::Water) cell.setFillColor(sf::Color(35, 75, 115));
                else cell.setFillColor(sf::Color(105, 60, 35));

                window.draw(cell);

                if (state.mine_grid[x][y]) {
                    sf::CircleShape mine(6.0f);
                    mine.setFillColor(sf::Color(230, 40, 40));
                    mine.setOrigin(6.0f, 6.0f);
                    mine.setPosition(x * cellSize + boardOffset + cellSize/2, y * cellSize + boardOffset + cellSize/2);
                    window.draw(mine);
                }
            }
        }

        if (state.grenade_box.active) {
            sf::CircleShape gren(8.0f, 4);
            gren.setFillColor(sf::Color(50, 210, 50));
            gren.setOrigin(8.0f, 8.0f);
            gren.setPosition(state.grenade_box.pos.x * cellSize + boardOffset + cellSize/2, state.grenade_box.pos.y * cellSize + boardOffset + cellSize/2);
            window.draw(gren);
        }

        // Draw Zombies
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            const auto& z = state.zombies[i];
            if (z->hp <= 0) {
                sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
                deadZ.setFillColor(sf::Color(15, 15, 15, 160));
                deadZ.setPosition(z->pos.x * cellSize + boardOffset + 4.0f, z->pos.y * cellSize + boardOffset + 4.0f);
                window.draw(deadZ);
                continue;
            }
            sf::RectangleShape zVisual(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
            zVisual.setPosition(z->pos.x * cellSize + boardOffset + 3.0f, z->pos.y * cellSize + boardOffset + 3.0f);

            if (z->type == ZombieType::Fast) zVisual.setFillColor(sf::Color(135, 200, 45));
            else if (z->type == ZombieType::Exploding) zVisual.setFillColor(sf::Color(220, 110, 15));
            else if (z->type == ZombieType::Vampire) zVisual.setFillColor(sf::Color(130, 30, 130));
            else zVisual.setFillColor(sf::Color(40, 140, 55));

            window.draw(zVisual);

            if (hasFont) {
                sf::Text zIdStr;
                zIdStr.setFont(boardFont);
                zIdStr.setString(std::to_string(i + 1));
                zIdStr.setCharacterSize(14);
                zIdStr.setFillColor(sf::Color::White);
                zIdStr.setStyle(sf::Text::Bold);
                
                sf::FloatRect textRect = zIdStr.getLocalBounds();
                zIdStr.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
                zIdStr.setPosition(z->pos.x * cellSize + boardOffset + cellSize/2.0f, z->pos.y * cellSize + boardOffset + cellSize/2.0f);
                window.draw(zIdStr);
            }
        }

        // Draw Human
        sf::CircleShape hVisual(cellSize / 2.5f);
        hVisual.setFillColor(sf::Color::White);
        hVisual.setPosition(state.human.pos.x * cellSize + boardOffset + 4.0f, state.human.pos.y * cellSize + boardOffset + 4.0f);
        window.draw(hVisual);

        // Draw Visual FX
        if (state.active_fx.type != FXType::None) {
            float progress = state.active_fx.timer / state.active_fx.max_duration;
            sf::Uint8 alpha = static_cast<sf::Uint8>(progress * 255);

            if (state.active_fx.type == FXType::Pistol) {
                sf::Vertex bulletLine[] = {
                    sf::Vertex(state.active_fx.start_p, sf::Color(255, 255, 100, alpha)),
                    sf::Vertex(state.active_fx.end_p, sf::Color(255, 60, 60, alpha))
                };
                window.draw(bulletLine, 2, sf::Lines);
            }
            else if (state.active_fx.type == FXType::Knife) {
                sf::Vector2f dir = state.active_fx.end_p - state.active_fx.start_p;
                float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
                if (len > 0) {
                    sf::Vector2f unitDir = dir / len;
                    sf::Vector2f perpDir(-unitDir.y, unitDir.x);

                    sf::ConvexShape blade;
                    blade.setPointCount(3);
                    blade.setPoint(0, state.active_fx.start_p + perpDir * 5.0f);
                    blade.setPoint(1, state.active_fx.start_p - perpDir * 5.0f);
                    blade.setPoint(2, state.active_fx.start_p + unitDir * (cellSize * 1.1f));
                    blade.setFillColor(sf::Color(0, 255, 200, alpha));
                    window.draw(blade);
                }
            }
            else if (state.active_fx.type == FXType::Shotgun) {
                for (auto p : state.active_fx.blast_cells) {
                    sf::RectangleShape orangeZone(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    orangeZone.setFillColor(sf::Color(255, 130, 30, static_cast<sf::Uint8>(progress * 130)));
                    orangeZone.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset);
                    window.draw(orangeZone);
                }
                if (!state.active_fx.blast_cells.empty()) {
                    sf::ConvexShape coneVisual;
                    coneVisual.setPointCount(3);
                    coneVisual.setPoint(0, state.active_fx.start_p);
                    coneVisual.setPoint(1, state.getCellCenter(state.active_fx.blast_cells.front().x, state.active_fx.blast_cells.front().y, cellSize, boardOffset));
                    coneVisual.setPoint(2, state.getCellCenter(state.active_fx.blast_cells.back().x, state.active_fx.blast_cells.back().y, cellSize, boardOffset));
                    coneVisual.setFillColor(sf::Color(255, 160, 50, static_cast<sf::Uint8>(progress * 45)));
                    window.draw(coneVisual);
                }
            }
            else if (state.active_fx.type == FXType::Explosion) {
                for (auto p : state.active_fx.blast_cells) {
                    sf::RectangleShape fireGrid(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    fireGrid.setFillColor(sf::Color(255, 60, 10, static_cast<sf::Uint8>(progress * 180)));
                    fireGrid.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset);
                    window.draw(fireGrid);
                }
            }
        }

        // --- ImGui HUD Drawing ---
        ImGui::SetNextWindowPos(ImVec2(670, 20));
        ImGui::SetNextWindowSize(ImVec2(710, 690));
        ImGui::Begin("Tactical Commander Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[TURN]: %d/%d", state.current_turn, state.turn_limit);
        ImGui::SameLine(); ImGui::Text(" | [STAMINA]: %d", state.human.stamina);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), " | [VITAL HEALTH]: %d", state.human.hp);
        
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("END TURN (PASS_WAVE)", ImVec2(210, 42))) { 
            if (state.phase == TurnPhase::HumanTurn) state.start_zombie_phase(); 
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        if (ImGui::Button("Tactical Knife (oo)")) { state.input_mode = InputMode::TargetKnife; } ImGui::SameLine();
        if (ImGui::Button(("Handgun (" + std::to_string(state.human.pistol_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetPistol; } ImGui::SameLine();
        if (ImGui::Button(("Shotgun (" + std::to_string(state.human.shotgun_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetShotgun; } ImGui::SameLine();
        if (ImGui::Button(("Frag Grenade (" + std::to_string(state.human.grenades) + ")").c_str())) { state.input_mode = InputMode::TargetGrenade; } ImGui::SameLine();
        if (ImGui::Button(("Landmine (" + std::to_string(state.human.mines) + ")").c_str())) {
            if (!state.game_over && !state.game_won && state.human.stamina >= 1 && state.human.mines > 0 && state.phase == TurnPhase::HumanTurn) {
                state.mine_grid[state.human.pos.x][state.human.pos.y] = true;
                state.human.mines--;
                state.human.stamina--;
                state.add_log("[MINE] Strategic claymore planted under current standing tile!", ImVec4(1, 0.7f, 0, 1));
            }
        }

        if (state.phase == TurnPhase::ZombieAnimating) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "[WARNING]: HOSTILES ARE MOVING, PLEASE WAIT...");
        } else if (state.input_mode != InputMode::MoveMode) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[TARGET MODE]: CLICK ON THE BOARD GRID TO ENGAGE WEAPON FIRE!");
        } else {
            ImGui::Text("[MODE]: Click surrounding adjacent tiles to navigate movement.");
        }

        ImGui::Separator();

        ImGui::TextColored(ImVec4(1, 0.75f, 0, 1), "=== HOSTILE INTEL SPECIFICATIONS ===");
        ImGui::BeginChild("ZombieTableScroll", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(3, "zom_cols");
        ImGui::SetColumnWidth(0, 80); ImGui::SetColumnWidth(1, 250);
        ImGui::Text("ID"); ImGui::NextColumn(); ImGui::Text("Chop Classification Squad"); ImGui::NextColumn(); ImGui::Text("Vital Status"); ImGui::NextColumn();
        ImGui::Separator();
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            ImGui::Text("#%zu", i + 1); ImGui::NextColumn();
            ImGui::Text("%s", state.zombies[i]->name.c_str()); ImGui::NextColumn();
            if (state.zombies[i]->hp <= 0) {
                ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 0.6f), "[DEAD] TERMINATED");
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d HP Remaining", state.zombies[i]->hp);
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0, 1, 0.7f, 1), "--- BLACK-BOX CENTRAL BATTLE LOGS ---");
        ImGui::BeginChild("LogScrollBox", ImVec2(0, 260), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (const auto& log : state.logs) {
            ImGui::TextColored(log.color, "%s", log.text.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        if (state.game_over || state.game_won) {
            ImGui::OpenPopup("EndGameModal");
            ImGui::SetNextWindowPos(ImVec2(320, 220), ImGuiCond_Appearing);
            if (ImGui::BeginPopupModal("EndGameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (state.game_over) ImGui::TextColored(ImVec4(1, 0.1f, 0.1f, 1), "DEFEAT! Human forces collapsed under the zombie outbreak.");
                else ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "VICTORY! All hostile entities completely purged from sector.");
                ImGui::Separator();
                if (ImGui::Button("Exit Combat Area", ImVec2(180, 35))) { window.close(); }
                ImGui::EndPopup();
            }
        }

        ImGui::End();

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
