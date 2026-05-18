#include "GameState.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>

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
                if (t == Terrain::Wall ) continue;
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
                                  active_config.count_exploding + active_config.count_vampire +
                                  active_config.count_sick;
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
        active_config.count_exploding = 1; active_config.count_vampire = 0; active_config.count_sick = 1;
    } 
    else if (level == 1) { 
        active_config.human_hp = 5;
        active_config.initial_stamina = 6;
        active_config.pistol_ammo = 12; active_config.shotgun_ammo = 6;
        active_config.grenades = 3;     active_config.mines = 2;
        active_config.molotovs = 3;
        active_config.count_normal = 4; active_config.count_fast = 3;
        active_config.count_exploding = 2; active_config.count_vampire = 1; active_config.count_sick = 2;
    } 
    else { 
        active_config.human_hp = 3;
        active_config.initial_stamina = 5;
        active_config.pistol_ammo = 6;  active_config.shotgun_ammo = 3;
        active_config.grenades = 1;     active_config.mines = 1;
        active_config.molotovs = 1;
        active_config.count_normal = 6; active_config.count_fast = 5;
        active_config.count_exploding = 4; active_config.count_vampire = 2; active_config.count_sick = 3;
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
    outFile << "ZOM_SICK "        << active_config.count_sick << "\n";
    outFile << "SHIELD "          << (active_config.spawn_shield ? 1 : 0) << "\n";
    outFile << "CUSTOM_MAP "      << (active_config.custom_map_mode ? 1 : 0) << "\n";
    outFile << "ENABLE_ENV "      << (active_config.enable_environment ? 1 : 0) << "\n";
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
        else if (key == "ZOM_SICK")     active_config.count_sick = val;
        else if (key == "SHIELD")       active_config.spawn_shield = (val == 1);
        else if (key == "CUSTOM_MAP")   active_config.custom_map_mode = (val == 1);
        else if (key == "ENABLE_ENV")   active_config.enable_environment = (val == 1);
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
    active_grenades.clear();
    active_fx = VisualFX{};
    dark_cloud_active = false;
    last_environment_event = "Clear skies";
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
    human.is_paralyzed = false;

    mine_grid.assign(width, std::vector<bool>(height, false));

    if (active_config.custom_map_mode) {
        if (active_config.custom_grid.size() != static_cast<size_t>(width) || 
            active_config.custom_grid[0].size() != static_cast<size_t>(height)) {
            active_config.custom_grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
            active_config.custom_human_pos = {1, 1};
        }
        grid = active_config.custom_grid;
        human.pos = active_config.custom_human_pos;
        if (grid[human.pos.x][human.pos.y] == Terrain::Wall) {
            grid[human.pos.x][human.pos.y] = Terrain::Dirt;
        }
    } 
    else {
        grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
        std::uniform_int_distribution<int> dist_x(1, width - 2);
        std::uniform_int_distribution<int> dist_y(1, height - 2);

        int total_cells = width * height;
        for (int i = 0; i < total_cells / 15; ++i) { grid[dist_x(rng)][dist_y(rng)] = Terrain::Wall; }
        for (int i = 0; i < total_cells / 30; ++i) { grid[dist_x(rng)][dist_y(rng)] = Terrain::Water; }

        human.pos = {dist_x(rng), dist_y(rng)};
        while (grid[human.pos.x][human.pos.y] == Terrain::Wall) {
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
                if (grid[z_pos.x][z_pos.y] == Terrain::Wall) invalid_pos = true;
                if (z_pos == human.pos) invalid_pos = true;
                if (active_config.spawn_shield && std::abs(z_pos.x - human.pos.x) <= 3 && std::abs(z_pos.y - human.pos.y) <= 3) invalid_pos = true;
                for (const auto& z : zombies) { if (z->pos == z_pos) { invalid_pos = true; break; } }
                if (!invalid_pos || attempts > 200) break;
                z_pos = {dist_x(rng), dist_y(rng)};
                attempts++;
            }
            if (grid[z_pos.x][z_pos.y] == Terrain::Wall) {
                grid[z_pos.x][z_pos.y] = Terrain::Dirt;
            }
            if (z_type == ZombieType::Normal) zombies.push_back(std::make_unique<NormalZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Fast) zombies.push_back(std::make_unique<FastZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Exploding) zombies.push_back(std::make_unique<ExplodingZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Vampire) zombies.push_back(std::make_unique<VampireZombie>(z_pos, max_hp, name, z_type));
            else if (z_type == ZombieType::Sick) zombies.push_back(std::make_unique<SickZombie>(z_pos, max_hp, name, z_type));
        }
    };

    spawn_zombie_lambda(ZombieType::Normal, active_config.count_normal, "Normal Zom", 2);
    spawn_zombie_lambda(ZombieType::Fast, active_config.count_fast, "Fast Sprinter", 2);
    spawn_zombie_lambda(ZombieType::Exploding, active_config.count_exploding, "Exploder", 3);
    spawn_zombie_lambda(ZombieType::Vampire, active_config.count_vampire, "Vampire Dracula", 4);
    spawn_zombie_lambda(ZombieType::Sick, active_config.count_sick, "Sick Carrier", 2);

    add_log("Tactical Battleground initialized with dynamic safety boundaries!", ImVec4(0, 1, 1, 1));
    add_log("=== HUMAN TURN " + std::to_string(current_turn) + " START ===", ImVec4(1.0f, 0.95f, 0.25f, 1.0f));
    turn_banner_fx.type = FXType::Electricity;
    turn_banner_fx.timer = 1.5f;
    turn_banner_fx.max_duration = 1.5f;
}

void GameState::add_log(const std::string& text, ImVec4 color) { 
    std::string out = text;
    if (use_vietnamese) {
        static const std::unordered_map<std::string, std::string> exact = {
            {"Tactical Battleground initialized with dynamic safety boundaries!", "Chien truong da khoi tao thanh cong voi ranh gioi an toan dong."},
            {"[ENV] Clear skies. Nothing happens.", "[MT] Troi quang. Khong co gi xay ra."},
            {"-> Human is struck by lightning! -1 HP.", "-> Human bi set danh! -1 HP."},
            {"-> Molotov landed in water. Fizzled out!", "-> Bom xang roi xuong nuoc va tat."},
            {"-> Molotov created a fire zone!", "-> Bom xang tao ra vung lua!"},
            {"[RADIO] Pistol shot hits no target.", "[RADIO] Phat ban luc khong trung muc tieu."}
        };
        auto it = exact.find(text);
        if (it != exact.end()) out = it->second;
        else if (text.rfind("=== HUMAN TURN ", 0) == 0 && text.find(" START ===") != std::string::npos) {
            std::string turn = text.substr(14, text.find(" START ===") - 14);
            out = "=== BAT DAU LUOT HUMAN " + turn + " ===";
        } else if (text.rfind("[RADIO] Pistol round lands on Zombie #", 0) == 0) {
            out = "[RADIO] Dan luc trung Zombie #" + text.substr(37);
        } else if (text.rfind("[RADIO] Pistol round misses Zombie #", 0) == 0) {
            out = "[RADIO] Dan luc hut Zombie #" + text.substr(38);
        } else if (text.rfind("[RADIO] Blade connects with Zombie #", 0) == 0) {
            out = "[RADIO] Dao da trung Zombie #" + text.substr(34);
        } else if (text.rfind("[RADIO] Shotgun blast rips through Zombie #", 0) == 0) {
            out = "[RADIO] Dan shotgun xe nat Zombie #" + text.substr(41);
        } else if (text.rfind("[RADIO] Zombie #", 0) == 0 && text.find(" collided with Zombie #") != std::string::npos) {
            size_t pos1 = 16;
            size_t pos2 = text.find(" collided with Zombie #");
            std::string z1 = text.substr(pos1, pos2 - pos1);
            size_t pos3 = pos2 + 23;
            size_t pos4 = text.find(". Both take 1 collision damage.");
            std::string z2 = text.substr(pos3, pos4 - pos3);
            out = "[RADIO] Zombie #" + z1 + " va cham voi Zombie #" + z2 + ". Ca hai nhan 1 sat thuong va dap.";
        } else if (text.rfind("[RADIO] Shotgun recoil pushes Human back 1 tile.", 0) == 0) {
            out = "[RADIO] Luc giat shotgun day lui Human 1 o.";
        } else if (text.rfind("[RADIO] Recoil impact! Human loses 1 HP and the wall is destroyed.", 0) == 0) {
            out = "[RADIO] Va dap luc giat! Human mat 1 HP va o tuong bi pha huy.";
        } else if (text.rfind("[EXPLOSION] Wall at (", 0) == 0 && text.find(") destroyed.") != std::string::npos) {
            size_t pos1 = 21;
            size_t pos2 = text.find(") destroyed.");
            std::string coords = text.substr(pos1, pos2 - pos1);
            out = "[RADIO] Tuong tai (" + coords + ") bi pha huy boi vu no.";
        } else if (text.rfind("[RADIO] Grenade timer expires. Detonation!", 0) == 0) {
            out = "[RADIO] Het thoi gian no cua luu dan. Kich no!";
        } else if (text.rfind("[FIRE] ", 0) == 0) {
            out = "[LUA] " + text.substr(7);
        } else if (text.rfind("[ENV] ", 0) == 0) {
            out = "[MT] " + text.substr(6);
        } else if (text.rfind("[SHOCK] ", 0) == 0) {
            out = "[DIEN] " + text.substr(8);
        }
    }
    logs.push_back({out, color}); 
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

bool GameState::is_blocked_by_wall(Position start, Position end) const {
    int dx = end.x - start.x;
    int dy = end.y - start.y;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps <= 1) return false;
    
    for (int i = 1; i < steps; ++i) {
        float t = (float)i / steps;
        int cx = std::round(start.x + dx * t);
        int cy = std::round(start.y + dy * t);
        if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
            if (grid[cx][cy] == Terrain::Wall) {
                return true;
            }
        }
    }
    return false;
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

void GameState::check_mine_interactions() {
    if (human.hp > 0 && mine_grid[human.pos.x][human.pos.y]) {
        mine_grid[human.pos.x][human.pos.y] = false;
        add_log("[RADIO] WATCH OUT! Human stepped on a mine. Stand by for detonation!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        trigger_explosion(human.pos.x, human.pos.y);
    }
}


bool GameState::is_blocking_cell(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return true;
    return grid[x][y] == Terrain::Wall ;
}

bool GameState::has_living_entity_at(Position p) const {
    if (human.hp > 0 && human.pos == p) return true;
    for (const auto& z : zombies) {
        if (z->hp > 0 && z->pos == p) return true;
    }
    return false;
}

bool GameState::is_conductive_cell(Position p) const {
    if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) return false;
    return grid[p.x][p.y] == Terrain::Water || has_living_entity_at(p);
}

std::vector<Position> GameState::get_conductive_cluster(Position start) const {
    std::vector<Position> cluster;
    if (!is_conductive_cell(start)) return cluster;

    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::queue<Position> q;
    q.push(start);
    visited[start.x][start.y] = true;

    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!q.empty()) {
        Position cur = q.front();
        q.pop();
        cluster.push_back(cur);

        for (const auto& d : dirs) {
            Position next{cur.x + d[0], cur.y + d[1]};
            if (next.x < 0 || next.x >= width || next.y < 0 || next.y >= height) continue;
            if (visited[next.x][next.y] || !is_conductive_cell(next)) continue;
            visited[next.x][next.y] = true;
            q.push(next);
        }
    }
    return cluster;
}

void GameState::apply_windstorm(int dx, int dy) {
    last_environment_event = "Windstorm";
    active_fx.type = FXType::Wind;
    active_fx.timer = 1.6f;
    active_fx.max_duration = 1.6f;
    active_fx.dx = dx;
    active_fx.dy = dy;
    active_fx.blast_cells.clear();

    struct EntityRef { bool human; size_t idx; Position pos; };
    std::vector<EntityRef> refs;
    if (human.hp > 0) refs.push_back({true, 0, human.pos});
    for (size_t i = 0; i < zombies.size(); ++i) {
        if (zombies[i]->hp > 0) refs.push_back({false, i, zombies[i]->pos});
    }

    std::sort(refs.begin(), refs.end(), [dx, dy](const EntityRef& a, const EntityRef& b) {
        return a.pos.x * dx + a.pos.y * dy > b.pos.x * dx + b.pos.y * dy;
    });

    auto initially_occupied = [&](Position p, const EntityRef& self) {
        for (const auto& other : refs) {
            if (other.human == self.human && other.idx == self.idx) continue;
            if (other.pos == p) return true;
        }
        return false;
    };
    auto currently_occupied = [&](Position p, const EntityRef& self) {
        if (!self.human && human.hp > 0 && human.pos == p) return true;
        for (size_t i = 0; i < zombies.size(); ++i) {
            if (zombies[i]->hp > 0 && (self.human || self.idx != i) && zombies[i]->pos == p) return true;
        }
        return false;
    };

    int moved = 0;
    for (const auto& ref : refs) {
        Position current = ref.human ? human.pos : zombies[ref.idx]->pos;
        Position original = ref.pos;
        Position back{original.x - dx, original.y - dy};
        Position front{original.x + dx, original.y + dy};
        Position target{current.x + dx, current.y + dy};
        if (is_blocking_cell(front.x, front.y) || initially_occupied(front, ref)) continue;
        if (is_blocking_cell(back.x, back.y) || initially_occupied(back, ref)) continue;
        if (is_blocking_cell(target.x, target.y) || currently_occupied(target, ref)) continue;

        if (ref.human) human.pos = target;
        else zombies[ref.idx]->pos = target;
        active_fx.blast_cells.push_back(front);
        moved++;
    }

    int grenades_blown = 0;
    for (auto& g : active_grenades) {
        if (g.active) {
            Position g_target{g.pos.x + dx, g.pos.y + dy};
            if (!is_blocking_cell(g_target.x, g_target.y)) {
                g.pos = g_target;
                grenades_blown++;
            }
        }
    }

    std::string wind_log = "[ENV] Windstorm blows " + std::to_string(dx) + "," + std::to_string(dy) + " and pushes " + std::to_string(moved) + " entities";
    if (grenades_blown > 0) {
        wind_log += " and " + std::to_string(grenades_blown) + " grenade(s)";
    }
    wind_log += ".";
    add_log(wind_log, ImVec4(0.65f, 0.85f, 1.0f, 1.0f));
    check_fire_interactions();
}

void GameState::apply_heavy_rain() {
    last_environment_event = "Heavy rain";
    active_fx.type = FXType::Rain;
    active_fx.timer = 1.6f;
    active_fx.max_duration = 1.6f;
    active_fx.blast_cells.clear();

    std::vector<Position> soaked;
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] != Terrain::Dirt) continue;
            bool next_to_water = false;
            for (const auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[nx][ny] == Terrain::Water) {
                    next_to_water = true;
                    break;
                }
            }
            float threshold = next_to_water ? 0.35f : 0.08f;
            if (chance(rng) < threshold) soaked.push_back({x, y});
        }
    }
    for (const auto& p : soaked) grid[p.x][p.y] = Terrain::Water;
    active_fx.blast_cells = soaked;
    add_log("[ENV] Heavy rain floods " + std::to_string(soaked.size()) + " dirt cells.", ImVec4(0.35f, 0.65f, 1.0f, 1.0f));
}

void GameState::apply_dark_clouds() {
    last_environment_event = "Dark clouds";
    dark_cloud_active = true;
    active_fx.type = FXType::DarkCloud;
    active_fx.timer = 1.6f;
    active_fx.max_duration = 1.6f;
    add_log("[ENV] Dark clouds cover the board until the next environment turn.", ImVec4(0.45f, 0.45f, 0.65f, 1.0f));
}

void GameState::apply_lightning_strike() {
    last_environment_event = "Lightning";
    std::vector<Position> cells;
    std::vector<double> weights;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (is_blocking_cell(x, y)) continue;
            Position p{x, y};
            bool entity = has_living_entity_at(p);
            double w = 1.0;
            if (grid[x][y] == Terrain::Water) w *= 4.0;
            if (entity) w *= 2.0;
            cells.push_back(p);
            weights.push_back(w);
        }
    }
    if (cells.empty()) return;

    std::discrete_distribution<int> pick(weights.begin(), weights.end());
    Position strike = cells[pick(rng)];
    std::vector<Position> conductive_cluster = get_conductive_cluster(strike);
    active_fx.type = FXType::Lightning;
    active_fx.timer = 1.8f;
    active_fx.max_duration = 1.8f;
    active_fx.cx = strike.x;
    active_fx.cy = strike.y;
    active_fx.blast_cells = conductive_cluster;

    add_log("[ENV] Lightning strikes (" + std::to_string(strike.x) + ", " + std::to_string(strike.y) + ")!", ImVec4(1.0f, 1.0f, 0.35f, 1.0f));

    if (human.hp > 0 && human.pos == strike) {
        human.hp = std::max(0, human.hp - 1);
        floating_texts.push_back({human.pos, -1, 1.0f, 1.0f});
        add_log("-> Human is struck by lightning! -1 HP.", ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
    }
    for (auto& z : zombies) {
        if (z->hp > 0 && z->pos == strike) {
            z->hp -= 1;
            floating_texts.push_back({z->pos, -1, 1.0f, 1.0f});
            add_log("-> " + z->name + " is struck by lightning! -1 HP.", ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
            if (z->hp <= 0 && z->type == ZombieType::Exploding) trigger_explosion(z->pos.x, z->pos.y);
        }
    }

    for (const auto& p : conductive_cluster) {
        if (human.hp > 0 && human.pos == p) human.is_paralyzed = true;
        for (auto& z : zombies) {
            if (z->hp > 0 && z->pos == p) z->is_paralyzed = true;
        }
    }
    if (!conductive_cluster.empty()) {
        add_log("-> Electricity spreads through " + std::to_string(conductive_cluster.size()) + " conductive cells; occupants are paralyzed next action.", ImVec4(0.45f, 0.9f, 1.0f, 1.0f));
    }
    check_victory_conditions();
}

void GameState::resolve_environment_turn() {
    if (dark_cloud_active) dark_cloud_active = false;

    std::discrete_distribution<int> event_dist({58, 16, 14, 4, 8});
    int event = event_dist(rng);
    if (event == 0) {
        last_environment_event = "Clear skies";
        add_log("[ENV] Clear skies. Nothing happens.", ImVec4(0.6f, 0.75f, 0.85f, 1.0f));
        active_fx = VisualFX{};
    } else if (event == 1) {
        const std::vector<std::pair<int, int>> dirs{{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}};
        std::uniform_int_distribution<int> dir_dist(0, static_cast<int>(dirs.size()) - 1);
        auto [dx, dy] = dirs[dir_dist(rng)];
        apply_windstorm(dx, dy);
    } else if (event == 2) {
        apply_heavy_rain();
    } else if (event == 3) {
        apply_dark_clouds();
    } else {
        apply_lightning_strike();
    }
}

void GameState::start_environment_phase() {
    if (game_over || game_won || !active_config.enable_environment) return;
    phase = TurnPhase::EnvironmentAnimating;
    environment_action_timer = 0.0f;
    resolve_environment_turn();
}

void GameState::finish_environment_phase() {
    if (!game_over) {
        current_turn++;
        std::uniform_int_distribution<int> dist_stam(1, 6);
        human.stamina = human.is_paralyzed ? 0 : dist_stam(rng);
        if (human.is_paralyzed) {
            add_log("[SHOCK] Human is paralyzed this turn and cannot act.", ImVec4(0.45f, 0.9f, 1.0f, 1.0f));
        }
        if (grid[human.pos.x][human.pos.y] == Terrain::Fire && !human.is_burning) {
            human.is_burning = true;
            add_log("[FIRE] Human started turn standing in fire!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        }
    }

    phase = TurnPhase::HumanTurn;
    add_log("=== HUMAN TURN " + std::to_string(current_turn) + " START ===", ImVec4(1.0f, 0.95f, 0.25f, 1.0f));
    turn_banner_fx.type = FXType::Electricity;
    turn_banner_fx.timer = 1.5f;
    turn_banner_fx.max_duration = 1.5f;
    check_victory_conditions();
}

void GameState::update_environment_logic(float dt) {
    if (phase != TurnPhase::EnvironmentAnimating) return;
    
    // If a detonated explosion is currently animating, wait for it!
    if (active_fx.type != FXType::None && environment_action_timer >= ENVIRONMENT_STEP_DELAY) {
        return;
    }
    
    environment_action_timer += dt;
    
    if (active_fx.type == FXType::None || environment_action_timer >= ENVIRONMENT_STEP_DELAY) {
        // Environment event has finished! Now tick grenades before starting human turn.
        bool triggered_explosion = false;
        for (auto& g : active_grenades) {
            if (g.active) {
                g.turns_left--;
                if (g.turns_left <= 0) {
                    add_log("[RADIO] Grenade timer expires. Detonation!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
                    trigger_explosion(g.pos.x, g.pos.y);
                    g.active = false;
                    triggered_explosion = true;
                }
            }
        }
        active_grenades.erase(std::remove_if(active_grenades.begin(), active_grenades.end(), [](const GrenadeTimer& g) { return !g.active; }), active_grenades.end());
        
        if (triggered_explosion) {
            environment_action_timer = ENVIRONMENT_STEP_DELAY; // lock environment timer to wait for explosion
            return;
        }
        
        finish_environment_phase();
    }
}

void GameState::trigger_explosion(int cx, int cy) { 
    add_log("[EXPLOSION] Detonated at (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f)); 
    
    Terrain center_t = grid[cx][cy];
    int radius = (center_t == Terrain::Water) ? 1 : 2;

    active_fx.type = FXType::Explosion; 
    active_fx.timer = 2.0f; 
    active_fx.max_duration = 2.0f; 
    active_fx.blast_cells.clear(); 

    for (int dx = -radius; dx <= radius; ++dx) { 
        for (int dy = -radius; dy <= radius; ++dy) { 
            int nx = cx + dx, ny = cy + dy; 
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) { 
                if (is_blocked_by_wall(Position{cx, cy}, Position{nx, ny})) continue;
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
    
    // Destroy adjacent wall cells to the explosion center
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            int nx = cx + dx, ny = cy + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                if (grid[nx][ny] == Terrain::Wall) {
                    grid[nx][ny] = Terrain::Dirt;
                    add_log("[EXPLOSION] Wall at (" + std::to_string(nx + 1) + ", " + std::to_string(ny + 1) + ") destroyed.", ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
                }
            }
        }
    }

    check_victory_conditions(); 
}

void GameState::zombie_single_step(size_t idx) { 
    if (game_over || game_won) return; 
    auto& zom = zombies[idx]; 
    if (zom->is_paralyzed) return;
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
                if (grid[nx][ny] == Terrain::Wall ) continue; 
                
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
            add_log("[RADIO] Zombie #" + std::to_string(idx + 1) + " stepped on a mine. Stand by for detonation!", ImVec4(1.0f, 0.38f, 0.22f, 1.0f));
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
                    add_log("[RADIO] Blade connects with Zombie #" + std::to_string(i + 1) + ".", ImVec4(0.35f, 1.0f, 0.7f, 1.0f));
                    active_fx.type = FXType::Knife; 
                    active_fx.timer = 0.35f; 
                    active_fx.max_duration = 0.35f; 
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
            if (cx < 0 || cx >= width || cy < 0 || cy >= height) { 
                break; 
            } 
            if (grid[cx][cy] == Terrain::Wall) {
                hit_pos = {cx, cy};
                break;
            }
            hit_pos = {cx, cy}; 
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) { 
                    hit_something = true; break; 
                } 
            } 
            if (hit_something) break; 
        } 

        active_fx.type = FXType::Pistol; 
        active_fx.timer = 0.45f; 
        active_fx.max_duration = 0.45f; 
        active_fx.start_p = hCenter; 
        active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset); 

        bool any_target = false;
        for (size_t i = 0; i < zombies.size(); ++i) { 
            if (zombies[i]->hp > 0 && zombies[i]->pos == hit_pos) { 
                any_target = true;
                int dist = distance(human.pos, hit_pos); 
                double chance = (dist <= 2) ? 1.0 : (dist == 3 ? 0.7 : (dist == 4 ? 0.4 : 0.0)); 
                std::uniform_real_distribution<double> dist_chance(0.0, 1.0); 
                if (dist_chance(rng) <= chance) { 
                    zombies[i]->hp -= 1; 
                    floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                    add_log("[RADIO] Pistol round lands on Zombie #" + std::to_string(i + 1) + ".", ImVec4(1.0f, 0.95f, 0.35f, 1.0f));
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) trigger_explosion(hit_pos.x, hit_pos.y); 
                } else {
                    add_log("[RADIO] Pistol round misses Zombie #" + std::to_string(i + 1) + ".", ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                }
                break; 
            } 
        }
        if (!any_target) add_log("[RADIO] Pistol shot hits no target.", ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    } else if (input_mode == InputMode::TargetShotgun) { 
        if (human.shotgun_ammo <= 0) { input_mode = InputMode::MoveMode; return; } 
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        human.shotgun_ammo -= 1; 
        human.stamina -= 1; 
        active_fx.type = FXType::Shotgun; 
        active_fx.timer = 0.65f; 
        active_fx.max_duration = 0.65f; 
        active_fx.blast_cells.clear(); 
        active_fx.start_p = hCenter; 

        // CHECK IF THE FIRST CELL (PEAK OF CONE) IS A WALL!
        int first_x = human.pos.x + vx;
        int first_y = human.pos.y + vy;
        bool is_first_cell_wall = false;
        if (first_x >= 0 && first_x < width && first_y >= 0 && first_y < height) {
            if (grid[first_x][first_y] == Terrain::Wall) {
                is_first_cell_wall = true;
            }
        }

        if (is_first_cell_wall) {
            // Recoil target (1 step opposite to fire direction)
            int rx = human.pos.x - vx;
            int ry = human.pos.y - vy;
            
            bool can_recoil = false;
            if (rx >= 0 && rx < width && ry >= 0 && ry < height) {
                if (grid[rx][ry] != Terrain::Wall) {
                    bool zombie_behind = false;
                    for (const auto& z : zombies) {
                        if (z->hp > 0 && z->pos == Position{rx, ry}) {
                            zombie_behind = true;
                            break;
                        }
                    }
                    if (!zombie_behind) {
                        can_recoil = true;
                    }
                }
            }
            
            grid[first_x][first_y] = Terrain::Dirt;
            if (can_recoil) {
                human.pos = {rx, ry};
                add_log("[RADIO] Shotgun recoil pushes Human back 1 tile.", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                check_fire_interactions();
                check_mine_interactions();
            } else {
                human.hp = std::max(0, human.hp - 1);
                floating_texts.push_back({human.pos, -1, 1.0f, 1.0f});
                add_log("[RADIO] Recoil impact! Human loses 1 HP and the wall is destroyed.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            }
            
            // Recoil completely blocks the blast: exit early
            input_mode = InputMode::MoveMode;
            check_victory_conditions();
            return;
        }

        if (vx != 0 && vy != 0) {
            // Diagonal shot: Half of 4x4 square with vertex at (1,1) relative to player, reaching (1,4) and (4,1)
            for (int dx_step = 0; dx_step <= 3; ++dx_step) {
                for (int dy_step = 0; dy_step <= 3; ++dy_step) {
                    if (dx_step + dy_step <= 3) {
                        int fx = human.pos.x + vx + dx_step * vx;
                        int fy = human.pos.y + vy + dy_step * vy;
                        if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                            if (!is_blocked_by_wall(human.pos, Position{fx, fy})) {
                                active_fx.blast_cells.push_back({fx, fy});
                            }
                        }
                    }
                }
            }
        } else {
            // Straight shot: 3-step expanding cone (exactly 9 cells)
            int px = -vy, py = vx; 
            for (int step = 1; step <= 3; ++step) { 
                int cx = human.pos.x + vx * step; 
                int cy = human.pos.y + vy * step; 
                int spread = step - 1; 
                for (int s = -spread; s <= spread; ++s) { 
                    int fx = cx + px * s; 
                    int fy = cy + py * s; 
                    if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                        if (!is_blocked_by_wall(human.pos, Position{fx, fy})) {
                            active_fx.blast_cells.push_back({fx, fy});
                        }
                    }
                } 
            }
        } 

        struct HitZombie {
            size_t index;
            int dist_proj;
        };
        std::vector<HitZombie> hit_zombies;
        for (size_t i = 0; i < zombies.size(); ++i) {
            if (zombies[i]->hp <= 0) continue;
            Position z_pos = zombies[i]->pos;
            if (grid[z_pos.x][z_pos.y] == Terrain::Wall) continue;
            
            bool is_in_blast = false;
            for (auto p : active_fx.blast_cells) {
                if (p == z_pos) {
                    is_in_blast = true;
                    break;
                }
            }
            if (is_in_blast) {
                int proj = z_pos.x * vx + z_pos.y * vy;
                hit_zombies.push_back({i, proj});
            }
        }

        std::sort(hit_zombies.begin(), hit_zombies.end(), [](const HitZombie& a, const HitZombie& b) {
            return a.dist_proj > b.dist_proj;
        });

        for (auto hz : hit_zombies) {
            size_t i = hz.index;
            if (zombies[i]->hp <= 0) continue;

            zombies[i]->hp -= 1;
            floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
            add_log("[RADIO] Shotgun blast rips through Zombie #" + std::to_string(i + 1) + ".", ImVec4(1.0f, 0.8f, 0.35f, 1.0f));

            if (zombies[i]->hp <= 0) {
                if (zombies[i]->type == ZombieType::Exploding) {
                    trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                }
                continue;
            }

            int rx = zombies[i]->pos.x + vx;
            int ry = zombies[i]->pos.y + vy;

            if (rx < 0 || rx >= width || ry < 0 || ry >= height || grid[rx][ry] == Terrain::Wall) {
                zombies[i]->hp -= 1;
                floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                    trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                }
            } else {
                size_t blocking_zombie_idx = -1;
                bool is_blocked_by_zombie = false;
                for (size_t k = 0; k < zombies.size(); ++k) {
                    if (zombies[k]->hp > 0 && zombies[k]->pos == Position{rx, ry}) {
                        is_blocked_by_zombie = true;
                        blocking_zombie_idx = k;
                        break;
                    }
                }

                if (is_blocked_by_zombie) {
                    zombies[i]->hp -= 1;
                    floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});

                    zombies[blocking_zombie_idx]->hp -= 1;
                    floating_texts.push_back({zombies[blocking_zombie_idx]->pos, -1, 1.0f, 1.0f});

                    add_log("[RADIO] Zombie #" + std::to_string(i + 1) + " collided with Zombie #" + std::to_string(blocking_zombie_idx + 1) + ". Both take 1 collision damage.", ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                        trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                    }
                    if (zombies[blocking_zombie_idx]->hp <= 0 && zombies[blocking_zombie_idx]->type == ZombieType::Exploding) {
                        trigger_explosion(zombies[blocking_zombie_idx]->pos.x, zombies[blocking_zombie_idx]->pos.y);
                    }
                } else {
                    zombies[i]->pos = {rx, ry};
                }
            }
        } 
    } else if (input_mode == InputMode::TargetGrenade) { 
        if (human.grenades <= 0) { input_mode = InputMode::MoveMode; return; } 
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        human.grenades -= 1; 
        human.stamina -= 1; 
        std::uniform_int_distribution<int> dist_steps(1, 6); 
        int total_steps = dist_steps(rng); 
        
        Position landing_pos = human.pos;
        bool all_walls = true;
        for (int step = 1; step <= total_steps; ++step) { 
            int nx = human.pos.x + vx * step; 
            int ny = human.pos.y + vy * step; 
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) break; 
            if (grid[nx][ny] != Terrain::Wall) {
                landing_pos = {nx, ny};
                all_walls = false;
            }
        } 
        int cx = landing_pos.x;
        int cy = landing_pos.y;
        
        if (all_walls) {
            add_log("[RADIO] CHOI NGU! Luu dan va vao tuong doi lai ngay duoi chan Human va NO NGAY LAP TUC!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            trigger_explosion(human.pos.x, human.pos.y);
        } else {
            GrenadeTimer g;
            g.active = true; 
            g.pos = {cx, cy}; 
            g.turns_left = 1; // Explodes after 1 ZombieAnimating phases
            active_grenades.push_back(g);
            
            // Trigger grenade throw flying animation
            sf::Vector2f hCenter = getCellCenter(human.pos.x, human.pos.y, cellSize, boardOffset);
            sf::Vector2f tCenter = getCellCenter(cx, cy, cellSize, boardOffset);
            active_fx.type = FXType::GrenadeFly;
            active_fx.timer = 0.5f;
            active_fx.max_duration = 0.5f;
            active_fx.start_p = hCenter;
            active_fx.end_p = tCenter;

            add_log("[RADIO] Grenade launched to (" + std::to_string(cx + 1) + ", " + std::to_string(cy + 1) + "), fuse is live.", ImVec4(0.6f, 1.0f, 0.55f, 1.0f));
        }
    } else if (input_mode == InputMode::TargetMolotov) {
        if (human.molotovs <= 0) { input_mode = InputMode::MoveMode; return; }
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y); 
        if (vx == 0 && vy == 0) return; 
        
        human.molotovs -= 1; human.stamina -= 1;
        std::uniform_int_distribution<int> dist_steps(1, 6); 
        int max_steps = dist_steps(rng); 
        
        Position landing_pos = human.pos;
        bool hit_zombie = false;
        bool all_walls = true;
        
        for (int step = 1; step <= max_steps; ++step) { 
            int cx = human.pos.x + vx * step; 
            int cy = human.pos.y + vy * step; 
            if (cx < 0 || cx >= width || cy < 0 || cy >= height) break; 
            
            if (grid[cx][cy] != Terrain::Wall) {
                landing_pos = {cx, cy};
                all_walls = false;
            }
            
            bool current_zombie_hit = false;
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) { 
                    landing_pos = {cx, cy};
                    all_walls = false;
                    zombies[i]->is_burning = true;
                    add_log("-> Molotov hit " + zombies[i]->name + " directly! Target ignited.", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    hit_zombie = true;
                    current_zombie_hit = true;
                    break; 
                } 
            } 
            if (current_zombie_hit) break;
        }
        Position hit_pos = landing_pos;
        
        if (all_walls) {
            add_log("[RADIO] CHOI NGU! Molotov va vao tuong vo ngay duoi chan Human va BOC CHAY NGAY LAP TUC!", ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
        } else {
            sf::Vector2f hCenter = getCellCenter(human.pos.x, human.pos.y, cellSize, boardOffset);
            active_fx.type = FXType::Molotov; 
            active_fx.timer = 0.5f; 
            active_fx.max_duration = 0.5f; 
            active_fx.start_p = hCenter; 
            active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset);
        }
        
        if (grid[hit_pos.x][hit_pos.y] == Terrain::Water) {
            add_log("-> Molotov landed in water. Fizzled out!", ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        } else if (grid[hit_pos.x][hit_pos.y] == Terrain::Dirt) {
            grid[hit_pos.x][hit_pos.y] = Terrain::Fire;
            fire_cells.push_back({hit_pos, 1}); 
            add_log("-> Molotov created a fire zone!", ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
            add_log("[RADIO] Fire spreads at (" + std::to_string(hit_pos.x + 1) + ", " + std::to_string(hit_pos.y + 1) + "). Keep distance!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
        }
        check_fire_interactions();
    }
    input_mode = InputMode::MoveMode; 
    check_victory_conditions(); 
}

void GameState::start_zombie_phase() { 
    if (game_over || game_won) return; 
    
    if (human.is_paralyzed) {
        human.is_paralyzed = false;
        human.stamina = 0;
    }

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
        if (active_zombie_substep == 0) {
            for (auto& z : zombies) {
                if (z->hp > 0 && z->is_burning) {
                    z->hp -= 1;
                    floating_texts.push_back({z->pos, -1, 1.0f, 1.0f});
                    z->is_burning = false;
                    add_log("[FIRE] " + z->name + " suffered 1 Burn Damage!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    if (z->hp <= 0 && z->type == ZombieType::Exploding) trigger_explosion(z->pos.x, z->pos.y);
                }
            }
            
            if (!active_config.enable_environment) {
                for (auto& g : active_grenades) {
                    if (g.active) {
                        g.turns_left--;
                        if (g.turns_left <= 0) {
                            add_log("[RADIO] Grenade timer expires. Detonation!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
                            trigger_explosion(g.pos.x, g.pos.y);
                            g.active = false;
                        }
                    }
                }
                active_grenades.erase(std::remove_if(active_grenades.begin(), active_grenades.end(), [](const GrenadeTimer& g) { return !g.active; }), active_grenades.end());
            }
            
            if (active_fx.type != FXType::None || !attack_animations.empty()) {
                active_zombie_substep = 1; // Wait for the detonated explosion / fire animations to finish
                return;
            }
        }
        
        active_zombie_substep = 0; // Reset substep
        check_victory_conditions();
        if (!game_over && !game_won) {
            if (active_config.enable_environment) start_environment_phase();
            else finish_environment_phase();
        }
        return; 
    } 
    auto& zom = zombies[active_zombie_idx]; 
    if (zom->hp <= 0 || game_over) { 
        active_zombie_idx++; active_zombie_substep = 0; return; 
    } 
    if (zom->is_paralyzed) {
        add_log("[SHOCK] " + zom->name + " is paralyzed and loses this action.", ImVec4(0.45f, 0.9f, 1.0f, 1.0f));
        zom->is_paralyzed = false;
        active_zombie_idx++;
        active_zombie_substep = 0;
        return;
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
                if (zom->type == ZombieType::Sick) {
                    int turns_left = std::max(0, turn_limit - current_turn);
                    int halved_left = turns_left / 2;
                    turn_limit = current_turn + halved_left;
                    add_log("-> Infection! Remaining turns halved (" + std::to_string(turns_left) + " -> " + std::to_string(halved_left) + ").", ImVec4(0.82f, 0.93f, 0.24f, 1.0f));
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
    else if (current_turn > turn_limit) game_over = true;
    else { 
        bool all_dead = true; 
        for (const auto& z : zombies) { if (z->hp > 0) { all_dead = false; break; } } 
        if (all_dead) game_won = true; 
    } 
}
