#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/System/Vector2.hpp>

enum class Terrain { Dirt, Water, Wall, Fire, Grass };
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade, TargetMolotov };
enum class ZombieType { Normal, Fast, Exploding, Vampire, Sick };
enum class TurnPhase { HumanTurn, ZombieAnimating, EnvironmentAnimating };

// Added Bite and Scratch FX
enum class FXType { None, Knife, Pistol, Shotgun, Explosion, Molotov, Bite, Scratch, Wind, Rain, DarkCloud, Lightning, Electricity, GrenadeFly };
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
    int ratio_grass = 20;
    int ratio_dirt = 60;

    int human_hp = 5;
    int initial_stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int mines = 2;
    int molotovs = 3; 
    int turn_limit = 50;

    int count_normal = 3;
    int count_fast = 2;
    int count_exploding = 2;
    int count_vampire = 1;
    int count_sick = 1;

    bool spawn_shield = true;      
    bool custom_map_mode = false;  
    bool enable_environment = true;

    int env_prob_clear = 58;
    int env_prob_wind = 16;
    int env_prob_rain = 14;
    int env_prob_clouds = 4;
    int env_prob_lightning = 8;

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
};

// System for dynamic floating damage/heal numbers
struct FloatingText {
    Position pos;
    int amount; 
    float timer = 1.0f;
    float max_duration = 1.0f;
};

#endif // TYPES_H
