#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Types.h"
#include "Entities.h"
#include <vector>
#include <memory>
#include <random>

// Forward declaration for AudioManager
class AudioManager;

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
    IceSlideAnimation ice_slide_animation;
    std::vector<TerrainTransitionAnimation> terrain_transitions;

    std::mt19937 rng;
    Terrain editor_selected_terrain = Terrain::Wall;

    std::vector<LootDrop> loot_drops; // Loot rơi khi zombie chết

    // Audio management
    bool music_enabled = true;
    bool sfx_enabled = true;
    float music_volume = 50.0f;

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
    void propagate_gradual_forest_fire();
    void set_cell_on_fire(int x, int y);
    bool is_blocking_cell(int x, int y) const;
    bool has_living_entity_at(Position p) const;
    bool is_conductive_cell(Position p) const;
    std::vector<Position> get_conductive_cluster(Position start) const;
    void apply_windstorm(int dx, int dy);
    void apply_heavy_rain();
    void apply_dark_clouds();
    void apply_lightning_strike();
    void apply_heatwave();   // Nắng nóng gay gắt
    void apply_blizzard();   // Băng giá

    // Melt all ice cells adjacent (8-dir) to a given cell due to heat transfer
    void melt_adjacent_ice(int cx, int cy);

    // Loot drop system
    void spawn_loot_at(Position pos);
    void check_loot_pickup();
    void spawn_loot_for_newly_dead(); // Gọi sau check_victory_conditions
    // Hủy loot tại các ô trong danh sách (do nổ, sét, lửa, shotgun)
    void destroy_loot_at_cells(const std::vector<Position>& cells);
    // Kiểm tra và hủy loot nếu ô của nó bị chuyển thành lửa
    void check_loot_on_fire();
    // Giải đóng băng loot và grenade khi ô Ice tan thành Water
    void thaw_loot_and_grenades_at(const std::vector<Position>& cells);

    // Ice terrain helpers
    // Returns true if entity slid (and turn should end immediately for that entity)
    bool try_ice_slide(bool is_human, size_t zombie_idx, int move_dx, int move_dy);
    // Ice Pick: break the ice tile human is standing on (Ice -> Water)
    void use_ice_pick();

    // Audio methods
    void initAudio();
    void playBackgroundMusic(const std::string& track);
    void stopBackgroundMusic();
    void setMusicVolume(float volume);
    void setSfxEnabled(bool enabled);

    bool export_challenge_file(const std::string& path);
    bool import_challenge_file(const std::string& path);
};

#endif // GAMESTATE_H
