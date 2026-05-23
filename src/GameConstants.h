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
            constexpr int SHOTGUN_AMMO = 5;
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
            constexpr int INITIAL_STAMINA = 3;
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

        // Claymore Mine parameters
        constexpr int MINE_EXPLOSION_RADIUS = 1;
        constexpr int MINE_DAMAGE = 2;
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
    }

    // =========================================================================
    // SECTION 6: ENVIRONMENTAL STIMULI & EVENTS
    // =========================================================================
    namespace Environment {
        // Flood transition chances
        constexpr float RAIN_FLOOD_THRESHOLD_WATER_ADJACENT = 0.30f;
        constexpr float RAIN_FLOOD_THRESHOLD_ISOLATED = 0.10f;

        // Lightning strikes
        constexpr double LIGHTNING_WEIGHT_DEFAULT = 1.0;
        constexpr double LIGHTNING_WEIGHT_WATER = 4.0;
        constexpr double LIGHTNING_WEIGHT_ENTITY = 2.0;
        constexpr int LIGHTNING_HP_DAMAGE = 1;
    }
}

#endif // GAME_CONSTANTS_H
