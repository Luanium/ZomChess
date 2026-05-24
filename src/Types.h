#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/System/Vector2.hpp>

#include "GameConstants.h"

enum class Terrain { Dirt, Water, Wall, Fire, Forest, Ice };
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade, TargetMolotov };
enum class ZombieType { Normal, Fast, Exploding, Vampire, Sick };
enum class TurnPhase { HumanTurn, ZombieAnimating, EnvironmentAnimating };

// Added Bite and Scratch FX
enum class FXType { None, Knife, Pistol, Shotgun, Explosion, Molotov, Bite, Scratch, Wind, Rain, DarkCloud, Lightning, Electricity, GrenadeFly, Heatwave, Blizzard };
enum class GameScene { MainMenu, Playing, MapEditor };

struct Position {
    int x;
    int y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct GameConfig {
    int map_width = 15;
    int map_height = 15;

    int ratio_wall = 10;
    int ratio_water = 10;
    int ratio_forest = 15;
    int ratio_dirt = 55;
    int ratio_ice = 10;  // Ice terrain ratio for procedural generation

    int human_hp = GameConstants::Difficulty::Medium::HUMAN_HP;
    int initial_stamina = GameConstants::Difficulty::Medium::INITIAL_STAMINA;
    int pistol_ammo = GameConstants::Difficulty::Medium::PISTOL_AMMO;
    int shotgun_ammo = GameConstants::Difficulty::Medium::SHOTGUN_AMMO;
    int grenades = GameConstants::Difficulty::Medium::GRENADES;
    int mines = GameConstants::Difficulty::Medium::MINES;
    int molotovs = GameConstants::Difficulty::Medium::MOLOTOVS; 
    int turn_limit = 50;

    int count_normal = GameConstants::Difficulty::Medium::COUNT_NORMAL;
    int count_fast = GameConstants::Difficulty::Medium::COUNT_FAST;
    int count_exploding = GameConstants::Difficulty::Medium::COUNT_EXPLODING;
    int count_vampire = GameConstants::Difficulty::Medium::COUNT_VAMPIRE;
    int count_sick = GameConstants::Difficulty::Medium::COUNT_SICK;

    bool spawn_shield = true;      
    bool custom_map_mode = false;  
    bool enable_environment = true;

    // Weather probabilities (must sum to 100)
    int env_prob_clear = 50;
    int env_prob_wind = 14;
    int env_prob_rain = 12;
    int env_prob_clouds = 4;
    int env_prob_lightning = 8;
    int env_prob_heatwave = 6;   // Nắng nóng gay gắt
    int env_prob_blizzard = 6;   // Băng giá

    Position custom_human_pos{1, 1};
    std::vector<std::vector<Terrain>> custom_grid; 
};

struct LogLine {
    std::string text;
    ImVec4 color;
};

struct GrenadeTimer {
    bool active = false;
    Position pos{0, 0};
    int turns_left = 2;
};

struct FireCell {
    Position pos;
    int duration; 
};

// Animation for ice slide (entity moves step by step)
struct IceSlideAnimation {
    bool active = false;
    bool is_human = false;
    size_t zombie_idx = 0;
    std::vector<Position> path;  // Path of positions to slide through
    int current_step = 0;
    float step_timer = 0.0f;
    float step_duration = 0.1f;  // Time per step
};

// Animation for terrain transitions (smooth color change)
struct TerrainTransitionAnimation {
    Position pos;
    Terrain from_terrain;
    Terrain to_terrain;
    float timer = 0.0f;
    float max_duration = 0.8f;  // Smooth transition over 0.8 seconds
};

struct VisualFX {
    FXType type = FXType::None;
    float timer = 0.0f;
    float max_duration = 0.35f;
    sf::Vector2f start_p;
    sf::Vector2f end_p;
    std::vector<Position> blast_cells;
    std::vector<Position> extinguished_cells;
    int cx = -1;
    int cy = -1;
    int dx = 0;
    int dy = 0;
    std::string banner_text = "";
    float overlay_intensity = 0.0f;  // For heatwave/blizzard overlay
};

// System for dynamic floating damage/heal numbers
struct FloatingText {
    Position pos;
    int amount; 
    float timer = 1.0f;
    float max_duration = 1.0f;
};

#endif // TYPES_H
