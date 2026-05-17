module ZomChessInteractive

using GLMakie 
using Random

export GameState, init_game, launch_game

# --- 1. Enums and Basic Types ---
@enum TerrainType Dirt Water Obstacle Wall
# Thêm Enum để quản lý chế độ hành động hiện tại của người chơi
@enum ActionMode Move ModeKnife ModePistol ModeShotgun ModeGrenade ModeMine

mutable struct Position
    x::Int
    y::Int
end
Base.:(==)(p1::Position, p2::Position) = p1.x == p2.x && p1.y == p2.y

# --- 2. Entities ---
abstract type AbstractEntity end
abstract type AbstractZombie <: AbstractEntity end

# Bổ sung kho vũ khí/đạn dược cho Human theo tài liệu thiết kế 
mutable struct Human <: AbstractEntity
    pos::Position
    hp::Int
    stamina::Int
    pistol_ammo::Int
    shotgun_ammo::Int
    grenades::Int
    mines::Int
    current_mode::ActionMode # Chế độ hành động hiện tại
end

# Khai báo các loại Zombie và lượng máu tương ứng [cite: 18, 19]
mutable struct NormalZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct FastZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct ExplodingZombie <: AbstractZombie pos::Position; hp::Int; name::String end
mutable struct VampireZombie <: AbstractZombie pos::Position; hp::Int; name::String end

# --- 3. Game State ---
mutable struct GameState
    width::Int
    height::Int
    grid::Matrix{TerrainType}
    human::Human
    zombies::Vector{AbstractZombie}
    turn_limit::Int
    current_turn::Int
end

# --- 4. Initialization ---
function init_game(; width=15, height=15, num_zombies=5)
    grid = fill(Dirt, width, height)
    grid[:, 1] .= Wall; grid[:, height] .= Wall
    grid[1, :] .= Wall; grid[width, :] .= Wall

    # Thêm vật cản ngẫu nhiên [cite: 20]
    for _ in 1:10
        rx, ry = rand(2:width-1), rand(2:height-1)
        grid[rx, ry] = Obstacle
    end
    
    # Thêm hồ nước ngẫu nhiên [cite: 19, 20]
    for _ in 1:5
        rx, ry = rand(2:width-1), rand(2:height-1)
        if grid[rx, ry] == Dirt
            grid[rx, ry] = Water
        end
    end

    # Khởi tạo Human với thông số vũ khí và mặc định ở chế độ Di chuyển (Move) [cite: 8, 14, 15, 16, 17, 18]
    human = Human(Position(rand(2:width-1), rand(2:height-1)), 5, 6, 12, 6, 3, 2, Move) 
    
    zombies = AbstractZombie[]
    z_types = [
        (NormalZombie, 2, "Zom Thường"),     # [cite: 18]
        (FastZombie, 2, "Zom Nhanh"),       # [cite: 18]
        (ExplodingZombie, 3, "Zom Nổ"),     # [cite: 18]
        (VampireZombie, 4, "Zom Dracula")   # [cite: 19]
    ]
    
    for i in 1:num_zombies
        zt, hp, name = rand(z_types)
        zx, zy = rand(2:width-1), rand(2:height-1)
        while Position(zx, zy) == human.pos || grid[zx, zy] == Wall
            zx, zy = rand(2:width-1), rand(2:height-1)
        end
        push!(zombies, zt(Position(zx, zy), hp, name))
    end

    return GameState(width, height, grid, human, zombies, 50, 1)
end

# --- 5. Interactive UI and Logic ---
function launch_game()
    state = init_game()
    state_obs = Observable(state)

    # Nới rộng cửa sổ lên 1200 để chứa giao diện trực quan hơn
    fig = Figure(size = (1200, 900))
    
    # Chia Figure tổng: 
    # Hàng 1: Thanh trạng thái & Nút End Turn
    # Hàng 2: Bảng chọn vũ khí (Weapon Toolbar)
    # Hàng 3: Chia làm 2 cột (Bàn cờ | Bảng danh sách Zombie)
    fig[1, 1:2] = top_bar = GridLayout()
    fig[2, 1:2] = weapon_bar = GridLayout()
    
    colsize!(fig.layout, 2, Fixed(300)) 

    # ------------------------------------------------------------
    # HÀNG 1: THANH TRẠNG THÁI TỔNG QUAN
    # ------------------------------------------------------------
    btn_end_turn = Button(top_bar[1, 1], label = "End Turn", fontsize = 18, width = 120, buttoncolor = :tomato)
    
    dyn_info = lift(s -> "  Lượt: $(s.current_turn)/$(s.turn_limit)  |  Thể lực (Stamina): $(s.human.stamina)  |  Máu (HP): $(s.human.hp)", state_obs)
    Label(top_bar[1, 2], dyn_info, fontsize = 16, halign = :left, font = :bold)
    
    # Hiển thị chế độ đang chọn hành động
    dyn_mode = lift(s -> "CHẾ ĐỘ ĐANG CHỌN: " * uppercase(string(s.human.current_mode)), state_obs)
    Label(top_bar[1, 3], dyn_mode, fontsize = 16, halign = :right, color = :blue, font = :bold)

    # ------------------------------------------------------------
    # HÀNG 2: BẢNG CHỌN VŨ KHÍ (WEAPON SELECTOR TOOLBAR)
    # ------------------------------------------------------------
    btn_move = Button(weapon_bar[1, 1], label = "Di Chuyển", fontsize = 14)
    btn_knife = Button(weapon_bar[1, 2], label = "Dao (∞)", fontsize = 14)
    
    # Hiển thị số lượng đạn/vũ khí động bám sát theo kho đồ 
    lbl_pistol = lift(s -> "Súng Ngắn ($(s.human.pistol_ammo))", state_obs)
    btn_pistol = Button(weapon_bar[1, 3], label = lbl_pistol, fontsize = 14)
    
    lbl_shotgun = lift(s -> "Shotgun ($(s.human.shotgun_ammo))", state_obs)
    btn_shotgun = Button(weapon_bar[1, 4], label = lbl_shotgun, fontsize = 14)
    
    lbl_grenade = lift(s -> "Lựu Đạn ($(s.human.grenades))", state_obs)
    btn_grenade = Button(weapon_bar[1, 5], label = lbl_grenade, fontsize = 14)
    
    lbl_mine = lift(s -> "Mìn ($(s.human.mines))", state_obs)
    btn_mine = Button(weapon_bar[1, 6], label = lbl_mine, fontsize = 14)

    # ------------------------------------------------------------
    # HÀNG 3 - CỘT TRÁI: BÀN CỜ (AXIS)
    # ------------------------------------------------------------
    ax = Axis(fig[3, 1], aspect = DataAspect())
    hidespines!(ax)
    hidedecorations!(ax)

    # Vẽ Địa hình [cite: 19, 20]
    for x in 1:state.width
        for y in 1:state.height
            color = state.grid[x, y] == Wall ? :darkgray : 
                    state.grid[x, y] == Obstacle ? :dimgray : 
                    state.grid[x, y] == Water ? :cyan3 : :saddlebrown
            poly!(ax, Rect(x-0.5, y-0.5, 1, 1), color=color, strokewidth=1, strokecolor=:black)
        end
    end

    # Vẽ Thực thể màu sắc theo chủng loại
    human_pos = lift(s -> Point2f(s.human.pos.x, s.human.pos.y), state_obs)
    
    get_z_color(z) = z isa FastZombie ? :yellowgreen : 
                     z isa ExplodingZombie ? :orange : 
                     z isa VampireZombie ? :purple : :forestgreen

    zombie_pos = lift(s -> [Point2f(z.pos.x, z.pos.y) for z in s.zombies], state_obs)
    zombie_colors = lift(s -> [get_z_color(z) for z in s.zombies], state_obs)

    scatter!(ax, zombie_pos, color=zombie_colors, markersize=30, marker=:rect)
    scatter!(ax, human_pos, color=:white, markersize=35, marker=:star5)

    # ------------------------------------------------------------
    # HÀNG 3 - CỘT PHẢI: BẢNG THÔNG TIN ZOMBIE
    # ------------------------------------------------------------
    fig[3, 2] = sidebar = GridLayout()
    Box(sidebar[1, 1], color = (:black, 0.05), strokewidth = 1, strokecolor = :gray)
    v_layout = sidebar[1, 1] = GridLayout()
    
    Label(v_layout[1, 1:3], "DANH SÁCH ZOMBIE", fontsize = 18, font = :bold, halign = :center)
    Label(v_layout[2, 1], "STT", font = :bold, halign = :left)
    Label(v_layout[2, 2], "Chủng loại", font = :bold, halign = :left)
    Label(v_layout[2, 3], "Máu (HP)", font = :bold, halign = :center)

    num_zom = length(state.zombies)
    for i in 1:num_zom
        Label(v_layout[2 + i, 1], "#$i", fontsize = 15, halign = :left)
        
        z_name = lift(s -> s.zombies[i].name, state_obs)
        Label(v_layout[2 + i, 2], z_name, fontsize = 15, halign = :left)
        
        z_hp = lift(s -> "$(s.zombies[i].hp) HP", state_obs)
        Label(v_layout[2 + i, 3], z_hp, fontsize = 15, halign = :center, color = :red)
    end
    
    # GIẢI QUYẾT TRIỆT ĐỂ LỖI MA TRẬN LƯỚI: 
    # Bước 1: Đặt một Label rỗng vào hàng tiếp theo để Makie chính thức tạo ra hàng đó.
    last_row_idx = 2 + num_zom + 1
    Label(v_layout[last_row_idx, 1], "") 
    
    # Bước 2: Giờ hàng đã tồn tại hợp lệ, ta có thể đặt kích thước co giãn an toàn!
    rowsize!(v_layout, last_row_idx, Relative(1))

    # ------------------------------------------------------------
    # SỰ KIỆN CLICK NÚT TRÊN WEAPON TOOLBAR
    # ------------------------------------------------------------
    on(btn_move.clicks) do _; state_obs[].human.current_mode = Move; notify(state_obs); end
    on(btn_knife.clicks) do _; state_obs[].human.current_mode = ModeKnife; notify(state_obs); end
    on(btn_pistol.clicks) do _; state_obs[].human.current_mode = ModePistol; notify(state_obs); end
    on(btn_shotgun.clicks) do _; state_obs[].human.current_mode = ModeShotgun; notify(state_obs); end
    on(btn_grenade.clicks) do _; state_obs[].human.current_mode = ModeGrenade; notify(state_obs); end
    on(btn_mine.clicks) do _; state_obs[].human.current_mode = ModeMine; notify(state_obs); end

    # ------------------------------------------------------------
    # XỬ LÝ SỰ KIỆN CLICK CHUỘT TRÊN BÀN CỜ
    # ------------------------------------------------------------
    on(events(fig).mousebutton) do event
        if event.button == Mouse.left && event.action == Mouse.press
            mouse_pos = mouseposition(ax.scene)
            target_x = round(Int, mouse_pos[1])
            target_y = round(Int, mouse_pos[2])

            current_state = state_obs[]
            h_pos = current_state.human.pos

            if 1 <= target_x <= current_state.width && 1 <= target_y <= current_state.height
                
                # --- CHẾ ĐỘ 1: DI CHUYỂN ---
                if current_state.human.current_mode == Move
                    dx = abs(target_x - h_pos.x)
                    dy = abs(target_y - h_pos.y)

                    if dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)
                        if current_state.grid[target_x, target_y] != Wall && 
                           current_state.grid[target_x, target_y] != Obstacle
                            
                            cost = current_state.grid[target_x, target_y] == Water ? 2 : 1 # [cite: 19]
                            
                            if current_state.human.stamina >= cost
                                current_state.human.pos.x = target_x
                                current_state.human.pos.y = target_y
                                current_state.human.stamina -= cost
                                notify(state_obs)
                            else
                                println("Không đủ stamina để di chuyển!")
                            end
                        end
                    end

                # --- CHẾ ĐỘ 2: SỬ DỤNG VŨ KHÍ (MẪU ĐỂ TEST LOGIC) ---
                else
                    mode = current_state.human.current_mode
                    println("Khai hoả vũ khí ở chế độ: $mode tại ô ($target_x, $target_y)")
                    
                    if current_state.human.stamina >= 1
                        # Trừ tạm thời 1 stamina và cập nhật trạng thái vũ khí mẫu 
                        current_state.human.stamina -= 1
                        
                        if mode == ModePistol && current_state.human.pistol_ammo > 0
                            current_state.human.pistol_ammo -= 1 # [cite: 14]
                        elseif mode == ModeShotgun && current_state.human.shotgun_ammo > 0
                            current_state.human.shotgun_ammo -= 1 # [cite: 15]
                        end
                        
                        # Đoạn mã test: giảm thử máu Zombie khi ta click tấn công
                        if !isempty(current_state.zombies) && current_state.zombies[1].hp > 0
                            current_state.zombies[1].hp -= 1
                        end
                        
                        notify(state_obs)
                    else
                        println("Không đủ stamina để tấn công!")
                    end
                end
            end
        end
    end

    on(btn_end_turn.clicks) do _
        current_state = state_obs[]
        current_state.human.stamina = rand(0:6) # [cite: 8]
        current_state.current_turn += 1
        notify(state_obs)
    end

    display(fig)
    return fig
end

end # module
