#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Types.h"
#include "Entities.h"
#include <vector>
#include <memory>
#include <random>

class GameState {
public:
    int width = 15;
    int height = 15;
    std::vector<std::vector<Terrain>> grid;
    std::vector<std::vector<bool>> mine_grid;
    Human human;
    std::vector<std::unique_ptr<Zombie>> zombies;
    int turn_limit = 50;
    int current_turn = 1;
    InputMode input_mode = InputMode::MoveMode;
    GrenadeTimer grenade_box;
    std::vector<LogLine> logs;
    bool game_over = false;
    bool game_won = false;

    TurnPhase phase = TurnPhase::HumanTurn;
    size_t active_zombie_idx = 0;
    int active_zombie_substep = 0;
    float zombie_action_timer = 0.0f;
    const float ZOMBIE_STEP_DELAY = 0.35f;

    VisualFX active_fx;
    std::mt19937 rng;

    GameState();

    void init_game();
    void add_log(const std::string& text, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    int distance(Position p1, Position p2);
    std::pair<int, int> get_8_direction(int dx, int dy);
    sf::Vector2f getCellCenter(int x, int y, float cellSize, float offset);
    void trigger_explosion(int cx, int cy);
    void zombie_single_step(size_t idx);
    void handle_weapon_click(int tx, int ty, float cellSize, float boardOffset);
    void start_zombie_phase();
    void update_zombie_logic(float dt);
    void check_victory_conditions();
};

#endif // GAMESTATE_H
