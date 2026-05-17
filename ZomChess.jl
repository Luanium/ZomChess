module ZomChessInteractive

using GLMakie 
using Random

export GameState, init_game, launch_game

# --- 1. Enums and Basic Types ---
@enum TerrainType Dirt Water Obstacle Wall
@enum InputMode MoveMode TargetKnife TargetPistol TargetShotgun TargetGrenade

mutable struct Position
    x::Int
    y::Int
end
Base.:(==)(p1::Position, p2::Position) = p1.x == p2.x && p1.y == p2.y

mutable struct GrenadeTimer
    active::Bool
    pos::Position
end

# --- 2. Entities ---
abstract type AbstractEntity end
abstract type AbstractZombie <: AbstractEntity end

mutable struct Human <: AbstractEntity
    pos::Position
    hp::Int
    stamina::Int
    pistol_ammo::Int
    shotgun_ammo::Int
    grenades::Int
    mines::Int
end

mutable struct NormalZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct FastZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct ExplodingZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct VampireZombie <: AbstractZombie pos::Position; hp::Int; name::String end

struct LogLine
    text::String
    color::Symbol
end

# --- 3. Game State ---
mutable struct GameState
    width::Int
    height::Int
    grid::Matrix{TerrainType}
    mine_grid::Matrix{Bool}
    human::Human
    zombies::Vector{AbstractZombie}
    turn_limit::Int
    current_turn::Int
    input_mode::InputMode
    grenade_box::GrenadeTimer
    logs::Vector{LogLine} 
    game_over::Bool
    game_won::Bool
end

# --- 4. Initialization ---
function init_game(; width=15, height=15, num_zombies=6)
    grid = fill(Dirt, width, height)
    mine_grid = fill(false, width, height)
    grid[:, 1] .= Wall; grid[:, height] .= Wall
    grid[1, :] .= Wall; grid[width, :] .= Wall

    for _ in 1:12
        rx, ry = rand(2:width-1), rand(2:height-1)
        grid[rx, ry] = Obstacle
    end
    
    for _ in 1:6
        rx, ry = rand(2:width-1), rand(2:height-1)
        if grid[rx, ry] == Dirt
            grid[rx, ry] = Water
        end
    end

    human = Human(Position(rand(2:width-1), rand(2:height-1)), 5, 6, 12, 6, 3, 2) 
    
    zombies = AbstractZombie[]
    z_types = [
        (NormalZombie, 2, "Zom Thuong"),
        (FastZombie, 2, "Zom Nhanh"),
        (ExplodingZombie, 3, "Zom No"),
        (VampireZombie, 4, "Zom Dracula")
    ]
    
    for i in 1:num_zombies
        zt, hp, name = rand(z_types)
        zx, zy = rand(2:width-1), rand(2:height-1)
        while Position(zx, zy) == human.pos || grid[zx, zy] == Wall || grid[zx, zy] == Obstacle
            zx, zy = rand(2:width-1), rand(2:height-1)
        end
        push!(zombies, zt(Position(zx, zy), hp, name))
    end

    init_logs = [
        LogLine("Chao mung den voi ZomChess!", :cyan),
        LogLine("Su dung chuot click ban co de hanh dong.", :lightgray)
    ]

    return GameState(width, height, grid, mine_grid, human, zombies, 50, 1, MoveMode, GrenadeTimer(false, Position(0,0)), init_logs, false, false)
end

# --- 5. Helper Functions ---
function distance(pos1::Position, pos2::Position)
    return max(abs(pos1.x - pos2.x), abs(pos1.y - pos2.y))
end

function get_8_direction(dx::Int, dy::Int)
    if dx == 0 && dy == 0 return (0, 0) end
    if abs(dx) == abs(dy)
        return (sign(dx), sign(dy))
    elseif abs(dx) > abs(dy)
        return (sign(dx), 0)
    else
        return (0, sign(dy))
    end
end

function add_game_log!(state::GameState, msg::String, color::Symbol=:lightgray)
    push!(state.logs, LogLine(msg, color))
    if length(state.logs) > 150 
        popfirst!(state.logs)
    end
end

function check_victory_conditions!(state::GameState)
    if state.human.hp <= 0
        state.game_over = true
        add_game_log!(state, "=== BAN DA THUAN TRAN! HUMAN DA CHET ===", :red)
    elseif all(z -> z.hp <= 0, state.zombies)
        state.game_won = true
        add_game_log!(state, "=== CHUC MUNG THANG LOI! DA DIET GIAI TOAN BO ZOMBIE ===", :gold)
    end
end

# --- 6. AI & Explosion Logic ---
function zombie_single_step!(state::GameState, idx::Int, zom::AbstractZombie, flash_cells_obs)
    h_pos = state.human.pos
    lambda = 1.2 
    valid_moves = Position[]
    weights = Float64[]
    
    current_dist = distance(zom.pos, h_pos)

    for dx in -1:1
        for dy in -1:1
            nx = zom.pos.x + dx
            ny = zom.pos.y + dy
            
            if 1 <= nx <= state.width && 1 <= ny <= state.height
                target_pos = Position(nx, ny)
                
                if target_pos == h_pos
                    continue
                end
                
                if state.grid[nx, ny] == Wall || state.grid[nx, ny] == Obstacle
                    continue
                end
                
                has_conflict = false
                for (other_idx, other_zom) in enumerate(state.zombies)
                    if other_idx != idx && other_zom.hp > 0 && other_zom.pos == target_pos
                        has_conflict = true
                        break
                    end
                end
                if has_conflict; continue; end
                
                new_dist = distance(target_pos, h_pos)
                w = exp(-new_dist / lambda)
                
                if new_dist > current_dist
                    w *= 0.05 
                elseif new_dist == current_dist && (dx != 0 || dy != 0)
                    w *= 0.3
                end
                
                push!(valid_moves, target_pos)
                push!(weights, w)
            end
        end
    end
    
    if !isempty(valid_moves)
        total_w = sum(weights)
        if total_w > 0
            probabilities = weights ./ total_w
            r = rand()
            cumulative_p = 0.0
            chosen_pos = valid_moves[end]
            
            for (m, p) in zip(valid_moves, probabilities)
                cumulative_p += p
                if r <= cumulative_p
                    chosen_pos = m
                    break
                end
            end
            zom.pos = chosen_pos
        end
        
        if state.mine_grid[zom.pos.x, zom.pos.y]
            zx, zy = Int(zom.pos.x), Int(zom.pos.y)
            add_game_log!(state, "[MIN NO] Zom #$idx ($(zom.name)) dap phai min o ($zx, $zy)!", :red)
            state.mine_grid[zx, zy] = false
            trigger_explosion!(state, zx, zy, flash_cells_obs)
        end
    end
end

function trigger_explosion!(state::GameState, cx::Int, cy::Int, flash_cells_obs)
    add_game_log!(state, "[EXPLOSION] Vu no lon bung phat tai tam ($cx, $cy)!", :orange)
    
    cells = Point2f[]
    for dx in -1:1, dy in -1:1
        if 1 <= cx+dx <= state.width && 1 <= cy+dy <= state.height
            push!(cells, Point2f(cx+dx, cy+dy))
        end
    end
    flash_cells_obs[] = cells
    sleep(0.2)
    flash_cells_obs[] = Point2f[]

    if max(abs(state.human.pos.x - cx), abs(state.human.pos.y - cy)) <= 1
        state.human.hp = max(0, state.human.hp - 1)
        add_game_log!(state, "-> Human dinh sat thuong no lan (-1 HP)!", :crimson)
    end
    
    for (idx, zom) in enumerate(state.zombies)
        if zom.hp > 0 && max(abs(zom.pos.x - cx), abs(zom.pos.y - cy)) <= 1
            zom.hp -= 1
            add_game_log!(state, "-> Zom #$idx ($(zom.name)) ganh sat thuong no (-1 HP).", :orange)
            
            if zom.hp <= 0 && zom isa ExplodingZombie
                add_game_log!(state, "   [LIEN HOAN NO] Kich hoat phat no day chuyen!", :red)
                trigger_explosion!(state, zom.pos.x, zom.pos.y, flash_cells_obs)
            end
            
            vx = sign(zom.pos.x - cx)
            vy = sign(zom.pos.y - cy)
            if vx != 0 || vy != 0
                px = zom.pos.x + vx
                py = zom.pos.y + vy
                if 1 <= px <= state.width && 1 <= py <= state.height && state.grid[px, py] != Wall && state.grid[px, py] != Obstacle
                    zom.pos.x = px
                    zom.pos.y = py
                else
                    zom.hp -= 1
                    add_game_log!(state, "   RAM! Zombie va vao tuong khi vang ra! -1 HP!", :red)
                    if zom.hp <= 0 && zom isa ExplodingZombie
                        trigger_explosion!(state, zom.pos.x, zom.pos.y, flash_cells_obs)
                    end
                end
            end
        end
    end
    check_victory_conditions!(state)
end

function check_zombie_death!(state::GameState, idx::Int, zom::AbstractZombie, flash_cells_obs)
    if zom.hp <= 0 && zom isa ExplodingZombie
        add_game_log!(state, "[TU VI DAO] Zom No #$idx guc nga va phat no!", :red)
        trigger_explosion!(state, zom.pos.x, zom.pos.y, flash_cells_obs)
    end
    check_victory_conditions!(state)
end

# --- 7. Weapon Action Logic ---
function handle_weapon_click!(state::GameState, tx::Int, ty::Int, flash_cells_obs, line_fx_obs, state_obs, rebuild_logs_fn)
    human = state.human
    mode = state.input_mode
    
    if human.stamina < 1
        add_game_log!(state, "Khong du the luc (stamina) de hanh dong!", :yellow)
        state.input_mode = MoveMode
        return
    end

    if mode == TargetKnife
        target_pos = Position(tx, ty)
        if distance(human.pos, target_pos) <= 1
            for (idx, zom) in enumerate(state.zombies)
                if zom.hp > 0 && zom.pos == target_pos
                    zom.hp -= 1
                    human.stamina -= 1
                    add_game_log!(state, "[KNIFE] XOET! Dam trung Zombie #$idx. HP con: $(zom.hp)", :springgreen)
                    
                    flash_cells_obs[] = [Point2f(tx, ty)]
                    sleep(0.1)
                    flash_cells_obs[] = Point2f[]

                    check_zombie_death!(state, idx, zom, flash_cells_obs)
                    state.input_mode = MoveMode
                    return
                end
            end
            add_game_log!(state, "O chi dinh khong co muc tieu song!", :yellow)
        else
            add_game_log!(state, "Dao qua ngan! Chon o sat ben canh.", :yellow)
        end

    elseif mode == TargetPistol
        if human.pistol_ammo <= 0; add_game_log!(state, "Het dan Sung ngan!", :yellow); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.pistol_ammo -= 1
        human.stamina -= 1
        add_game_log!(state, "[PISTOL] DOANG! Khai hoa sung ngan huong ($vx, $vy)...", :lightgray)
        
        end_x, end_y = human.pos.x + vx * 5, human.pos.y + vy * 5
        line_fx_obs[] = [Point2f(human.pos.x, human.pos.y), Point2f(end_x, end_y)]
        sleep(0.15)
        line_fx_obs[] = Point2f[]

        hit_target = false
        for step in 1:5
            curr_x = human.pos.x + vx * step
            curr_y = human.pos.y + vy * step
            if curr_x < 1 || curr_x > state.width || curr_y < 1 || curr_y > state.height || state.grid[curr_x, curr_y] == Wall || state.grid[curr_x, curr_y] == Obstacle
                break
            end
            for (idx, zom) in enumerate(state.zombies)
                if zom.hp > 0 && zom.pos.x == curr_x && zom.pos.y == curr_y
                    hit_chance = step <= 2 ? 1.0 : step == 3 ? 0.7 : step == 4 ? 0.4 : 0.0
                    if rand() <= hit_chance
                        zom.hp -= 1
                        add_game_log!(state, "-> Ban trung Zom #$idx o cu ly $step o. HP: $(zom.hp)", :springgreen)
                        check_zombie_death!(state, idx, zom, flash_cells_obs)
                    else
                        add_game_log!(state, "-> Dan bay chech muc tieu!", :yellow)
                    end
                    hit_target = true
                    break
                end
            end
            if hit_target; break; end
        end

    elseif mode == TargetShotgun
        if human.shotgun_ammo <= 0; add_game_log!(state, "Het dan Shotgun!", :yellow); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.shotgun_ammo -= 1
        human.stamina -= 1
        add_game_log!(state, "[SHOTGUN] DOANG! Khai hoa Shotgun huong ($vx, $vy)!", :lightgray)
        
        cone_cells = Point2f[]
        for step in 1:3
            cx = human.pos.x + vx * step
            cy = human.pos.y + vy * step
            if 1 <= cx <= state.width && 1 <= cy <= state.height
                push!(cone_cells, Point2f(cx, cy))
                if vx != 0 && vy == 0
                    push!(cone_cells, Point2f(cx, cy - 1), Point2f(cx, cy + 1))
                elseif vy != 0 && vx == 0
                    push!(cone_cells, Point2f(cx - 1, cy), Point2f(cx + 1, cy))
                end
            end
        end
        flash_cells_obs[] = cone_cells
        sleep(0.15)
        flash_cells_obs[] = Point2f[]

        for step in 1:3
            curr_x = human.pos.x + vx * step
            curr_y = human.pos.y + vy * step
            if curr_x < 1 || curr_x > state.width || curr_y < 1 || curr_y > state.height; break; end
            
            found_zom = false
            for (idx, zom) in enumerate(state.zombies)
                if zom.hp > 0 && zom.pos.x == curr_x && zom.pos.y == curr_y
                    zom.hp -= 1
                    add_game_log!(state, "-> Thoi bay Zom #$idx lui ve sau (-1 HP)!", :springgreen)
                    
                    px = zom.pos.x + vx
                    py = zom.pos.y + vy
                    if 1 <= px <= state.width && 1 <= py <= state.height && state.grid[px, py] != Wall && state.grid[px, py] != Obstacle
                        zom.pos.x = px
                        zom.pos.y = py
                        check_zombie_death!(state, idx, zom, flash_cells_obs)
                    else
                        zom.hp -= 1
                        add_game_log!(state, "   RAM! Zombie va vao tuong khi vang ra! Chiu them sat thuong!", :orange)
                        check_zombie_death!(state, idx, zom, flash_cells_obs)
                    end
                    found_zom = true
                    break
                end
            end
            if found_zom; break; end
        end

    elseif mode == TargetGrenade
        if human.grenades <= 0; add_game_log!(state, "Het Luu dan!", :yellow); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.grenades -= 1
        human.stamina -= 1
        
        total_steps = rand(2:5)
        cx, cy = human.pos.x, human.pos.y
        add_game_log!(state, "[GRENADE] VUT... Huong nem ($vx, $vy), tam bay du kien: $total_steps o", :lawngreen)
        
        for step in 1:total_steps
            next_x = cx + vx
            next_y = cy + vy
            
            if next_x < 1 || next_x > state.width || next_y < 1 || next_y > state.height || state.grid[next_x, next_y] == Wall || state.grid[next_x, next_y] == Obstacle
                add_game_log!(state, "   [VAT CAN] Luu dan bi chan tai buoc $step o vi tri ($cx, $cy)!", :yellow)
                break
            end
            cx, cy = next_x, next_y
            
            state.grenade_box.active = true
            state.grenade_box.pos = Position(cx, cy)
            rebuild_logs_fn(state)
            notify(state_obs)
            sleep(0.15)
        end
        
        state.grenade_box.active = true
        state.grenade_box.pos = Position(cx, cy)
        add_game_log!(state, "[TIMER] Luu dan da nam im tai o ($cx, $cy)...", :lawngreen)
    end

    state.input_mode = MoveMode
    check_victory_conditions!(state)
end

# --- 8. Interactive UI Layout ---
function launch_game()
    state = init_game()
    state_obs = Observable(state)

    flash_cells = Observable(Point2f[])
    line_fx = Observable(Point2f[])

    fig = Figure(size = (1400, 850), backgroundcolor = :gray12)
    
    # Clean split: 2 major columns
    left_layout = fig[1, 1] = GridLayout()
    right_layout = fig[1, 2] = GridLayout()
    
    colsize!(fig.layout, 1, Relative(0.65))
    colsize!(fig.layout, 2, Fixed(460)) 

    # --- 8.1 TOP CONTROL BAR ---
    top_bar = left_layout[1, 1] = GridLayout()
    rowsize!(left_layout, 1, Fixed(65))

    btn_end_turn = Button(top_bar[1, 1], label = "END TURN", fontsize = 16, width = 120, height = 40, buttoncolor = :tomato, labelcolor = :white, font = :bold)
    
    dyn_info = lift(s -> " [LUOT]: $(s.current_turn)/$(s.turn_limit)   |   [STAMINA]: $(s.human.stamina)   |   [MAU - HP]: $(s.human.hp)  " * (s.input_mode == MoveMode ? "[MODE: DI CHUYEN]" : "[MODE: CHON MUC TIEU VU KHI]"), state_obs)
    Label(top_bar[1, 2], dyn_info, fontsize = 16, halign = :left, font = :bold, color = :whitesmoke)

    # --- 8.2 WEAPON TOOLBAR ---
    weapon_bar = left_layout[2, 1] = GridLayout()
    rowsize!(left_layout, 2, Fixed(50))

    btn_knife = Button(weapon_bar[1, 1], label = "Dao (oo)", fontsize = 13, height = 35)
    btn_pistol = Button(weapon_bar[1, 2], label = lift(s -> "Sung Ngan ($(s.human.pistol_ammo))", state_obs), fontsize = 13, height = 35)
    btn_shotgun = Button(weapon_bar[1, 3], label = lift(s -> "Shotgun ($(s.human.shotgun_ammo))", state_obs), fontsize = 13, height = 35)
    btn_grenade = Button(weapon_bar[1, 4], label = lift(s -> "Luu Dan ($(s.human.grenades))", state_obs), fontsize = 13, height = 35)
    btn_mine = Button(weapon_bar[1, 5], label = lift(s -> "Dat Min ($(s.human.mines))", state_obs), fontsize = 13, height = 35, buttoncolor = :gold4, labelcolor = :white)

    # --- 8.3 MAIN TACTICAL BOARD ---
    # CRITICAL FIX: tellheight=false, tellwidth=false guarantees the board NEVER collapses or shrinks to 0
    ax = Axis(left_layout[3, 1], aspect = DataAspect(), backgroundcolor = :black, tellheight = false, tellwidth = false)
    rowsize!(left_layout, 3, Auto())
    
    hidespines!(ax)
    hidedecorations!(ax)
    
    xlims!(ax, 0.0, state.width + 1.0)
    ylims!(ax, 0.0, state.height + 1.0)

    for x in 1:state.width
        for y in 1:state.height
            color = state.grid[x, y] == Wall ? :gray30 : 
                    state.grid[x, y] == Obstacle ? :gray18 : 
                    state.grid[x, y] == Water ? :deepskyblue4 : :sienna
            poly!(ax, Rect(x-0.5, y-0.5, 1, 1), color=color, strokewidth=1.0, strokecolor=:gray15)
        end
    end

    human_pos = lift(s -> Point2f(s.human.pos.x, s.human.pos.y), state_obs)
    get_z_color(z) = z isa FastZombie ? :yellowgreen : z isa ExplodingZombie ? :darkorange : z isa VampireZombie ? :darkmagenta : :darkgreen

    zombie_dead_pos = lift(s -> let pts = Point2f[Point2f(z.pos.x, z.pos.y) for z in s.zombies if z.hp <= 0]
        isempty(pts) ? [Point2f(-10, -10)] : pts
    end, state_obs)

    zombie_alive_pos = lift(s -> let pts = Point2f[Point2f(z.pos.x, z.pos.y) for z in s.zombies if z.hp > 0]
        isempty(pts) ? [Point2f(-10, -10)] : pts
    end, state_obs)

    zombie_alive_colors = lift(s -> let cls = [get_z_color(z) for z in s.zombies if z.hp > 0]
        isempty(cls) ? [:black] : cls
    end, state_obs)
    
    mine_positions = lift(s -> let pts = [Point2f(x, y) for x in 1:s.width, y in 1:s.height if s.mine_grid[x, y]]
        isempty(pts) ? [Point2f(-10, -10)] : pts
    end, state_obs)

    grenade_pos = lift(s -> s.grenade_box.active ? [Point2f(s.grenade_box.pos.x, s.grenade_box.pos.y)] : [Point2f(-100, -100)], state_obs)

    scatter!(ax, mine_positions, color=:red, markersize=14, marker=:circle)
    scatter!(ax, grenade_pos, color=:lawngreen, markersize=22, marker=:utriangle) 
    scatter!(ax, zombie_dead_pos, color=:black, markersize=34, marker=:rect, strokewidth=1.0, strokecolor=:gray40)
    scatter!(ax, zombie_dead_pos, color=:red, markersize=16, marker=:x)
    scatter!(ax, zombie_alive_pos, color=zombie_alive_colors, markersize=34, marker=:rect)
    
    zombie_alive_labels = lift(s -> let lbs = ["#$i" for (i, z) in enumerate(s.zombies) if z.hp > 0]
        isempty(lbs) ? [""] : lbs
    end, state_obs)
    text!(ax, zombie_alive_pos, text = zombie_alive_labels, color = :whitesmoke, fontsize = 13, font = :bold, align = (:center, :center))
    scatter!(ax, human_pos, color=:white, markersize=38, marker=:star5)

    lines!(ax, line_fx, color=:gold, linewidth=4, transparency=true)
    scatter!(ax, flash_cells, color=(:red, 0.6), markersize=42, marker=:rect)

    # --- 8.4 SIDEBAR ENEMY LIST ---
    Box(right_layout[1, 1], color = :gray20, strokewidth = 1, strokecolor = :gray35, cornerradius = 4)
    z_layout = right_layout[1, 1] = GridLayout(padding = (12, 12, 12, 12))
    rowsize!(right_layout, 1, Fixed(250)) # Strictly bounded height to prevent visual overlap

    Label(z_layout[1, 1:3], "=== DANH SACH KE DICH ===", fontsize = 15, font = :bold, color = :gold, halign = :center)
    Label(z_layout[2, 1], "STT", font = :bold, color = :lightgray, halign = :left)
    Label(z_layout[2, 2], "Chung loai", font = :bold, color = :lightgray, halign = :left)
    Label(z_layout[2, 3], "Mau (HP)", font = :bold, color = :lightgray, halign = :center)

    num_zom = length(state.zombies)
    for i in 1:num_zom
        Label(z_layout[2 + i, 1], " #$i", fontsize = 13, color = :whitesmoke, halign = :left)
        Label(z_layout[2 + i, 2], lift(s -> s.zombies[i].name, state_obs), fontsize = 13, color = :whitesmoke, halign = :left)
        z_hp_text = lift(s -> s.zombies[i].hp <= 0 ? "[X] DA CHET" : "$(s.zombies[i].hp) HP", state_obs)
        z_hp_color = lift(s -> s.zombies[i].hp <= 0 ? :red : :springgreen, state_obs)
        Label(z_layout[2 + i, 3], z_hp_text, fontsize = 13, halign = :center, color = z_hp_color, font = :bold)
    end

    # --- 8.5 BATTLE LOG WINDOW WITH STRICTLY ISOLATED ROWS ---
    log_container = right_layout[2, 1] = GridLayout()
    rowsize!(right_layout, 2, Auto()) 
    Label(log_container[1, 1:2], "--- NHAT KY CHIEN TRUONG ---", fontsize = 13, font = :bold, color = :cyan, halign = :center)
    
    Box(log_container[2, 1:2], color = :black, strokewidth = 2, strokecolor = :firebrick)
    log_sub_layout = log_container[2, 1] = GridLayout(padding = (12, 12, 12, 12), halign = :left, valign = :top)
    
    # Create 15 structural placeholder labels
    log_labels = [Label(log_sub_layout[r, 1], "", fontsize = 13, halign = :left, padding = (1,1,1,1)) for r in 1:15]
    
    # CRITICAL FIX: Explicitly lock the height of each row cell to completely ban text overlapping
    for r in 1:15
        rowsize!(log_sub_layout, r, Fixed(24))
    end
    
    log_slider = Slider(log_container[2, 2], range = 1:1, startvalue = 1, horizontal = false, tellwidth = true, width = 15)

    function rebuild_scrollable_logs!(curr_state)
        n_logs = length(curr_state.logs)
        max_display = 15
        
        if n_logs <= max_display
            log_slider.range[] = 1:1
            start_idx = 1
        else
            max_start = n_logs - max_display + 1
            log_slider.range[] = 1:max_start
            
            # Auto-scroll downward when new events arrive
            if log_slider.value[] < 1 || log_slider.value[] > max_start
                set_close_to!(log_slider, max_start)
            end
            start_idx = log_slider.value[]
        end
        
        # Inject textual data dynamically into fixed geometry layout
        for r in 1:max_display
            curr_idx = start_idx + r - 1
            if curr_idx <= n_logs
                line = curr_state.logs[curr_idx]
                log_labels[r].text = " " * line.text
                log_labels[r].color = line.color
                log_labels[r].font = (line.color in [:cyan, :red, :crimson, :orange, :gold]) ? :bold : :regular
            else
                log_labels[r].text = "" 
            end
        end
    end

    on(log_slider.value) do _
        rebuild_scrollable_logs!(state_obs[])
    end

    rebuild_scrollable_logs!(state)

    # --- 8.6 OVERLAY SYSTEM ---
    overlay_box = Box(fig[1, :], color = (:black, 0.85), visible = false)
    overlay_label = Label(fig[1, :], "GAME STATUS", fontsize = 36, font = :bold, color = :white, visible = false)

    function update_game_status_visuals!(curr_state)
        if curr_state.game_over
            overlay_label.text = "GAME OVER\nHuman da hy sinh!"
            overlay_label.color = :crimson
            overlay_box.visible = true
            overlay_label.visible = true
        elseif curr_state.game_won
            overlay_label.text = "VICTORY!\nBan da quet sach lu doan Zombie!"
            overlay_label.color = :gold
            overlay_box.visible = true
            overlay_label.visible = true
        end
    end

    # --- 9. INTERACTIVE LISTENERS ---
    on(btn_knife.clicks) do _; if state.game_over || state.game_won return end; state_obs[].input_mode = TargetKnife; notify(state_obs); end
    on(btn_pistol.clicks) do _; if state.game_over || state.game_won return end; state_obs[].input_mode = TargetPistol; notify(state_obs); end
    on(btn_shotgun.clicks) do _; if state.game_over || state.game_won return end; state_obs[].input_mode = TargetShotgun; notify(state_obs); end
    on(btn_grenade.clicks) do _; if state.game_over || state.game_won return end; state_obs[].input_mode = TargetGrenade; notify(state_obs); end
    
    on(btn_mine.clicks) do _
        if state.game_over || state.game_won return end
        curr_state = state_obs[]
        if curr_state.human.stamina >= 1 && curr_state.human.mines > 0
            hx, hy = curr_state.human.pos.x, curr_state.human.pos.y
            curr_state.mine_grid[hx, hy] = true
            curr_state.human.mines -= 1
            curr_state.human.stamina -= 1
            add_game_log!(curr_state, "[MIN] Cai min thanh cong tai o ($hx, $hy)!", :gold)
            rebuild_scrollable_logs!(curr_state)
            notify(state_obs)
        end
    end

    on(events(fig).mousebutton) do event
        if state.game_over || state.game_won return end  
        if event.button == Mouse.left && event.action == Mouse.press
            mouse_pos = mouseposition(ax.scene)
            target_x = round(Int, mouse_pos[1])
            target_y = round(Int, mouse_pos[2])

            current_state = state_obs[]
            h_pos = current_state.human.pos

            if 1 <= target_x <= current_state.width && 1 <= target_y <= current_state.height
                if current_state.input_mode == MoveMode
                    dx = abs(target_x - h_pos.x)
                    dy = abs(target_y - h_pos.y)
                    if dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)
                        if current_state.grid[target_x, target_y] != Wall && current_state.grid[target_x, target_y] != Obstacle
                            
                            is_blocked_by_zom = false
                            for zom in current_state.zombies
                                if zom.hp > 0 && zom.pos == Position(target_x, target_y)
                                    is_blocked_by_zom = true
                                    break
                                end
                            end
                            
                            if is_blocked_by_zom
                                add_game_log!(current_state, "Khong the di vao o cua Zombie dang song!", :yellow)
                                rebuild_scrollable_logs!(current_state)
                                return
                            end

                            cost = current_state.grid[target_x, target_y] == Water ? 2 : 1
                            if current_state.human.stamina >= cost
                                current_state.human.pos.x = target_x
                                current_state.human.pos.y = target_y
                                current_state.human.stamina -= cost
                                add_game_log!(current_state, "Human di chuyen sang o ($target_x, $target_y).", :lightgray)
                                rebuild_scrollable_logs!(current_state)
                                notify(state_obs)
                            end
                        end
                    end
                else
                    @async begin
                        handle_weapon_click!(current_state, target_x, target_y, flash_cells, line_fx, state_obs, rebuild_scrollable_logs!)
                        rebuild_scrollable_logs!(current_state)
                        update_game_status_visuals!(current_state)
                        notify(state_obs)
                    end
                end
            end
        end
    end

    on(btn_end_turn.clicks) do _
        if state.game_over || state.game_won return end
        
        @async begin 
            current_state = state_obs[]
            
            add_game_log!(current_state, "=== QUAN DICH BAT DAU DI CHUYEN LUOT ===", :yellow)
            rebuild_scrollable_logs!(current_state)
            
            for (idx, zom) in enumerate(current_state.zombies)
                if zom.hp <= 0; continue; end
                
                moves_this_turn = zom isa FastZombie ? 2 : 1
                step = 1
                while step <= moves_this_turn
                    if zom.hp <= 0 || current_state.game_over; break; end
                    
                    zombie_single_step!(current_state, idx, zom, flash_cells)
                    notify(state_obs)
                    sleep(0.2) 
                    
                    if zom.hp > 0 && distance(zom.pos, current_state.human.pos) <= 1
                        current_state.human.hp = max(0, current_state.human.hp - 1)
                        add_game_log!(current_state, "[ATTACK] Zom #$idx ($(zom.name)) can ban! -1 HP.", :crimson)
                        
                        if zom isa VampireZombie
                            zom.hp += 1
                            add_game_log!(current_state, "   [HUT MAU] Vampire Zom duoc +1 HP!", :magenta)
                        end
                        
                        if zom isa FastZombie && step == moves_this_turn
                            moves_this_turn += 1
                            add_game_log!(current_state, "   [KICH TOC] Fast Zom can trung, kich hoat di them o!", :yellowgreen)
                        end
                        
                        rebuild_scrollable_logs!(current_state)
                        notify(state_obs)
                        sleep(0.15)
                    end
                    step += 1
                end
            end
            
            if current_state.grenade_box.active && !current_state.game_over
                gx = current_state.grenade_box.pos.x
                gy = current_state.grenade_box.pos.y
                add_game_log!(current_state, "[!] Luu dan tai ($gx, $gy) TICK... TICK... NO TUNG!", :crimson)
                trigger_explosion!(current_state, gx, gy, flash_cells)
                current_state.grenade_box.active = false
            end

            if !current_state.game_over
                current_state.current_turn += 1
                current_state.human.stamina = rand(2:6)
                current_state.input_mode = MoveMode
                add_game_log!(current_state, "--- LUOT MOI: VONG $(current_state.current_turn) (The luc hoi phuc: +$(current_state.human.stamina)) ---", :cyan)
            end

            check_victory_conditions!(current_state)
            rebuild_scrollable_logs!(current_state)
            update_game_status_visuals!(current_state)
            notify(state_obs)
        end
    end

    display(fig)
    return fig
end

end # module
