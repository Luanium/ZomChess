#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <cmath>
#include <algorithm>

// --- 1. Enumerations & Structs ---
enum class Terrain { Dirt, Water, Obstacle, Wall };
enum class InputMode { MoveMode, TargetKnife, TargetPistol, TargetShotgun, TargetGrenade };
enum class ZombieType { Normal, Fast, Exploding, Vampire };

struct Position {
    int x;
    int y;
    bool operator==(const Position& other) const = default;
};

struct LogLine {
    std::string text;
    ImVec4 color;
    bool isBold;
};

struct GrenadeTimer {
    bool active = false;
    Position pos{0, 0};
};

// --- 2. Entity Classes ---
struct Human {
    Position pos;
    int hp = 5;
    int stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int 用mines = 2; // mìn cố định
};

class Zombie {
public:
    Position pos;
    int hp;
    std::string name;
    ZombieType type;

    Zombie(Position p, int h, std::string n, ZombieType t) : pos(p), hp(h), name(n), type(t) {}
    virtual ~Zombie() = default;
    virtual int getMovesPerTurn() const { return 1; }
};

class NormalZombie : public Zombie { public: using Zombie::Zombie; };
class FastZombie : public Zombie { public: using Zombie::Zombie; int getMovesPerTurn() const override { return 2; } };
class ExplodingZombie : public Zombie { public: using Zombie::Zombie; };
class VampireZombie : public Zombie { public: using Zombie::Zombie; };

// --- 3. Game State Class ---
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

    // Các biến phục vụ hiệu ứng Animation làm chậm độc lập với khung hình
    std::vector<Position> flash_cells;
    sf::Vector2f line_fx_start, line_fx_end;
    bool is_drawing_line = false;
    float fx_timer = 0.0f;
    float fx_duration = 0.45f; 

    // Bộ sinh số ngẫu nhiên chuẩn C++
    std::mt19937 rng{std::random_device{}()};

    GameState() { init_game(); }

    void add_log(const std::string& text, ImVec4 color = ImVec4(0.75f, 0.75f, 0.75f, 1.0f), bool isBold = false) {
        logs.push_back({text, color, isBold});
        if (logs.size() > 150) logs.erase(logs.begin());
    }

    int distance(Position p1, Position p2) {
        return std::max(std::abs(p1.x - p2.x), std::abs(p1.y - p2.y));
    }

    std::pair<int, int> get_8_direction(int dx, int dy) {
        if (dx == 0 && dy == 0) return {0, 0};
        if (std::abs(dx) == std::abs(dy)) return {dx > 0 ? 1 : -1, dy > 0 ? 1 : -1};
        if (std::abs(dx) > std::abs(dy)) return {dx > 0 ? 1 : -1, 0};
        return {0, dy > 0 ? 1 : -1};
    }

    void check_victory_conditions() {
        if (human.hp <= 0) {
            game_over = true;
            add_log("=== BAN DA THUAN TRAN! HUMAN DA CHET ===", ImVec4(1.0f, 0.2f, 0.2f, 1.0f), true);
        } else {
            bool all_dead = true;
            for (const auto& z : zombies) {
                if (z->hp > 0) { all_dead = false; break; }
            }
            if (all_dead) {
                game_won = true;
                add_log("=== CHUC MUNG THANG LOI! DA DIET SACH ZOMBIE ===", ImVec4(1.0f, 0.85f, 0.0f, 1.0f), true);
            }
        }
    }

    void init_game() {
        grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
        mine_grid.assign(width, std::vector<bool>(height, false));

        // Tạo tường bao quanh biên
        for (int x = 0; x < width; ++x) { grid[x][0] = Terrain::Wall; grid[x][height - 1] = Terrain::Wall; }
        for (int y = 0; y < height; ++y) { grid[0][y] = Terrain::Wall; grid[width - 1][y] = Terrain::Wall; }

        std::uniform_int_distribution<int> dist_grid(1, width - 2);
        // Sinh chướng ngại vật ngẫu nhiên
        for (int i = 0; i < 12; ++i) { grid[dist_grid(rng)][dist_grid(rng)] = Terrain::Obstacle; }
        for (int i = 0; i < 6; ++i) {
            int rx = dist_grid(rng), ry = dist_grid(rng);
            if (grid[rx][ry] == Terrain::Dirt) grid[rx][ry] = Terrain::Water;
        }

        human.pos = {dist_grid(rng), dist_grid(rng)};
        while (grid[human.pos.x][human.pos.y] == Terrain::Wall || grid[human.pos.x][human.pos.y] == Terrain::Obstacle) {
            human.pos = {dist_grid(rng), dist_grid(rng)};
        }

        // Khởi tạo danh sách quái vật
        std::vector<std::tuple<ZombieType, int, std::string>> z_types = {
            {ZombieType::Normal, 2, "Zom Thuong"},
            {ZombieType::Fast, 2, "Zom Nhanh"},
            {ZombieType::Exploding, 3, "Zom No"},
            {ZombieType::Vampire, 4, "Zom Dracula"}
        };
        std::uniform_int_distribution<int> dist_type(0, 3);

        for (int i = 0; i < 8; ++i) {
            auto [zt, zhp, zname] = z_types[dist_type(rng)];
            Position z_pos = {dist_grid(rng), dist_grid(rng)};
            while (z_pos == human.pos || grid[z_pos.x][z_pos.y] == Terrain::Wall || grid[z_pos.x][z_pos.y] == Terrain::Obstacle) {
                z_pos = {dist_grid(rng), dist_grid(rng)};
            }

            if (zt == ZombieType::Normal) zombies.push_back(std::make_unique<NormalZombie>(z_pos, zhp, zname, zt));
            else if (zt == ZombieType::Fast) zombies.push_back(std::make_unique<FastZombie>(z_pos, zhp, zname, zt));
            else if (zt == ZombieType::Exploding) zombies.push_back(std::make_unique<ExplodingZombie>(z_pos, zhp, zname, zt));
            else if (zt == ZombieType::Vampire) zombies.push_back(std::make_unique<VampireZombie>(z_pos, zhp, zname, zt));
        }

        add_log("Chao mung den voi ZomChess C++ Standard Edition!", ImVec4(0.0f, 1.0f, 1.0f, 1.0f), true);
        add_log("Su dung chuot click vao ban co chien thuat de hanh dong.", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    }

    void trigger_explosion(int cx, int cy) {
        add_log("[EXPLOSION] Vu no lon bung phat tai tam (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f), true);
        
        flash_cells.clear();
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (cx + dx >= 0 && cx + dx < width && cy + dy >= 0 && cy + dy < height) {
                    flash_cells.push_back({cx + dx, cy + dy});
                }
            }
        }
        is_drawing_line = false; 
        fx_timer = fx_duration; // Kích hoạt bộ đếm thời gian animation vẽ vùng nổ

        if (std::max(std::abs(human.pos.x - cx), std::abs(human.pos.y - cy)) <= 1) {
            human.hp = std::max(0, human.hp - 1);
            add_log("-> Human dinh sat thuong no lan (-1 HP)!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        }

        for (size_t i = 0; i < zombies.size(); ++i) {
            auto& zom = zombies[i];
            if (zom->hp > 0 && std::max(std::abs(zom->pos.x - cx), std::abs(zom->pos.y - cy)) <= 1) {
                zom->hp -= 1;
                add_log("-> Zom #" + std::to_string(i+1) + " (" + zom->name + ") ganh sat thuong no (-1 HP).", ImVec4(1.0f, 0.6f, 0.0f, 1.0f));

                if (zom->hp <= 0 && zom->type == ZombieType::Exploding) {
                    add_log("   [LIEN HOAN NO] Kich hoat phat no day chuyen!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                    trigger_explosion(zom->pos.x, zom->pos.y);
                }
                
                // Đẩy lùi Zombie
                int vx = (zom->pos.x - cx > 0) ? 1 : (zom->pos.x - cx < 0 ? -1 : 0);
                int vy = (zom->pos.y - cy > 0) ? 1 : (zom->pos.y - cy < 0 ? -1 : 0);
                if (vx != 0 || vy != 0) {
                    int px = zom->pos.x + vx;
                    int py = zom->pos.y + vy;
                    if (px >= 0 && px < width && py >= 0 && py < height && grid[px][py] != Terrain::Wall && grid[px][py] != Terrain::Obstacle) {
                        zom->pos = {px, py};
                    } else {
                        zom->hp -= 1;
                        add_log("   RAM! Zombie va vao vat can khi vang ra! -1 HP!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                        if (zom->hp <= 0 && zom->type == ZombieType::Exploding) {
                            trigger_explosion(zom->pos.x, zom->pos.y);
                        }
                    }
                }
            }
        }
        check_victory_conditions();
    }

    void zombie_single_step(size_t idx) {
        if (game_over || game_won) return;
        auto& zom = zombies[idx];
        double lambda = 1.2;
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
                    if (grid[nx][ny] == Terrain::Wall || grid[nx][ny] == Terrain::Obstacle) continue;

                    bool conflict = false;
                    for (size_t o = 0; o < zombies.size(); ++o) {
                        if (o != idx && zombies[o]->hp > 0 && zombies[o]->pos == target) { conflict = true; break; }
                    }
                    if (conflict) continue;

                    int new_dist = distance(target, human.pos);
                    double w = std::exp(-new_dist / lambda);

                    if (new_dist > current_dist) w *= 0.05;
                    else if (new_dist == current_dist && (dx != 0 || dy != 0)) w *= 0.3;

                    valid_moves.push_back(target);
                    weights.push_back(w);
                }
            }
        }

        if (!valid_moves.empty()) {
            std::discrete_distribution<int> dist(weights.begin(), weights.end());
            zom->pos = valid_moves[dist(rng)];

            if (mine_grid[zom->pos.x][zom->pos.y]) {
                add_log("[MIN NO] Zom #" + std::to_string(idx+1) + " dap phai min o (" + std::to_string(zom->pos.x) + ", " + std::to_string(zom->pos.y) + ")!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                mine_grid[zom->pos.x][zom->pos.y] = false;
                trigger_explosion(zom->pos.x, zom->pos.y);
            }
        }
    }

    void handle_weapon_click(int tx, int ty) {
        if (human.stamina < 1) {
            add_log("Khong du the luc (stamina) de hanh dong!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            input_mode = InputMode::MoveMode;
            return;
        }

        Position target{tx, ty};

        if (input_mode == InputMode::TargetKnife) {
            if (distance(human.pos, target) <= 1) {
                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == target) {
                        zombies[i]->hp -= 1;
                        human.stamina -= 1;
                        add_log("[KNIFE] XOET! Dam trung Zombie #" + std::to_string(i+1) + ". HP con: " + std::to_string(zombies[i]->hp), ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
                        
                        flash_cells = {target};
                        fx_timer = fx_duration;

                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            trigger_explosion(target.x, target.y);
                        }
                        input_mode = InputMode::MoveMode;
                        check_victory_conditions();
                        return;
                    }
                }
                add_log("O chi dinh khong co muc tieu song!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            } else {
                add_log("Dao ngan! Vui long chon o ngay sat ben cạnh.", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }
        } 
        else if (input_mode == InputMode::TargetPistol) {
            if (human.pistol_ammo <= 0) { add_log("Het dan Sung ngan!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.pistol_ammo -= 1;
            human.stamina -= 1;
            add_log("[PISTOL] DOANG! Khai hoa sung ngan huong (" + std::to_string(vx) + ", " + std::to_string(vy) + ")...", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            float cellSize = 40.0f;
            line_fx_start = sf::Vector2f(human.pos.x * cellSize + 40.0f, human.pos.y * cellSize + 40.0f);
            line_fx_end = sf::Vector2f((human.pos.x + vx * 5) * cellSize + 40.0f, (human.pos.y + vy * 5) * cellSize + 40.0f);
            is_drawing_line = true;
            fx_timer = fx_duration;

            bool hit = false;
            for (int step = 1; step <= 5; ++step) {
                int cx = human.pos.x + vx * step;
                int cy = human.pos.y + vy * step;
                if (cx < 0 || cx >= width || cy < 0 || cy >= height || grid[cx][cy] == Terrain::Wall || grid[cx][cy] == Terrain::Obstacle) break;

                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) {
                        double chance = (step <= 2) ? 1.0 : (step == 3 ? 0.7 : (step == 4 ? 0.4 : 0.0));
                        std::uniform_real_distribution<double> dist_chance(0.0, 1.0);
                        if (dist_chance(rng) <= chance) {
                            zombies[i]->hp -= 1;
                            add_log("-> Ban trung Zom #" + std::to_string(i+1) + " o cu ly " + std::to_string(step) + " o.", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                            if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                                trigger_explosion(cx, cy);
                            }
                        } else {
                            add_log("-> Dan bay chech muc tieu!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                        }
                        hit = true; break;
                    }
                }
                if (hit) break;
            }
        }
        else if (input_mode == InputMode::TargetShotgun) {
            if (human.shotgun_ammo <= 0) { add_log("Het dan Shotgun!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.shotgun_ammo -= 1;
            human.stamina -= 1;
            add_log("[SHOTGUN] DOANG! Thổi bay mọi thu theo huong tịnh tiến!", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            flash_cells.clear();
            for (int step = 1; step <= 3; ++step) {
                int cx = human.pos.x + vx * step;
                int cy = human.pos.y + vy * step;
                if (cx >= 0 && cx < width && cy >= 0 && cy < height) {
                    flash_cells.push_back({cx, cy});
                }
            }
            fx_timer = fx_duration;

            for (int step = 1; step <= 3; ++step) {
                int cx = human.pos.x + vx * step;
                int cy = human.pos.y + vy * step;
                if (cx < 0 || cx >= width || cy < 0 || cy >= height) break;

                bool found = false;
                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) {
                        zombies[i]->hp -= 1;
                        int px = zombies[i]->pos.x + vx;
                        int py = zombies[i]->pos.y + vy;
                        if (px >= 0 && px < width && py >= 0 && py < height && grid[px][py] != Terrain::Wall && grid[px][py] != Terrain::Obstacle) {
                            zombies[i]->pos = {px, py};
                        } else {
                            zombies[i]->hp -= 1;
                            add_log("   RAM! Zombie va vao tuong sau luc day! -1 HP!", ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                        }
                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                        }
                        found = true; break;
                    }
                }
                if (found) break;
            }
        }
        else if (input_mode == InputMode::TargetGrenade) {
            if (human.grenades <= 0) { add_log("Het Luu dan!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.grenades -= 1;
            human.stamina -= 1;

            std::uniform_int_distribution<int> dist_steps(2, 5);
            int total_steps = dist_steps(rng);
            int cx = human.pos.x, cy = human.pos.y;

            for (int step = 1; step <= total_steps; ++step) {
                int nx = cx + vx;
                int ny = cy + vy;
                if (nx < 0 || nx >= width || ny < 0 || ny >= height || grid[nx][ny] == Terrain::Wall || grid[nx][ny] == Terrain::Obstacle) {
                    add_log("   [VAT CAN] Luu dan va vao tuong o buoc " + std::to_string(step), ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    break;
                }
                cx = nx; cy = ny;
            }
            grenade_box.active = true;
            grenade_box.pos = {cx, cy};
            add_log("[TIMER] Luu dan da nam im tai o (" + std::to_string(cx) + ", " + std::to_string(cy) + ")...", ImVec4(0.5f, 1.0f, 0.0f, 1.0f));
        }

        input_mode = InputMode::MoveMode;
        check_victory_conditions();
    }

    void end_turn() {
        if (game_over || game_won) return;
        add_log("=== QUAN DICH BAT DAU LURK DI CHUYEN ===", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

        for (size_t i = 0; i < zombies.size(); ++i) {
            if (zombies[i]->hp <= 0) continue;
            if (game_over) break; // [SỬA LỖI LẬP TỨC]: Người chơi chết phát bẻ gãy luôn luồng chạy của zombie tiếp theo

            int moves = zombies[i]->getMovesPerTurn();
            for (int step = 1; step <= moves; ++step) {
                if (zombies[i]->hp <= 0 || game_over) break; // [SỬA LỖI ZOMBIE NHANH]: Chặn tuyệt đối bước chạy thứ 2 nếu người chơi đã chết trước đó

                zombie_single_step(i);

                if (zombies[i]->hp > 0 && distance(zombies[i]->pos, human.pos) <= 1) {
                    human.hp = std::max(0, human.hp - 1);
                    add_log("[ATTACK] Zom #" + std::to_string(i+1) + " can ban! -1 HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

                    if (zombies[i]->type == ZombieType::Vampire) {
                        zombies[i]->hp += 1;
                        add_log("   [HUT MAU] Dracula Zom duoc +1 HP!", ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
                    }
                    check_victory_conditions();
                    if (game_over) break; // Ngắt vòng lặp bước chạy nhỏ lập tức
                }
            }
        }

        if (grenade_box.active && !game_over) {
            add_log("[!] Luu dan TICK... TICK... NO TUNG!", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            trigger_explosion(grenade_box.pos.x, grenade_box.pos.y);
            grenade_box.active = false;
        }

        if (!game_over) {
            current_turn++;
            std::uniform_int_distribution<int> dist_stam(2, 6);
            human.stamina = dist_stam(rng);
            input_mode = InputMode::MoveMode;
            add_log("--- LUOT MOI: VONG " + std::to_string(current_turn) + " (The luc hoi phuc: +" + std::to_string(human.stamina) + ") ---", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        }
        check_victory_conditions();
    }
};

// --- 4. Main Program & Application Engine ---
int main() {
    sf::RenderWindow window(sf::VideoMode(1350, 720), "ZomChess C++20 Clean Edition");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    GameState state;
    sf::Clock deltaClock;

    float cellSize = 40.0f;
    float boardOffset = 20.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed)
                window.close();

            if (!ImGui::GetIO().WantCaptureMouse && event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !state.game_over && !state.game_won) {
                    int mx = event.mouseButton.x;
                    int my = event.mouseButton.y;

                    int tx = (mx - boardOffset) / cellSize;
                    int ty = (my - boardOffset) / cellSize;

                    if (tx >= 0 && tx < state.width && ty >= 0 && ty < state.height) {
                        if (state.input_mode == InputMode::MoveMode) {
                            int dx = std::abs(tx - state.human.pos.x);
                            int dy = std::abs(ty - state.human.pos.y);
                            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                                if (state.grid[tx][ty] != Terrain::Wall && state.grid[tx][ty] != Terrain::Obstacle) {
                                    bool blocked = false;
                                    for (const auto& z : state.zombies) {
                                        if (z->hp > 0 && z->pos == Position{tx, ty}) { blocked = true; break; }
                                    }
                                    if (!blocked) {
                                        int cost = (state.grid[tx][ty] == Terrain::Water) ? 2 : 1;
                                        if (state.human.stamina >= cost) {
                                            state.human.pos = {tx, ty};
                                            state.human.stamina -= cost;
                                            state.add_log("Human di chuyen sang o (" + std::to_string(tx) + ", " + std::to_string(ty) + ").");
                                        }
                                    } else {
                                        state.add_log("Khong the di vao o cua Zombie dang song!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                                    }
                                }
                            }
                        } else {
                            state.handle_weapon_click(tx, ty);
                        }
                    }
                }
            }
        }

        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // Quản lý thời gian vẽ hiệu ứng Animation (Giảm dần theo giây thực tế)
        if (state.fx_timer > 0.0f) {
            state.fx_timer -= dt.asSeconds();
            if (state.fx_timer <= 0.0f) {
                state.flash_cells.clear();
                state.is_drawing_line = false;
            }
        }

        window.clear(sf::Color(30, 30, 30));

        // --- RENDER 1: BÀN CỜ TACTICAL CHẮC CHẮN (Tọa độ cứng, tuyệt đối không bị thay đổi kích thước) ---
        for (int x = 0; x < state.width; ++x) {
            for (int y = 0; y < state.height; ++y) {
                sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                cell.setPosition(x * cellSize + boardOffset, y * cellSize + boardOffset);

                if (state.grid[x][y] == Terrain::Wall) cell.setFillColor(sf::Color(70, 70, 70));
                else if (state.grid[x][y] == Terrain::Obstacle) cell.setFillColor(sf::Color(45, 45, 45));
                else if (state.grid[x][y] == Terrain::Water) cell.setFillColor(sf::Color(25, 75, 125));
                else cell.setFillColor(sf::Color(115, 65, 30));

                window.draw(cell);

                if (state.mine_grid[x][y]) {
                    sf::CircleShape mine(6.0f);
                    mine.setFillColor(sf::Color::Red);
                    mine.setOrigin(6.0f, 6.0f);
                    mine.setPosition(x * cellSize + boardOffset + cellSize/2, y * cellSize + boardOffset + cellSize/2);
                    window.draw(mine);
                }
            }
        }

        // Vẽ Quả Lựu đạn hẹn giờ
        if (state.grenade_box.active) {
            sf::CircleShape gren(9.0f, 3); // Tam giác màu xanh lá
            gren.setFillColor(sf::Color::Green);
            gren.setOrigin(9.0f, 9.0f);
            gren.setPosition(state.grenade_box.pos.x * cellSize + boardOffset + cellSize/2, state.grenade_box.pos.y * cellSize + boardOffset + cellSize/2);
            window.draw(gren);
        }

        // Vẽ danh sách Zombie lên bàn cờ
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            const auto& z = state.zombies[i];
            if (z->hp <= 0) {
                // Xác Zombie chết (Dấu X màu đen)
                sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
                deadZ.setFillColor(sf::Color(10, 10, 10, 150));
                deadZ.setPosition(z->pos.x * cellSize + boardOffset + 4.0f, z->pos.y * cellSize + boardOffset + 4.0f);
                window.draw(deadZ);
                continue;
            }
            sf::RectangleShape zVisual(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
            zVisual.setPosition(z->pos.x * cellSize + boardOffset + 4.0f, z->pos.y * cellSize + boardOffset + 4.0f);

            if (z->type == ZombieType::Fast) zVisual.setFillColor(sf::Color(150, 220, 50));
            else if (z->type == ZombieType::Exploding) zVisual.setFillColor(sf::Color(230, 120, 20));
            else if (z->type == ZombieType::Vampire) zVisual.setFillColor(sf::Color(140, 30, 140));
            else zVisual.setFillColor(sf::Color(30, 120, 40));

            window.draw(zVisual);
        }

        // Vẽ Người chơi (Ngôi sao hoặc hình tròn màu trắng lớn)
        sf::CircleShape hVisual(cellSize / 2.6f);
        hVisual.setFillColor(sf::Color::White);
        hVisual.setPosition(state.human.pos.x * cellSize + boardOffset + 4.0f, state.human.pos.y * cellSize + boardOffset + 4.0f);
        window.draw(hVisual);

        // Vẽ hiệu ứng đạn bay/ nổ lan làm chậm độc lập
        if (state.fx_timer > 0.0f) {
            if (state.is_drawing_line) {
                sf::Vertex line[] = { sf::Vertex(state.line_fx_start, sf::Color::Yellow), sf::Vertex(state.line_fx_end, sf::Color::Yellow) };
                window.draw(line, 2, sf::Lines);
            }
            for (auto p : state.flash_cells) {
                sf::RectangleShape flash(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                flash.setFillColor(sf::Color(255, 0, 0, 140));
                flash.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset);
                window.draw(flash);
            }
        }

        // --- RENDER 2: ĐỒ HỌA GIAO DIỆN DEAR IMGUI (Cố định thanh cuộn và layout bên phải) ---
        ImGui::SetNextWindowPos(ImVec2(660, 20));
        ImGui::SetNextWindowSize(ImVec2(660, 680));
        ImGui::Begin("He Thong Dieu Khien Chien Truong", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // Thanh trạng thái trên cùng
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[VONG LUOT]: %d/%d", state.current_turn, state.turn_limit);
        ImGui::SameLine(); ImGui::Text(" | [STAMINA]: %d", state.human.stamina);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), " | [HP]: %d", state.human.hp);
        
        ImGui::Separator();

        // Toolbar Vũ khí hành động
        if (ImGui::Button("END TURN", ImVec2(120, 35))) { state.end_turn(); }
        ImGui::SameLine();
        if (ImGui::Button("Dao (oo)")) { state.input_mode = InputMode::TargetKnife; }
        ImGui::SameLine();
        if (ImGui::Button(("Sung Ngan (" + std::to_string(state.human.pistol_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetPistol; }
        ImGui::SameLine();
        if (ImGui::Button(("Shotgun (" + std::to_string(state.human.shotgun_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetShotgun; }
        ImGui::SameLine();
        if (ImGui::Button(("Luu Dan (" + std::to_string(state.human.grenades) + ")").c_str())) { state.input_mode = InputMode::TargetGrenade; }
        ImGui::SameLine();
        if (ImGui::Button(("Cai Min (" + std::to_string(state.human.用mines) + ")").c_str())) {
            if (!state.game_over && !state.game_won && state.human.stamina >= 1 && state.human.用mines > 0) {
                state.mine_grid[state.human.pos.x][state.human.pos.y] = true;
                state.human.用mines--;
                state.human.stamina--;
                state.add_log("[MIN] Cai min thanh cong tai o hien tai!", ImVec4(1, 0.8f, 0, 1));
            }
        }

        if (state.input_mode != InputMode::MoveMode) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[CHE DO]: CLICK VAO BAN CO DE KIEN TRONG TAM BAN VU KHI!");
        } else {
            ImGui::Text("[CHE DO]: Click o canh ben canh de di chuyển.");
        }

        ImGui::Separator();

        // [SỬA LỖI THANH CUỘN]: Cửa sổ danh sách quái vật, có thanh cuộn tự động khi vượt quá khung hình hiển thị
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "=== DANH SACH KE DICH ===");
        ImGui::BeginChild("ZombieListChild", ImVec2(0, 160), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(3, "zombie_table");
        ImGui::SetColumnWidth(0, 60); ImGui::SetColumnWidth(1, 200);
        ImGui::Text("STT"); ImGui::NextColumn(); ImGui::Text("Chủng loại"); ImGui::NextColumn(); ImGui::Text("Máu (HP)"); ImGui::NextColumn();
        ImGui::Separator();
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            ImGui::Text("#%zu", i + 1); ImGui::NextColumn();
            ImGui::Text("%s", state.zombies[i]->name.c_str()); ImGui::NextColumn();
            if (state.zombies[i]->hp <= 0) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "[X] DA CHET");
            } else {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d HP", state.zombies[i]->hp);
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::Separator();

        // [SỬA LỖI THANH CUỘN]: Cửa sổ Nhật ký chiến trường có tính năng tự động cuộn xuống dưới cùng khi có log mới
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "--- NHAT KY CHIEN TRUONG ---");
        ImGui::BeginChild("LogsChild", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (const auto& log : state.logs) {
            ImGui::TextColored(log.color, "%s", log.text.c_str());
        }
        // Luôn tự động kéo chuột cuộn xuống dòng nhật ký mới nhất
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        // Màn hình kết thúc Game đè lên giao diện chính
        if (state.game_over || state.game_won) {
            ImGui::OpenPopup("Game Status Over");
            ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_Appearing);
            if (ImGui::BeginPopupModal("Game Status Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (state.game_over) ImGui::TextColored(ImVec4(1, 0, 0, 1), "GAME OVER! Ban da hy sinh.");
                else ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "VICTORY! Ban da chien thang quai vat.");
                ImGui::Separator();
                if (ImGui::Button("Thoat game", ImVec2(120, 0))) { window.close(); }
                ImGui::EndPopup();
            }
        }

        ImGui::End();

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
