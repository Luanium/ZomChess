#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Types.h"
#include "Entities.h"
#include <vector>
#include <memory>
#include <random>

class GameState {
public:
    GameScene current_scene = GameScene::MainMenu;
    GameConfig active_config;                     
    bool use_vietnamese = false;
    bool human_sick_stamina_penalty = false;
    
    std::string tr(const std::string& en, const std::string& vi) const {
        return use_vietnamese ? vi : en;
    }
    
    char export_filename[64] = "my_custom_challenge.zom";
    char import_filename[64] = "my_custom_challenge.zom";

    int width = 15;
    int height = 15;
    std::vector<std::vector<Terrain>> grid;
    std::vector<std::vector<bool>> mine_grid;
    Human human;
    std::vector<std::unique_ptr<Zombie>> zombies;
    std::vector<FireCell> fire_cells; 

    int turn_limit = 50;
    int current_turn = 1;
    InputMode input_mode = InputMode::MoveMode;
    std::vector<GrenadeTimer> active_grenades;
    std::vector<LogLine> logs;
    bool game_over = false;
    bool game_won = false;

    TurnPhase phase = TurnPhase::HumanTurn;
    size_t active_zombie_idx = 0;
    int active_zombie_substep = 0;
    float zombie_action_timer = 0.0f;
    const float ZOMBIE_STEP_DELAY = 0.35f;
    float environment_action_timer = 0.0f;
    const float ENVIRONMENT_STEP_DELAY = 0.9f;
    bool dark_cloud_active = false;
    std::string last_environment_event = "Clear skies";

    VisualFX active_fx;
    VisualFX turn_banner_fx;
    std::vector<VisualFX> attack_animations; 
    std::vector<FloatingText> floating_texts;

    std::mt19937 rng;
    Terrain editor_selected_terrain = Terrain::Wall;

    GameState();

    void init_game();
    void apply_quick_difficulty(int difficulty_level); 
    int calculate_available_spawn_cells();           
    bool is_zombie_count_valid();                    

    void add_log(const std::string& text, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    int distance(Position p1, Position p2);
    std::pair<int, int> get_8_direction(int dx, int dy);
    sf::Vector2f getCellCenter(int x, int y, float cellSize, float offset);
    bool is_blocked_by_wall(Position start, Position end) const;
    
    void trigger_explosion(int cx, int cy, bool is_zombie_exploding = false);
    void zombie_single_step(size_t idx);
    void handle_weapon_click(int tx, int ty, float cellSize, float boardOffset);
    void start_zombie_phase();
    void start_environment_phase();
    void resolve_environment_turn();
    void finish_environment_phase();
    void update_zombie_logic(float dt);
    void update_environment_logic(float dt);
    void check_victory_conditions();
    void check_fire_interactions();
    void check_mine_interactions();
    void propagate_grass_fire(int cx, int cy);
    void set_cell_on_fire(int x, int y);
    bool is_blocking_cell(int x, int y) const;
    bool has_living_entity_at(Position p) const;
    bool is_conductive_cell(Position p) const;
    std::vector<Position> get_conductive_cluster(Position start) const;
    void apply_windstorm(int dx, int dy);
    void apply_heavy_rain();
    void apply_dark_clouds();
    void apply_lightning_strike();

    bool export_challenge_file(const std::string& path);
    bool import_challenge_file(const std::string& path);
};

#endif // GAMESTATE_H
