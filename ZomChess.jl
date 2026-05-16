module ZomChessInteractive

using GLMakie # Replaced CairoMakie for desktop interactivity
using Random

export GameState, init_game, launch_game

# --- 1. Enums and Basic Types ---
@enum TerrainType Dirt Water Obstacle Wall

mutable struct Position
    x::Int
    y::Int
end
Base.:(==)(p1::Position, p2::Position) = p1.x == p2.x && p1.y == p2.y

# --- 2. Entities ---
abstract type AbstractEntity end
abstract type AbstractZombie <: AbstractEntity end

mutable struct Human <: AbstractEntity
    pos::Position
    hp::Int
    stamina::Int
end

mutable struct NormalZombie <: AbstractZombie
    pos::Position
    hp::Int
end

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
    # Build Walls
    grid[:, 1] .= Wall; grid[:, height] .= Wall
    grid[1, :] .= Wall; grid[width, :] .= Wall

    # Add random obstacles
    for _ in 1:10
        rx, ry = rand(2:width-1), rand(2:height-1)
        grid[rx, ry] = Obstacle
    end

    human = Human(Position(rand(2:width-1), rand(2:height-1)), 5, 6) # 6 initial stamina

    zombies = AbstractZombie[]
    for _ in 1:num_zombies
        push!(zombies, NormalZombie(Position(rand(2:width-1), rand(2:height-1)), 2))
    end

    return GameState(width, height, grid, human, zombies, 50, 1)
end

# --- 5. Interactive UI and Logic ---
function launch_game()
    state = init_game()

    # 1. Wrap the entire state in an Observable to track changes
    state_obs = Observable(state)

    fig = Figure(size = (800, 800))

    # Title that updates dynamically when state_obs changes
    dyn_title = lift(s -> "ZomChess - Turn: $(s.current_turn) | Stamina: $(s.human.stamina) | HP: $(s.human.hp)", state_obs)
    ax = Axis(fig[1, 1], aspect = DataAspect(), title = dyn_title)

    hidespines!(ax)
    hidedecorations!(ax)

    # 2. Draw Static Terrain (Doesn't need to be reactive)
    for x in 1:state.width
        for y in 1:state.height
            color = state.grid[x, y] == Wall ? :darkgray :
                state.grid[x, y] == Obstacle ? :dimgray : :saddlebrown
            poly!(ax, Rect(x-0.5, y-0.5, 1, 1), color=color, strokewidth=1, strokecolor=:black)
        end
    end

    # 3. Draw Reactive Entities
    # Extract positions dynamically whenever state_obs is notified
    human_pos = lift(s -> Point2f(s.human.pos.x, s.human.pos.y), state_obs)
    zombie_pos = lift(s -> [Point2f(z.pos.x, z.pos.y) for z in s.zombies], state_obs)

        scatter!(ax, zombie_pos, color=:forestgreen, markersize=30, marker=:rect)
        scatter!(ax, human_pos, color=:white, markersize=35, marker=:star5)

        # 4. Input Handling: Mouse Click on Axis
        on(events(fig).mousebutton) do event
            if event.button == Mouse.left && event.action == Mouse.press
                # Convert screen pixel coordinates to axis grid coordinates
                mouse_pos = mouseposition(ax.scene)
                target_x = round(Int, mouse_pos[1])
                target_y = round(Int, mouse_pos[2])

                current_state = state_obs[]
                h_pos = current_state.human.pos

                # Check if clicked inside bounds
                if 1 <= target_x <= current_state.width && 1 <= target_y <= current_state.height

                    # Check distance (Movement allows horizontal, vertical, and diagonal)
                    dx = abs(target_x - h_pos.x)
                    dy = abs(target_y - h_pos.y)

                    if dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)
                        # Check if tile is walkable
                        if current_state.grid[target_x, target_y] != Wall &&
                            current_state.grid[target_x, target_y] != Obstacle

                        if current_state.human.stamina > 0
                            # Move human
                            current_state.human.pos.x = target_x
                            current_state.human.pos.y = target_y
                            current_state.human.stamina -= 1

                            # Trigger UI update
                            notify(state_obs)
                        else
                            println("Out of stamina! End turn to continue.")
                        end
                        end
                    end
                end
            end
        end

        # 5. Keyboard Handling: Press 'Space' to end turn
        on(events(fig).keyboardbutton) do event
            if event.key == Keyboard.space && event.action == Keyboard.press
                current_state = state_obs[]

                # Reset stamina for next turn (using your 0-6 rule)
                current_state.human.stamina = rand(0:6)
                current_state.current_turn += 1

                # TODO: Add `zombie_turn!(current_state)` here before updating UI

                notify(state_obs)
            end
        end

        display(fig)
        return fig
    end

end # module
