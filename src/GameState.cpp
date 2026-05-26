#define _USE_MATH_DEFINES

#include "GameState.h"
#include "GameConstants.h"
#include "AudioManager.h"
#include "SoundSynth.h"
#include "embedded/menu_theme.h"
#include "embedded/battle_theme.h"
#include "embedded/victory_theme.h"
#include "embedded/defeat_theme.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>

GameState::GameState() : rng(std::random_device{}()) {
    active_config.custom_grid.assign(active_config.map_width, std::vector<Terrain>(active_config.map_height, Terrain::Dirt));
}

void GameState::initAudio() {
    AudioManager& audio = AudioManager::getInstance();

    // Load nhạc từ dữ liệu nhúng trong executable
    audio.loadMusicFromMemory("menu",    menu_theme_ogg,    menu_theme_ogg_len);
    audio.loadMusicFromMemory("battle",  battle_theme_ogg,  battle_theme_ogg_len);
    audio.loadMusicFromMemory("victory", victory_theme_ogg, victory_theme_ogg_len);
    audio.loadMusicFromMemory("defeat",  defeat_theme_ogg,  defeat_theme_ogg_len);
    audio.setMusicVolume(music_volume);

    // Tổng hợp tất cả sound effects từ code — không cần file ngoài
    SoundSynth::registerAll(audio);
}

void GameState::playBackgroundMusic(const std::string& track) {
    if (!music_enabled) {
        AudioManager::getInstance().stopMusic();
        return;
    }
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

// Shorthand helper used throughout GameState
static inline void sfx(const std::string& name) {
    AudioManager::getInstance().playSound(name);
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
                if (std::abs(x - h_pos.x) <= GameConstants::MapGen::SPAWN_SHIELD_RADIUS && std::abs(y - h_pos.y) <= GameConstants::MapGen::SPAWN_SHIELD_RADIUS) continue;
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
    active_config.enable_environment = true;

    if (level == 0) { // Easy — open map, mild weather, lots of room to maneuver
        active_config.human_hp = GameConstants::Difficulty::Easy::HUMAN_HP;
        active_config.initial_stamina = GameConstants::Difficulty::Easy::INITIAL_STAMINA;
        active_config.pistol_ammo = GameConstants::Difficulty::Easy::PISTOL_AMMO; 
        active_config.shotgun_ammo = GameConstants::Difficulty::Easy::SHOTGUN_AMMO;
        active_config.grenades = GameConstants::Difficulty::Easy::GRENADES;     
        active_config.mines = GameConstants::Difficulty::Easy::MINES;
        active_config.molotovs = GameConstants::Difficulty::Easy::MOLOTOVS;
        active_config.count_normal = GameConstants::Difficulty::Easy::COUNT_NORMAL; 
        active_config.count_fast = GameConstants::Difficulty::Easy::COUNT_FAST;
        active_config.count_exploding = GameConstants::Difficulty::Easy::COUNT_EXPLODING; 
        active_config.count_vampire = GameConstants::Difficulty::Easy::COUNT_VAMPIRE; 
        active_config.count_sick = GameConstants::Difficulty::Easy::COUNT_SICK;
        // Terrain: mostly open dirt, light obstacles, small water/forest patches, no ice
        active_config.ratio_dirt = 60;
        active_config.ratio_wall = 8;
        active_config.ratio_water = 10;
        active_config.ratio_forest = 18;
        active_config.ratio_ice = 4;
        // Weather: mostly clear, light wind/rain, rare extremes
        active_config.env_prob_clear = 60;
        active_config.env_prob_wind = 14;
        active_config.env_prob_rain = 12;
        active_config.env_prob_clouds = 4;
        active_config.env_prob_lightning = 4;
        active_config.env_prob_heatwave = 3;
        active_config.env_prob_blizzard = 3;
    } 
    else if (level == 1) { // Medium — balanced terrain, varied weather
        active_config.human_hp = GameConstants::Difficulty::Medium::HUMAN_HP;
        active_config.initial_stamina = GameConstants::Difficulty::Medium::INITIAL_STAMINA;
        active_config.pistol_ammo = GameConstants::Difficulty::Medium::PISTOL_AMMO; 
        active_config.shotgun_ammo = GameConstants::Difficulty::Medium::SHOTGUN_AMMO;
        active_config.grenades = GameConstants::Difficulty::Medium::GRENADES;     
        active_config.mines = GameConstants::Difficulty::Medium::MINES;
        active_config.molotovs = GameConstants::Difficulty::Medium::MOLOTOVS;
        active_config.count_normal = GameConstants::Difficulty::Medium::COUNT_NORMAL; 
        active_config.count_fast = GameConstants::Difficulty::Medium::COUNT_FAST;
        active_config.count_exploding = GameConstants::Difficulty::Medium::COUNT_EXPLODING; 
        active_config.count_vampire = GameConstants::Difficulty::Medium::COUNT_VAMPIRE; 
        active_config.count_sick = GameConstants::Difficulty::Medium::COUNT_SICK;
        // Terrain: balanced mix with moderate hazards
        active_config.ratio_dirt = 52;
        active_config.ratio_wall = 10;
        active_config.ratio_water = 12;
        active_config.ratio_forest = 18;
        active_config.ratio_ice = 8;
        // Weather: balanced, all events possible
        active_config.env_prob_clear = 50;
        active_config.env_prob_wind = 14;
        active_config.env_prob_rain = 12;
        active_config.env_prob_clouds = 4;
        active_config.env_prob_lightning = 8;
        active_config.env_prob_heatwave = 6;
        active_config.env_prob_blizzard = 6;
    } 
    else if (level == 2) { // Hard — dense terrain, hostile weather
        active_config.human_hp = GameConstants::Difficulty::Hard::HUMAN_HP;
        active_config.initial_stamina = GameConstants::Difficulty::Hard::INITIAL_STAMINA;
        active_config.pistol_ammo = GameConstants::Difficulty::Hard::PISTOL_AMMO;  
        active_config.shotgun_ammo = GameConstants::Difficulty::Hard::SHOTGUN_AMMO;
        active_config.grenades = GameConstants::Difficulty::Hard::GRENADES;     
        active_config.mines = GameConstants::Difficulty::Hard::MINES;
        active_config.molotovs = GameConstants::Difficulty::Hard::MOLOTOVS;
        active_config.count_normal = GameConstants::Difficulty::Hard::COUNT_NORMAL; 
        active_config.count_fast = GameConstants::Difficulty::Hard::COUNT_FAST;
        active_config.count_exploding = GameConstants::Difficulty::Hard::COUNT_EXPLODING; 
        active_config.count_vampire = GameConstants::Difficulty::Hard::COUNT_VAMPIRE; 
        active_config.count_sick = GameConstants::Difficulty::Hard::COUNT_SICK;
        // Terrain: more walls/water/ice, less open space
        active_config.ratio_dirt = 42;
        active_config.ratio_wall = 13;
        active_config.ratio_water = 14;
        active_config.ratio_forest = 16;
        active_config.ratio_ice = 15;
        // Weather: less clear, more dangerous events
        active_config.env_prob_clear = 38;
        active_config.env_prob_wind = 14;
        active_config.env_prob_rain = 12;
        active_config.env_prob_clouds = 6;
        active_config.env_prob_lightning = 12;
        active_config.env_prob_heatwave = 9;
        active_config.env_prob_blizzard = 9;
    }
    else { // Unfair — chaotic terrain, extreme weather
        active_config.human_hp = GameConstants::Difficulty::Unfair::HUMAN_HP;
        active_config.initial_stamina = GameConstants::Difficulty::Unfair::INITIAL_STAMINA;
        active_config.pistol_ammo = GameConstants::Difficulty::Unfair::PISTOL_AMMO;  
        active_config.shotgun_ammo = GameConstants::Difficulty::Unfair::SHOTGUN_AMMO;
        active_config.grenades = GameConstants::Difficulty::Unfair::GRENADES;     
        active_config.mines = GameConstants::Difficulty::Unfair::MINES;
        active_config.molotovs = GameConstants::Difficulty::Unfair::MOLOTOVS;
        active_config.count_normal = GameConstants::Difficulty::Unfair::COUNT_NORMAL; 
        active_config.count_fast = GameConstants::Difficulty::Unfair::COUNT_FAST;
        active_config.count_exploding = GameConstants::Difficulty::Unfair::COUNT_EXPLODING; 
        active_config.count_vampire = GameConstants::Difficulty::Unfair::COUNT_VAMPIRE; 
        active_config.count_sick = GameConstants::Difficulty::Unfair::COUNT_SICK;
        // Terrain: heavily obstructed, lots of ice and water hazards
        active_config.ratio_dirt = 32;
        active_config.ratio_wall = 15;
        active_config.ratio_water = 16;
        active_config.ratio_forest = 14;
        active_config.ratio_ice = 23;
        // Weather: rarely clear, frequent extreme events
        active_config.env_prob_clear = 22;
        active_config.env_prob_wind = 14;
        active_config.env_prob_rain = 12;
        active_config.env_prob_clouds = 6;
        active_config.env_prob_lightning = 16;
        active_config.env_prob_heatwave = 15;
        active_config.env_prob_blizzard = 15;
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
    outFile << "RATIO_WALL "      << active_config.ratio_wall << "\n";
    outFile << "RATIO_WATER "     << active_config.ratio_water << "\n";
    outFile << "RATIO_FOREST "    << active_config.ratio_forest << "\n";
    outFile << "RATIO_DIRT "      << active_config.ratio_dirt << "\n";
    outFile << "RATIO_ICE "       << active_config.ratio_ice << "\n";
    outFile << "CUST_HUMAN_X "    << active_config.custom_human_pos.x << "\n";
    outFile << "CUST_HUMAN_Y "    << active_config.custom_human_pos.y << "\n";
    outFile << "ENV_PROB_CLEAR "  << active_config.env_prob_clear << "\n";
    outFile << "ENV_PROB_WIND "   << active_config.env_prob_wind << "\n";
    outFile << "ENV_PROB_RAIN "   << active_config.env_prob_rain << "\n";
    outFile << "ENV_PROB_CLOUDS " << active_config.env_prob_clouds << "\n";
    outFile << "ENV_PROB_LIGHT "  << active_config.env_prob_lightning << "\n";
    outFile << "ENV_PROB_HEAT "   << active_config.env_prob_heatwave << "\n";
    outFile << "ENV_PROB_BLIZ "   << active_config.env_prob_blizzard << "\n";

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
        else if (key == "RATIO_WALL")   active_config.ratio_wall = val;
        else if (key == "RATIO_WATER")  active_config.ratio_water = val;
        else if (key == "RATIO_FOREST") active_config.ratio_forest = val;
        else if (key == "RATIO_DIRT")   active_config.ratio_dirt = val;
        else if (key == "RATIO_ICE")    active_config.ratio_ice = val;
        else if (key == "ENV_PROB_CLEAR")  active_config.env_prob_clear = val;
        else if (key == "ENV_PROB_WIND")   active_config.env_prob_wind = val;
        else if (key == "ENV_PROB_RAIN")   active_config.env_prob_rain = val;
        else if (key == "ENV_PROB_CLOUDS") active_config.env_prob_clouds = val;
        else if (key == "ENV_PROB_LIGHT")  active_config.env_prob_lightning = val;
        else if (key == "ENV_PROB_HEAT")   active_config.env_prob_heatwave = val;
        else if (key == "ENV_PROB_BLIZ")   active_config.env_prob_blizzard = val;
    }
    inFile.close();
    return true;
}

void GameState::init_game() {
    zombies.clear(); 
    logs.clear();
    fire_cells.clear();
    loot_drops.clear();
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
        
        float total_ratio = active_config.ratio_wall + active_config.ratio_water + active_config.ratio_forest + active_config.ratio_dirt + active_config.ratio_ice;
        if (total_ratio <= 0.0f) {
            active_config.ratio_wall = 10;
            active_config.ratio_water = 10;
            active_config.ratio_forest = 20;
            active_config.ratio_dirt = 60;
            active_config.ratio_ice = 0;
            total_ratio = 100.0f;
        }

        int total_cells = width * height;
        int target_wall = std::round(total_cells * (active_config.ratio_wall / total_ratio));
        int target_water = std::round(total_cells * (active_config.ratio_water / total_ratio));
        int target_forest = std::round(total_cells * (active_config.ratio_forest / total_ratio));
        int target_ice = std::round(total_cells * (active_config.ratio_ice / total_ratio));

        auto grow_terrain = [&](Terrain t, int target_count, float cluster_prob) {
            int current_count = 0;
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    if (grid[x][y] == t) current_count++;
                }
            }
            std::vector<Position> frontier;
            std::uniform_int_distribution<int> dist_x(0, width - 1);
            std::uniform_int_distribution<int> dist_y(0, height - 1);
            std::uniform_real_distribution<float> prob(0.0f, 1.0f);
            
            while (current_count < target_count) {
                int sx = dist_x(rng);
                int sy = dist_y(rng);
                int attempts = 0;
                while (grid[sx][sy] != Terrain::Dirt && attempts < 200) {
                    sx = dist_x(rng);
                    sy = dist_y(rng);
                    attempts++;
                }
                if (attempts >= 200) break;
                grid[sx][sy] = t;
                current_count++;
                if (current_count >= target_count) break;
                
                frontier.clear();
                frontier.push_back(Position{sx, sy});
                
                size_t front_idx = 0;
                while (front_idx < frontier.size() && current_count < target_count) {
                    Position curr = frontier[front_idx++];
                    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                    int indices[4] = {0, 1, 2, 3};
                    std::shuffle(indices, indices + 4, rng);
                    for (int i = 0; i < 4; ++i) {
                        int nx = curr.x + dirs[indices[i]][0];
                        int ny = curr.y + dirs[indices[i]][1];
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            if (grid[nx][ny] == Terrain::Dirt) {
                                if (prob(rng) < cluster_prob) {
                                    grid[nx][ny] = t;
                                    current_count++;
                                    frontier.push_back(Position{nx, ny});
                                    if (current_count >= target_count) break;
                                }
                            }
                        }
                    }
                }
            }
        };

        grow_terrain(Terrain::Water, target_water, GameConstants::MapGen::CLUSTER_PROB_WATER);
        grow_terrain(Terrain::Forest, target_forest, GameConstants::MapGen::CLUSTER_PROB_FOREST);
        grow_terrain(Terrain::Wall, target_wall, GameConstants::MapGen::CLUSTER_PROB_WALL);
        grow_terrain(Terrain::Ice, target_ice, GameConstants::MapGen::CLUSTER_PROB_WATER); // Ice clusters like water

        std::uniform_int_distribution<int> dist_x(0, width - 1);
        std::uniform_int_distribution<int> dist_y(0, height - 1);

        human.pos = {dist_x(rng), dist_y(rng)};
        int spawn_attempts = 0;
        while ((grid[human.pos.x][human.pos.y] == Terrain::Wall || grid[human.pos.x][human.pos.y] == Terrain::Water) && spawn_attempts < 500) {
            human.pos = {dist_x(rng), dist_y(rng)};
            spawn_attempts++;
        }
        if (grid[human.pos.x][human.pos.y] == Terrain::Wall || grid[human.pos.x][human.pos.y] == Terrain::Water) {
            grid[human.pos.x][human.pos.y] = Terrain::Dirt;
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
                if (active_config.spawn_shield && std::abs(z_pos.x - human.pos.x) <= GameConstants::MapGen::SPAWN_SHIELD_RADIUS && std::abs(z_pos.y - human.pos.y) <= GameConstants::MapGen::SPAWN_SHIELD_RADIUS) invalid_pos = true;
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

    spawn_zombie_lambda(ZombieType::Normal, active_config.count_normal, "Normal Zom", GameConstants::Zombies::BASE_HP_NORMAL);
    spawn_zombie_lambda(ZombieType::Fast, active_config.count_fast, "Fast Sprinter", GameConstants::Zombies::BASE_HP_FAST);
    spawn_zombie_lambda(ZombieType::Exploding, active_config.count_exploding, "Exploder", GameConstants::Zombies::BASE_HP_EXPLODING);
    spawn_zombie_lambda(ZombieType::Vampire, active_config.count_vampire, "Vampire Dracula", GameConstants::Zombies::BASE_HP_VAMPIRE);
    spawn_zombie_lambda(ZombieType::Sick, active_config.count_sick, "Sick Carrier", GameConstants::Zombies::BASE_HP_SICK);

    //add_log("Tactical Battleground initialized with dynamic safety boundaries!", ImVec4(0, 1, 1, 1));
    add_log("=== HUMAN TURN " + std::to_string(current_turn) + " START ===", ImVec4(1.0f, 0.95f, 0.25f, 1.0f));
    turn_banner_fx.type = FXType::Electricity;
    turn_banner_fx.timer = 1.5f;
    turn_banner_fx.max_duration = 1.5f;
    turn_banner_fx.banner_text = "";
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
    if (steps == 0) return false;

    int prev_cx = start.x;
    int prev_cy = start.y;

    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / steps;
        float fx = start.x + dx * t;
        float fy = start.y + dy * t;
        
        bool x_half = (std::abs(fx - std::round(fx)) == 0.5f);
        bool y_half = (std::abs(fy - std::round(fy)) == 0.5f);
        
        int cx1 = std::floor(fx);
        int cx2 = std::ceil(fx);
        int cy1 = std::floor(fy);
        int cy2 = std::ceil(fy);

        if (i < steps) {
            if (cx1 >= 0 && cx1 < width && cy1 >= 0 && cy1 < height && grid[cx1][cy1] == Terrain::Wall) return true;
            if (x_half && cx2 >= 0 && cx2 < width && cy1 >= 0 && cy1 < height && grid[cx2][cy1] == Terrain::Wall) return true;
            if (y_half && cx1 >= 0 && cx1 < width && cy2 >= 0 && cy2 < height && grid[cx1][cy2] == Terrain::Wall) return true;
            if (x_half && y_half && cx2 >= 0 && cx2 < width && cy2 >= 0 && cy2 < height && grid[cx2][cy2] == Terrain::Wall) return true;
        }

        int curr_cx = std::round(fx);
        int curr_cy = std::round(fy);
        
        prev_cx = curr_cx;
        prev_cy = curr_cy;
    }
    return false;
}

void GameState::check_fire_interactions() {
    if (grid[human.pos.x][human.pos.y] == Terrain::Water) {
        if (human.is_burning) {
            human.is_burning = false;
            add_log(tr("[FIRE] Human stepped into water! Burning extinguished.", "[FIRE] Human da buoc vao nuoc! Lua da bi dap tat."), ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
        }
    } else if (grid[human.pos.x][human.pos.y] == Terrain::Fire) {
        if (human.is_burning) {
            human.hp = std::max(0, human.hp - 1);
            floating_texts.push_back({human.pos, -1, 1.0f, 1.0f});
            add_log(tr("[FIRE] Human entered fire while already burning! Suffered 1 immediate Fire damage.", "[FIRE] Human di vao o lua khi dang bi thieu dot! Bi tru 1 mau lap tuc."), ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            check_victory_conditions();
        } else {
            human.is_burning = true;
            add_log(tr("[FIRE] Human stepped into a blazing fire!", "[FIRE] Human da buoc vao mot o lua thieu dot!"), ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
        }
    }

    for (auto& z : zombies) {
        if (z->hp > 0) {
            if (grid[z->pos.x][z->pos.y] == Terrain::Water) {
                if (z->is_burning) {
                    z->is_burning = false;
                    add_log(tr("[FIRE] " + z->name + " stepped into water! Burning extinguished.", "[FIRE] " + z->name + " da buoc vao nuoc! Lua da bi dap tat."), ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
                }
            } else if (grid[z->pos.x][z->pos.y] == Terrain::Fire) {
                if (z->is_burning) {
                    z->hp -= 1;
                    floating_texts.push_back({z->pos, -1, 1.0f, 1.0f});
                    add_log(tr("[FIRE] " + z->name + " entered fire while already burning! Suffered 1 immediate Fire damage.", "[FIRE] " + z->name + " di vao o lua khi dang bi thieu dot! Bi tru 1 mau lap tuc."), ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    if (z->hp <= 0 && z->type == ZombieType::Exploding) {
                        queue_explosion(z->pos.x, z->pos.y, true);
                    }
                    check_victory_conditions();
                } else {
                    z->is_burning = true;
                    add_log(tr("[FIRE] " + z->name + " stepped into a blazing fire!", "[FIRE] " + z->name + " da buoc vao mot o lua thieu dot!"), ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                }
            }
        }
    }

    std::vector<Position> fire_sources;
    if (human.is_burning) fire_sources.push_back(human.pos);
    for (const auto& z : zombies) {
        if (z->hp > 0 && z->is_burning) fire_sources.push_back(z->pos);
    }

    for (const auto& p : fire_sources) {
        if (!human.is_burning && distance(human.pos, p) <= 1) {
            if (grid[human.pos.x][human.pos.y] != Terrain::Water) {
                human.is_burning = true;
                add_log(tr("[FIRE] Human caught fire from close proximity!", "[FIRE] Human bat lua do dung qua gan nguon lua!"), ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
            }
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && !z->is_burning && distance(z->pos, p) <= 1) {
                if (grid[z->pos.x][z->pos.y] != Terrain::Water) {
                    z->is_burning = true;
                    add_log(tr("[FIRE] " + z->name + " caught fire from close proximity!", "[FIRE] " + z->name + " bat lua do dung qua gan nguon lua!"), ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
                }
            }
        }
    }
}

void GameState::check_mine_interactions() {
    if (human.hp > 0 && mine_grid[human.pos.x][human.pos.y]) {
        mine_grid[human.pos.x][human.pos.y] = false;
        add_log("[RADIO] WATCH OUT! Human stepped on a mine. Stand by for detonation!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        queue_explosion(human.pos.x, human.pos.y);
    }
}

// Ice slide logic:
// Called after an entity steps onto an Ice cell.
// Counts consecutive ice cells ahead in the move direction.
// If >= SLIDE_TRIGGER_LENGTH, rolls for a slide.
// On slide: entity moves to the last ice cell in the run.
// If the cell after the last ice cell is a blocker, entity is stunned (turn ends immediately).
// Returns true if a slide occurred (caller should end the entity's turn).
bool GameState::try_ice_slide(bool is_human, size_t zombie_idx, int move_dx, int move_dy, bool& out_stunned) {
    // Guard: zero direction means no movement, nothing to do
    if (move_dx == 0 && move_dy == 0) return false;
    // Guard: zombie index out of range
    if (!is_human && zombie_idx >= zombies.size()) return false;

    Position& pos = is_human ? human.pos : zombies[zombie_idx]->pos;

    // Count consecutive ice cells starting from current position (inclusive)
    int ice_run = 0;
    for (int step = 0; step < GameConstants::Ice::SLIDE_TRIGGER_LENGTH; ++step) {
        int nx = pos.x + move_dx * step;
        int ny = pos.y + move_dy * step;
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) break;
        if (grid[nx][ny] == Terrain::Ice) ice_run++;
        else break;
    }

    if (ice_run < GameConstants::Ice::SLIDE_TRIGGER_LENGTH) return false;

    // Exponential probability: each additional step has decreasing chance.
    // Base chance for the minimum slide (SLIDE_TRIGGER_LENGTH cells) = SLIDE_CHANCE.
    // Each extra step beyond that multiplies by SLIDE_DECAY.
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    const float SLIDE_DECAY = 0.75f; // probability multiplier per extra step (increased from 0.55f for longer slides)

    // Find how far the entity can slide, rolling per step
    int max_steps = width + height;
    Position slide_dest = pos;
    bool has_blocker_after = false;

    for (int step = 1; step <= max_steps; ++step) {
        int nx = pos.x + move_dx * step;
        int ny = pos.y + move_dy * step;

        // Hit map boundary
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
            has_blocker_after = true;
            break;
        }

        // Hit non-ice terrain
        if (grid[nx][ny] != Terrain::Ice) {
            if (grid[nx][ny] == Terrain::Wall) has_blocker_after = true;
            break;
        }

        // Check entity blocker
        bool blocked_by_entity = false;
        if (is_human) {
            for (const auto& z : zombies) {
                if (z->hp > 0 && z->pos == Position{nx, ny}) { blocked_by_entity = true; break; }
            }
        } else {
            if (human.hp > 0 && human.pos == Position{nx, ny}) blocked_by_entity = true;
            if (!blocked_by_entity) {
                for (size_t k = 0; k < zombies.size(); ++k) {
                    if (k != zombie_idx && zombies[k]->hp > 0 && zombies[k]->pos == Position{nx, ny}) {
                        blocked_by_entity = true; break;
                    }
                }
            }
        }
        if (blocked_by_entity) {
            has_blocker_after = true;
            break;
        }

        // Exponential probability: first step uses SLIDE_CHANCE, each extra step decays
        // Using higher SLIDE_DECAY (0.75) to increase probability of longer slides
        float step_prob = GameConstants::Ice::SLIDE_CHANCE * std::pow(SLIDE_DECAY, step - 1);
        if (roll(rng) >= step_prob) break; // Stop here — didn't slide this far

        slide_dest = {nx, ny};
    }

    if (slide_dest == pos) return false; // No actual movement

    // Check blocker right after final destination (for stun)
    if (!has_blocker_after) {
        int after_x = slide_dest.x + move_dx;
        int after_y = slide_dest.y + move_dy;
        if (after_x < 0 || after_x >= width || after_y < 0 || after_y >= height) {
            has_blocker_after = true;
        } else if (grid[after_x][after_y] == Terrain::Wall) {
            has_blocker_after = true;
        } else {
            if (is_human) {
                for (const auto& z : zombies) {
                    if (z->hp > 0 && z->pos == Position{after_x, after_y}) { has_blocker_after = true; break; }
                }
            } else {
                if (human.hp > 0 && human.pos == Position{after_x, after_y}) has_blocker_after = true;
                if (!has_blocker_after) {
                    for (size_t k = 0; k < zombies.size(); ++k) {
                        if (k != zombie_idx && zombies[k]->hp > 0 && zombies[k]->pos == Position{after_x, after_y}) {
                            has_blocker_after = true; break;
                        }
                    }
                }
            }
        }
    }

    std::string entity_name = is_human ? "Human" : zombies[zombie_idx]->name;
    add_log(tr("[ICE] " + entity_name + " slides on ice to (" + std::to_string(slide_dest.x + 1) + ", " + std::to_string(slide_dest.y + 1) + ")!",
               "[BANG] " + entity_name + " truot bang den (" + std::to_string(slide_dest.x + 1) + ", " + std::to_string(slide_dest.y + 1) + ")!"),
            ImVec4(0.7f, 0.9f, 1.0f, 1.0f));

    // Build visual animation path (bounded)
    ice_slide_animation.active = true;
    ice_slide_animation.is_human = is_human;
    ice_slide_animation.zombie_idx = zombie_idx;
    ice_slide_animation.path.clear();
    ice_slide_animation.current_step = 0;
    ice_slide_animation.step_timer = 0.0f;
    {
        Position cur = pos;
        int steps_built = 0;
        while (cur != slide_dest && steps_built < max_steps) {
            ice_slide_animation.path.push_back(cur);
            cur.x += move_dx;
            cur.y += move_dy;
            steps_built++;
        }
        ice_slide_animation.path.push_back(slide_dest);
    }

    // Move entity to final destination immediately (logical position).
    // Animation path is purely visual.
    pos = slide_dest;

    // Apply stun BEFORE check_fire_interactions
    if (has_blocker_after) {
        if (is_human) {
            out_stunned = true;
            human.stamina = 0;
            add_log(tr("[ICE] Human slammed into an obstacle! Turn ends immediately.",
                       "[BANG] Human truot vao vat can! Ket thuc luot ngay lap tuc."),
                    ImVec4(0.6f, 0.85f, 1.0f, 1.0f));
        } else {
            out_stunned = true;
            add_log(tr("[ICE] " + entity_name + " slammed into an obstacle! Action ends.",
                       "[BANG] " + entity_name + " truot vao vat can! Ket thuc hanh dong."),
                    ImVec4(0.6f, 0.85f, 1.0f, 1.0f));
        }
    }

    // Run terrain interactions LAST — these can trigger explosions/kills
    check_fire_interactions();
    check_mine_interactions();

    return true; // Slide occurred
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
    // Only Water and Ice tiles conduct electricity.
    // An entity standing on Water/Ice also conducts (included via tile check).
    // Entities on Dirt/Forest/Fire are insulators — they do NOT extend the cluster.
    return grid[p.x][p.y] == Terrain::Water || grid[p.x][p.y] == Terrain::Ice;
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

void GameState::melt_adjacent_ice(int cx, int cy) {
    const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (const auto& d : dirs) {
        int nx = cx + d[0], ny = cy + d[1];
        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
        if (grid[nx][ny] != Terrain::Ice) continue;
        grid[nx][ny] = Terrain::Water;
        terrain_transitions.push_back({Position{nx, ny}, Terrain::Ice, Terrain::Water, 0.0f, 0.8f});
        thaw_loot_and_grenades_at({{nx, ny}});
        add_log(tr("[ICE] Heat transfer melted ice at (" + std::to_string(nx + 1) + ", " + std::to_string(ny + 1) + ")!",
                   "[BANG] Truyen nhiet lam tan bang tai (" + std::to_string(nx + 1) + ", " + std::to_string(ny + 1) + ")!"),
                ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
        // Unfreeze any entity standing on the melted cell
        if (human.hp > 0 && human.pos == Position{nx, ny} && human.is_frozen) {
            human.is_frozen = false;
            add_log(tr("[ICE] Ice melted under Human! Unfrozen.", "[BANG] Bang tan duoi chan Nguoi! Giai bang."),
                    ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && z->pos == Position{nx, ny} && z->is_frozen) {
                z->is_frozen = false;
                z->frozen_turns = 0;
                add_log(tr("[ICE] Ice melted under " + z->name + "! Unfrozen.",
                           "[BANG] Bang tan duoi chan " + z->name + "! Giai bang."),
                        ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }
        }
    }
}

void GameState::apply_windstorm(int dx, int dy) {
    last_environment_event = "Windstorm";
    sfx("wind");
    active_fx.type = FXType::Wind;
    active_fx.timer = 3.2f;
    active_fx.max_duration = 3.2f;
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
        // Frozen entities cannot be pushed by wind
        if (ref.human && human.is_frozen) continue;
        if (!ref.human && zombies[ref.idx]->is_frozen) continue;

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
            // Grenade bị kẹt dưới băng không bị gió thổi
            if (g.frozen_under_ice) continue;
            Position g_target{g.pos.x + dx, g.pos.y + dy};
            if (!is_blocking_cell(g_target.x, g_target.y)) {
                g.pos = g_target;
                grenades_blown++;
            }
        }
    }

    // Gió thổi loot drops (cơ chế giống grenade)
    int loots_blown = 0;
    for (auto& ld : loot_drops) {
        // Loot bị kẹt dưới băng không bị gió thổi
        if (ld.frozen_under_ice) continue;
        Position l_target{ld.pos.x + dx, ld.pos.y + dy};
        if (!is_blocking_cell(l_target.x, l_target.y)) {
            ld.pos = l_target;
            loots_blown++;
        }
    }

    std::string wind_log = "[ENV] Windstorm blows " + std::to_string(dx) + "," + std::to_string(dy) + " and pushes " + std::to_string(moved) + " entities";
    if (grenades_blown > 0) {
        wind_log += " and " + std::to_string(grenades_blown) + " grenade(s)";
    }
    if (loots_blown > 0) {
        wind_log += " and " + std::to_string(loots_blown) + " loot drop(s)";
    }
    wind_log += ".";
    add_log(wind_log, ImVec4(0.65f, 0.85f, 1.0f, 1.0f));
    // Sau khi gió thổi, kiểm tra loot có rơi vào ô lửa không
    check_loot_on_fire();
    check_fire_interactions();
}

void GameState::apply_heavy_rain() {
    last_environment_event = "Heavy rain";
    sfx("rain");
    active_fx.type = FXType::Rain;
    active_fx.timer = 3.2f;
    active_fx.max_duration = 3.2f;
    active_fx.blast_cells.clear();
    active_fx.extinguished_cells.clear();

    std::vector<Position> soaked;
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] != Terrain::Dirt) continue;
            bool next_to_water = false;
            bool next_to_forest = false;
            for (const auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    if (grid[nx][ny] == Terrain::Water) next_to_water = true;
                    if (grid[nx][ny] == Terrain::Forest) next_to_forest = true;
                }
            }
            float threshold = next_to_water ? GameConstants::Environment::RAIN_FLOOD_THRESHOLD_WATER_ADJACENT : GameConstants::Environment::RAIN_FLOOD_THRESHOLD_ISOLATED;
            if (chance(rng) < threshold) {
                soaked.push_back({x, y});
            } else if (next_to_forest && chance(rng) < GameConstants::Environment::RAIN_FOREST_SPREAD_CHANCE) {
                // Dirt adjacent to forest may grow into forest during heavy rain
                soaked.push_back({x, y}); // will be overridden below — use separate list
            }
        }
    }

    // Separate: forest spread (dirt adjacent to forest -> forest)
    std::vector<Position> new_forest;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] != Terrain::Dirt) continue;
            bool next_to_forest = false;
            for (const auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[nx][ny] == Terrain::Forest) {
                    next_to_forest = true; break;
                }
            }
            if (next_to_forest && chance(rng) < GameConstants::Environment::RAIN_FOREST_SPREAD_CHANCE) {
                new_forest.push_back({x, y});
            }
        }
    }

    // Flood: dirt -> water (re-compute cleanly, not mixing with forest spread)
    std::vector<Position> flooded;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] != Terrain::Dirt) continue;
            // Skip cells that will become forest
            bool will_forest = false;
            for (const auto& p : new_forest) { if (p.x == x && p.y == y) { will_forest = true; break; } }
            if (will_forest) continue;
            bool next_to_water = false;
            for (const auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[nx][ny] == Terrain::Water) {
                    next_to_water = true; break;
                }
            }
            float threshold = next_to_water ? GameConstants::Environment::RAIN_FLOOD_THRESHOLD_WATER_ADJACENT : GameConstants::Environment::RAIN_FLOOD_THRESHOLD_ISOLATED;
            if (chance(rng) < threshold) flooded.push_back({x, y});
        }
    }

    for (const auto& p : flooded) {
        grid[p.x][p.y] = Terrain::Water;
        terrain_transitions.push_back({p, Terrain::Dirt, Terrain::Water, 0.0f, 0.8f});
    }
    for (const auto& p : new_forest) {
        grid[p.x][p.y] = Terrain::Forest;
        terrain_transitions.push_back({p, Terrain::Dirt, Terrain::Forest, 0.0f, 0.8f});
    }
    active_fx.blast_cells = flooded;

    std::vector<Position> extinguished;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] == Terrain::Fire) {
                grid[x][y] = Terrain::Dirt;
                extinguished.push_back({x, y});
            }
        }
    }
    
    fire_cells.erase(std::remove_if(fire_cells.begin(), fire_cells.end(), [&](const FireCell& fc) {
        for (const auto& p : extinguished) {
            if (fc.pos == p) return true;
        }
        return false;
    }), fire_cells.end());

    active_fx.extinguished_cells = extinguished;

    std::string rain_log = "[ENV] Heavy rain floods " + std::to_string(flooded.size()) + " dirt cells";
    if (!new_forest.empty()) rain_log += ", grows " + std::to_string(new_forest.size()) + " new forest cells";
    rain_log += ", extinguishes " + std::to_string(extinguished.size()) + " fire cells.";
    add_log(rain_log, ImVec4(0.35f, 0.65f, 1.0f, 1.0f));

    int cured_count = 0;
    if (human.is_burning) {
        human.is_burning = false;
        cured_count++;
    }
    for (auto& z : zombies) {
        if (z->hp > 0 && z->is_burning) {
            z->is_burning = false;
            cured_count++;
        }
    }
    if (cured_count > 0) {
        add_log(tr("[FIRE] Heavy rain extinguished burning status for " + std::to_string(cured_count) + " entities!",
                   "[LUA] Con mua lon da dap tat trang thai burned cho " + std::to_string(cured_count) + " thuc the!"),
                ImVec4(0.35f, 0.75f, 1.0f, 1.0f));
    }
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
    sfx("lightning");
    std::vector<Position> cells;
    std::vector<double> weights;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (is_blocking_cell(x, y)) continue;
            Position p{x, y};
            bool entity = has_living_entity_at(p);
            double w = GameConstants::Environment::LIGHTNING_WEIGHT_DEFAULT;
            if (grid[x][y] == Terrain::Water) w *= GameConstants::Environment::LIGHTNING_WEIGHT_WATER;
            if (entity) w *= GameConstants::Environment::LIGHTNING_WEIGHT_ENTITY;
            cells.push_back(p);
            weights.push_back(w);
        }
    }
    if (cells.empty()) return;

    std::discrete_distribution<int> pick(weights.begin(), weights.end());
    Position strike = cells[pick(rng)];
    std::vector<Position> conductive_cluster = get_conductive_cluster(strike);
    active_fx.type = FXType::Lightning;
    active_fx.timer = 3.6f;
    active_fx.max_duration = 3.6f;
    active_fx.cx = strike.x;
    active_fx.cy = strike.y;
    active_fx.blast_cells = conductive_cluster;

    add_log("[ENV] Lightning strikes (" + std::to_string(strike.x + 1) + ", " + std::to_string(strike.y + 1) + ")!", ImVec4(1.0f, 1.0f, 0.35f, 1.0f));

    // Hủy loot tại ô bị sét đánh trực tiếp
    destroy_loot_at_cells({strike});

    if (grid[strike.x][strike.y] == Terrain::Forest) {
        // Only ignite if no living entity is standing on the cell
        if (!has_living_entity_at(strike)) {
            add_log("[ENV] Lightning set the forest at (" + std::to_string(strike.x + 1) + ", " + std::to_string(strike.y + 1) + ") on fire!", ImVec4(1.0f, 0.45f, 0.0f, 1.0f));
            set_cell_on_fire(strike.x, strike.y);
        } else {
            add_log("[ENV] Lightning struck the forest at (" + std::to_string(strike.x + 1) + ", " + std::to_string(strike.y + 1) + ") — entity present, fire suppressed!", ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
        }
    }

    if (human.hp > 0 && human.pos == strike) {
        human.hp = std::max(0, human.hp - GameConstants::Environment::LIGHTNING_HP_DAMAGE);
        floating_texts.push_back({human.pos, -GameConstants::Environment::LIGHTNING_HP_DAMAGE, 1.0f, 1.0f});
        add_log("-> Human is struck by lightning! -" + std::to_string(GameConstants::Environment::LIGHTNING_HP_DAMAGE) + " HP.", ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
        // Direct strike on the struck cell unfreezes the entity
        if (human.is_frozen) {
            human.is_frozen = false;
            add_log(tr("[ICE] Lightning shattered the ice! Human unfrozen.",
                       "[BANG] Set danh vo bang! Nguoi duoc giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        }
    }
    for (auto& z : zombies) {
        if (z->hp > 0 && z->pos == strike) {
            z->hp -= GameConstants::Environment::LIGHTNING_HP_DAMAGE;
            floating_texts.push_back({z->pos, -GameConstants::Environment::LIGHTNING_HP_DAMAGE, 1.0f, 1.0f});
            add_log("-> " + z->name + " is struck by lightning! -" + std::to_string(GameConstants::Environment::LIGHTNING_HP_DAMAGE) + " HP.", ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
            // Direct strike on the struck cell unfreezes the entity
            if (z->is_frozen) {
                z->is_frozen = false;
                z->frozen_turns = 0;
                add_log(tr("[ICE] Lightning shattered the ice! " + z->name + " unfrozen.",
                           "[BANG] Set danh vo bang! " + z->name + " duoc giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }
            if (z->hp <= 0 && z->type == ZombieType::Exploding) queue_explosion(z->pos.x, z->pos.y, true);
        }
    }

    // Step 1: Paralyze entities standing ON conductive (Water/Ice) cells in the cluster
    int paralyzed_count = 0;
    for (const auto& p : conductive_cluster) {
        if (human.hp > 0 && human.pos == p && !human.is_paralyzed) {
            human.is_paralyzed = true;
            paralyzed_count++;
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && z->pos == p && !z->is_paralyzed) {
                z->is_paralyzed = true;
                paralyzed_count++;
            }
        }
    }

    // Step 2: Contact spread — entity standing orthogonally adjacent to a paralyzed
    // entity on Water/Ice also gets shocked (simulates touching a live body).
    // Only spreads one hop; entities on Dirt/Forest adjacent to empty Water/Ice are safe.
    const int adj4[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    std::vector<Position> shocked_positions;
    for (const auto& p : conductive_cluster) {
        if (human.hp > 0 && human.pos == p && human.is_paralyzed)
            shocked_positions.push_back(human.pos);
        for (const auto& z : zombies)
            if (z->hp > 0 && z->pos == p && z->is_paralyzed)
                shocked_positions.push_back(z->pos);
    }
    for (const auto& sp : shocked_positions) {
        for (const auto& d : adj4) {
            Position nb{sp.x + d[0], sp.y + d[1]};
            if (nb.x < 0 || nb.x >= width || nb.y < 0 || nb.y >= height) continue;
            // Entities already on conductive tiles are handled in step 1
            if (grid[nb.x][nb.y] == Terrain::Water || grid[nb.x][nb.y] == Terrain::Ice) continue;
            if (human.hp > 0 && human.pos == nb && !human.is_paralyzed) {
                human.is_paralyzed = true;
                paralyzed_count++;
            }
            for (auto& z : zombies) {
                if (z->hp > 0 && z->pos == nb && !z->is_paralyzed) {
                    z->is_paralyzed = true;
                    paralyzed_count++;
                }
            }
        }
    }

    if (!conductive_cluster.empty() || paralyzed_count > 0) {
        sfx("electricity");
        add_log("-> Electricity spreads through " + std::to_string(conductive_cluster.size()) +
                " conductive cells; " + std::to_string(paralyzed_count) +
                " entity/entities paralyzed.", ImVec4(0.45f, 0.9f, 1.0f, 1.0f));
    }
    check_victory_conditions();
}

void GameState::apply_heatwave() {
    last_environment_event = "Heatwave";
    active_fx.type = FXType::Heatwave;  // Use Heatwave FX type
    active_fx.timer = 3.2f;
    active_fx.max_duration = 3.2f;
    active_fx.overlay_intensity = 0.0f;
    active_fx.blast_cells.clear();
    active_fx.extinguished_cells.clear();

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    const int all_dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

    // Step 1: All ice cells melt into water
    std::vector<Position> melted_ice;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] == Terrain::Ice) {
                grid[x][y] = Terrain::Water;
                melted_ice.push_back({x, y});
                terrain_transitions.push_back({Position{x, y}, Terrain::Ice, Terrain::Water, 0.0f, 0.8f});
            }
        }
    }

    // Unfreeze any entities that were standing on now-melted ice
    if (!melted_ice.empty()) {
        thaw_loot_and_grenades_at(melted_ice);
        if (human.hp > 0 && human.is_frozen) {
            for (const auto& p : melted_ice) {
                if (human.pos == p) {
                    human.is_frozen = false;
                    add_log(tr("[RADIO] Human thawed out! Ice melted beneath them.",
                               "[RADIO] Nguoi duoc giai bang! Bang tan duoi chan."),
                            ImVec4(1.0f, 0.8f, 0.5f, 1.0f));
                    break;
                }
            }
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && z->is_frozen) {
                for (const auto& p : melted_ice) {
                    if (z->pos == p) {
                        z->is_frozen = false;
                        z->frozen_turns = 0;
                        add_log(tr("[RADIO] " + z->name + " thawed out! Ice melted beneath them.",
                                   "[RADIO] " + z->name + " duoc giai bang! Bang tan duoi chan."),
                                ImVec4(1.0f, 0.8f, 0.5f, 1.0f));
                        break;
                    }
                }
            }
        }
    }

    // Step 2: Water cells may evaporate. Probability decreases linearly with number of adjacent water cells.
    // 0 water neighbors -> 100% evaporate; 8 water neighbors -> 0% evaporate
    std::vector<Position> evaporated;
    // Snapshot water cells before evaporation
    std::vector<std::vector<bool>> is_water(width, std::vector<bool>(height, false));
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
            is_water[x][y] = (grid[x][y] == Terrain::Water);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (!is_water[x][y]) continue;
            int water_neighbors = 0;
            for (const auto& d : all_dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && is_water[nx][ny])
                    water_neighbors++;
            }
            // Linear: 0 neighbors -> 1.0 chance, 8 neighbors -> 0.0 chance
            float evap_chance = 1.0f - (water_neighbors / 8.0f);
            if (chance(rng) < evap_chance) {
                evaporated.push_back({x, y});
                terrain_transitions.push_back({Position{x, y}, Terrain::Water, Terrain::Dirt, 0.0f, 0.8f});
            }
        }
    }
    for (const auto& p : evaporated) grid[p.x][p.y] = Terrain::Dirt;
    active_fx.extinguished_cells = evaporated; // reuse for visual

    // Step 3: Forest cells may die from drought
    std::vector<Position> dried_forest;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] == Terrain::Forest && chance(rng) < GameConstants::Environment::HEATWAVE_FOREST_DROUGHT_CHANCE) {
                dried_forest.push_back({x, y});
                terrain_transitions.push_back({Position{x, y}, Terrain::Forest, Terrain::Dirt, 0.0f, 0.8f});
            }
        }
    }
    for (const auto& p : dried_forest) grid[p.x][p.y] = Terrain::Dirt;

    std::string log = "[ENV] Heatwave!";
    if (!melted_ice.empty()) log += " Melted " + std::to_string(melted_ice.size()) + " ice cells.";
    if (!evaporated.empty()) log += " Evaporated " + std::to_string(evaporated.size()) + " water cells.";
    if (!dried_forest.empty()) log += " Drought killed " + std::to_string(dried_forest.size()) + " forest cells.";
    add_log(tr(log, "[MT] Nang nong gay gat! Bang tan, nuoc boc hoi, rung kho het."), ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
    // Nếu ô nước/băng dưới loot chuyển thành đất, human có thể nhặt ngay
    check_loot_pickup();
}

void GameState::apply_blizzard() {
    last_environment_event = "Blizzard";
    active_fx.type = FXType::Blizzard;  // Use Blizzard FX type
    active_fx.timer = 3.2f;
    active_fx.max_duration = 3.2f;
    active_fx.overlay_intensity = 0.0f;
    active_fx.blast_cells.clear();
    active_fx.extinguished_cells.clear();

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    const int orth_dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    // Find all water cells; randomly pick seed cells to freeze
    // When a seed freezes, its entire orthogonally-connected water cluster also freezes
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::vector<Position> frozen_cells;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (grid[x][y] != Terrain::Water || visited[x][y]) continue;
            // Roll for this cell to be a freeze seed
            if (chance(rng) >= GameConstants::Environment::BLIZZARD_FREEZE_CHANCE) continue;

            // BFS: freeze entire connected water cluster
            std::queue<Position> q;
            q.push({x, y});
            visited[x][y] = true;
            while (!q.empty()) {
                Position cur = q.front(); q.pop();
                frozen_cells.push_back(cur);
                terrain_transitions.push_back({cur, Terrain::Water, Terrain::Ice, 0.0f, 0.8f});
                for (const auto& d : orth_dirs) {
                    int nx = cur.x + d[0], ny = cur.y + d[1];
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    if (visited[nx][ny] || grid[nx][ny] != Terrain::Water) continue;
                    visited[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }
    }

    for (const auto& p : frozen_cells) grid[p.x][p.y] = Terrain::Ice;
    active_fx.blast_cells = frozen_cells;

    // Đánh dấu loot và grenade đang nằm trên ô vừa đóng băng là bị kẹt dưới băng
    for (auto& ld : loot_drops) {
        for (const auto& p : frozen_cells) {
            if (ld.pos == p) { ld.frozen_under_ice = true; break; }
        }
    }
    for (auto& g : active_grenades) {
        if (g.active) {
            for (const auto& p : frozen_cells) {
                if (g.pos == p) { g.frozen_under_ice = true; break; }
            }
        }
    }

    // Apply frozen effect to entities standing on newly frozen cells
    if (human.hp > 0) {
        for (const auto& p : frozen_cells) {
            if (human.pos == p) {
                human.is_frozen = true;
                add_log(tr("[RADIO] Human frozen! Cannot act next turn.",
                           "[RADIO] Nguoi bi dong bang! Khong the hanh dong luot toi."),
                        ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                break;
            }
        }
    }

    for (auto& z : zombies) {
        if (z->hp > 0) {
            for (const auto& p : frozen_cells) {
                if (z->pos == p) {
                    z->is_frozen = true;
                    // Sprinters thaw faster (1 turn), others need 2 turns
                    z->frozen_turns = (z->type == ZombieType::Fast) ? 1 : 2;
                    add_log(tr("[RADIO] " + z->name + " frozen for " + std::to_string(z->frozen_turns) + " turn(s)!",
                               "[RADIO] " + z->name + " bi dong bang " + std::to_string(z->frozen_turns) + " luot!"),
                            ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
                    break;
                }
            }
        }
    }

    add_log(tr("[ENV] Blizzard! " + std::to_string(frozen_cells.size()) + " water cell(s) frozen into ice.",
               "[MT] Bao tuyet! " + std::to_string(frozen_cells.size()) + " o nuoc dong bang thanh bang."),
            ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
}

void GameState::resolve_environment_turn() {
    if (dark_cloud_active) dark_cloud_active = false;

    std::discrete_distribution<int> event_dist({
        static_cast<double>(active_config.env_prob_clear),
        static_cast<double>(active_config.env_prob_wind),
        static_cast<double>(active_config.env_prob_rain),
        static_cast<double>(active_config.env_prob_clouds),
        static_cast<double>(active_config.env_prob_lightning),
        static_cast<double>(active_config.env_prob_heatwave),
        static_cast<double>(active_config.env_prob_blizzard)
    });
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
    } else if (event == 4) {
        apply_lightning_strike();
    } else if (event == 5) {
        apply_heatwave();
    } else {
        apply_blizzard();
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
        if (human_sick_stamina_penalty) {
            if (human.stamina > 0) {
                human.stamina = std::max(0, human.stamina - 1);
                add_log(tr("[INFECT] Sick Zombie bite penalty! Stamina reduced by 1.", "[LAY NHIEM] Hieu ung Zombie nhiem doc! Tru 1 stamina."), ImVec4(0.82f, 0.93f, 0.24f, 1.0f));
            }
            human_sick_stamina_penalty = false;
        }
        check_fire_interactions();
    }

    phase = TurnPhase::HumanTurn;
    input_mode = InputMode::MoveMode;
    add_log("=== HUMAN TURN " + std::to_string(current_turn) + " START ===", ImVec4(1.0f, 0.95f, 0.25f, 1.0f));
    turn_banner_fx.type = FXType::Electricity;
    turn_banner_fx.timer = 1.5f;
    turn_banner_fx.max_duration = 1.5f;
    turn_banner_fx.banner_text = "";
    // Fire spreads once after environment phase ends
    propagate_gradual_forest_fire();
    check_victory_conditions();
}

void GameState::update_environment_logic(float dt) {
    if (phase != TurnPhase::EnvironmentAnimating) return;
    
    // Update terrain transitions
    for (auto it = terrain_transitions.begin(); it != terrain_transitions.end();) {
        it->timer += dt;
        if (it->timer >= it->max_duration) {
            it = terrain_transitions.erase(it);
        } else {
            ++it;
        }
    }
    
    // Simply wait for the active environment FX animation to finish!
    if (active_fx.type != FXType::None) {
        return;
    }
    
    // Environment event has finished! Now tick grenades before starting human turn.
    bool triggered_explosion = false;
    for (auto& g : active_grenades) {
        if (g.active) {
            g.turns_left--;
            if (g.turns_left <= 0) {
                add_log("[RADIO] Grenade timer expires. Detonation!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
                queue_explosion(g.pos.x, g.pos.y);
                g.active = false;
                triggered_explosion = true;
            }
        }
    }
    active_grenades.erase(std::remove_if(active_grenades.begin(), active_grenades.end(), [](const GrenadeTimer& g) { return !g.active; }), active_grenades.end());
    
    if (triggered_explosion) {
        // Wait for grenade explosion animation to finish!
        return;
    }
    
    finish_environment_phase();
}

void GameState::queue_explosion(int cx, int cy, bool is_zombie_exploding) { explosion_queue.push({cx, cy, is_zombie_exploding}); }

void GameState::execute_explosion_internal(int cx, int cy, bool is_zombie_exploding) { 
    add_log("[EXPLOSION] Detonated at (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
    sfx("explosion");
    
    Terrain center_t = grid[cx][cy];
    int radius = is_zombie_exploding ? 1 : 2;
    if (center_t == Terrain::Water || center_t == Terrain::Ice) {
        radius = std::max(0, radius - 1);
    }

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
            }
        }
    }

    // Hủy loot trong vùng nổ
    destroy_loot_at_cells(active_fx.blast_cells);

    auto get_entity_at = [&](Position p) -> std::pair<std::string, void*> {
        if (human.hp > 0 && human.pos == p) return {"human", &human};
        for (size_t i = 0; i < zombies.size(); ++i) {
            if (zombies[i]->hp > 0 && zombies[i]->pos == p) return {"zombie", zombies[i].get()};
        }
        return {"none", nullptr};
    };

    struct DamageEvent {
        bool is_human;
        size_t zombie_idx;
        int damage;
        bool has_push;
        Position push_target;
        bool push_blocked;
        std::string block_type; // "wall", "entity", "none"
        void* blocking_entity;
        bool is_blocking_entity_human;
        size_t blocking_zombie_idx;
    };

    std::vector<DamageEvent> events;

    // Check Human
    bool human_hit = false;
    for (auto cell : active_fx.blast_cells) {
        if (human.pos == cell) {
            human_hit = true;
            break;
        }
    }

    if (human_hit) {
        int dist = std::max(std::abs(human.pos.x - cx), std::abs(human.pos.y - cy));
        DamageEvent ev{true, 0, 0, false, {0,0}, false, "none", nullptr, false, 0};
        if (dist == 0) {
            ev.damage = 3;
        } else if (dist == 1) {
            ev.damage = 2;
            ev.has_push = true;
            int dx = human.pos.x - cx;
            int dy = human.pos.y - cy;
            ev.push_target = {human.pos.x + dx, human.pos.y + dy};
            
            if (ev.push_target.x < 0 || ev.push_target.x >= width || ev.push_target.y < 0 || ev.push_target.y >= height) {
                ev.push_blocked = true;
                ev.block_type = "wall";
            } else if (grid[ev.push_target.x][ev.push_target.y] == Terrain::Wall) {
                ev.push_blocked = true;
                ev.block_type = "wall";
            } else {
                auto [ent_type, ent_ptr] = get_entity_at(ev.push_target);
                if (ent_type != "none") {
                    ev.push_blocked = true;
                    ev.block_type = "entity";
                    ev.blocking_entity = ent_ptr;
                    ev.is_blocking_entity_human = (ent_type == "human");
                    if (ent_type == "zombie") {
                        for (size_t j = 0; j < zombies.size(); ++j) {
                            if (zombies[j].get() == ent_ptr) {
                                ev.blocking_zombie_idx = j;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            ev.damage = 1;
        }
        events.push_back(ev);
    }

    // Check Zombies
    for (size_t i = 0; i < zombies.size(); ++i) {
        auto& zom = zombies[i];
        if (zom->hp <= 0) continue;
        
        bool zom_hit = false;
        for (auto cell : active_fx.blast_cells) {
            if (zom->pos == cell) {
                zom_hit = true;
                break;
            }
        }
        
        if (zom_hit) {
            int dist = std::max(std::abs(zom->pos.x - cx), std::abs(zom->pos.y - cy));
            DamageEvent ev{false, i, 0, false, {0,0}, false, "none", nullptr, false, 0};
            if (dist == 0) {
                ev.damage = 3;
            } else if (dist == 1) {
                ev.damage = 2;
                ev.has_push = true;
                int dx = zom->pos.x - cx;
                int dy = zom->pos.y - cy;
                ev.push_target = {zom->pos.x + dx, zom->pos.y + dy};
                
                if (ev.push_target.x < 0 || ev.push_target.x >= width || ev.push_target.y < 0 || ev.push_target.y >= height) {
                    ev.push_blocked = true;
                    ev.block_type = "wall";
                } else if (grid[ev.push_target.x][ev.push_target.y] == Terrain::Wall) {
                    ev.push_blocked = true;
                    ev.block_type = "wall";
                } else {
                    auto [ent_type, ent_ptr] = get_entity_at(ev.push_target);
                    if (ent_type != "none") {
                        ev.push_blocked = true;
                        ev.block_type = "entity";
                        ev.blocking_entity = ent_ptr;
                        ev.is_blocking_entity_human = (ent_type == "human");
                        if (ent_type == "zombie") {
                            for (size_t j = 0; j < zombies.size(); ++j) {
                                if (zombies[j].get() == ent_ptr) {
                                    ev.blocking_zombie_idx = j;
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                ev.damage = 1;
            }
            events.push_back(ev);
        }
    }

    std::vector<int> zombie_extra_damage(zombies.size(), 0);
    int human_extra_damage = 0;

    for (const auto& ev : events) {
        int final_damage = ev.damage;
        if (ev.has_push) {
            if (ev.push_blocked) {
                final_damage += 1;
                add_log(ev.is_human ? "-> Human recoil blocked by obstacle! suffers +1 impact damage." 
                                   : "-> Zombie #" + std::to_string(ev.zombie_idx+1) + " recoil blocked! suffers +1 impact damage.", 
                        ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                
                if (ev.block_type == "entity") {
                    if (ev.is_blocking_entity_human) {
                        human_extra_damage += 1;
                        add_log("-> Blocking Human suffers 1 impact damage!", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    } else {
                        zombie_extra_damage[ev.blocking_zombie_idx] += 1;
                        add_log("-> Blocking Zombie #" + std::to_string(ev.blocking_zombie_idx+1) + " suffers 1 impact damage!", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    }
                }
            } else {
                if (ev.is_human) {
                    human.pos = ev.push_target;
                    add_log("-> Human is blown back by the blast!", ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                    check_fire_interactions();
                    check_mine_interactions();
                } else {
                    zombies[ev.zombie_idx]->pos = ev.push_target;
                    add_log("-> Zombie #" + std::to_string(ev.zombie_idx+1) + " is blown back by the blast!", ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                    check_fire_interactions();
                }
            }
        }

        if (ev.is_human) {
            human_extra_damage += final_damage;
        } else {
            zombie_extra_damage[ev.zombie_idx] += final_damage;
        }
    }

    if (human_extra_damage > 0) {
        human.hp = std::max(0, human.hp - human_extra_damage);
        floating_texts.push_back({human.pos, -human_extra_damage, 1.0f, 1.0f});
        add_log("-> Human total explosion damage: -" + std::to_string(human_extra_damage) + " HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    }

    std::vector<Position> chain_reactions;

    for (size_t i = 0; i < zombies.size(); ++i) {
        if (zombie_extra_damage[i] > 0 && zombies[i]->hp > 0) {
            zombies[i]->hp -= zombie_extra_damage[i];
            floating_texts.push_back({zombies[i]->pos, -zombie_extra_damage[i], 1.0f, 1.0f});
            add_log("-> Zombie #" + std::to_string(i+1) + " total explosion damage: -" + std::to_string(zombie_extra_damage[i]) + " HP.", ImVec4(1.0f, 0.5f, 0.1f, 1.0f));
            
            if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                chain_reactions.push_back(zombies[i]->pos);
            }
        }
    }

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

    // Ice cells in blast radius melt into water; unfreeze any entities standing on them
    int ice_melted = 0;
    std::vector<Position> explosion_melted_ice;
    for (const auto& cell : active_fx.blast_cells) {
        if (grid[cell.x][cell.y] == Terrain::Ice) {
            grid[cell.x][cell.y] = Terrain::Water;
            ice_melted++;
            explosion_melted_ice.push_back(cell);
            // Unfreeze entities on this cell
            if (human.hp > 0 && human.pos == cell && human.is_frozen) {
                human.is_frozen = false;
                add_log(tr("[ICE] Explosion melted ice under Human! Unfrozen.",
                           "[BANG] Vu no lam tan bang duoi chan Nguoi! Giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }
            for (auto& z : zombies) {
                if (z->hp > 0 && z->pos == cell && z->is_frozen) {
                    z->is_frozen = false;
                    z->frozen_turns = 0;
                    add_log(tr("[ICE] Explosion melted ice under " + z->name + "! Unfrozen.",
                               "[BANG] Vu no lam tan bang duoi chan " + z->name + "! Giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                }
            }
        }
    }
    if (!explosion_melted_ice.empty()) thaw_loot_and_grenades_at(explosion_melted_ice);
    if (ice_melted > 0) {
        add_log(tr("[ICE] Explosion melted " + std::to_string(ice_melted) + " ice cell(s) into water!",
                   "[BANG] Vu no lam tan chay " + std::to_string(ice_melted) + " o bang thanh nuoc!"),
                ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    }

    check_victory_conditions(); 

    for (auto pos : chain_reactions) {
        queue_explosion(pos.x, pos.y, true);
    }
}

void GameState::zombie_single_step(size_t idx) { 
    if (game_over || game_won) return; 
    auto& zom = zombies[idx]; 
    if (zom->is_paralyzed) return;
    // Frozen: skip action — handled by update_zombie_logic which counts down turns
    if (zom->is_frozen) return;
    double lambda = GameConstants::Zombies::AI_LAMBDA; 
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
                if (new_dist > current_dist) w *= GameConstants::Zombies::AI_AWAY_WEIGHT; 
                else if (new_dist == current_dist && (dx != 0 || dy != 0)) w *= GameConstants::Zombies::AI_PERPENDICULAR_WEIGHT; 
                valid_moves.push_back(target); 
                weights.push_back(w); 
            } 
        } 
    } 

        if (!valid_moves.empty()) { 
        std::discrete_distribution<int> dist(weights.begin(), weights.end()); 
        Position old_pos = zom->pos;
        zom->pos = valid_moves[dist(rng)]; 
        if (mine_grid[zom->pos.x][zom->pos.y]) { 
            mine_grid[zom->pos.x][zom->pos.y] = false; 
            add_log("[RADIO] Zombie #" + std::to_string(idx + 1) + " stepped on a mine. Stand by for detonation!", ImVec4(1.0f, 0.38f, 0.22f, 1.0f));
            queue_explosion(zom->pos.x, zom->pos.y); 
        }
        // Ice slide check for zombie — only if it actually moved and is still alive
        if (zom->hp > 0 && grid[zom->pos.x][zom->pos.y] == Terrain::Ice) {
            int move_dx = zom->pos.x - old_pos.x;
            int move_dy = zom->pos.y - old_pos.y;
            if (move_dx != 0 || move_dy != 0) {
                bool stun = false;
                try_ice_slide(false, idx, move_dx, move_dy, stun);
                if (stun) {
                    // Force the end of zombie's turn
                    active_zombie_idx++;
                    active_zombie_substep = 0;
                }
                // try_ice_slide already calls check_fire_interactions + check_mine_interactions
                return;
            }
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

    if (input_mode == InputMode::TargetPistol || input_mode == InputMode::TargetShotgun || 
        input_mode == InputMode::TargetGrenade || input_mode == InputMode::TargetMolotov) {
        if (std::abs(tx - human.pos.x) > 1 || std::abs(ty - human.pos.y) > 1 || (tx == human.pos.x && ty == human.pos.y)) {
            return; // Ignore clicks outside the directional arrows
        }
    }

    if (input_mode == InputMode::TargetKnife) { 
        if (distance(human.pos, target) <= GameConstants::Weapons::KNIFE_RANGE) { 
            for (size_t i = 0; i < zombies.size(); ++i) { 
                if (zombies[i]->hp > 0 && zombies[i]->pos == target) { 
                    zombies[i]->hp -= GameConstants::Weapons::KNIFE_DAMAGE; 
                    floating_texts.push_back({zombies[i]->pos, -GameConstants::Weapons::KNIFE_DAMAGE, 1.0f, 1.0f});
                    human.stamina -= GameConstants::Weapons::KNIFE_STAMINA_COST; 
                    add_log("[RADIO] Blade connects with Zombie #" + std::to_string(i + 1) + ".", ImVec4(0.35f, 1.0f, 0.7f, 1.0f));
                    sfx("knife");
                    active_fx.type = FXType::Knife; 
                    active_fx.timer = 0.35f; 
                    active_fx.max_duration = 0.35f; 
                    active_fx.start_p = hCenter; 
                    active_fx.end_p = tCenter; 
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) queue_explosion(target.x, target.y, true); 
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
        human.stamina -= GameConstants::Weapons::PISTOL_STAMINA_COST; 
        sfx("pistol");
        Position hit_pos = human.pos; 
        bool hit_something = false; 

        for (int step = 1; step <= GameConstants::Weapons::PISTOL_RANGE; ++step) { 
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
                double chance = std::exp(-((double)(dist - 1) / GameConstants::Weapons::PISTOL_ACCURACY_LAMBDA));
                std::uniform_real_distribution<double> dist_chance(0.0, 1.0); 
                if (dist_chance(rng) <= chance) { 
                    zombies[i]->hp -= GameConstants::Weapons::PISTOL_DAMAGE; 
                    floating_texts.push_back({zombies[i]->pos, -GameConstants::Weapons::PISTOL_DAMAGE, 1.0f, 1.0f});
                    add_log("[RADIO] Pistol round lands on Zombie #" + std::to_string(i + 1) + ".", ImVec4(1.0f, 0.95f, 0.35f, 1.0f));
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) queue_explosion(hit_pos.x, hit_pos.y, true); 
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
        sfx("shotgun");
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
            int first_x = human.pos.x + vx;
            int first_y = human.pos.y + vy;
            // Diagonal shot: Half of 4x4 square with vertex at (1,1) relative to player, reaching (1,4) and (4,1)
            for (int dx_step = 0; dx_step <= 3; ++dx_step) {
                for (int dy_step = 0; dy_step <= 3; ++dy_step) {
                    if (dx_step + dy_step <= 3) {
                        int fx = human.pos.x + vx + dx_step * vx;
                        int fy = human.pos.y + vy + dy_step * vy;
                        if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                            if (!is_blocked_by_wall(Position{first_x, first_y}, Position{fx, fy})) {
                                active_fx.blast_cells.push_back({fx, fy});
                            }
                        }
                    }
                }
            }
        } else {
            // Straight shot: 3-step expanding cone (exactly 9 cells)
            int first_x = human.pos.x + vx;
            int first_y = human.pos.y + vy;
            int px = -vy, py = vx; 
            for (int step = 1; step <= 3; ++step) { 
                int cx = human.pos.x + vx * step; 
                int cy = human.pos.y + vy * step; 
                int spread = step - 1; 
                for (int s = -spread; s <= spread; ++s) { 
                    int fx = cx + px * s; 
                    int fy = cy + py * s; 
                    if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                        if (!is_blocked_by_wall(Position{first_x, first_y}, Position{fx, fy})) {
                            active_fx.blast_cells.push_back({fx, fy});
                        }
                    }
                } 
            }
        } 

        // Hủy loot trong vùng đạn shotgun
        destroy_loot_at_cells(active_fx.blast_cells);

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

            // Shotgun blast unfreezes frozen zombies
            if (zombies[i]->is_frozen) {
                zombies[i]->is_frozen = false;
                zombies[i]->frozen_turns = 0;
                add_log(tr("[ICE] Shotgun blast shattered the ice! Zombie #" + std::to_string(i + 1) + " unfrozen.",
                           "[BANG] Dan shotgun vo bang! Zombie #" + std::to_string(i + 1) + " duoc giai bang."),
                        ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }

            if (zombies[i]->hp <= 0) {
                if (zombies[i]->type == ZombieType::Exploding) {
                    queue_explosion(zombies[i]->pos.x, zombies[i]->pos.y, true);
                }
                continue;
            }

            int rx = zombies[i]->pos.x + vx;
            int ry = zombies[i]->pos.y + vy;

            if (rx < 0 || rx >= width || ry < 0 || ry >= height || grid[rx][ry] == Terrain::Wall) {
                zombies[i]->hp -= 1;
                floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                    queue_explosion(zombies[i]->pos.x, zombies[i]->pos.y, true);
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
                        queue_explosion(zombies[i]->pos.x, zombies[i]->pos.y, true);
                    }
                    if (zombies[blocking_zombie_idx]->hp <= 0 && zombies[blocking_zombie_idx]->type == ZombieType::Exploding) {
                        queue_explosion(zombies[blocking_zombie_idx]->pos.x, zombies[blocking_zombie_idx]->pos.y, true);
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
        sfx("grenade_land");
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
            add_log("[RADIO] Stupid! That direction is full of walls, the grenade falls at Human's feet and explodes immediately!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            queue_explosion(human.pos.x, human.pos.y);
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
        sfx("molotov");
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
                    // Frozen entity hit by molotov: unfreeze instead of ignite, no fire spread
                    if (zombies[i]->is_frozen) {
                        zombies[i]->is_frozen = false;
                        zombies[i]->frozen_turns = 0;
                        add_log(tr("-> Molotov hit frozen " + zombies[i]->name + "! Ice melted — unfrozen but not ignited. No fire spread.",
                                   "-> Bom xang trung " + zombies[i]->name + " dang dong bang! Bang tan — giai bang nhung khong chay. Khong lan lua."),
                                ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                        hit_zombie = true;
                        current_zombie_hit = true;
                    } else {
                        zombies[i]->hp -= 1;
                        floating_texts.push_back({zombies[i]->pos, -1, 1.0f, 1.0f});
                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            queue_explosion(zombies[i]->pos.x, zombies[i]->pos.y, true);
                        }
                        
                        if (grid[cx][cy] == Terrain::Water) {
                            add_log(tr("-> Molotov hit " + zombies[i]->name + " in water! Target took 1 impact damage, but fire fizzled.",
                                       "-> Bom xang trung " + zombies[i]->name + " duoi nuoc! Mat 1 mau va lua bi dap tat."), ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                            zombies[i]->is_burning = false;
                        } else {
                            add_log(tr("-> Molotov hit " + zombies[i]->name + " directly! Took 1 impact damage and ignited.",
                                       "-> Bom xang trung " + zombies[i]->name + " truc tiep! Mat 1 mau va bi boc chay."), ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                            zombies[i]->is_burning = false; // Temporarily false so check_fire_interactions ignites it without extra immediate damage
                        }
                        hit_zombie = true;
                        current_zombie_hit = true;
                    }
                    break; 
                } 
            } 
            if (current_zombie_hit) break;
        }
        Position hit_pos = landing_pos;
        
        if (all_walls) {
            add_log("[RADIO] Stupid! That direction is full of walls, the molotov falls at Human's feet and ignites immediately!", ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
        } else {
            sf::Vector2f hCenter = getCellCenter(human.pos.x, human.pos.y, cellSize, boardOffset);
            active_fx.type = FXType::Molotov; 
            active_fx.timer = 0.5f; 
            active_fx.max_duration = 0.5f; 
            active_fx.start_p = hCenter; 
            active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset);
        }
        
        if (grid[hit_pos.x][hit_pos.y] == Terrain::Water) {
            // Molotov fizzles in water — no heat transfer, no ice melting
            add_log("-> Molotov landed in water. Fizzled out!", ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        } else if (grid[hit_pos.x][hit_pos.y] == Terrain::Ice) {
            grid[hit_pos.x][hit_pos.y] = Terrain::Water;
            terrain_transitions.push_back({hit_pos, Terrain::Ice, Terrain::Water, 0.0f, 0.8f});
            thaw_loot_and_grenades_at({hit_pos});
            add_log(tr("[ICE] Molotov melted the ice at (" + std::to_string(hit_pos.x + 1) + ", " + std::to_string(hit_pos.y + 1) + ")! Ice -> Water.",
                       "[BANG] Bom xang lam tan chay bang tai (" + std::to_string(hit_pos.x + 1) + ", " + std::to_string(hit_pos.y + 1) + ")! Bang -> Nuoc."),
                    ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            // Unfreeze any entity standing on the now-melted cell
            if (human.hp > 0 && human.pos == hit_pos && human.is_frozen) {
                human.is_frozen = false;
                add_log(tr("[ICE] Molotov melted ice under Human! Unfrozen.",
                           "[BANG] Bom xang tan bang duoi chan Nguoi! Giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }
            for (auto& z : zombies) {
                if (z->hp > 0 && z->pos == hit_pos && z->is_frozen) {
                    z->is_frozen = false;
                    z->frozen_turns = 0;
                    add_log(tr("[ICE] Molotov melted ice under " + z->name + "! Unfrozen.",
                               "[BANG] Bom xang tan bang duoi chan " + z->name + "! Giai bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                }
            }
            // Heat also melts adjacent ice cells
            melt_adjacent_ice(hit_pos.x, hit_pos.y);
        } else if (grid[hit_pos.x][hit_pos.y] == Terrain::Dirt || grid[hit_pos.x][hit_pos.y] == Terrain::Forest) {
            // Check if a frozen entity is on the landing cell — if so, no fire spread
            bool frozen_entity_on_cell = false;
            if (human.hp > 0 && human.pos == hit_pos && human.is_frozen) frozen_entity_on_cell = true;
            if (!frozen_entity_on_cell) {
                for (const auto& z : zombies) {
                    if (z->hp > 0 && z->pos == hit_pos && z->is_frozen) { frozen_entity_on_cell = true; break; }
                }
            }
            if (frozen_entity_on_cell) {
                // Unfreeze the entity but no fire
                if (human.hp > 0 && human.pos == hit_pos && human.is_frozen) {
                    human.is_frozen = false;
                    add_log(tr("[ICE] Molotov heat melted the ice around Human! Unfrozen, no fire spread.",
                               "[BANG] Nhiet bom xang tan bang quanh Nguoi! Giai bang, khong lan lua."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                }
                for (auto& z : zombies) {
                    if (z->hp > 0 && z->pos == hit_pos && z->is_frozen) {
                        z->is_frozen = false;
                        z->frozen_turns = 0;
                        add_log(tr("[ICE] Molotov heat melted the ice around " + z->name + "! Unfrozen, no fire spread.",
                                   "[BANG] Nhiet bom xang tan bang quanh " + z->name + "! Giai bang, khong lan lua."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                    }
                }
            } else {
                set_cell_on_fire(hit_pos.x, hit_pos.y);
                add_log("[RADIO] Fire spreads at (" + std::to_string(hit_pos.x + 1) + ", " + std::to_string(hit_pos.y + 1) + "). Keep distance!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
            }
            // Heat from fire/molotov melts adjacent ice cells regardless
            melt_adjacent_ice(hit_pos.x, hit_pos.y);
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
    
    // Fire spreads once at the START of zombie phase (after human turn ends)
    propagate_gradual_forest_fire();

    phase = TurnPhase::ZombieAnimating; 
    active_zombie_idx = 0; 
    active_zombie_substep = 0; 
    zombie_action_timer = 0.0f;
}

void GameState::update_zombie_logic(float dt) { 
    // Update terrain transitions (purely visual, always runs)
    for (auto it = terrain_transitions.begin(); it != terrain_transitions.end();) {
        it->timer += dt;
        if (it->timer >= it->max_duration) {
            it = terrain_transitions.erase(it);
        } else {
            ++it;
        }
    }
    
    // Update ice slide animation (purely visual — logical position already at destination)
    if (ice_slide_animation.active) {
        ice_slide_animation.step_timer += dt;
        if (ice_slide_animation.step_timer >= ice_slide_animation.step_duration) {
            ice_slide_animation.step_timer = 0.0f;
            ice_slide_animation.current_step++;
            if (ice_slide_animation.current_step >= (int)ice_slide_animation.path.size()) {
                ice_slide_animation.active = false;
            }
        }
        // Don't return — let zombie phase logic continue normally
        // The visual position is overridden in rendering based on ice_slide_animation
    }

    if (phase != TurnPhase::ZombieAnimating || active_fx.type != FXType::None || !attack_animations.empty() || ice_slide_animation.active) return; 
    
    if (active_zombie_idx >= zombies.size()) { 
        if (active_zombie_substep == 0) {
            for (auto& z : zombies) {
                if (z->hp > 0 && z->is_burning) {
                    z->hp -= 1;
                    floating_texts.push_back({z->pos, -1, 1.0f, 1.0f});
                    z->is_burning = false;
                    add_log("[FIRE] " + z->name + " suffered 1 Burn Damage!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    if (z->hp <= 0 && z->type == ZombieType::Exploding) queue_explosion(z->pos.x, z->pos.y, true);
                }
            }
            
            // Fire spreads once after ALL zombies have finished their turns
            propagate_gradual_forest_fire();

            if (!active_config.enable_environment) {
                for (auto& g : active_grenades) {
                    if (g.active) {
                        g.turns_left--;
                        if (g.turns_left <= 0) {
                            add_log("[RADIO] Grenade timer expires. Detonation!", ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
                            queue_explosion(g.pos.x, g.pos.y);
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
    if (zom->is_frozen) {
        zom->frozen_turns--;
        if (zom->frozen_turns <= 0) {
            zom->is_frozen = false;
            zom->frozen_turns = 0;
            add_log(tr("[ICE] " + zom->name + " thawed out! Frozen effect expired.",
                       "[BANG] " + zom->name + " da giai bang! Het hieu ung dong bang."), ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        } else {
            add_log(tr("[ICE] " + zom->name + " is frozen solid! Loses this action. (" + std::to_string(zom->frozen_turns) + " turn(s) left)",
                       "[BANG] " + zom->name + " bi dong cung! Mat hanh dong nay. (Con " + std::to_string(zom->frozen_turns) + " luot)"), ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
        }
        active_zombie_idx++;
        active_zombie_substep = 0;
        return;
    }
    zombie_action_timer += dt;
    if (zom->pending_attack) {
        if (zombie_action_timer >= 0.2f) { // Delay before attack
            zombie_action_timer = 0.0f;
            zom->pending_attack = false;
            
            // Re-verify they are still adjacent and human is alive
            int dx = std::abs(zom->pos.x - human.pos.x);
            int dy = std::abs(zom->pos.y - human.pos.y);
            if (human.hp > 0 && zom->hp > 0 && dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                int dmg = 0;
                VisualFX atk_fx;
                atk_fx.cx = human.pos.x;
                atk_fx.cy = human.pos.y;
                atk_fx.timer = 0.5f;
                atk_fx.max_duration = 0.5f;

                bool in_water = (grid[zom->pos.x][zom->pos.y] == Terrain::Water);
                if (dx + dy == 1) { // Orthogonal -> Bite
                    dmg = in_water ? GameConstants::Zombies::BITE_WATER_DAMAGE : GameConstants::Zombies::BITE_DAMAGE;
                    atk_fx.type = FXType::Bite;
                    sfx("zombie_bite");
                    if (in_water) {
                        add_log("-> " + zom->name + " BITES human sluggishly in water! -" + std::to_string(GameConstants::Zombies::BITE_WATER_DAMAGE) + " HP.", ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
                    } else {
                        add_log("-> " + zom->name + " BITES human! -" + std::to_string(GameConstants::Zombies::BITE_DAMAGE) + " HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                    }
                } else { // Diagonal -> Scratch
                    dmg = in_water ? GameConstants::Zombies::SCRATCH_WATER_DAMAGE : GameConstants::Zombies::SCRATCH_DAMAGE;
                    atk_fx.type = FXType::Scratch;
                    sfx("zombie_scratch");
                    if (in_water) {
                        add_log("-> " + zom->name + " tries to SCRATCH human but flails harmlessly in water! " + std::to_string(GameConstants::Zombies::SCRATCH_WATER_DAMAGE) + " HP.", ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    } else {
                        add_log("-> " + zom->name + " SCRATCHES human! -" + std::to_string(GameConstants::Zombies::SCRATCH_DAMAGE) + " HP.", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    }
                }
                
                human.hp = std::max(0, human.hp - dmg); 
                floating_texts.push_back({human.pos, -dmg, 1.0f, 1.0f});
                attack_animations.push_back(atk_fx);

                if (zom->type == ZombieType::Vampire && dmg > 0) {
                    zom->hp += dmg;
                    floating_texts.push_back({zom->pos, dmg, 1.0f, 1.0f});
                }
                if (zom->type == ZombieType::Sick) {
                    if (dx + dy == 1) { // Orthogonal -> Bite
                        int turns_left = std::max(0, turn_limit - current_turn);
                        int halved_left = turns_left / 2;
                        turn_limit = current_turn + halved_left;
                        add_log("-> Infection! Remaining turns halved (" + std::to_string(turns_left) + " -> " + std::to_string(halved_left) + ") & -1 Stamina next turn.", ImVec4(0.82f, 0.93f, 0.24f, 1.0f));
                        human_sick_stamina_penalty = true;
                    }
                }
                check_victory_conditions();
            }
            active_zombie_idx++;
            active_zombie_substep = 0;
        }
        return;
    }

    if (zombie_action_timer >= std::max(0.05f, ZOMBIE_STEP_DELAY)) { 
        zombie_action_timer = 0.0f; 
        
        size_t old_idx = active_zombie_idx;
        zombie_single_step(active_zombie_idx); 

        if (game_over || game_won) return;
        
        if (zom->hp <= 0) {
            if (active_zombie_idx == old_idx) { // Ensure we don't skip a zombie if it already advanced
                active_zombie_idx++; active_zombie_substep = 0; 
            }
            return;
        }
        
        if (active_zombie_idx != old_idx) {
            // Zombie turn ended from stun or max moves inside zombie_single_step
            return;
        }
        
        if (zom->hp > 0) { 
            int dx = std::abs(zom->pos.x - human.pos.x);
            int dy = std::abs(zom->pos.y - human.pos.y);
            
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                zom->pending_attack = true;
                return; // Wait for timer to execute attack
            }
        } 
        active_zombie_substep++; 
        int max_moves = zom->getMovesPerTurn();
        if (grid[zom->pos.x][zom->pos.y] == Terrain::Water) {
            if (max_moves > GameConstants::TerrainPenalties::WATER_MOVES_MAX_SPRINTER) max_moves = GameConstants::TerrainPenalties::WATER_MOVES_MAX_SPRINTER;
        }
        if (active_zombie_substep >= max_moves) { 
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
    // Spawn loot for any zombie that just died
    spawn_loot_for_newly_dead();
}

void GameState::propagate_gradual_forest_fire() {
    std::vector<Position> to_catch_fire;
    std::vector<Position> ice_to_melt;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            bool near_fire = false;
            for (const auto& d : dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[nx][ny] == Terrain::Fire) {
                    near_fire = true;
                    break;
                }
            }
            if (!near_fire) continue;

            if (grid[x][y] == Terrain::Forest) {
                to_catch_fire.push_back({x, y});
            } else if (grid[x][y] == Terrain::Ice) {
                ice_to_melt.push_back({x, y});
            }
        }
    }

    // Melt ice adjacent to fire
    for (const auto& p : ice_to_melt) {
        grid[p.x][p.y] = Terrain::Water;
        terrain_transitions.push_back({p, Terrain::Ice, Terrain::Water, 0.0f, 0.8f});
        thaw_loot_and_grenades_at({p});
        add_log(tr("[ICE] Fire heat melted ice at (" + std::to_string(p.x + 1) + ", " + std::to_string(p.y + 1) + ")!",
                   "[BANG] Nhiet lua lam tan bang tai (" + std::to_string(p.x + 1) + ", " + std::to_string(p.y + 1) + ")!"),
                ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
        // Unfreeze any entity on the melted cell
        if (human.hp > 0 && human.pos == p && human.is_frozen) {
            human.is_frozen = false;
            add_log(tr("[ICE] Ice melted under Human! Unfrozen.", "[BANG] Bang tan duoi chan Nguoi! Giai bang."),
                    ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        }
        for (auto& z : zombies) {
            if (z->hp > 0 && z->pos == p && z->is_frozen) {
                z->is_frozen = false;
                z->frozen_turns = 0;
                add_log(tr("[ICE] Ice melted under " + z->name + "! Unfrozen.",
                           "[BANG] Bang tan duoi chan " + z->name + "! Giai bang."),
                        ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            }
        }
    }
    
    if (!to_catch_fire.empty()) {
        add_log(tr("[FIRE] Forest wildfire spreads gradually to " + std::to_string(to_catch_fire.size()) + " forest cells!",
                   "[FIRE] Lua rung lan tu tu sang " + std::to_string(to_catch_fire.size()) + " o rung tiep theo!"), ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
        for (const auto& p : to_catch_fire) {
            grid[p.x][p.y] = Terrain::Fire;
            bool exists = false;
            for (const auto& fc : fire_cells) {
                if (fc.pos == p) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                fire_cells.push_back({p, 2});
            }
            
            if (human.hp > 0 && human.pos == p) {
                if (!human.is_burning) {
                    human.is_burning = true;
                    add_log(tr("[FIRE] Human is caught in the forest wildfire! Started burning.", "[FIRE] Human bi mac ket trong dam chay rung! Bat dau thieu dot."), ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
                }
            }
            for (auto& z : zombies) {
                if (z->hp > 0 && z->pos == p) {
                    if (!z->is_burning) {
                        z->is_burning = true;
                        add_log(tr("[FIRE] " + z->name + " is caught in the forest wildfire! Started burning.", "[FIRE] " + z->name + " bi mac ket trong dam chay rung! Bat dau thieu dot."), ImVec4(1.0f, 0.3f, 0.0f, 1.0f));
                    }
                }
            }
        }
        // Hủy loot tại các ô rừng vừa bốc lửa
        destroy_loot_at_cells(to_catch_fire);
        check_fire_interactions();
    }
}

void GameState::set_cell_on_fire(int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    if (grid[x][y] == Terrain::Forest || grid[x][y] == Terrain::Dirt) {
        grid[x][y] = Terrain::Fire;
        bool exists = false;
        for (const auto& fc : fire_cells) {
            if (fc.pos.x == x && fc.pos.y == y) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            fire_cells.push_back({Position{x, y}, 2});
            sfx("fire");
        }
        // Hủy loot tại ô vừa bốc lửa
        destroy_loot_at_cells({{x, y}});
        check_fire_interactions();
    }
}

void GameState::destroy_loot_at_cells(const std::vector<Position>& cells) {
    int destroyed = 0;
    loot_drops.erase(std::remove_if(loot_drops.begin(), loot_drops.end(),
        [&](const LootDrop& ld) {
            for (const auto& p : cells) {
                if (ld.pos == p) { destroyed++; return true; }
            }
            return false;
        }), loot_drops.end());
    if (destroyed > 0) {
        add_log(tr("[LOOT] " + std::to_string(destroyed) + " loot drop(s) destroyed!",
                   "[LOOT] " + std::to_string(destroyed) + " goi do bi pha huy!"),
                ImVec4(0.6f, 0.4f, 0.1f, 1.0f));
    }
}

void GameState::check_loot_on_fire() {
    std::vector<Position> fire_positions;
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
            if (grid[x][y] == Terrain::Fire)
                fire_positions.push_back({x, y});
    if (!fire_positions.empty())
        destroy_loot_at_cells(fire_positions);
}

void GameState::thaw_loot_and_grenades_at(const std::vector<Position>& cells) {
    // Khi ô Ice tan thành Water, giải đóng băng loot và grenade bị kẹt tại đó
    for (auto& ld : loot_drops) {
        if (!ld.frozen_under_ice) continue;
        for (const auto& p : cells) {
            if (ld.pos == p) { ld.frozen_under_ice = false; break; }
        }
    }
    for (auto& g : active_grenades) {
        if (!g.active || !g.frozen_under_ice) continue;
        for (const auto& p : cells) {
            if (g.pos == p) { g.frozen_under_ice = false; break; }
        }
    }
}

void GameState::use_ice_pick() {
    if (human.hp <= 0) return;
    int px = human.pos.x, py = human.pos.y;
    if (grid[px][py] != Terrain::Ice) return;

    int cost = human.is_frozen
        ? GameConstants::Weapons::ICE_PICK_STAMINA_COST_FROZEN
        : GameConstants::Weapons::ICE_PICK_STAMINA_COST_NORMAL;

    if (human.stamina < cost) {
        add_log(tr("[ICE PICK] Not enough stamina! Need " + std::to_string(cost) + " stamina.",
                   "[CUOC PHA BANG] Khong du the luc! Can " + std::to_string(cost) + " the luc."),
                ImVec4(0.5f, 0.75f, 1.0f, 1.0f));
        return;
    }

    human.stamina -= cost;
    grid[px][py] = Terrain::Water;
    terrain_transitions.push_back({Position{px, py}, Terrain::Ice, Terrain::Water, 0.0f, 0.8f});
    thaw_loot_and_grenades_at({{px, py}});
    sfx("ice_pick");

    if (human.is_frozen) {
        human.is_frozen = false;
        add_log(tr("[ICE PICK] Human breaks the ice and escapes! Ice -> Water. (-2 stamina)",
                   "[CUOC PHA BANG] Nguoi pha bang thoat ra! Bang -> Nuoc. (-2 the luc)"),
                ImVec4(0.5f, 0.9f, 1.0f, 1.0f));
    } else {
        add_log(tr("[ICE PICK] Human breaks the ice underfoot! Ice -> Water. (-1 stamina)",
                   "[CUOC PHA BANG] Nguoi pha bang duoi chan! Bang -> Nuoc. (-1 the luc)"),
                ImVec4(0.5f, 0.9f, 1.0f, 1.0f));
    }

    // Auto-pickup any loot now accessible
    check_loot_pickup();
    input_mode = InputMode::MoveMode;
}

void GameState::spawn_loot_for_newly_dead() {
    for (auto& z : zombies) {
        if (z->hp <= 0 && !z->loot_spawned) {
            z->loot_spawned = true;
            spawn_loot_at(z->pos);
        }
    }
}

void GameState::spawn_loot_at(Position pos) {
    // Xác suất loot: 55% junk, 45% item hữu ích
    // Trong 45% item: phân bổ theo độ hiếm
    std::uniform_int_distribution<int> roll(0, 99);
    int r = roll(rng);

    LootType type;
    if (r < 75) {
        type = LootType::Junk;
    } else if (r < 80) {
        type = LootType::PistolAmmo;   // 5%
    } else if (r < 85) {
        type = LootType::StaminaPotion;// 4%
    } else if (r < 89) {
        type = LootType::HealthPotion; // 4%
    } else if (r < 93) {
        type = LootType::ShotgunAmmo;  // 3%
    } else if (r < 96) {
        type = LootType::Grenade;      // 2%
    } else if (r < 98) {
        type = LootType::Molotov;      // 2%
    } else {
        type = LootType::Mine;         // 2%
    }

    // Xóa loot cũ ở cùng vị trí nếu có
    loot_drops.erase(std::remove_if(loot_drops.begin(), loot_drops.end(),
        [&](const LootDrop& ld) { return ld.pos == pos; }), loot_drops.end());

    loot_drops.push_back({pos, type, 0.0f});
}

void GameState::check_loot_pickup() {
    if (human.hp <= 0) return;

    // Ô nước và ô Ice thông thường đều nhặt được bình thường
    // Chỉ loot bị kẹt dưới băng (frozen_under_ice) mới cần xử lý đặc biệt

    for (auto it = loot_drops.begin(); it != loot_drops.end(); ) {
        if (it->pos != human.pos) { ++it; continue; }

        // Loot bị kẹt dưới băng: cần tốn 1 stamina để phá băng
        if (it->frozen_under_ice) {
            if (human.stamina >= 1) {
                human.stamina -= 1;
                it->frozen_under_ice = false;
                add_log(tr("[LOOT] You break through the ice to reach the loot! (-1 stamina)",
                           "[LOOT] Ban pha bang de lay do! (-1 the luc)"),
                        ImVec4(0.5f, 0.85f, 1.0f, 1.0f));
                // Tiếp tục xuống để nhặt loot ngay
            } else {
                add_log(tr("[LOOT] Loot is frozen under ice. Need 1 stamina to break through!",
                           "[LOOT] Do bi ket duoi bang. Can 1 the luc de pha bang!"),
                        ImVec4(0.5f, 0.75f, 1.0f, 1.0f));
                ++it; continue;
            }
        }

        switch (it->type) {
                case LootType::Junk:
                    sfx("footstep"); // âm thanh nhặt đồ vô dụng
                    add_log(tr("[LOOT] You found... a zombie's old sock. Useless.",
                               "[LOOT] Ban nhat duoc... chiec tat cu cua zombie. Vo dung."),
                            ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    break;
                case LootType::HealthPotion:
                    human.hp = std::min(human.hp + 2, 20); // cap tối đa 20
                    floating_texts.push_back({human.pos, +2, 1.0f, 1.0f});
                    sfx("heal");
                    add_log(tr("[LOOT] Health Potion! +2 HP.", "[LOOT] Binh mau! +2 HP."),
                            ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
                    break;
                case LootType::StaminaPotion:
                    human.stamina = 6;
                    sfx("heal");
                    add_log(tr("[LOOT] Stamina Potion! Stamina restored to 6.", "[LOOT] Binh the luc! Stamina phuc hoi ve 6."),
                            ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                    break;
                case LootType::PistolAmmo:
                    human.pistol_ammo += 3;
                    sfx("footstep");
                    add_log(tr("[LOOT] Pistol Ammo! +3 rounds.", "[LOOT] Dan luc! +3 vien."),
                            ImVec4(1.0f, 0.95f, 0.35f, 1.0f));
                    break;
                case LootType::ShotgunAmmo:
                    human.shotgun_ammo += 1;
                    sfx("footstep");
                    add_log(tr("[LOOT] Shotgun Shell! +1 shell.", "[LOOT] Dan shotgun! +1 vien."),
                            ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
                    break;
                case LootType::Grenade:
                    human.grenades += 1;
                    sfx("footstep");
                    add_log(tr("[LOOT] Grenade! +1 grenade.", "[LOOT] Luu dan! +1 qua."),
                            ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                    break;
                case LootType::Molotov:
                    human.molotovs += 1;
                    sfx("footstep");
                    add_log(tr("[LOOT] Molotov Cocktail! +1 molotov.", "[LOOT] Bom xang! +1 chai."),
                            ImVec4(1.0f, 0.45f, 0.1f, 1.0f));
                    break;
                case LootType::Mine:
                    human.mines += 1;
                    sfx("footstep");
                    add_log(tr("[LOOT] Land Mine! +1 mine.", "[LOOT] Min! +1 qua."),
                            ImVec4(0.9f, 0.9f, 0.2f, 1.0f));
                    break;
            }
            it = loot_drops.erase(it);
    }
}
