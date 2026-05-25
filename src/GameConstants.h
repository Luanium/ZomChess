#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

namespace GameConstants {

    // =========================================================================
    // SECTION 1: MAP GENERATION & BOUNDARIES
    // =========================================================================
    namespace MapGen {
        constexpr float CLUSTER_PROB_WATER = 0.25f;  // Organic water hazard clustering probability
        constexpr float CLUSTER_PROB_FOREST = 0.25f; // Organic forest wood clustering probability
        constexpr float CLUSTER_PROB_WALL = 0.15f;   // Wall obstacle clustering probability

        // Spawn shielding parameters
        constexpr int SPAWN_SHIELD_RADIUS = 2;       // 5x5 shield around human spawn location

        // Map dimension limits (used by menu sliders)
        constexpr int MAP_WIDTH_MIN  = 5;
        constexpr int MAP_WIDTH_MAX  = 50;
        constexpr int MAP_HEIGHT_MIN = 5;
        constexpr int MAP_HEIGHT_MAX = 50;
    }

    // =========================================================================
    // SECTION 2: HUMAN & DIFFICULTY CONFIGURATIONS
    // =========================================================================
    namespace Difficulty {
        // Preset Level 0: Easy
        namespace Easy {
            constexpr int HUMAN_HP = 10;
            constexpr int INITIAL_STAMINA = 6;
            constexpr int PISTOL_AMMO = 20;
            constexpr int SHOTGUN_AMMO = 10;
            constexpr int GRENADES = 5;
            constexpr int MINES = 4;
            constexpr int MOLOTOVS = 4;
            constexpr int COUNT_NORMAL = 5;
            constexpr int COUNT_FAST = 2;
            constexpr int COUNT_EXPLODING = 1;
            constexpr int COUNT_VAMPIRE = 1;
            constexpr int COUNT_SICK = 1;
            constexpr int TURN_LIMIT = 50;

            // Terrain ratios (sum = 100)
            constexpr int RATIO_DIRT    = 60;
            constexpr int RATIO_WALL    = 8;
            constexpr int RATIO_WATER   = 10;
            constexpr int RATIO_FOREST  = 18;
            constexpr int RATIO_ICE     = 4;

            // Weather probabilities (sum = 100)
            constexpr int ENV_PROB_CLEAR     = 60;
            constexpr int ENV_PROB_WIND      = 14;
            constexpr int ENV_PROB_RAIN      = 12;
            constexpr int ENV_PROB_CLOUDS    = 4;
            constexpr int ENV_PROB_LIGHTNING = 4;
            constexpr int ENV_PROB_HEATWAVE  = 3;
            constexpr int ENV_PROB_BLIZZARD  = 3;
        }

        // Preset Level 1: Medium (Default)
        namespace Medium {
            constexpr int HUMAN_HP = 7;
            constexpr int INITIAL_STAMINA = 5;
            constexpr int PISTOL_AMMO = 15;
            constexpr int SHOTGUN_AMMO = 8;
            constexpr int GRENADES = 4;
            constexpr int MINES = 3;
            constexpr int MOLOTOVS = 3;
            constexpr int COUNT_NORMAL = 7;
            constexpr int COUNT_FAST = 4;
            constexpr int COUNT_EXPLODING = 3;
            constexpr int COUNT_VAMPIRE = 2;
            constexpr int COUNT_SICK = 2;
            constexpr int TURN_LIMIT = 50;

            // Terrain ratios (sum = 100)
            constexpr int RATIO_DIRT    = 52;
            constexpr int RATIO_WALL    = 10;
            constexpr int RATIO_WATER   = 12;
            constexpr int RATIO_FOREST  = 18;
            constexpr int RATIO_ICE     = 8;

            // Weather probabilities (sum = 100)
            constexpr int ENV_PROB_CLEAR     = 50;
            constexpr int ENV_PROB_WIND      = 14;
            constexpr int ENV_PROB_RAIN      = 12;
            constexpr int ENV_PROB_CLOUDS    = 4;
            constexpr int ENV_PROB_LIGHTNING = 8;
            constexpr int ENV_PROB_HEATWAVE  = 6;
            constexpr int ENV_PROB_BLIZZARD  = 6;
        }

        // Preset Level 2: Hard
        namespace Hard {
            constexpr int HUMAN_HP = 3;
            constexpr int INITIAL_STAMINA = 4;
            constexpr int PISTOL_AMMO = 10;
            constexpr int SHOTGUN_AMMO = 6;
            constexpr int GRENADES = 3;
            constexpr int MINES = 2;
            constexpr int MOLOTOVS = 3;
            constexpr int COUNT_NORMAL = 9;
            constexpr int COUNT_FAST = 4;
            constexpr int COUNT_EXPLODING = 4;
            constexpr int COUNT_VAMPIRE = 3;
            constexpr int COUNT_SICK = 2;
            constexpr int TURN_LIMIT = 50;

            // Terrain ratios (sum = 100)
            constexpr int RATIO_DIRT    = 42;
            constexpr int RATIO_WALL    = 13;
            constexpr int RATIO_WATER   = 14;
            constexpr int RATIO_FOREST  = 16;
            constexpr int RATIO_ICE     = 15;

            // Weather probabilities (sum = 100)
            constexpr int ENV_PROB_CLEAR     = 38;
            constexpr int ENV_PROB_WIND      = 14;
            constexpr int ENV_PROB_RAIN      = 12;
            constexpr int ENV_PROB_CLOUDS    = 6;
            constexpr int ENV_PROB_LIGHTNING = 12;
            constexpr int ENV_PROB_HEATWAVE  = 9;
            constexpr int ENV_PROB_BLIZZARD  = 9;
        }

        // Preset Level 3: Unfair
        namespace Unfair {
            constexpr int HUMAN_HP = 1;
            constexpr int INITIAL_STAMINA = 6;
            constexpr int PISTOL_AMMO = 4;
            constexpr int SHOTGUN_AMMO = 3;
            constexpr int GRENADES = 1;
            constexpr int MINES = 1;
            constexpr int MOLOTOVS = 1;
            constexpr int COUNT_NORMAL = 8;
            constexpr int COUNT_FAST = 4;
            constexpr int COUNT_EXPLODING = 6;
            constexpr int COUNT_VAMPIRE = 3;
            constexpr int COUNT_SICK = 3;
            constexpr int TURN_LIMIT = 50;

            // Terrain ratios (sum = 100)
            constexpr int RATIO_DIRT    = 32;
            constexpr int RATIO_WALL    = 15;
            constexpr int RATIO_WATER   = 16;
            constexpr int RATIO_FOREST  = 14;
            constexpr int RATIO_ICE     = 23;

            // Weather probabilities (sum = 100)
            constexpr int ENV_PROB_CLEAR     = 22;
            constexpr int ENV_PROB_WIND      = 14;
            constexpr int ENV_PROB_RAIN      = 12;
            constexpr int ENV_PROB_CLOUDS    = 6;
            constexpr int ENV_PROB_LIGHTNING = 16;
            constexpr int ENV_PROB_HEATWAVE  = 15;
            constexpr int ENV_PROB_BLIZZARD  = 15;
        }

        // Slider bounds for custom configuration in the menu
        namespace SliderBounds {
            constexpr int HUMAN_HP_MIN       = 1;
            constexpr int HUMAN_HP_MAX       = 20;
            constexpr int INITIAL_STAMINA_MIN = 1;
            constexpr int INITIAL_STAMINA_MAX = 6;
            constexpr int PISTOL_AMMO_MIN    = 0;
            constexpr int PISTOL_AMMO_MAX    = 50;
            constexpr int SHOTGUN_AMMO_MIN   = 0;
            constexpr int SHOTGUN_AMMO_MAX   = 50;
            constexpr int GRENADES_MIN       = 0;
            constexpr int GRENADES_MAX       = 20;
            constexpr int MINES_MIN          = 0;
            constexpr int MINES_MAX          = 20;
            constexpr int MOLOTOVS_MIN       = 0;
            constexpr int MOLOTOVS_MAX       = 20;
            constexpr int TURN_LIMIT_MIN     = 5;
            constexpr int TURN_LIMIT_MAX     = 200;
            constexpr int COUNT_NORMAL_MAX   = 50;
            constexpr int COUNT_FAST_MAX     = 50;
            constexpr int COUNT_EXPLODING_MAX = 50;
            constexpr int COUNT_VAMPIRE_MAX  = 50;
            constexpr int COUNT_SICK_MAX     = 50;
        }
    }

    // =========================================================================
    // SECTION 3: WEAPONS & COMBAT LOGIC
    // =========================================================================
    namespace Weapons {
        // Knife / Blade parameters
        constexpr int KNIFE_STAMINA_COST = 1;
        constexpr int KNIFE_DAMAGE = 1;
        constexpr int KNIFE_RANGE = 1;

        // Pistol parameters
        constexpr int PISTOL_STAMINA_COST = 1;
        constexpr int PISTOL_DAMAGE = 1;
        constexpr int PISTOL_RANGE = 20;
        constexpr double PISTOL_ACCURACY_LAMBDA = 4.3210;

        // Shotgun parameters
        constexpr int SHOTGUN_STAMINA_COST = 1;
        constexpr int SHOTGUN_DAMAGE = 1;
        constexpr int SHOTGUN_RANGE = 3; // straightforward step range
        constexpr int SHOTGUN_WALL_IMPACT_DAMAGE = 1; // self damage when blasting wall adjacent to you

        // Grenade parameters
        constexpr int GRENADE_STAMINA_COST = 1;
        constexpr int GRENADE_FUSE_TURNS = 1;
        constexpr int GRENADE_BLAST_RADIUS = 2;
        constexpr int GRENADE_CENTER_DAMAGE = 3;
        constexpr int GRENADE_MID_DAMAGE = 2;
        constexpr int GRENADE_OUTER_DAMAGE = 1;
        constexpr int GRENADE_MIN_STEPS = 1;
        constexpr int GRENADE_MAX_STEPS = 6;

        // Molotov parameters
        constexpr int MOLOTOV_STAMINA_COST = 1;
        constexpr int MOLOTOV_FIRE_DURATION = 2;
        constexpr int MOLOTOV_MIN_STEPS = 1;
        constexpr int MOLOTOV_MAX_STEPS = 6;

        // Claymore Mine parameters
        constexpr int MINE_EXPLOSION_RADIUS = 1;
        constexpr int MINE_DAMAGE = 2;

        // Ice Pick parameters
        constexpr int ICE_PICK_STAMINA_COST_NORMAL = 1;  // Stamina cost when not frozen
        constexpr int ICE_PICK_STAMINA_COST_FROZEN = 2;  // Stamina cost when frozen

        // Explosion damage tiers (center, mid ring, outer ring)
        constexpr int EXPLOSION_CENTER_DAMAGE = 3;
        constexpr int EXPLOSION_MID_DAMAGE    = 2;
        constexpr int EXPLOSION_OUTER_DAMAGE  = 1;
        constexpr int EXPLOSION_IMPACT_DAMAGE = 1; // extra damage when blast pushes entity into obstacle

        // Grenade blast radius when thrown by human vs zombie exploder
        constexpr int GRENADE_HUMAN_RADIUS   = 2;
        constexpr int GRENADE_ZOMBIE_RADIUS  = 1;
    }

    // =========================================================================
    // SECTION 4: ZOMBIE ARCHETYPES & AI STATS
    // =========================================================================
    namespace Zombies {
        // Base health for different classes (when spawned organically)
        constexpr int BASE_HP_NORMAL = 2;
        constexpr int BASE_HP_FAST = 2;
        constexpr int BASE_HP_EXPLODING = 3;
        constexpr int BASE_HP_VAMPIRE = 4;
        constexpr int BASE_HP_SICK = 2;

        // Pathfinding AI weights
        constexpr double AI_LAMBDA = 1.2;
        constexpr double AI_AWAY_WEIGHT = 0.05;
        constexpr double AI_PERPENDICULAR_WEIGHT = 0.3;

        // Movement
        constexpr int MOVES_PER_TURN_NORMAL = 1;
        constexpr int MOVES_PER_TURN_FAST = 2;

        // Combat actions
        constexpr int BITE_DAMAGE = 2;
        constexpr int BITE_WATER_DAMAGE = 1;
        constexpr int SCRATCH_DAMAGE = 1;
        constexpr int SCRATCH_WATER_DAMAGE = 0;

        // Vampire passive heal
        constexpr int VAMPIRE_HEAL_ON_HIT = 1;

        // Exploding Zombie explosion parameters
        constexpr int EXPLODER_RADIUS = 1;

        // Frozen turns when hit by blizzard
        constexpr int FROZEN_TURNS_NORMAL  = 2; // Normal/Exploding/Vampire/Sick
        constexpr int FROZEN_TURNS_FAST    = 1; // Fast sprinters thaw quicker

        // Collision damage (zombie pushed into another entity)
        constexpr int COLLISION_DAMAGE = 1;
    }

    // =========================================================================
    // SECTION 5: WATER MOVEMENT & TERRAIN PENALTIES
    // =========================================================================
    namespace TerrainPenalties {
        constexpr int WATER_MOVES_MAX_SPRINTER = 1;  // sprinters max moves per turn capped to 1 in water
        constexpr int WATER_MOVEMENT_COST = 2;       // stamina cost to move into water
        constexpr int FREEZE_BREAK_STAMINA_COST = 2; // stamina cost for human to break free from ice freeze
    }

    // =========================================================================
    // SECTION 5b: ICE TERRAIN
    // =========================================================================
    namespace Ice {
        // Number of consecutive ice cells required to trigger a slide
        constexpr int SLIDE_TRIGGER_LENGTH = 2;
        // Probability of sliding when entering ice with >= SLIDE_TRIGGER_LENGTH consecutive ice ahead
        constexpr float SLIDE_CHANCE = 0.5f;
        // Probability multiplier per extra step beyond the minimum slide length
        constexpr float SLIDE_DECAY = 0.8f;
        // Duration of each visual step in the ice slide animation (seconds)
        constexpr float SLIDE_STEP_DURATION = 0.1f;
        // Duration of terrain transition animation (seconds)
        constexpr float TERRAIN_TRANSITION_DURATION = 0.8f;
    }

    // =========================================================================
    // SECTION 6: ENVIRONMENTAL STIMULI & EVENTS
    // =========================================================================
    namespace Environment {
        // Flood transition chances
        constexpr float RAIN_FLOOD_THRESHOLD_WATER_ADJACENT = 0.30f;
        constexpr float RAIN_FLOOD_THRESHOLD_ISOLATED = 0.10f;
        // Rain: forest spread to adjacent dirt
        constexpr float RAIN_FOREST_SPREAD_CHANCE = 0.12f;

        // Lightning strikes
        constexpr double LIGHTNING_WEIGHT_DEFAULT = 1.0;
        constexpr double LIGHTNING_WEIGHT_WATER = 4.0;
        constexpr double LIGHTNING_WEIGHT_ENTITY = 2.0;
        constexpr int LIGHTNING_HP_DAMAGE = 1;

        // Heatwave (nắng nóng gay gắt)
        // Chance a water cell evaporates (base, before neighbor penalty)
        constexpr float HEATWAVE_WATER_EVAPORATE_BASE = 1.0f; // 100% if isolated
        // Chance a forest cell dies (drought)
        constexpr float HEATWAVE_FOREST_DROUGHT_CHANCE = 0.15f;

        // Blizzard (băng giá)
        // Chance a water cell freezes (triggers connected-component freeze)
        constexpr float BLIZZARD_FREEZE_CHANCE = 0.25f;
    }

    // =========================================================================
    // SECTION 7: GAMEPLAY TIMING & DELAYS
    // =========================================================================
    namespace Timing {
        // Zombie phase timing
        constexpr float ZOMBIE_STEP_DELAY = 0.35f;  // Delay between zombie steps

        // Environment phase timing
        constexpr float ENVIRONMENT_STEP_DELAY = 0.9f;  // Delay between environment events

        // FX durations
        constexpr float FX_TURN_BANNER_DURATION = 1.5f;
        constexpr float FX_ACTIVE_DURATION = 0.35f;
        constexpr float FX_EXPLOSION_DURATION = 2.0f;
        constexpr float FX_KNIFE_DURATION = 0.35f;
        constexpr float FX_PISTOL_DURATION = 0.45f;
        constexpr float FX_SHOTGUN_DURATION = 0.65f;
        constexpr float FX_GRENADE_FLY_DURATION = 0.5f;
        constexpr float FX_MOLOTOV_DURATION = 0.5f;
        constexpr float FX_BITE_DURATION = 0.5f;
        constexpr float FX_LIGHTNING_DURATION = 3.6f;
        constexpr float FX_RAIN_DURATION = 3.2f;
        constexpr float FX_WIND_DURATION = 3.2f;
        constexpr float FX_DARK_CLOUD_DURATION = 1.6f;
        constexpr float FX_HEATWAVE_DURATION = 3.2f;
        constexpr float FX_BLIZZARD_DURATION = 3.2f;
    }

    // =========================================================================
    // SECTION 8: GAME CONFIGURATION DEFAULTS
    // =========================================================================
    namespace Defaults {
        // Map configuration
        constexpr int MAP_WIDTH = 15;
        constexpr int MAP_HEIGHT = 15;
        constexpr int VIEW_CELLS = 15;  // Number of cells visible in the viewport

        // Terrain ratios (sum = 100%) — used as fallback when config is invalid
        constexpr int RATIO_WALL   = 10;
        constexpr int RATIO_WATER  = 10;
        constexpr int RATIO_FOREST = 15;
        constexpr int RATIO_DIRT   = 60;
        constexpr int RATIO_ICE    = 5;

        // GameConfig default terrain ratios (initial values in Types.h)
        constexpr int CONFIG_RATIO_DIRT   = 55;
        constexpr int CONFIG_RATIO_WALL   = 10;
        constexpr int CONFIG_RATIO_WATER  = 10;
        constexpr int CONFIG_RATIO_FOREST = 15;
        constexpr int CONFIG_RATIO_ICE    = 10;

        // Turn limit
        constexpr int TURN_LIMIT = 20;

        // Human spawn position (default for custom maps)
        constexpr int CUSTOM_HUMAN_X = 8;
        constexpr int CUSTOM_HUMAN_Y = 8;

        // Environment probabilities (sum = 100%) — used as fallback when config is invalid
        constexpr int ENV_PROB_CLEAR     = 50;
        constexpr int ENV_PROB_WIND      = 14;
        constexpr int ENV_PROB_RAIN      = 12;
        constexpr int ENV_PROB_CLOUDS    = 4;
        constexpr int ENV_PROB_LIGHTNING = 8;
        constexpr int ENV_PROB_HEATWAVE  = 6;
        constexpr int ENV_PROB_BLIZZARD  = 6;

        // Human default stats (fallback when not set by difficulty)
        constexpr int HUMAN_DEFAULT_HP           = 6;
        constexpr int HUMAN_DEFAULT_STAMINA      = 6;
        constexpr int HUMAN_DEFAULT_PISTOL_AMMO  = 15;
        constexpr int HUMAN_DEFAULT_SHOTGUN_AMMO = 8;
        constexpr int HUMAN_DEFAULT_GRENADES     = 5;
        constexpr int HUMAN_DEFAULT_MINES        = 5;
        constexpr int HUMAN_DEFAULT_MOLOTOVS     = 5;

        // Fire duration (in turns)
        constexpr int FIRE_DURATION = 2;

        // Stamina regen range (rolled each turn)
        constexpr int STAMINA_REGEN_MIN = 1;
        constexpr int STAMINA_REGEN_MAX = 6;

        // Loot system
        constexpr int LOOT_HP_POTION_AMOUNT      = 2;   // HP restored by health potion
        constexpr int LOOT_HP_POTION_CAP         = 20;  // Maximum HP cap when picking up health potion
        constexpr int LOOT_STAMINA_RESTORE       = 6;   // Stamina restored by stamina potion
        constexpr int LOOT_PISTOL_AMMO_AMOUNT    = 3;   // Pistol ammo gained from loot
        constexpr int LOOT_SHOTGUN_AMMO_AMOUNT   = 1;   // Shotgun ammo gained from loot

        // Loot spawn probabilities (out of 100)
        constexpr int LOOT_PROB_JUNK         = 75; // 0..74
        constexpr int LOOT_PROB_PISTOL_AMMO  = 80; // 75..79
        constexpr int LOOT_PROB_STAMINA_POT  = 85; // 80..84
        constexpr int LOOT_PROB_HEALTH_POT   = 89; // 85..88
        constexpr int LOOT_PROB_SHOTGUN_AMMO = 93; // 89..92
        constexpr int LOOT_PROB_GRENADE      = 96; // 93..95
        constexpr int LOOT_PROB_MOLOTOV      = 98; // 96..97
        // Mine: 98..99 (remainder)
    }

    // =========================================================================
    // SECTION 9: LOGIC CONSTANTS
    // =========================================================================
    namespace Logic {
        // Distance calculation (Chebyshev distance for grid movement)
        constexpr int MAX_DISTANCE = 20;  // Maximum range for pistol

        // Grid bounds
        constexpr int MIN_COORD = 0;
        constexpr int MAX_MAP_SIZE = 25;  // Maximum map dimension

        // Turn phases
        constexpr int INITIAL_TURN = 1;

        // Human stamina threshold
        constexpr int STAMINA_THRESHOLD = 0;

        // Zombie collision damage
        constexpr int ZOMBIE_COLLISION_DAMAGE = 1;

        // Mine explosion parameters
        constexpr int MINE_EXPLOSION_RADIUS = 1;
        constexpr int MINE_DAMAGE = 2;

        // Explosion damage (center, mid, outer)
        constexpr int EXPLOSION_CENTER_DAMAGE = 3;
        constexpr int EXPLOSION_MID_DAMAGE = 2;
        constexpr int EXPLOSION_OUTER_DAMAGE = 1;

        // Shotgun recoil damage
        constexpr int SHOTGUN_RECOIL_DAMAGE = 1;

        // Water movement cost
        constexpr int WATER_MOVEMENT_COST = 2;

        // Human default stamina regen range
        constexpr int HUMAN_STAMINA_REGEN_MIN = 1;
        constexpr int HUMAN_STAMINA_REGEN_MAX = 6;

        // Human default projectile steps range
        constexpr int HUMAN_PROJECTILE_MIN_STEPS = 1;
        constexpr int HUMAN_PROJECTILE_MAX_STEPS = 6;

        // Human default fire damage
        constexpr int HUMAN_FIRE_DAMAGE = 1;

        // Human default recoil blocked damage
        constexpr int HUMAN_RECOIL_BLOCKED_DAMAGE = 1;

        // Human default explosion impact damage
        constexpr int HUMAN_EXPLOSION_IMPACT_DAMAGE = 1;

        // Human default zombie collision damage
        constexpr int HUMAN_ZOMBIE_COLLISION_DAMAGE = 1;

        // Human default lightning damage
        constexpr int HUMAN_LIGHTNING_DAMAGE = 1;

        // Human default sick zombie stamina penalty
        constexpr int HUMAN_SICK_ZOMBIA_STAMINA_PENALTY = 1;
    }
}

#endif // GAME_CONSTANTS_H
