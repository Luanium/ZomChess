#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <imgui.h>
#include <SFML/System/Vector2.hpp>

enum class Terrain { Dirt, Water, Obstacle, Wall };
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade };
enum class ZombieType { Normal, Fast, Exploding, Vampire };
enum class TurnPhase { HumanTurn, ZombieAnimating };
enum class FXType { None, Knife, Pistol, Shotgun, Explosion };
enum class GameScene { MainMenu, Playing, MapEditor }; // Thêm cảnh thiết kế Map

struct Position {
    int x;
    int y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// Cấu trúc chứa toàn bộ thông tin thiết lập trận đấu và bản đồ
struct GameConfig {
    // Kích thước bàn cờ
    int map_width = 15;
    int map_height = 15;

    // Chỉ số Nhân vật
    int human_hp = 5;
    int initial_stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int mines = 2;
    int turn_limit = 50;

    // Số lượng Zombie
    int count_normal = 3;
    int count_fast = 2;
    int count_exploding = 2;
    int count_vampire = 1;

    // Cơ chế tính năng mới
    bool spawn_shield = true;      // Bảo hiểm 7x7 ô xung quanh Human
    bool custom_map_mode = false;  // Chế độ chơi Map tự xây dựng

    // Dữ liệu dùng cho Custom Map
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

struct VisualFX {
    FXType type = FXType::None;
    float timer = 0.0f;
    float max_duration = 0.35f;
    sf::Vector2f start_p;
    sf::Vector2f end_p;
    std::vector<Position> blast_cells;
};

#endif // TYPES_H
