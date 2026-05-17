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

// --- 1. Toàn bộ Enumerations & Structs nền tảng ---
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

// Cấu trúc quản lý hiệu ứng đồ họa kéo dài theo thời gian thực
struct VisualFX {
    FXType type = FXType::None;
    float timer = 0.0f;
    float max_duration = 0.35f;
    sf::Vector2f start_p;
    sf::Vector2f end_p;
    std::vector<Position> blast_cells; // Dùng cho nổ lan hoặc vùng bắn súng săn
};

// --- 2. Các thực thể Nhân vật (Entities) ---
struct Human {
    Position pos;
    int hp = 5;
    int stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int mines = 2;
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

// --- 3. Trái tim quản lý logic và trạng thái vòng lặp Game ---
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

    // Quản lý thứ tự luồng điều khiển chuyển động của Zombie
    TurnPhase phase = TurnPhase::HumanTurn;
    size_t active_zombie_idx = 0;
    int active_zombie_substep = 0;
    float zombie_action_timer = 0.0f;
    const float ZOMBIE_STEP_DELAY = 0.35f; // Thời gian giãn cách giữa mỗi bước đi của từng Zombie

    // Đối tượng chứa hiệu ứng hoạt họa đang diễn ra
    VisualFX active_fx;

    std::mt19937 rng{std::random_device{}()};

    GameState() { init_game(); }

    void add_log(const std::string& text, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f)) {
        logs.push_back({text, color});
        if (logs.size() > 100) logs.erase(logs.begin());
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

    sf::Vector2f getCellCenter(int x, int y, float cellSize, float offset) {
        return sf::Vector2f(x * cellSize + offset + cellSize / 2.0f, y * cellSize + offset + cellSize / 2.0f);
    }

    void trigger_explosion(int cx, int cy) {
        add_log("[EXPLOSION] Phat no vung cuc lon tai vung toa do (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
        
        // Thiết lập vung hỏa hoạn nhấp nháy 9 ô
        active_fx.type = FXType::Explosion;
        active_fx.timer = 0.4f;
        active_fx.max_duration = 0.4f;
        active_fx.blast_cells.clear();

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    active_fx.blast_cells.push_back({nx, ny});
                }
            }
        }

        if (distance(human.pos, {cx, cy}) <= 1) {
            human.hp = std::max(0, human.hp - 1);
            add_log("-> Sức ep lan tỏa khien Human thiet hai (-1 HP)!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        }

        for (size_t i = 0; i < zombies.size(); ++i) {
            auto& zom = zombies[i];
            if (zom->hp > 0 && distance(zom->pos, {cx, cy}) <= 1) {
                zom->hp -= 1;
                add_log("-> Zombie #" + std::to_string(i+1) + " dinh boi thiet hai lan (-1 HP).", ImVec4(1.0f, 0.6f, 0.1f, 1.0f));

                if (zom->hp <= 0 && zom->type == ZombieType::Exploding) {
                    trigger_explosion(zom->pos.x, zom->pos.y);
                }
                
                // Day lui thuc the sau vu no
                int vx = (zom->pos.x - cx > 0) ? 1 : (zom->pos.x - cx < 0 ? -1 : 0);
                int vy = (zom->pos.y - cy > 0) ? 1 : (zom->pos.y - cy < 0 ? -1 : 0);
                if (vx != 0 || vy != 0) {
                    int px = zom->pos.x + vx;
                    int py = zom->pos.y + vy;
                    if (px >= 0 && px < width && py >= 0 && py < height && grid[px][py] != Terrain::Wall && grid[px][py] != Terrain::Obstacle) {
                        zom->pos = {px, py};
                    } else {
                        zom->hp -= 1;
                        add_log("   [VA CHAM] Quai vat vang dap vao vat can cung! Roi them 1 HP!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
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
                add_log("[BAY MIN] Zombie #" + std::to_string(idx+1) + " giam phai min phong thu o (" + std::to_string(zom->pos.x) + ", " + std::to_string(zom->pos.y) + ")!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                mine_grid[zom->pos.x][zom->pos.y] = false;
                trigger_explosion(zom->pos.x, zom->pos.y);
            }
        }
    }

    void handle_weapon_click(int tx, int ty, float cellSize, float boardOffset) {
        if (human.stamina < 1) {
            add_log("The luc can kiet (stamina < 1), hanh dong that bai!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            input_mode = InputMode::MoveMode;
            return;
        }

        Position target{tx, ty};
        sf::Vector2f hCenter = getCellCenter(human.pos.x, human.pos.y, cellSize, boardOffset);
        sf::Vector2f tCenter = getCellCenter(tx, ty, cellSize, boardOffset);

        if (input_mode == InputMode::TargetKnife) {
            if (distance(human.pos, target) <= 1) {
                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == target) {
                        zombies[i]->hp -= 1;
                        human.stamina -= 1;
                        add_log("[KNIFE] Chém thọc dao găm vao Zombie #" + std::to_string(i+1) + "!", ImVec4(0.1f, 1.0f, 0.6f, 1.0f));
                        
                        // Kich hoat Animation hinh dao dam
                        active_fx.type = FXType::Knife;
                        active_fx.timer = 0.2f;
                        active_fx.max_duration = 0.2f;
                        active_fx.start_p = hCenter;
                        active_fx.end_p = tCenter;

                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            trigger_explosion(target.x, target.y);
                        }
                        input_mode = InputMode::MoveMode;
                        check_victory_conditions();
                        return;
                    }
                }
                add_log("O ban chon khong co vat the song phu hop!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            } else {
                add_log("Dao gam chi co the sat thuong pham vi sat vách!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }
        } 
        else if (input_mode == InputMode::TargetPistol) {
            if (human.pistol_ammo <= 0) { add_log("Sung ngan het dan!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.pistol_ammo -= 1;
            human.stamina -= 1;
            add_log("[PISTOL] Khai hỏa sung ngan huong ban (" + std::to_string(vx) + ", " + std::to_string(vy) + ")...", ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

            Position hit_pos = human.pos;
            bool hit_something = false;

            for (int step = 1; step <= 5; ++step) {
                int cx = human.pos.x + vx * step;
                int cy = human.pos.y + vy * step;
                if (cx < 0 || cx >= width || cy < 0 || cy >= height || grid[cx][cy] == Terrain::Wall || grid[cx][cy] == Terrain::Obstacle) {
                    hit_pos = {cx, cy}; break;
                }
                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == Position{cx, cy}) {
                        hit_pos = {cx, cy}; hit_something = true; break;
                    }
                }
                if (hit_something) break;
                hit_pos = {cx, cy};
            }

            // Hoạt họa đuong đạn ghim tu nguoi choi den diem va cham dau tien
            active_fx.type = FXType::Pistol;
            active_fx.timer = 0.25f;
            active_fx.max_duration = 0.25f;
            active_fx.start_p = hCenter;
            active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset);

            // Tinh toan sat thuong neu trung quai
            for (size_t i = 0; i < zombies.size(); ++i) {
                if (zombies[i]->hp > 0 && zombies[i]->pos == hit_pos) {
                    int dist = distance(human.pos, hit_pos);
                    double chance = (dist <= 2) ? 1.0 : (dist == 3 ? 0.7 : (dist == 4 ? 0.4 : 0.0));
                    std::uniform_real_distribution<double> dist_chance(0.0, 1.0);
                    if (dist_chance(rng) <= chance) {
                        zombies[i]->hp -= 1;
                        add_log("-> Dan xuyen thau Zombie #" + std::to_string(i+1) + "! -1 HP.", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            trigger_explosion(hit_pos.x, hit_pos.y);
                        }
                    } else {
                        add_log("-> Vien đan bay lech huong trong gang tấc!", ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                    }
                    break;
                }
            }
        }
        else if (input_mode == InputMode::TargetShotgun) {
            if (human.shotgun_ammo <= 0) { add_log("Shotgun da het co so dan!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.shotgun_ammo -= 1;
            human.stamina -= 1;
            add_log("[SHOTGUN] OÀNG! Khai hoa vung hoa luc hinh non quet sach cac o!", ImVec4(1.0f, 0.4f, 0.2f, 1.0f));

            active_fx.type = FXType::Shotgun;
            active_fx.timer = 0.35f;
            active_fx.max_duration = 0.35f;
            active_fx.blast_cells.clear();
            active_fx.start_p = hCenter;

            // Xử lý logic hỏa lực hình nón quét rộng 3 lớp tam giác tịnh tiến
            int px = -vy, py = vx; // Vector vuong goc de mo rong hinh non
            for (int step = 1; step <= 3; ++step) {
                int cx = human.pos.x + vx * step;
                int cy = human.pos.y + vy * step;
                int spread = step - 1; // Do mo rong theo tung tang hinh tam giac
                
                for (int s = -spread; s <= spread; ++s) {
                    int fx = cx + px * s;
                    int fy = cy + py * s;
                    if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                        active_fx.blast_cells.push_back({fx, fy});
                    }
                }
            }

            // Gây sát thương lan rộng trong vung tam giac hỏa lực
            for (auto p : active_fx.blast_cells) {
                if (grid[p.x][p.y] == Terrain::Wall || grid[p.x][p.y] == Terrain::Obstacle) continue;
                for (size_t i = 0; i < zombies.size(); ++i) {
                    if (zombies[i]->hp > 0 && zombies[i]->pos == p) {
                        zombies[i]->hp -= 1;
                        add_log("-> Quet lua ghim vao Zombie #" + std::to_string(i+1) + ". Giam 1 HP.");
                        
                        // Đẩy lui thực thể
                        int rx = p.x + vx;
                        int ry = p.y + vy;
                        if (rx >= 0 && rx < width && ry >= 0 && ry < height && grid[rx][ry] != Terrain::Wall && grid[rx][ry] != Terrain::Obstacle) {
                            zombies[i]->pos = {rx, ry};
                        } else {
                            zombies[i]->hp -= 1;
                            add_log("   [LUC DAY] Zombie dap gay tuong do áp luc dan! Roi them 1 HP!");
                        }

                        if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                            trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                        }
                    }
                }
            }
        }
        else if (input_mode == InputMode::TargetGrenade) {
            if (human.grenades <= 0) { add_log("Hết luu đạn ném tay!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
            auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
            if (vx == 0 && vy == 0) return;

            human.grenades -= 1;
            human.stamina -= 1;

            std::uniform_int_distribution<int> dist_steps(2, 4);
            int total_steps = dist_steps(rng);
            int cx = human.pos.x, cy = human.pos.y;

            for (int step = 1; step <= total_steps; ++step) {
                int nx = cx + vx;
                int ny = cy + vy;
                if (nx < 0 || nx >= width || ny < 0 || ny >= height || grid[nx][ny] == Terrain::Wall || grid[nx][ny] == Terrain::Obstacle) {
                    add_log("   [VẬT CẢN] Lựu đạn chạm vách va vang lai!", ImVec4(1.0f, 0.8f, 0.0f, 1.0f)); break;
                }
                cx = nx; cy = ny;
            }
            grenade_box.active = true;
            grenade_box.pos = {cx, cy};
            add_log("[HẸN GIỜ] Quả luu đạn xoay vòng roi nam im tai o (" + std::to_string(cx) + ", " + std::to_string(cy) + ")...", ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        }

        input_mode = InputMode::MoveMode;
        check_victory_conditions();
    }

    void start_zombie_phase() {
        if (game_over || game_won) return;
        add_log("=== QUÂN ĐỊCH BẮT ĐẦU DI CHUYỂN TUẦN TỰ ===", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        phase = TurnPhase::ZombieAnimating;
        active_zombie_idx = 0;
        active_zombie_substep = 0;
        zombie_action_timer = 0.0f;
    }

    // Tiến trình cập nhật trạng thái động của game loop thời gian thực
    void update_zombie_logic(float dt) {
        if (phase != TurnPhase::ZombieAnimating) return;
        if (active_fx.type != FXType::None) return; // Đóng băng bước đi của quái nếu hoạt họa cũ chưa phát xong

        if (active_zombie_idx >= zombies.size()) {
            // Hoan thanh toan bộ lượt di chuyển của Zombie
            if (grenade_box.active && !game_over) {
                add_log("[!] KÍCH NỔ: Luu đạn hen gio TICK.. TICK.. BÙM!", ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
                trigger_explosion(grenade_box.pos.x, grenade_box.pos.y);
                grenade_box.active = false;
            }
            if (!game_over) {
                current_turn++;
                std::uniform_int_distribution<int> dist_stam(2, 6);
                human.stamina = dist_stam(rng);
                add_log("--- LƯỢT MỚI: VÒNG " + std::to_string(current_turn) + " (Thể lực +" + std::to_string(human.stamina) + ") ---", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            }
            phase = TurnPhase::HumanTurn;
            check_victory_conditions();
            return;
        }

        auto& zom = zombies[active_zombie_idx];
        if (zom->hp <= 0 || game_over) {
            active_zombie_idx++; active_zombie_substep = 0; return;
        }

        zombie_action_timer += dt;
        if (zombie_action_timer >= std::max(0.05f, ZOMBIE_STEP_DELAY)) {
            zombie_action_timer = 0.0f;

            // Di chuyển quái vật 1 ô đơn lẻ
            zombie_single_step(active_zombie_idx);

            // Kiểm tra áp sát tấn công người chơi
            if (zom->hp > 0 && distance(zom->pos, human.pos) <= 1) {
                human.hp = std::max(0, human.hp - 1);
                add_log("[ATTACK] Kẻ địch #" + std::to_string(active_zombie_idx+1) + " (" + zom->name + ") cào rách vai Human! -1 HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                
                if (zom->type == ZombieType::Vampire) {
                    zom->hp += 1;
                    add_log("   [HÚT MÁU] Ác quỷ Dracula đuợc hồi phục +1 HP!", ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
                }
                check_victory_conditions();
            }

            active_zombie_substep++;
            if (active_zombie_substep >= zom->getMovesPerTurn()) {
                active_zombie_idx++;
                active_zombie_substep = 0;
            }
        }
    }

    void check_victory_conditions() {
        if (human.hp <= 0) {
            game_over = true;
        } else {
            bool all_dead = true;
            for (const auto& z : zombies) { if (z->hp > 0) { all_dead = false; break; } }
            if (all_dead) game_won = true;
        }
    }

    void init_game() {
        grid.assign(width, std::vector<Terrain>(height, Terrain::Dirt));
        mine_grid.assign(width, std::vector<bool>(height, false));

        for (int x = 0; x < width; ++x) { grid[x][0] = Terrain::Wall; grid[x][height - 1] = Terrain::Wall; }
        for (int y = 0; y < height; ++y) { grid[0][y] = Terrain::Wall; grid[width - 1][y] = Terrain::Wall; }

        std::uniform_int_distribution<int> dist_grid(1, width - 2);
        for (int i = 0; i < 12; ++i) { grid[dist_grid(rng)][dist_grid(rng)] = Terrain::Obstacle; }
        for (int i = 0; i < 6; ++i) {
            int rx = dist_grid(rng), ry = dist_grid(rng);
            if (grid[rx][ry] == Terrain::Dirt) grid[rx][ry] = Terrain::Water;
        }

        human.pos = {dist_grid(rng), dist_grid(rng)};
        while (grid[human.pos.x][human.pos.y] == Terrain::Wall || grid[human.pos.x][human.pos.y] == Terrain::Obstacle) {
            human.pos = {dist_grid(rng), dist_grid(rng)};
        }

        std::vector<std::tuple<ZombieType, int, std::string>> z_types = {
            {ZombieType::Normal, 2, "Zom Thường"},
            {ZombieType::Fast, 2, "Zom Nhanh"},
            {ZombieType::Exploding, 3, "Zom Nổ"},
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

        add_log("Chào mừng đến với Trận địa ZomChess C++20 Cải tiến Đồ họa!", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
    }
};

// --- 4. Khởi chạy Ứng dụng & Dựng hình Đồ họa học ---
int main() {
    sf::RenderWindow window(sf::VideoMode(1400, 740), "ZomChess C++20 Refined Tactical Edition");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    // A. [NÂNG CẤP UI]: Phóng đại Font chữ & Thiết lập giao diện màu quân đội đặc thù
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.35f; // Tăng kích thước chữ toàn diện lên 135% cực kỳ dễ nhìn

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 6.0f;     // Bo tròn khuy bấm mềm mại
    style.WindowRounding = 8.0f;    // Bo tròn viền khung điều khiển
    style.FramePadding = ImVec2(10, 8); // Tăng đệm phím nhấn to rộng, dễ bấm chuột

    // Bảng phối màu hiện đại: Nền xám đen sâu thẳm, Khuy bấm xanh quân sự Tactical tương phản lớn
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.18f, 0.38f, 0.28f, 1.0f);        // Xanh lá quân đội đặc chủng
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.52f, 0.38f, 1.0f); // Sáng bừng lên khi rê chuột qua
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.30f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.25f, 0.28f, 1.0f);

    // B. [FONT CHỮ BÀN CỜ]: Nạp phông chữ vector tiêu chuẩn có sẵn trên Ubuntu để vẽ Số thứ tự Zombie
    sf::Font boardFont;
    bool hasFont = boardFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");

    GameState state;
    sf::Clock deltaClock;

    float cellSize = 42.0f;
    float boardOffset = 20.0f;

    // --- VÒNG LẶP GAME CHUẨN HÓA THỨ TỰ ---
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed)
                window.close();

            if (!ImGui::GetIO().WantCaptureMouse && event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && !state.game_over && !state.game_won && state.phase == TurnPhase::HumanTurn) {
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
                                            state.add_log("Human di chuyen den o (" + std::to_string(tx) + ", " + std::to_string(ty) + ").");
                                        }
                                    }
                                }
                            }
                        } else {
                            state.handle_weapon_click(tx, ty, cellSize, boardOffset);
                        }
                    }
                }
            }
        }

        // 1. TÍNH DELTA TIME VÀ KHỞI TẠO FRAMESCOPE CHO IMGUI TRƯỚC TIÊN
        sf::Time dt = deltaClock.restart();
        float dtSeconds = dt.asSeconds();
        ImGui::SFML::Update(window, dt); // <-- Khắc phục hoàn toàn lỗi Assertion g.WithinFrameScope

        // 2. CẬP NHẬT LOGIC GAME THỜI GIAN THỰC
        if (state.active_fx.type != FXType::None) {
            state.active_fx.timer -= dtSeconds;
            if (state.active_fx.timer <= 0.0f) {
                state.active_fx.type = FXType::None;
            }
        }
        state.update_zombie_logic(dtSeconds);

        // 3. XÓA NỀN VÀ VẼ TOÀN BỘ ĐỒ HỌA SFML (BÀN CỜ, NHÂN VẬT, ANIMATION)
        window.clear(sf::Color(25, 26, 28));

        // Vẽ ô bàn cờ
        for (int x = 0; x < state.width; ++x) {
            for (int y = 0; y < state.height; ++y) {
                sf::RectangleShape cell(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                cell.setPosition(x * cellSize + boardOffset, y * cellSize + boardOffset);

                if (state.grid[x][y] == Terrain::Wall) cell.setFillColor(sf::Color(60, 62, 66));
                else if (state.grid[x][y] == Terrain::Obstacle) cell.setFillColor(sf::Color(40, 41, 43));
                else if (state.grid[x][y] == Terrain::Water) cell.setFillColor(sf::Color(35, 75, 115));
                else cell.setFillColor(sf::Color(105, 60, 35));

                window.draw(cell);

                if (state.mine_grid[x][y]) {
                    sf::CircleShape mine(6.0f);
                    mine.setFillColor(sf::Color(230, 40, 40));
                    mine.setOrigin(6.0f, 6.0f);
                    mine.setPosition(x * cellSize + boardOffset + cellSize/2, y * cellSize + boardOffset + cellSize/2);
                    window.draw(mine);
                }
            }
        }

        if (state.grenade_box.active) {
            sf::CircleShape gren(8.0f, 4);
            gren.setFillColor(sf::Color(50, 210, 50));
            gren.setOrigin(8.0f, 8.0f);
            gren.setPosition(state.grenade_box.pos.x * cellSize + boardOffset + cellSize/2, state.grenade_box.pos.y * cellSize + boardOffset + cellSize/2);
            window.draw(gren);
        }

        // Vẽ Zombies
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            const auto& z = state.zombies[i];
            if (z->hp <= 0) {
                sf::RectangleShape deadZ(sf::Vector2f(cellSize - 8.0f, cellSize - 8.0f));
                deadZ.setFillColor(sf::Color(15, 15, 15, 160));
                deadZ.setPosition(z->pos.x * cellSize + boardOffset + 4.0f, z->pos.y * cellSize + boardOffset + 4.0f);
                window.draw(deadZ);
                continue;
            }
            sf::RectangleShape zVisual(sf::Vector2f(cellSize - 6.0f, cellSize - 6.0f));
            zVisual.setPosition(z->pos.x * cellSize + boardOffset + 3.0f, z->pos.y * cellSize + boardOffset + 3.0f);

            if (z->type == ZombieType::Fast) zVisual.setFillColor(sf::Color(135, 200, 45));
            else if (z->type == ZombieType::Exploding) zVisual.setFillColor(sf::Color(220, 110, 15));
            else if (z->type == ZombieType::Vampire) zVisual.setFillColor(sf::Color(130, 30, 130));
            else zVisual.setFillColor(sf::Color(40, 140, 55));

            window.draw(zVisual);

            if (hasFont) {
                sf::Text zIdStr;
                zIdStr.setFont(boardFont);
                zIdStr.setString(std::to_string(i + 1));
                zIdStr.setCharacterSize(14);
                zIdStr.setFillColor(sf::Color::White);
                zIdStr.setStyle(sf::Text::Bold);
                
                sf::FloatRect textRect = zIdStr.getLocalBounds();
                zIdStr.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
                zIdStr.setPosition(z->pos.x * cellSize + boardOffset + cellSize/2.0f, z->pos.y * cellSize + boardOffset + cellSize/2.0f);
                window.draw(zIdStr);
            }
        }

        // Vẽ Human
        sf::CircleShape hVisual(cellSize / 2.5f);
        hVisual.setFillColor(sf::Color::White);
        hVisual.setPosition(state.human.pos.x * cellSize + boardOffset + 4.0f, state.human.pos.y * cellSize + boardOffset + 4.0f);
        window.draw(hVisual);

        // Vẽ các hiệu ứng đạn/nổ/chém (Visual FX)
        if (state.active_fx.type != FXType::None) {
            float progress = state.active_fx.timer / state.active_fx.max_duration;
            sf::Uint8 alpha = static_cast<sf::Uint8>(progress * 255);

            if (state.active_fx.type == FXType::Pistol) {
                sf::Vertex bulletLine[] = {
                    sf::Vertex(state.active_fx.start_p, sf::Color(255, 255, 100, alpha)),
                    sf::Vertex(state.active_fx.end_p, sf::Color(255, 60, 60, alpha))
                };
                window.draw(bulletLine, 2, sf::Lines);
            }
            else if (state.active_fx.type == FXType::Knife) {
                sf::Vector2f dir = state.active_fx.end_p - state.active_fx.start_p;
                float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
                if (len > 0) {
                    sf::Vector2f unitDir = dir / len;
                    sf::Vector2f perpDir(-unitDir.y, unitDir.x);

                    sf::ConvexShape blade;
                    blade.setPointCount(3);
                    blade.setPoint(0, state.active_fx.start_p + perpDir * 5.0f);
                    blade.setPoint(1, state.active_fx.start_p - perpDir * 5.0f);
                    blade.setPoint(2, state.active_fx.start_p + unitDir * (cellSize * 1.1f));
                    blade.setFillColor(sf::Color(0, 255, 200, alpha));
                    window.draw(blade);
                }
            }
            else if (state.active_fx.type == FXType::Shotgun) {
                for (auto p : state.active_fx.blast_cells) {
                    sf::RectangleShape orangeZone(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    orangeZone.setFillColor(sf::Color(255, 130, 30, static_cast<sf::Uint8>(progress * 130)));
                    orangeZone.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset);
                    window.draw(orangeZone);
                }
                if (!state.active_fx.blast_cells.empty()) {
                    sf::ConvexShape coneVisual;
                    coneVisual.setPointCount(3);
                    coneVisual.setPoint(0, state.active_fx.start_p);
                    coneVisual.setPoint(1, state.getCellCenter(state.active_fx.blast_cells.front().x, state.active_fx.blast_cells.front().y, cellSize, boardOffset));
                    coneVisual.setPoint(2, state.getCellCenter(state.active_fx.blast_cells.back().x, state.active_fx.blast_cells.back().y, cellSize, boardOffset));
                    coneVisual.setFillColor(sf::Color(255, 160, 50, static_cast<sf::Uint8>(progress * 45)));
                    window.draw(coneVisual);
                }
            }
            else if (state.active_fx.type == FXType::Explosion) {
                for (auto p : state.active_fx.blast_cells) {
                    sf::RectangleShape fireGrid(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                    fireGrid.setFillColor(sf::Color(255, 60, 10, static_cast<sf::Uint8>(progress * 180)));
                    fireGrid.setPosition(p.x * cellSize + boardOffset, p.y * cellSize + boardOffset);
                    window.draw(fireGrid);
                }
            }
        }

        // 4. DỰNG VÀ VẼ GIAO DIỆN IMGUI
        ImGui::SetNextWindowPos(ImVec2(670, 20));
        ImGui::SetNextWindowSize(ImVec2(710, 690));
        ImGui::Begin("Tactical Commander Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0, 1, 1, 1), "[VÒNG LƯỢT]: %d/%d", state.current_turn, state.turn_limit);
        ImGui::SameLine(); ImGui::Text(" | [STAMINA]: %d", state.human.stamina);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), " | [MÁU CƠ THỂ]: %d", state.human.hp);
        
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("END TURN (QUA LƯỢT)", ImVec2(210, 42))) { 
            if (state.phase == TurnPhase::HumanTurn) state.start_zombie_phase(); 
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        if (ImGui::Button("Dao Găm (oo)")) { state.input_mode = InputMode::TargetKnife; } ImGui::SameLine();
        if (ImGui::Button(("Súng Ngắn (" + std::to_string(state.human.pistol_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetPistol; } ImGui::SameLine();
        if (ImGui::Button(("Shotgun (" + std::to_string(state.human.shotgun_ammo) + ")").c_str())) { state.input_mode = InputMode::TargetShotgun; } ImGui::SameLine();
        if (ImGui::Button(("Lựu Đạn (" + std::to_string(state.human.grenades) + ")").c_str())) { state.input_mode = InputMode::TargetGrenade; } ImGui::SameLine();
        if (ImGui::Button(("Gài Mìn (" + std::to_string(state.human.mines) + ")").c_str())) {
            if (!state.game_over && !state.game_won && state.human.stamina >= 1 && state.human.mines > 0 && state.phase == TurnPhase::HumanTurn) {
                state.mine_grid[state.human.pos.x][state.human.pos.y] = true;
                state.human.mines--;
                state.human.stamina--;
                state.add_log("[MIN] Đặt thành công kíp nổ cố định tại vị trí đứng hiện tại!", ImVec4(1, 0.7f, 0, 1));
            }
        }

        if (state.phase == TurnPhase::ZombieAnimating) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "[CẢNH BÁO]: QUÂN ĐỊCH ĐANG DI CHUYỂN TUẦN TỰ, VUI LÒNG ĐỢI...");
        } else if (state.input_mode != InputMode::MoveMode) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[CHẾ ĐỘ TẤM NGẮM]: CLICK VÀO Ô BÀN CỜ ĐỂ KHAI HỎA VŨ KHÍ!");
        } else {
            ImGui::Text("[CHẾ ĐỘ]: Click vào các ô vây quanh để di chuyển.");
        }

        ImGui::Separator();

        ImGui::TextColored(ImVec4(1, 0.75f, 0, 1), "=== THỐNG KÊ QUÂN ĐỊCH TẠI ĐỊA BÀN ===");
        ImGui::BeginChild("ZombieTableScroll", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Columns(3, "zom_cols");
        ImGui::SetColumnWidth(0, 80); ImGui::SetColumnWidth(1, 250);
        ImGui::Text("Số hiệu"); ImGui::NextColumn(); ImGui::Text("Phân loại Chủng quái"); ImGui::NextColumn(); ImGui::Text("Trạng thái HP"); ImGui::NextColumn();
        ImGui::Separator();
        for (size_t i = 0; i < state.zombies.size(); ++i) {
            ImGui::Text("#%zu", i + 1); ImGui::NextColumn();
            ImGui::Text("%s", state.zombies[i]->name.c_str()); ImGui::NextColumn();
            if (state.zombies[i]->hp <= 0) {
                ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 0.6f), "[DEAD] TIÊU DIỆT");
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d HP Sinh lực", state.zombies[i]->hp);
            }
            ImGui::NextColumn();
        }
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0, 1, 0.7f, 1), "--- NHẬT KÝ GHI NHẬN TẬP TRUNG ---");
        ImGui::BeginChild("LogScrollBox", ImVec2(0, 260), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (const auto& log : state.logs) {
            ImGui::TextColored(log.color, "%s", log.text.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        if (state.game_over || state.game_won) {
            ImGui::OpenPopup("EndGameModal");
            ImGui::SetNextWindowPos(ImVec2(320, 220), ImGuiCond_Appearing);
            if (ImGui::BeginPopupModal("EndGameModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                if (state.game_over) ImGui::TextColored(ImVec4(1, 0.1f, 0.1f, 1), "THẤT BẠI! Lực lượng Human đã gục ngã trước đại dịch Zombie.");
                else ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "CHIẾN THẮNG! Toàn bộ khu vực đã được quét sạch quái vật bảo an.");
                ImGui::Separator();
                if (ImGui::Button("Thoát Khỏi Trận Địa", ImVec2(180, 35))) { window.close(); }
                ImGui::EndPopup();
            }
        }

        ImGui::End();

        // 5. KẾT THÚC KHUNG HÌNH VÀ RENDER LÊN MÀN HÌNH
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
