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

        // Molotov parameters
        constexpr int MOLOTOV_STAMINA_COST = 1;
        constexpr int MOLOTOV_FIRE_DURATION = 2;
        constexpr int MOLOTOV_MIN_STEPS = 1;
        constexpr int MOLOTOV_MAX_STEPS = 6;

        // Claymore Mine parameters
        constexpr int MINE_EXPLOSION_RADIUS = 1;
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
    }

    // =========================================================================
    // SECTION 5: WATER MOVEMENT & TERRAIN PENALTIES
    // =========================================================================
    namespace TerrainPenalties {
        constexpr int WATER_MOVES_MAX_SPRINTER = 1;  //sprinters max moves per turn capped to 1 in water
        constexpr int WATER_MOVEMENT_COST = 2;       // stamina cost to move into water
    }

    // =========================================================================
    // SECTION 5b: ICE TERRAIN
    // =========================================================================
    namespace Ice {
        // Number of consecutive ice cells required to trigger a slide
        constexpr int SLIDE_TRIGGER_LENGTH = 4;
        // Probability of sliding when entering ice with >= SLIDE_TRIGGER_LENGTH consecutive ice ahead
        constexpr float SLIDE_CHANCE = 0.5f;
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
    }

    // =========================================================================
    // SECTION 8: GAME CONFIGURATION DEFAULTS
    // =========================================================================
    namespace Defaults {
        // Map configuration
        constexpr int MAP_WIDTH = 15;
        constexpr int MAP_HEIGHT = 15;
        constexpr int VIEW_CELLS = 15;  // Number of cells visible in the viewport

        // Terrain ratios (sum = 100%)
        constexpr int RATIO_WALL = 10;
        constexpr int RATIO_WATER = 10;
        constexpr int RATIO_FOREST = 20;
        constexpr int RATIO_DIRT = 60;

        // Turn limit
        constexpr int TURN_LIMIT = 20;

        // Human spawn position (default for custom maps)
        constexpr int CUSTOM_HUMAN_X = 1;
        constexpr int CUSTOM_HUMAN_Y = 1;

        // Environment probabilities (sum = 100%)
        constexpr int ENV_PROB_CLEAR = 58;
        constexpr int ENV_PROB_WIND = 16;
        constexpr int ENV_PROB_RAIN = 14;
        constexpr int ENV_PROB_CLOUDS = 4;
        constexpr int ENV_PROB_LIGHTNING = 8;

        // Human default stats (fallback when not set by difficulty)
        constexpr int HUMAN_DEFAULT_HP = 5;
        constexpr int HUMAN_DEFAULT_STAMINA = 6;
        constexpr int HUMAN_DEFAULT_PISTOL_AMMO = 12;
        constexpr int HUMAN_DEFAULT_SHOTGUN_AMMO = 6;
        constexpr int HUMAN_DEFAULT_GRENADES = 3;
        constexpr int HUMAN_DEFAULT_MINES = 2;
        constexpr int HUMAN_DEFAULT_MOLOTOVS = 3;

        // Fire duration (in turns)
        constexpr int FIRE_DURATION = 2;

        // Stamina regen range
        constexpr int STAMINA_REGEN_MIN = 1;
        constexpr int STAMINA_REGEN_MAX = 6;

        // Grenade/Molotov steps range
        constexpr int PROJECTILE_MIN_STEPS = 1;
        constexpr int PROJECTILE_MAX_STEPS = 6;
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

        // Human default fire extinguish in water
        constexpr int HUMAN_FIRE_EXTINGUISH_IN_WATER = 1;

        // Human default fire damage on burning
        constexpr int HUMAN_FIRE_DAMAGE_ON_BURNING = 1;

        // Human default fire damage on entering fire
        constexpr int HUMAN_FIRE_DAMAGE_ENTERING_FIRE = 1;

        // Human default fire damage on catching fire
        constexpr int HUMAN_FIRE_DAMAGE_CATCHING_FIRE = 1;

        // Human default fire damage on proximity
        constexpr int HUMAN_FIRE_DAMAGE_PROXIMITY = 1;

        // Human default fire damage on burning entity
        constexpr int HUMAN_FIRE_DAMAGE_BURNING_ENTITY = 1;

        // Human default fire damage on fire cell
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CELL = 1;

        // Human default fire damage on fire interaction
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_INTERACTION = 1;

        // Human default fire damage on fire check
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK = 1;

        // Human default fire damage on fire resolve
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_RESOLVE = 1;

        // Human default fire damage on fire propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_PROPAGATION = 1;

        // Human default fire damage on fire spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_SPREAD = 1;

        // Human default fire damage on fire check interaction
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_INTERACTION = 1;

        // Human default fire damage on fire check resolve
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_RESOLVE = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION2 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD2 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION3 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD3 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION4 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD4 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION5 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD5 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION6 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD6 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION7 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD7 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION8 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD8 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION9 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD9 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION10 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD10 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION11 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD11 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION12 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD12 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION13 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD13 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION14 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD14 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION15 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD15 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION16 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD16 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION17 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD17 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION18 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD18 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION19 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD19 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION20 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD20 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION21 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD21 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION22 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD22 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION23 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD23 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION24 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD24 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION25 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD25 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION26 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD26 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION27 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD27 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION28 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD28 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION29 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD29 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION30 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD30 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION31 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD31 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION32 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD32 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION33 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD33 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION34 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD34 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION35 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD35 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION36 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD36 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION37 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD37 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION38 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD38 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION39 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD39 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION40 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD40 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION41 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD41 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION42 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD42 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION43 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD43 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION44 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD44 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION45 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD45 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION46 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD46 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION47 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD47 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION48 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD48 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION49 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD49 = 1;

        // Human default fire damage on fire check propagation
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_PROPAGATION50 = 1;

        // Human default fire damage on fire check spread
        constexpr int HUMAN_FIRE_DAMAGE_FIRE_CHECK_SPREAD50 = 1;
    }
}

#endif // GAME_CONSTANTS_H
