#include "GameState.h"
#include <cmath>
#include <algorithm>

GameState::GameState() : rng(std::random_device{}()) {
    init_game();
}

void GameState::add_log(const std::string& text, ImVec4 color) {
    logs.push_back({text, color});
    if (logs.size() > 100) logs.erase(logs.begin());
}

int GameState::distance(Position p1, Position p2) {
    return std::max(std::abs(p1.x - p2.x), std::abs(p1.y - p2.y));
}

std::pair<int, int> GameState::get_8_direction(int dx, int dy) {
    if (dx == 0 && dy == 0) return {0, 0};
    if (std::abs(dx) == std::abs(dy)) return {dx > 0 ? 1 : -1, dy > 0 ? 1 : -1};
    if (std::abs(dx) > std::abs(dy)) return {dx > 0 ? 1 : -1, 0};
    return {0, dy > 0 ? 1 : -1};
}

sf::Vector2f GameState::getCellCenter(int x, int y, float cellSize, float offset) {
    return sf::Vector2f(x * cellSize + offset + cellSize / 2.0f, y * cellSize + offset + cellSize / 2.0f);
}

void GameState::trigger_explosion(int cx, int cy) {
    add_log("[EXPLOSION] Massive explosion triggered at (" + std::to_string(cx) + ", " + std::to_string(cy) + ")!", ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
    
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
        add_log("-> Blast wave hits Human! Damage taken (-1 HP)!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    }

    for (size_t i = 0; i < zombies.size(); ++i) {
        auto& zom = zombies[i];
        if (zom->hp > 0 && distance(zom->pos, {cx, cy}) <= 1) {
            zom->hp -= 1;
            add_log("-> Zombie #" + std::to_string(i+1) + " caught in blast radius (-1 HP).", ImVec4(1.0f, 0.6f, 0.1f, 1.0f));

            if (zom->hp <= 0 && zom->type == ZombieType::Exploding) {
                trigger_explosion(zom->pos.x, zom->pos.y);
            }
            
            int vx = (zom->pos.x - cx > 0) ? 1 : (zom->pos.x - cx < 0 ? -1 : 0);
            int vy = (zom->pos.y - cy > 0) ? 1 : (zom->pos.y - cy < 0 ? -1 : 0);
            if (vx != 0 || vy != 0) {
                int px = zom->pos.x + vx;
                int py = zom->pos.y + vy;
                if (px >= 0 && px < width && py >= 0 && py < height && grid[px][py] != Terrain::Wall && grid[px][py] != Terrain::Obstacle) {
                    zom->pos = {px, py};
                } else {
                    zom->hp -= 1;
                    add_log("   [IMPACT] Zombie slammed into an obstacle! Extre damage (-1 HP)!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                    if (zom->hp <= 0 && zom->type == ZombieType::Exploding) {
                        trigger_explosion(zom->pos.x, zom->pos.y);
                    }
                }
            }
        }
    }
    check_victory_conditions();
}

void GameState::zombie_single_step(size_t idx) {
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
            add_log("[LANDMINE] Zombie #" + std::to_string(idx+1) + " stepped on a mine at (" + std::to_string(zom->pos.x) + ", " + std::to_string(zom->pos.y) + ")!", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            mine_grid[zom->pos.x][zom->pos.y] = false;
            trigger_explosion(zom->pos.x, zom->pos.y);
        }
    }
}

void GameState::handle_weapon_click(int tx, int ty, float cellSize, float boardOffset) {
    if (human.stamina < 1) {
        add_log("Exhausted (stamina < 1), action failed!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
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
                    add_log("[KNIFE] Slashed Zombie #" + std::to_string(i+1) + "!", ImVec4(0.1f, 1.0f, 0.6f, 1.0f));
                    
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
            add_log("No valid target found in that cell!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        } else {
            add_log("Knife can only target adjacent cells!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        }
    } 
    else if (input_mode == InputMode::TargetPistol) {
        if (human.pistol_ammo <= 0) { add_log("Pistol out of ammo!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
        if (vx == 0 && vy == 0) return;

        human.pistol_ammo -= 1;
        human.stamina -= 1;
        add_log("[PISTOL] Fired pistol towards direction (" + std::to_string(vx) + ", " + std::to_string(vy) + ")...", ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

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

        active_fx.type = FXType::Pistol;
        active_fx.timer = 0.25f;
        active_fx.max_duration = 0.25f;
        active_fx.start_p = hCenter;
        active_fx.end_p = getCellCenter(hit_pos.x, hit_pos.y, cellSize, boardOffset);

        for (size_t i = 0; i < zombies.size(); ++i) {
            if (zombies[i]->hp > 0 && zombies[i]->pos == hit_pos) {
                int dist = distance(human.pos, hit_pos);
                double chance = (dist <= 2) ? 1.0 : (dist == 3 ? 0.7 : (dist == 4 ? 0.4 : 0.0));
                std::uniform_real_distribution<double> dist_chance(0.0, 1.0);
                if (dist_chance(rng) <= chance) {
                    zombies[i]->hp -= 1;
                    add_log("-> Shot hit Zombie #" + std::to_string(i+1) + "! -1 HP.", ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                        trigger_explosion(hit_pos.x, hit_pos.y);
                    }
                } else {
                    add_log("-> The shot missed the target widely!", ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                }
                break;
            }
        }
    }
    else if (input_mode == InputMode::TargetShotgun) {
        if (human.shotgun_ammo <= 0) { add_log("Shotgun out of ammo!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
        auto [vx, vy] = get_8_direction(tx - human.pos.x, ty - human.pos.y);
        if (vx == 0 && vy == 0) return;

        human.shotgun_ammo -= 1;
        human.stamina -= 1;
        add_log("[SHOTGUN] BOOM! Unleashed cone blast fire!", ImVec4(1.0f, 0.4f, 0.2f, 1.0f));

        active_fx.type = FXType::Shotgun;
        active_fx.timer = 0.35f;
        active_fx.max_duration = 0.35f;
        active_fx.blast_cells.clear();
        active_fx.start_p = hCenter;

        int px = -vy, py = vx; 
        for (int step = 1; step <= 3; ++step) {
            int cx = human.pos.x + vx * step;
            int cy = human.pos.y + vy * step;
            int spread = step - 1; 
            
            for (int s = -spread; s <= spread; ++s) {
                int fx = cx + px * s;
                int fy = cy + py * s;
                if (fx >= 0 && fx < width && fy >= 0 && fy < height) {
                    active_fx.blast_cells.push_back({fx, fy});
                }
            }
        }

        for (auto p : active_fx.blast_cells) {
            if (grid[p.x][p.y] == Terrain::Wall || grid[p.x][p.y] == Terrain::Obstacle) continue;
            for (size_t i = 0; i < zombies.size(); ++i) {
                if (zombies[i]->hp > 0 && zombies[i]->pos == p) {
                    zombies[i]->hp -= 1;
                    add_log("-> Shrapnel shreds Zombie #" + std::to_string(i+1) + ". -1 HP.");
                    
                    int rx = p.x + vx;
                    int ry = p.y + vy;
                    if (rx >= 0 && rx < width && ry >= 0 && ry < height && grid[rx][ry] != Terrain::Wall && grid[rx][ry] != Terrain::Obstacle) {
                        zombies[i]->pos = {rx, ry};
                    } else {
                        zombies[i]->hp -= 1;
                        add_log("   [KNOCKBACK] Zombie crushed against obstacle due to impact! Extra damage (-1 HP)!");
                    }

                    if (zombies[i]->hp <= 0 && zombies[i]->type == ZombieType::Exploding) {
                        trigger_explosion(zombies[i]->pos.x, zombies[i]->pos.y);
                    }
                }
            }
        }
    }
    else if (input_mode == InputMode::TargetGrenade) {
        if (human.grenades <= 0) { add_log("Out of grenades!", ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); input_mode = InputMode::MoveMode; return; }
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
                add_log("   [OBSTACLE] Grenade bounced back from wall!", ImVec4(1.0f, 0.8f, 0.0f, 1.0f)); break;
            }
            cx = nx; cy = ny;
        }
        grenade_box.active = true;
        grenade_box.pos = {cx, cy};
        add_log("[TIMER] Grenade dropped at (" + std::to_string(cx) + ", " + std::to_string(cy) + ")...", ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
    }

    input_mode = InputMode::MoveMode;
    check_victory_conditions();
}

void GameState::start_zombie_phase() {
    if (game_over || game_won) return;
    add_log("=== ZOMBIES HIVE SQUAD TURN START ===", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    phase = TurnPhase::ZombieAnimating;
    active_zombie_idx = 0;
    active_zombie_substep = 0;
    zombie_action_timer = 0.0f;
}

void GameState::update_zombie_logic(float dt) {
    if (phase != TurnPhase::ZombieAnimating) return;
    if (active_fx.type != FXType::None) return; 

    if (active_zombie_idx >= zombies.size()) {
        if (grenade_box.active && !game_over) {
            add_log("[!] TIME-OUT DETONATION: Grenade TICK.. TICK.. BOOM!", ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
            trigger_explosion(grenade_box.pos.x, grenade_box.pos.y);
            grenade_box.active = false;
        }
        if (!game_over) {
            current_turn++;
            std::uniform_int_distribution<int> dist_stam(2, 6);
            human.stamina = dist_stam(rng);
            add_log("--- NEW TURN: WAVE " + std::to_string(current_turn) + " (Stamina +" + std::to_string(human.stamina) + ") ---", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
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

        zombie_single_step(active_zombie_idx);

        if (zom->hp > 0 && distance(zom->pos, human.pos) <= 1) {
            human.hp = std::max(0, human.hp - 1);
            add_log("[ATTACK] Enemy #" + std::to_string(active_zombie_idx+1) + " (" + zom->name + ") clawed Human! -1 HP.", ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            
            if (zom->type == ZombieType::Vampire) {
                zom->hp += 1;
                add_log("   [VAMPIRISM] Dracula generic fiend healed +1 HP!", ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
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

void GameState::check_victory_conditions() {
    if (human.hp <= 0) {
        game_over = true;
    } else {
        bool all_dead = true;
        for (const auto& z : zombies) { if (z->hp > 0) { all_dead = false; break; } }
        if (all_dead) game_won = true;
    }
}

void GameState::init_game() {
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
        {ZombieType::Normal, 2, "Normal Zom"},
        {ZombieType::Fast, 2, "Fast Zom"},
        {ZombieType::Exploding, 3, "Exploder"},
        {ZombieType::Vampire, 4, "Dracula"}
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

    add_log("Welcome to ZomChess Tactical Refinement Battleground!", ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
}
