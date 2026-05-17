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
    logs::Vector{String}
end

# --- 4. Initialization ---
function init_game(; width=15, height=15, num_zombies=5)
    grid = fill(Dirt, width, height)
    mine_grid = fill(false, width, height)
    grid[:, 1] .= Wall; grid[:, height] .= Wall
    grid[1, :] .= Wall; grid[width, :] .= Wall

    for _ in 1:10
        rx, ry = rand(2:width-1), rand(2:height-1)
        grid[rx, ry] = Obstacle
    end
    
    for _ in 1:5
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

    init_logs = ["Chao mung den voi ZomChess!", "Su dung chuot click ban co de hanh dong."]

    return GameState(width, height, grid, mine_grid, human, zombies, 50, 1, MoveMode, GrenadeTimer(false, Position(0,0)), init_logs)
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

function add_game_log!(state::GameState, msg::String)
    push!(state.logs, msg)
    if length(state.logs) > 50
        popfirst!(state.logs)
    end
end

# --- 6. AI Logic (Zombie Turn) ---
function zombie_single_step!(state::GameState, idx::Int, zom::AbstractZombie)
    h_pos = state.human.pos
    lambda = 3.0
    valid_moves = Position[]
    weights = Float64[]
    
    for dx in -1:1
        for dy in -1:1
            nx = zom.pos.x + dx
            ny = zom.pos.y + dy
            
            if 1 <= nx <= state.width && 1 <= ny <= state.height
                target_pos = Position(nx, ny)
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
                
                d = distance(target_pos, h_pos)
                push!(valid_moves, target_pos)
                push!(weights, exp(-d / lambda))
            end
        end
    end
    
    if !isempty(valid_moves)
        total_w = sum(weights)
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
        
        if state.mine_grid[zom.pos.x, zom.pos.y]
            zx, zy = Int(zom.pos.x), Int(zom.pos.y)
            add_game_log!(state, "[MIN NO] Zom #$idx ($(zom.name)) dap phai min tai o ($zx, $zy)!")
            state.mine_grid[zx, zy] = false
            trigger_explosion!(state, zx, zy)
        end
    end
end

function zombie_turn!(state::GameState)
    for (idx, zom) in enumerate(state.zombies)
        if zom.hp <= 0; continue; end
        
        moves_this_turn = zom isa FastZombie ? 2 : 1
        step = 1
        while step <= moves_this_turn
            if zom.hp <= 0; break; end
            
            zombie_single_step!(state, idx, zom)
            
            if zom.hp > 0 && distance(zom.pos, state.human.pos) <= 1
                state.human.hp -= 1
                add_game_log!(state, "[ATTACK] Zom #$idx ($(zom.name)) can ban! Ban mat 1 HP.")
                
                if zom isa VampireZombie
                    zom.hp += 1
                    add_game_log!(state, "   [HUT MAU] Vampire Zom duoc +1 HP!")
                end
                
                if zom isa FastZombie && step == moves_this_turn
                    moves_this_turn += 1
                    add_game_log!(state, "   [KICH TOC] Fast Zom can trung, di them 1 o!")
                end
            end
            step += 1
        end
    end
end

function trigger_explosion!(state::GameState, cx::Int, cy::Int)
    add_game_log!(state, "[EXPLOSION] Vu no lan toa tai tam ($cx, $cy)!")
    
    if max(abs(state.human.pos.x - cx), abs(state.human.pos.y - cy)) <= 1
        state.human.hp -= 1
        add_game_log!(state, "-> Human dinh sat thuong no lan (-1 HP)!")
    end
    
    for (idx, zom) in enumerate(state.zombies)
        if zom.hp > 0 && max(abs(zom.pos.x - cx), abs(zom.pos.y - cy)) <= 1
            zom.hp -= 1
            add_game_log!(state, "-> Zom #$idx ($(zom.name)) trung no (-1 HP).")
            
            if zom.hp <= 0 && zom isa ExplodingZombie
                add_game_log!(state, "   [LIEN HOAN NO] Kich hoat no day chuyen!")
                trigger_explosion!(state, zom.pos.x, zom.pos.y)
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
                    add_game_log!(state, "   RAM! Zombie dap lung vao tuong khi vang ra! -1 HP!")
                    if zom.hp <= 0 && zom isa ExplodingZombie
                        trigger_explosion!(state, zom.pos.x, zom.pos.y)
                    end
                end
            end
        end
    end
end

function check_zombie_death!(state::GameState, idx::Int, zom::AbstractZombie)
    if zom.hp <= 0 && zom isa ExplodingZombie
        add_game_log!(state, "[TU VI DAO] Zom No #$idx guc nga va phat no!")
        trigger_explosion!(state, zom.pos.x, zom.pos.y)
    end
end

# --- 7. Weapon Action Logic ---
function handle_weapon_click!(state::GameState, tx::Int, ty::Int)
    human = state.human
    mode = state.input_mode
    
    if human.stamina < 1
        add_game_log!(state, "Khong du the luc (stamina) de ra chieu!")
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
                    add_game_log!(state, "[KNIFE] XOET! Ban dam trung Zombie #$idx. HP con: $(zom.hp)")
                    check_zombie_death!(state, idx, zom)
                    state.input_mode = MoveMode
                    return
                end
            end
            add_game_log!(state, "O chi dinh khong co muc tieu hop le!")
        else
            add_game_log!(state, "Dao qua ngan! Hay chon o sat ben canh.")
        end

    elseif mode == TargetPistol
        if human.pistol_ammo <= 0; add_game_log!(state, "Het dan Sung ngan!"); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.pistol_ammo -= 1
        human.stamina -= 1
        add_game_log!(state, "[PISTOL] DOANG! Khai hoa sung ngan huong ($vx, $vy)...")
        
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
                        add_game_log!(state, "-> Trung Zom #$idx o khoang cach $step o. HP: $(zom.hp)")
                        check_zombie_death!(state, idx, zom)
                    else
                        add_game_log!(state, "-> Dan bay lech muc tieu do qua xa!")
                    end
                    hit_target = true
                    break
                end
            end
            if hit_target; break; end
        end

    elseif mode == TargetShotgun
        # FIXED typo: Sửa từ x_ammo sang human.shotgun_ammo công thức chuẩn
        if human.shotgun_ammo <= 0; add_game_log!(state, "Het dan Shotgun!"); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.shotgun_ammo -= 1
        human.stamina -= 1
        add_game_log!(state, "[SHOTGUN] DOANG! Shotgun khac lua huong ($vx, $vy)!")
        
        for step in 1:3
            curr_x = human.pos.x + vx * step
            curr_y = human.pos.y + vy * step
            if curr_x < 1 || curr_x > state.width || curr_y < 1 || curr_y > state.height; break; end
            
            found_zom = false
            for (idx, zom) in enumerate(state.zombies)
                if zom.hp > 0 && zom.pos.x == curr_x && zom.pos.y == curr_y
                    zom.hp -= 1
                    add_game_log!(state, "-> Thoi bay Zom #$idx lui ra sau (-1 HP)!")
                    
                    px = zom.pos.x + vx
                    py = zom.pos.y + vy
                    if 1 <= px <= state.width && 1 <= py <= state.height && state.grid[px, py] != Wall && state.grid[px, py] != Obstacle
                        zom.pos.x = px
                        zom.pos.y = py
                        check_zombie_death!(state, idx, zom)
                    else
                        zom.hp -= 1
                        add_game_log!(state, "   RAM! Zombie dap lung vao tuong! Chiu them sat thuong!")
                        check_zombie_death!(state, idx, zom)
                    end
                    found_zom = true
                    break
                end
            end
            if found_zom; break; end
        end

    elseif mode == TargetGrenade
        if human.grenades <= 0; add_game_log!(state, "Het Luu dan!"); state.input_mode = MoveMode; return; end
        vx, vy = get_8_direction(tx - human.pos.x, ty - human.pos.y)
        if vx == 0 && vy == 0; return; end
        
        human.grenades -= 1
        human.stamina -= 1
        
        total_steps = rand(1:6)
        cx, cy = human.pos.x, human.pos.y
        add_game_log!(state, "[GRENADE] VUT... Random nem xa: $total_steps o, huong ($vx, $vy)")
        
        for step in 1:total_steps
            next_x = cx + vx
            next_y = cy + vy
            is_collision = false
            hit_wall_x = false
            hit_wall_y = false
            
            if next_x < 1 || next_x > state.width || state.grid[next_x, cy] == Wall || state.grid[next_x, cy] == Obstacle
                hit_wall_x = true; is_collision = true
            end
            if next_y < 1 || next_y > state.height || state.grid[cx, next_y] == Wall || state.grid[cx, next_y] == Obstacle
                hit_wall_y = true; is_collision = true
            end
            if !hit_wall_x && !hit_wall_y
                if next_x < 1 || next_x > state.width || next_y < 1 || next_y > state.height || state.grid[next_x, next_y] == Wall || state.grid[next_x, next_y] == Obstacle
                    hit_wall_x = true; hit_wall_y = true; is_collision = true
                end
            end
            
            if is_collision
                if hit_wall_x; vx = -vx; end
                if hit_wall_y; vy = -vy; end
                add_game_log!(state, "   [NAY] Cham tuong o buoc $(step)! Doi huong sang ($vx, $vy)")
                
                next_x = clamp(cx + vx, 1, state.width)
                next_y = clamp(cy + vy, 1, state.height)
                
                if state.grid[next_x, next_y] == Wall || state.grid[next_x, next_y] == Obstacle
                    next_x, next_y = cx, cy
                end
            end
            cx, cy = next_x, next_y
        end
        
        cx = clamp(cx, 1, state.width)
        cy = clamp(cy, 1, state.height)
        
        state.grenade_box.active = true
        state.grenade_box.pos = Position(cx, cy)
        add_game_log!(state, "[TIMER] Luu dan da nam tai o ($cx, $cy)...")
    end

    state.input_mode = MoveMode
end

# --- 8. Interactive UI & Layout Redesign ---
function launch_game()
    state = init_game()
    state_obs = Observable(state)

    fig = Figure(size = (1400, 900), backgroundcolor = :gray15)
    
    left_layout = fig[1, 1] = GridLayout()
    right_layout = fig[1, 2] = GridLayout()
    
    colsize!(fig.layout, 1, Relative(0.70))
    colsize!(fig.layout, 2, Fixed(380))

    # --- 8.1 KHU VỰC ĐIỀU KHIỂN PHÍA TRÊN BÀN CỜ (TOP CONTROL) ---
    top_bar = left_layout[1, 1] = GridLayout()
    rowsize!(left_layout, 1, Fixed(90))

    btn_end_turn = Button(top_bar[1, 1], label = "END TURN", fontsize = 18, width = 130, height = 45, buttoncolor = :tomato, labelcolor = :white, font = :bold)
    
    dyn_info = lift(s -> " [LUOT]: $(s.current_turn)/$(s.turn_limit)   |   [STAMINA]: $(s.human.stamina)   |   [MAU - HP]: $(s.human.hp)", state_obs)
    Label(top_bar[1, 2], dyn_info, fontsize = 18, halign = :left, font = :bold, color = :whitesmoke)
    
    dyn_status = lift(s -> s.input_mode == MoveMode ? "[MODE] DI CHUYEN MAC DINH" : "[!] CHE DO VU KHI: CHON MUC TIEU TREN BAN CO!", state_obs)
    dyn_status_color = lift(s -> s.input_mode == MoveMode ? :springgreen : :orangered, state_obs)
    Label(top_bar[1, 3], dyn_status, fontsize = 15, color = dyn_status_color, font = :bold, halign = :right)

    # --- 8.2 THANH VŨ KHÍ (TOOLBAR) ---
    weapon_bar = left_layout[2, 1] = GridLayout()
    rowsize!(left_layout, 2, Fixed(50))

    btn_knife = Button(weapon_bar[1, 1], label = "Dao (oo)", fontsize = 14, height = 35)
    btn_pistol = Button(weapon_bar[1, 2], label = lift(s -> "Sung Ngan ($(s.human.pistol_ammo))", state_obs), fontsize = 14, height = 35)
    btn_shotgun = Button(weapon_bar[1, 3], label = lift(s -> "Shotgun ($(s.human.shotgun_ammo))", state_obs), fontsize = 14, height = 35)
    btn_grenade = Button(weapon_bar[1, 4], label = lift(s -> "Luu Dan ($(s.human.grenades))", state_obs), fontsize = 14, height = 35)
    btn_mine = Button(weapon_bar[1, 5], label = lift(s -> "Dat Min ($(s.human.mines))", state_obs), fontsize = 14, height = 35, buttoncolor = :gold4, labelcolor = :white)

    # --- 8.3 BÀN CỜ CHIẾN THUẬT (MAIN BOARD) ---
    ax = Axis(left_layout[3, 1], aspect = DataAspect(), backgroundcolor = :black)
    left_layout[3, 1] = ax
    hidespines!(ax)
    hidedecorations!(ax)
    
    xlims!(ax, 0.0, state.width + 0.5)
    ylims!(ax, 0.0, state.height + 0.5)

    for x in 1:state.width
        for y in 1:state.height
            color = state.grid[x, y] == Wall ? :gray35 : 
                    state.grid[x, y] == Obstacle ? :gray20 : 
                    state.grid[x, y] == Water ? :deepskyblue4 : :sienna
            poly!(ax, Rect(x-0.5, y-0.5, 1, 1), color=color, strokewidth=1.5, strokecolor=:gray10)
        end
    end

    human_pos = lift(s -> Point2f(s.human.pos.x, s.human.pos.y), state_obs)
    get_z_color(z) = z.hp <= 0 ? :black : z isa FastZombie ? :yellowgreen : z isa ExplodingZombie ? :darkorange : z isa VampireZombie ? :darkmagenta : :darkgreen

    zombie_pos = lift(s -> isempty(s.zombies) ? [Point2f(-10, -10)] : [Point2f(z.pos.x, z.pos.y) for z in s.zombies], state_obs)
    zombie_colors = lift(s -> isempty(s.zombies) ? [:black] : [get_z_color(z) for z in s.zombies], state_obs)
    
    mine_positions = lift(s -> let pts = [Point2f(x, y) for x in 1:s.width, y in 1:s.height if s.mine_grid[x, y]]
        isempty(pts) ? [Point2f(-10, -10)] : pts
    end, state_obs)

    grenade_pos = lift(s -> s.grenade_box.active ? [Point2f(s.grenade_box.pos.x, s.grenade_box.pos.y)] : [Point2f(-100, -100)], state_obs)

    scatter!(ax, mine_positions, color=:red, markersize=14, marker=:circle)
    scatter!(ax, grenade_pos, color=:lawngreen, markersize=22, marker=:utriangle) 
    scatter!(ax, zombie_pos, color=zombie_colors, markersize=34, marker=:rect)
    scatter!(ax, human_pos, color=:white, markersize=38, marker=:star5)

    zombie_labels = lift(s -> isempty(s.zombies) ? [""] : [z.hp <= 0 ? "X" : "#$i" for (i, z) in enumerate(s.zombies)], state_obs)
    text!(ax, zombie_pos, text = zombie_labels, color = :whitesmoke, fontsize = 14, font = :bold, align = (:center, :center))

    # --- 8.4 SIDEBAR HỆ THỐNG (DANH SÁCH + NHẬT KÝ KHUNG PHẢI) ---
    # ---- TAB 1: DANH SÁCH ZOMBIE ----
    Box(right_layout[1, 1], color = :gray25, strokewidth = 1, strokecolor = :gray40, cornerradius = 6)
    z_layout = right_layout[1, 1] = GridLayout(padding = (15, 15, 15, 15))
    
    Label(z_layout[1, 1:3], "=== DANH SACH KE DICH ===", fontsize = 16, font = :bold, color = :gold, halign = :center)
    Label(z_layout[2, 1], "STT", font = :bold, color = :lightgray, halign = :left)
    Label(z_layout[2, 2], "Chung loai", font = :bold, color = :lightgray, halign = :left)
    Label(z_layout[2, 3], "Mau (HP)", font = :bold, color = :lightgray, halign = :center)

    num_zom = length(state.zombies)
    for i in 1:num_zom
        Label(z_layout[2 + i, 1], " #$i", fontsize = 14, color = :whitesmoke, halign = :left)
        Label(z_layout[2 + i, 2], lift(s -> s.zombies[i].name, state_obs), fontsize = 14, color = :whitesmoke, halign = :left)
        z_hp_text = lift(s -> s.zombies[i].hp <= 0 ? "[X] TIEU DIET" : "$(s.zombies[i].hp) HP", state_obs)
        z_hp_color = lift(s -> s.zombies[i].hp <= 0 ? :crimson : :springgreen, state_obs)
        Label(z_layout[2 + i, 3], z_hp_text, fontsize = 14, halign = :center, color = z_hp_color, font = :bold)
    end

    # ---- TAB 2: KHU VỰC NHẬT KÝ ĐỒ HỌA SỬ DỤNG SCROLLABLE TEXT ---
    Box(right_layout[2, 1], color = :black, strokewidth = 2, strokecolor = :firebrick, cornerradius = 6)
    log_layout = right_layout[2, 1] = GridLayout(padding = (10, 10, 10, 10))
    
    Label(log_layout[1, 1], "--- NHAT KY CHIEN TRUONG ---", fontsize = 15, font = :bold, color = :cyan, halign = :center)
    
    # FIXED: Đã chỉnh sửa chính xác tên thuộc tính boxcolor_focused và boxcolor_hover theo chuẩn Makie.Textbox
    log_area = Textbox(log_layout[2, 1], 
                       placeholder = "Cho hanh dong...",
                       fontsize = 13,
                       textcolor = :whitesmoke,
                       boxcolor = :gray10,
                       boxcolor_focused = :gray10,
                       boxcolor_hover = :gray12,
                       bordercolor_focused = :gray10,
                       focused = false, # Tắt chế độ gõ phím để làm hộp log thuần túy cuộn chuột
                       displayed_string = join(state.logs, "\n"),
                       width = 340, 
                       height = 420)
    
    log_area.stored_string[] = join(state.logs, "\n")

    rowsize!(right_layout, 1, Relative(0.40))
    rowsize!(right_layout, 2, Relative(0.60))

    function refresh_log_box!(curr_state)
        log_area.displayed_string[] = join(curr_state.logs, "\n")
    end

    # --- 9. INTERACTIVE LISTENERS ---
    on(btn_knife.clicks) do _; state_obs[].input_mode = TargetKnife; notify(state_obs); end
    on(btn_pistol.clicks) do _; state_obs[].input_mode = TargetPistol; notify(state_obs); end
    on(btn_shotgun.clicks) do _; state_obs[].input_mode = TargetShotgun; notify(state_obs); end
    on(btn_grenade.clicks) do _; state_obs[].input_mode = TargetGrenade; notify(state_obs); end
    
    on(btn_mine.clicks) do _
        curr_state = state_obs[]
        if curr_state.human.stamina >= 1 && curr_state.human.mines > 0
            hx, hy = curr_state.human.pos.x, curr_state.human.pos.y
            curr_state.mine_grid[hx, hy] = true
            curr_state.human.mines -= 1
            curr_state.human.stamina -= 1
            add_game_log!(curr_state, "[MIN] Cai min thanh cong ngay duoi chan o ($hx, $hy)!")
            refresh_log_box!(curr_state)
            notify(state_obs)
        end
    end

    on(events(fig).mousebutton) do event
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
                            cost = current_state.grid[target_x, target_y] == Water ? 2 : 1
                            if current_state.human.stamina >= cost
                                current_state.human.pos.x = target_x
                                current_state.human.pos.y = target_y
                                current_state.human.stamina -= cost
                                add_game_log!(current_state, "Human di chuyen sang o ($target_x, $target_y).")
                                refresh_log_box!(current_state)
                                notify(state_obs)
                            end
                        end
                    end
                else
                    handle_weapon_click!(current_state, target_x, target_y)
                    refresh_log_box!(current_state)
                    notify(state_obs)
                end
            end
        end
    end

    on(btn_end_turn.clicks) do _
        current_state = state_obs[]
        zombie_turn!(current_state)
        
        current_state.current_turn += 1
        current_state.human.stamina = rand(0:6)
        current_state.input_mode = MoveMode
        add_game_log!(current_state, "--- LUOT MOI: VONG $(current_state.current_turn) (The luc: +$(current_state.human.stamina)) ---")

        if current_state.grenade_box.active
            gx = current_state.grenade_box.pos.x
            gy = current_state.grenade_box.pos.y
            add_game_log!(current_state, "[!] Luu dan tai ($gx, $gy) NO TUNG!")
            trigger_explosion!(current_state, gx, gy)
            current_state.grenade_box.active = false
        end

        refresh_log_box!(current_state)
        notify(state_obs)
    end

    display(fig)
    return fig
end

end # module
