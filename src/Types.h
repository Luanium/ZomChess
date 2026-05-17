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

struct Position {
    int x;
    int y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
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
