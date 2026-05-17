#include "GameState.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>

GameState::GameState() : rng(std::random_device{}()) {
    active_config.custom_grid.assign(active_config.map_width, std::vector<Terrain>(active_config.map_height, Terrain::Dirt));
}

int GameState::calculate_available_spawn_cells() {
    int w = active_config.map_width;
    int h = active_config.map_height;
    int total_allowed = 0;
    Position h_pos = active_config.custom_map_mode ? active_config.custom_human_pos : Position{w/2, h/2};

    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            if (active_config.custom_map_mode) {
                Terrain t = active_config.custom_grid[x][y];
                if (t == Terrain::Wall || t == Terrain::Obstacle) continue;
            }
            if (x == h_pos.x && y == h_pos.y) continue;
            if (active_config.spawn_shield) {
                if (std::abs(x - h_pos.x) <= 3 && std::abs(y - h_pos.y) <= 3) continue;
            }
            total_allowed++;
        }
    }
    return total_allowed;
}

bool GameState::is_zombie_count_valid() {
    int total_requested_zombies = active_config.count_normal + active_config.count_fast + 
                                  active_config.count_exploding + active_config.count_vampire;
    return total_requested_zombies <= calculate_available_spawn_cells();
}

void GameState::apply_quick_difficulty(int level) {
    active_config.custom_map_mode = false; 
    active_config.spawn_shield = true;
    active_config.map_width = 15;
    active_config.map_height = 15;
    active_config.turn_limit = 50;

    if (level == 0) { 
        active_config.human_hp = 8;
        active_config.initial_stamina = 7;
        active_config.pistol_ammo = 20; active_config.shotgun_ammo = 10;
        active_config.grenades = 5;     active_config.mines = 4;
        active_config.molotovs = 4;
        active_config.count_normal = 3; active_config.count_fast = 1;
        active_config.count_exploding = 1; active_config.count_vampire = 0;
    } 
    else if (level == 1) { 
        active_config.human_hp = 5;
        active_config.initial_stamina = 6;
        active_config.pistol_ammo = 12; active_config.shotgun_ammo = 6;
        active_config.grenades = 3;     active_config.mines = 2;
        active_config.molotovs = 3;
        active_config.count_normal = 4; active_config.count_fast = 3;
        active_config.count_exploding = 2; active_config.count_vampire = 1;
    } 
    else { 
        active_config.human_hp = 3;
        active_config.initial_stamina = 5;
        active_config.pistol_ammo = 6;  active_config.shotgun_ammo = 3;
        active_config.grenades = 1;     active_config.mines = 1;
        active_config.molotovs = 1;
        active_config.count_normal = 6; active_config.count_fast = 5;
        active_config.count_exploding = 4; active_config.count_vampire = 2;
    }
}

bool GameState::export_challenge_file(const std::string& path) {
    std::ofstream outFile(path);
    if (!outFile.is_open()) return false;
    outFile << "MAP_W "           << active_config.map_width << "\n";
    outFile << "MAP_H "           << active_config.map_height << "\n";
    outFile << "HUMAN_HP "        << active_config.human_hp << "\n";
    outFile << "INITIAL_STAMINA " << active_config.initial_stamina << "\n";
    outFile << "PISTOL_AMMO "     << active_config.pistol_ammo << "\n";
    outFile << "SHOTGUN_AMMO "    << active_config.shotgun_ammo << "\n";
    outFile << "GRENADES "        << active_config.grenades << "\n";
    outFile << "MINES "           << active_config.mines << "\n";
    outFile << "MOLOTOVS "        << active_config.molotovs << "\n";
    outFile << "TURN_LIMIT "      << active_config.turn_limit << "\n";
    outFile << "ZOM_NORMAL "      << active_config.count_normal << "\n";
    outFile << "ZOM_FAST "        << active_config.count_fast << "\n";
    outFile << "ZOM_EXPLODING "   << active_config.count_exploding << "\n";
    outFile << "ZOM_VAMPIRE "     << active_config.count_vampire << "\n";
    outFile << "SHIELD "          << (active_config.spawn_shield ? 1 : 0) << "\n";
    outFile << "CUSTOM_MAP "      << (active_config.custom_map_mode ? 1 : 0) << "\n";
    outFile << "CUST_HUMAN_X "    << active_config.custom_human_pos.x << "\n";
    outFile << "CUST_HUMAN_Y "    << active_config.custom_human_pos.y << "\n";

    if (active_config.custom_map_mode) {
        outFile << "GRID_DATA\n";
        for (int y = 0; y < active_config.map_height; ++y) {
            for (int x = 0; x < active_config.map_width; ++x) {
                outFile << static_cast<int>(active_config.custom_grid[x][y]) << " ";
            }
            outFile << "\n";
        }
    }
    outFile.close();
    return true;
}

bool GameState::import_challenge_file(const std::string& path) {
    std::ifstream inFile(path);
    if (!inFile.is_open()) return false;
    std::string key; int val;
    while (inFile >> key) {
        if (key == "GRID_DATA") {
            active_config.custom_grid.assign(active_config.map_width, std::vector<Terrain>(active_config.map_height, Terrain::Dirt));
            for (int y = 0; y < active_config.map_height; ++y) {
                for (int x = 0; x < active_config.map_width; ++x) {
                    int t_val;
                    if (inFile >> t_val) active_config.custom_grid[x][y] = static_cast<Terrain>(t_val);
                }
            }
            break; 
        }
        inFile >> val;
        if (key == "MAP_W")            active_config.map_width = val;
        else if (key == "MAP_H")       active_config.map_height = val;
        else if (key == "HUMAN_HP")    active_config.human_hp = val;
        else if (key == "INITIAL_STAMINA") active_config.initial_stamina = val;
        else if (key == "PISTOL_AMMO")  active_config.pistol_ammo = val;
        else if (key == "SHOTGUN_AMMO") active_config.shotgun_ammo = val;
        else if (key == "GRENADES")     active_config.grenades = val;
        else if (key == "MINES")        active_config.mines = val;
        else if (key == "MOLOTOVS")     active_config.molotovs = val;
        else if (key == "TURN_LIMIT")   active_config.turn_limit = val;
        else if (key == "ZOM_NORMAL")   active_config.count_normal = val;
        else if (key == "ZOM_FAST")     active_config.count_fast = val;
        else if (key == "ZOM_EXPLODING") active_config.count_exploding = val;
        else if (key == "ZOM_VAMPIRE")  active_config.count_vampire = val;
        else if (key == "SHIELD")       active_config.spawn_shield = (val == 1);
        else if (key == "CUSTOM_MAP")   active_config.custom_map_mode = (val == 1);
        else if (key == "CUST_HUMAN_X") active_config.custom_human_pos.x = val;
        else if (key == "CUST_HUMAN_Y") active_config.custom_human_pos.y = val;
    }
    inFile.close();
    return true;
}

void GameState::init_game() {
    zombies.clear(); 
    logs.clear();
    fire_cells.clear();
    floating_texts.clear();
    attack_animations.clear();
    game_over = false; 
    game_won = false;
    current_turn = 1; 
    phase = TurnPhase::HumanTurn; 
    input_mode = InputMode::MoveMode;

    width = active_config.map_width;
    height = active_config.map_height;
    turn_limit = active_config.turn_limit;
    
    human.hp = active_config.human_hp;
    human.stamina = active_config.initial_stamina;
    human.pistol_ammo = active_config.pistol_ammo;
    human.shotgun_ammo = active_config.shotgun_ammo;
    human.grenades = active_config.grenades;
    human.mines = active_config.mines;
    human.molotovs = active_config.molotovs;
    human.is_burning = false;

    mine_grid.assign(width, std::vector<bool>(height, false));

    if (active_config.custom_map_mode) {
        if (active_config.custom_grid.size() != static_cast<size_t>(width) || 
            active_config.custom_grid[0].size() != static_cast<size_t>(height)) {
            active_config.custom_grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
            active_config.custom_human_pos = {1, 1};
        }
        grid = active_config.custom_grid;
        human.pos = active_config.custom_human_pos;
    } 
    else {
        grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
        std::uniform_int_distribution<int> dist_x(1, width - 2);
        std::uniform_int_distribution<int> dist_y(1, height - 2);

        int total_cells = width * height;
        for (int i = 0; i < total_cells / 15; ++i) { grid[dist_x(rng)][dist_y(rng)] = Terrain::Obstacle; }
        for (int i = 0; i < total_cells / 30; ++i) { grid[dist_x(rng)][dist_y(rng)] = Terrain::Water; }

        human.pos = {dist_x(rng), dist_y(rng)};
        while (grid[human.pos.x][human.pos.y] == Terrain::Wall || grid[human.pos.x][human.pos.y] == Terrain::Obstacle) {
            human.pos = {dist_x(rng), dist_y(rng)};
        }
    }

    std::uniform_int_distribution<int> dist_x(0, width - 1);
    std::uniform_int_distribution<int> dist_y(0, height - 1);

    auto spawn_zombie_lambda = [&](ZombieType z_type, int count, const std::string& name, int max_hp) {
        for (int i = 0; i < count; ++i) {
            Position z_pos = {dist_x(rng), dist_y(rng)};
            int attempts = 0;
            while (true) {
                bool invalid_pos = false;
                if (z_pos == human.pos || grid[z_pos.x][z_pos.y] == Terrain::Wall || grid[z_pos.x][z_pos.y] == Terrain::Obstacle) invalid_pos = true;
                if (active_config.spawn_shield && std::abs(z_pos.x - human.pos.x) <= 3 && std::abs(z_pos.y - human.pos.y) <= 3) invalid_pos = true;
                for (const auto& z : zombies) { if (z->pos == z_pos) { invalid_pos = true; break; } }
                if (!invalid_pos || attempts > 200) break;
                z_pos = {dist_x(rng), dist_y(rng)};
                attempts++;
            }
            if (z_type == ZombieType::Normal) zombies.push_back(std::make_unique<NormalZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Fast) zombies.push_back(std::make_unique<FastZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Exploding) zombies.push_back(std::make_unique<ExplodingZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Vampire) zombies.push_back(std::make_unique<VampireZombie>(z_pos, max_hp, name, z_type));
        }
    };

    spawn_zombie_lambda(ZombieType::Normal, active_config.count_normal, "Normal Zom", 2);
    spawn_zombie_lambda(ZombieType::Fast, active_config.count_fast, "Fast Sprinter", 2);
    spawn_zombie_lambda(ZombieType::Exploding, active_config.count_exploding, "Exploder", 3);
    spawn_zombie_lambda(ZombieType::Vampire, active_config.count_vampire, "Vampire Dracula", 4);

    add_log("Tactical Battleground initialized with dynamic safety boundaries!", ImVec4(0, 1, 1, 1));
}

void GameState::add_log(const std::string& text, ImVec4 color) { 
    logs.push_back({text, color}); 
    if (logs.size() > 100) logs.erase(logs.begin()); 
}

int GameState::distance(Position p1, Position p2) { 
    return std::max(std::abs(p1.x - p2.x), std::abs(p1.y - p2.y)); 
}

std::pair<int, int> GameState::get_8_direction(int dx, int dy) { 
    if (dx == 0 && dy == 0) return {0, 0}; 
    if (std::abs(dx) == std::abs(dy)) return {dx > 0 ? 1 : -1, dy > 0 ? 1 : -1}; 
    if (std::abs(dx) > std::abs(dy)) return {dx > 0 ? 1 : -1, 0}; 
    return {0, dy > 0 ? 1 : -1}; 
}

sf::Vector2f GameState::getCellCenter(int x, int y, float cellSize, float offset) { 
    return sf::Vector2f(x * cellSize + offset + cellSize / 2.0f, y * cellSize + offset + cellSize / 2.0f); 
}

void GameState::check_fire_interactions() {
    if (grid[human.pos.x][human.pos.y] == Terrain::Fire && !human.is_burning) {
        human.is_burning = true;
        add_log("[FIRE] Human stepped into a blazing fire!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
    }
    for (auto& z : zombies) {
        if (z->hp > 0 && !z->is_burning && grid[z->pos.x][z->pos.y] == Terrain::Fire) {
            z->is_burning = true;
            add_log("[FIRE] " + z->name + " stepped into a blazing fire!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
        }
    }

    std::vector<Position> fire_sources;
    if (human.is_burning) fire_sources.push_back(human.pos);
    for (const auto& z : zombies) {
        if (z->hp > 0 && z->is_burning) fire_sources.push_back(z->pos);
    }

    for (const auto& p : fire_sources) {
        if (!human.is_burning && distance(human.pos, p) <= 1) {
            human.is_burning = true;
            add_log("[FIRE] Human caught fire from close proximity!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && !z->is_burning && distance(z->pos, p) <= 1) {
                z->is_burning = true;
                add_log("[FIRE] " + z->name + " caught fire from close proximity!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
            }
        }
    }
}

void GameState::trigger_explosion(int cx, int cy) { 
    add_log("[EXPLOSION] Detonated at (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f)); 
    
    Terrain center_t = grid[cx][cy];
    int radius = (center_t == Terrain::Water) ? 1 : 2;

    active_fx.type = FXType::Explosion; 
    active_fx.timer = 0.4f; 
    active_fx.max_duration = 0.4f; 
    active_fx.blast_cells.clear(); 

    for (int dx = -radius; dx <= radius; ++dx) { 
        for (int dy = -radius; dy <= radius; ++dy) { 
            int nx = cx + dx, ny = cy + dy; 
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) { 
                active_fx.blast_cells.push_back({nx, ny}); 
                
                if (human.pos.x == nx && human.pos.y == ny) {
                    human.hp = std::max(0, human.hp - 2);
                    floating_texts.push_back({human.pos, -2, 1.0f, 1.0f});
                    add_log("-> Human hit by blast! -2 HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); 
                }
                
                for (size_t i = 0; i < zombies.size(); ++i) { 
                    auto& zom = zombies[i]; 
                    if (zom->hp > 0 && zom->pos.x == nx && zom->pos.y == ny) {
                        zom->hp -= 2; 
                        floating_texts.push_back({zom->pos, -2, 1.0f, 1.0f});
                        add_log("-> Zombie #" + std::to_string(i+1) + " damaged by blast. -2 HP.", ImVec4(1.0f, 0.6f, 0.1f, 1.0f)); 
                        
                        if (zom->hp <= 0 && zom->type == ZombieType::Exploding) { 
                            trigger_explosion(zom->pos.x, zom->pos.y); 
                        }
                    }
                }
            } 
        } 
    } 
    check_victory_conditions(); 
}

void GameState::zombie_single_step(size_t idx) { 
    if (game_over || game_won) return; 
    auto& zom = zombies[idx]; 
    double lambda = 1.2; 
    int current_dist = distance(zom->pos, human.pos); 
    std::vector<Position> valid_moves; 
    std::vector<double> weights; 

    for (int dx = -1; dx <= 1; ++dx) { 
        for (int dy = -1; dy <= 1; ++dy) { 
            int nx = zom->pos.x + dx; 
            int ny = zom->pos.y + dy; 
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) { 
                Position target{nx, ny}; 
                if (target == human.pos) continue; 
                if (grid[nx][ny] == Terrain::Wall || grid[nx][ny] == Terrain::Obstacle) continue; 
                
                bool conflict = false; 
                for (size_t o = 0; o < zombies.size(); ++o) { 
                    if (o != idx && zombies[o]->hp > 0 && zombies[o]->pos == target) { 
                        conflict = true; 
                        break; 
                    } 
                } 
                if (conflict) continue; 

                int new_dist = distance(target, human.pos); 
                double w = std::exp(-new_dist / lambda); 
                if (new_dist > current_dist) w *= 0.05; 
                else if (new_dist == current_dist && (dx != 0 || dy != 0)) w *= 0.3; 
                valid_moves.push_back(target); 
                weights.push_back(w); 
            } 
        } 
    } 

    if (!valid_moves.empty()) { 
        std::discrete_distribution<int> dist(weights.begin(), weights.end()); 
        zom->pos = valid_moves[dist(rng)]; 
        if (mine_grid[zom->pos.x][zom->pos.y]) { 
            mine_grid[zom->pos.x][zom->pos.y] = false; 
            trigger_explosion(zom->pos.x, zom->pos.y); 
        } 
    } 
    check_fire_interactions();
}

void GameState::handle_weapon_click(int tx, int ty, float cellSize, float boardOffset) { 
    if (human.stamina < 1) { 
        input_mode = InputMode::MoveMode; 
        return; 
    } 
    Position target{tx, ty}; 
    sf::Vector2f hCenter = getCellCenter(human.pos.x, human.pos.y, cellSize, boardOffset); 
    sf::Vector2f tCenter = getCellCenter(tx, ty, cellSize, boardOffset); 

    if (input_mode == InputMode::TargetKnife) { 
        if (distance(human.pos, target) <= 1) { 
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == target) { 
                    zombies[i]->hp -= 1; 
                    floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                    human.stamina -= 1; 
                    active_fx.type = FXType::Knife; 
                    active_fx.timer = 0.2f; 
                    active_fx.max_duration = 0.2f; 
                    active_fx.start_p = hCenter; 
                    active_fx.end_p = tCenter; 
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) trigger_explosion(target.x, target.y); 
                    input_mode = InputMode::MoveMode; 
                    check_victory_conditions(); 
                    return; 
                } 
            } 
        } 
    } else if (input_mode == InputMode::TargetPistol) { 
        if (human.pistol_ammo <= 0) { input_mode = InputMode::MoveMode; return; } 
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        human.pistol_ammo -= 1; 
        human.stamina -= 1; 
        Position hit_pos = human.pos; 
        bool hit_something = false; 

        for (int step = 1; step <= 5; ++step) { 
            int cx = human.pos.x + vx * step; 
            int cy = human.pos.y + vy * step; 
            if (cx < 0 || cx >= width || cy < 0 || cy >= height || grid[cx][cy] == Terrain::Wall || grid[cx][cy] == Terrain::Obstacle) { 
                hit_pos = {cx, cy}; break; 
            } 
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) { 
                    hit_pos = {cx, cy}; hit_something = true; break; 
                } 
            } 
            if (hit_something) break; 
            hit_pos = {cx, cy}; 
        } 

        active_fx.type = FXType::Pistol; 
        active_fx.timer = 0.25f; 
        active_fx.max_duration = 0.25f; 
        active_fx.start_p = hCenter; 
        active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset); 

        for (size_t i = 0; i < zombies.size(); ++i) { 
            if (zombies[i]->hp > 0 && zombies[i]->pos == hit_pos) { 
                int dist = distance(human.pos, hit_pos); 
                double chance = (dist <= 2) ? 1.0 : (dist == 3 ? 0.7 : (dist == 4 ? 0.4 : 0.0)); 
                std::uniform_real_distribution<double> dist_chance(0.0, 1.0); 
                if (dist_chance(rng) <= chance) { 
                    zombies[i]->hp -= 1; 
                    floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) trigger_explosion(hit_pos.x, hit_pos.y); 
                } 
                break; 
            } 
        } 
    } else if (input_mode == InputMode::TargetShotgun) { 
        if (human.shotgun_ammo <= 0) { input_mode = InputMode::MoveMode; return; } 
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        human.shotgun_ammo -= 1; 
        human.stamina -= 1; 
        active_fx.type = FXType::Shotgun; 
        active_fx.timer = 0.35f; 
        active_fx.max_duration = 0.35f; 
        active_fx.blast_cells.clear(); 
        active_fx.start_p = hCenter; 

        int px = -vy, py = vx; 
        for (int step = 1; step <= 3; ++step) { 
            int cx = human.pos.x + vx * step; 
            int cy = human.pos.y + vy * step; 
            int spread = step - 1; 
            for (int s = -spread; s <= spread; ++s) { 
                int fx = cx + px * s; 
                int fy = cy + py * s; 
                if (fx >= 0 && fx < width && fy >= 0 && fy < height) active_fx.blast_cells.push_back({fx, fy}); 
            } 
        } 

        for (auto p : active_fx.blast_cells) { 
            if (grid[p.x][p.y] == Terrain::Wall || grid[p.x][p.y] == Terrain::Obstacle) continue; 
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == p) { 
                    zombies[i]->hp -= 1; 
                    floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                    int rx = p.x + vx, ry = p.y + vy; 
                    if (rx >= 0 && rx < width && ry >= 0 && ry < height && grid[rx][ry] != Terrain::Wall && grid[rx][ry] != Terrain::Obstacle) { 
                        zombies[i]->pos = {rx, ry}; 
                    } else { 
                        zombies[i]->hp -= 1; 
                        floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                    } 
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y); 
                } 
            } 
        } 
    } else if (input_mode == InputMode::TargetGrenade) { 
        if (human.grenades <= 0) { input_mode = InputMode::MoveMode; return; } 
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        human.grenades -= 1; 
        human.stamina -= 1; 
        std::uniform_int_distribution<int> dist_steps(2, 4); 
        int total_steps = dist_steps(rng); 
        int cx = human.pos.x, cy = human.pos.y; 

        for (int step = 1; step <= total_steps; ++step) { 
            int nx = cx + vx, ny = cy + vy; 
            if (nx < 0 || nx >= width || ny < 0 || ny >= height || grid[nx][ny] == Terrain::Wall || grid[nx][ny] == Terrain::Obstacle) break; 
            cx = nx; cy = ny; 
        } 
        grenade_box.active = true; 
        grenade_box.pos = {cx, cy}; 
        grenade_box.turns_left = 2; // Explodes after 2 ZombieAnimating phases
    } else if (input_mode == InputMode::TargetMolotov) {
        if (human.molotovs <= 0) { input_mode = InputMode::MoveMode; return; }
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        
        human.molotovs -= 1; human.stamina -= 1;
        std::uniform_int_distribution<int> dist_steps(1, 6); 
        int max_steps = dist_steps(rng); 
        
        Position hit_pos = human.pos;
        bool hit_zombie = false;
        
        for (int step = 1; step <= max_steps; ++step) { 
            int cx = human.pos.x + vx * step; 
            int cy = human.pos.y + vy * step; 
            if (cx < 0 || cx >= width || cy < 0 || cy >= height || grid[cx][cy] == Terrain::Wall || grid[cx][cy] == Terrain::Obstacle) break; 
            hit_pos = {cx, cy}; 
            
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == hit_pos) { 
                    hit_zombie = true; 
                    zombies[i]->is_burning = true;
                    add_log("-> Molotov hit " + zombies[i]->name + " directly! Target ignited.", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    break; 
                } 
            } 
            if (hit_zombie) break;
        }
        
        active_fx.type = FXType::Molotov; 
        active_fx.timer = 0.35f; 
        active_fx.max_duration = 0.35f; 
        active_fx.start_p = hCenter; 
        active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset);
        
        if (grid[hit_pos.x][hit_pos.y] == Terrain::Water) {
            add_log("-> Molotov landed in water. Fizzled out!", ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        } else if (grid[hit_pos.x][hit_pos.y] == Terrain::Dirt) {
            grid[hit_pos.x][hit_pos.y] = Terrain::Fire;
            fire_cells.push_back({hit_pos, 1}); 
            add_log("-> Molotov created a fire zone!", ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
        }
        check_fire_interactions();
    }
    input_mode = InputMode::MoveMode; 
    check_victory_conditions(); 
}

void GameState::start_zombie_phase() { 
    if (game_over || game_won) return; 
    
    if (human.is_burning) {
        human.hp = std::max(0, human.hp - 1);
        floating_texts.push_back({human.pos, -1, 1.0f, 1.0f});
        human.is_burning = false;
        add_log("[FIRE] Human suffered 1 Burn Damage!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    }
    
    for (auto it = fire_cells.begin(); it != fire_cells.end(); ) {
        if (it->duration <= 0) {
            if (grid[it->pos.x][it->pos.y] == Terrain::Fire) grid[it->pos.x][it->pos.y] = Terrain::Dirt;
            it = fire_cells.erase(it);
        } else {
            it->duration--;
            ++it;
        }
    }
    check_victory_conditions();
    if (game_over || game_won) return;
    
    phase = TurnPhase::ZombieAnimating; 
    active_zombie_idx = 0; 
    active_zombie_substep = 0; 
    zombie_action_timer = 0.0f; 
}

void GameState::update_zombie_logic(float dt) { 
    if (phase != TurnPhase::ZombieAnimating || active_fx.type != FXType::None || !attack_animations.empty()) return; 
    
    if (active_zombie_idx >= zombies.size()) { 
        for (auto& z : zombies) {
            if (z->hp > 0 && z->is_burning) {
                z->hp -= 1;
                floating_texts.push_back({z->pos, -1, 1.0f, 1.0f});
                z->is_burning = false;
                add_log("[FIRE] " + z->name + " suffered 1 Burn Damage!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                if (z->hp <= 0 && z->type == ZombieType::Exploding) trigger_explosion(z->pos.x, z->pos.y);
            }
        }
        
        if (grenade_box.active) {
            grenade_box.turns_left--;
            if (grenade_box.turns_left <= 0) {
                trigger_explosion(grenade_box.pos.x, grenade_box.pos.y);
                grenade_box.active = false;
            }
        }
        
        if (!game_over) { 
            current_turn++; 
            std::uniform_int_distribution<int> dist_stam(2, 6); 
            human.stamina = dist_stam(rng); 
            
            if (grid[human.pos.x][human.pos.y] == Terrain::Fire && !human.is_burning) {
                human.is_burning = true;
                add_log("[FIRE] Human started turn standing in fire!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            }
        } 
        phase = TurnPhase::HumanTurn; 
        check_victory_conditions(); 
        return; 
    } 
    auto& zom = zombies[active_zombie_idx]; 
    if (zom->hp <= 0 || game_over) { 
        active_zombie_idx++; active_zombie_substep = 0; return; 
    } 
    zombie_action_timer += dt; 
    if (zombie_action_timer >= std::max(0.05f, ZOMBIE_STEP_DELAY)) { 
        zombie_action_timer = 0.0f; 
        zombie_single_step(active_zombie_idx); 
        
        if (zom->hp > 0) { 
            int dx = std::abs(zom->pos.x - human.pos.x);
            int dy = std::abs(zom->pos.y - human.pos.y);
            
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                int dmg = 0;
                VisualFX atk_fx;
                atk_fx.cx = human.pos.x;
                atk_fx.cy = human.pos.y;
                atk_fx.timer = 0.5f;
                atk_fx.max_duration = 0.5f;

                if (dx + dy == 1) { // Orthogonal -> Bite
                    dmg = 2;
                    atk_fx.type = FXType::Bite;
                    add_log("-> " + zom->name + " BITES human! -2 HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                } else { // Diagonal -> Scratch
                    dmg = 1;
                    atk_fx.type = FXType::Scratch;
                    add_log("-> " + zom->name + " SCRATCHES human! -1 HP.", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                }
                
                human.hp = std::max(0, human.hp - dmg); 
                floating_texts.push_back({human.pos, -dmg, 1.0f, 1.0f});
                attack_animations.push_back(atk_fx);

                if (zom->type == ZombieType::Vampire) { 
                    zom->hp += 1; 
                    floating_texts.push_back({zom->pos, 1, 1.0f, 1.0f});
                } 
                check_victory_conditions(); 
            }
        } 
        active_zombie_substep++; 
        if (active_zombie_substep >= zom->getMovesPerTurn()) { 
            active_zombie_idx++; active_zombie_substep = 0; 
        } 
    } 
}

void GameState::check_victory_conditions() { 
    if (human.hp <= 0) game_over = true; 
    else { 
        bool all_dead = true; 
        for (const auto& z : zombies) { if (z->hp > 0) { all_dead = false; break; } } 
        if (all_dead) game_won = true; 
    } 
}
