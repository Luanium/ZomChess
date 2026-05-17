#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/System/Vector2.hpp>

// Thêm Terrain::Fire
enum class Terrain { Dirt, Water, Obstacle, Wall, Fire };
// Thêm TargetMolotov
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade, TargetMolotov };
enum class ZombieType { Normal, Fast, Exploding, Vampire };
enum class TurnPhase { HumanTurn, ZombieAnimating };
// Thêm Molotov FX
enum class FXType { None, Knife, Pistol, Shotgun, Explosion, Molotov };
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

    int human_hp = 5;
    int initial_stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int mines = 2;
    int molotovs = 3; // Số lượng bom xăng khởi tạo
    int turn_limit = 50;

    int count_normal = 3;
    int count_fast = 2;
    int count_exploding = 2;
    int count_vampire = 1;

    bool spawn_shield = true;      
    bool custom_map_mode = false;  

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
};

// Quản lý ô địa hình lửa
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
};

#endif // TYPES_H
