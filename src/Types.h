#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/System/Vector2.hpp>

#include "GameConstants.h"

enum class Terrain { Dirt, Water, Wall, Fire, Forest, Ice };
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade, TargetMolotov, UseIcePick };
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
    int map_width  = GameConstants::Defaults::MAP_WIDTH;
    int map_height = GameConstants::Defaults::MAP_HEIGHT;

    int ratio_wall   = GameConstants::Defaults::CONFIG_RATIO_WALL;
    int ratio_water  = GameConstants::Defaults::CONFIG_RATIO_WATER;
    int ratio_forest = GameConstants::Defaults::CONFIG_RATIO_FOREST;
    int ratio_dirt   = GameConstants::Defaults::CONFIG_RATIO_DIRT;
    int ratio_ice    = GameConstants::Defaults::CONFIG_RATIO_ICE;

    int human_hp      = GameConstants::Difficulty::Medium::HUMAN_HP;
    int initial_stamina = GameConstants::Difficulty::Medium::INITIAL_STAMINA;
    int pistol_ammo   = GameConstants::Difficulty::Medium::PISTOL_AMMO;
    int shotgun_ammo  = GameConstants::Difficulty::Medium::SHOTGUN_AMMO;
    int grenades      = GameConstants::Difficulty::Medium::GRENADES;
    int mines         = GameConstants::Difficulty::Medium::MINES;
    int molotovs      = GameConstants::Difficulty::Medium::MOLOTOVS;
    int turn_limit    = GameConstants::Defaults::TURN_LIMIT;

    int count_normal   = GameConstants::Difficulty::Medium::COUNT_NORMAL;
    int count_fast     = GameConstants::Difficulty::Medium::COUNT_FAST;
    int count_exploding = GameConstants::Difficulty::Medium::COUNT_EXPLODING;
    int count_vampire  = GameConstants::Difficulty::Medium::COUNT_VAMPIRE;
    int count_sick     = GameConstants::Difficulty::Medium::COUNT_SICK;

    bool spawn_shield = true;
    bool custom_map_mode = false;
    bool enable_environment = true;

    // Weather probabilities (must sum to 100)
    int env_prob_clear     = GameConstants::Defaults::ENV_PROB_CLEAR;
    int env_prob_wind      = GameConstants::Defaults::ENV_PROB_WIND;
    int env_prob_rain      = GameConstants::Defaults::ENV_PROB_RAIN;
    int env_prob_clouds    = GameConstants::Defaults::ENV_PROB_CLOUDS;
    int env_prob_lightning = GameConstants::Defaults::ENV_PROB_LIGHTNING;
    int env_prob_heatwave  = GameConstants::Defaults::ENV_PROB_HEATWAVE;  // Nắng nóng gay gắt
    int env_prob_blizzard  = GameConstants::Defaults::ENV_PROB_BLIZZARD;  // Băng giá

    Position custom_human_pos{GameConstants::Defaults::CUSTOM_HUMAN_X, GameConstants::Defaults::CUSTOM_HUMAN_Y};
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
    bool frozen_under_ice = false; // Bị kẹt dưới băng (ô nước bị đóng băng)
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
    float step_duration = GameConstants::Ice::SLIDE_STEP_DURATION;
};

// Animation for terrain transitions (smooth color change)
struct TerrainTransitionAnimation {
    Position pos;
    Terrain from_terrain;
    Terrain to_terrain;
    float timer = 0.0f;
    float max_duration = GameConstants::Ice::TERRAIN_TRANSITION_DURATION;
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

enum class LootType {
    Junk,         // Vô dụng
    HealthPotion, // +2 HP
    StaminaPotion,// Stamina về 6
    PistolAmmo,   // +3 đạn pistol
    ShotgunAmmo,  // +1 đạn shotgun
    Grenade,      // +1 lựu đạn
    Molotov,      // +1 bom xăng
    Mine          // +1 mìn
};

struct LootDrop {
    Position pos;
    LootType type;
    float blink_timer = 0.0f; // Dùng để nhấp nháy "?"
    bool frozen_under_ice = false; // Bị kẹt dưới băng (ô nước bị đóng băng)
};

#endif // TYPES_H
