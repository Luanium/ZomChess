#ifndef ENTITIES_H
#define ENTITIES_H

#include "Types.h"
#include <string>

struct Human {
    Position pos;
    int hp = GameConstants::Defaults::HUMAN_DEFAULT_HP;
    int stamina = GameConstants::Defaults::HUMAN_DEFAULT_STAMINA;
    int pistol_ammo = GameConstants::Defaults::HUMAN_DEFAULT_PISTOL_AMMO;
    int shotgun_ammo = GameConstants::Defaults::HUMAN_DEFAULT_SHOTGUN_AMMO;
    int grenades = GameConstants::Defaults::HUMAN_DEFAULT_GRENADES;
    int mines = GameConstants::Defaults::HUMAN_DEFAULT_MINES;
    int molotovs = GameConstants::Defaults::HUMAN_DEFAULT_MOLOTOVS; 
    int warp_ammo = GameConstants::Defaults::HUMAN_DEFAULT_WARP_AMMO;
    
    bool is_burning = false;
    bool is_paralyzed = false;
    bool is_frozen = false;    // Frozen: standing on water when it freezes
};

class Zombie {
public:
    Position pos;
    int hp;
    int max_hp;
    std::string name;
    ZombieType type;
    
    bool is_burning = false;
    bool is_paralyzed = false;
    bool is_frozen = false;    // Frozen: standing on water when it freezes (permanent until ice melts)
    bool loot_spawned = false; // Đã spawn loot khi chết chưa
    bool has_exploded = false;  // Exploding zombie: đã phát nổ chưa (dùng để hoãn loot drop)
    bool pending_attack = false; // Delay attack after movement
    bool kill_counted = false;   // Has this death been counted toward kills_this_turn

    // Clever Zombie weapon ammo (only used by ZombieType::Clever)
    int pistol_ammo   = 0;
    int shotgun_ammo  = 0;
    int grenades      = 0;
    int molotovs      = 0;
    int mines         = 0;
    int warp_ammo     = 0;
    bool extra_turn   = false; // Stamina potion grants one extra full turn

    Zombie(Position p, int h, std::string n, ZombieType t) 
        : pos(p), hp(h), max_hp(h), name(n), type(t) {}
    virtual ~Zombie() = default;
    virtual int getMovesPerTurn() const { return GameConstants::Zombies::MOVES_PER_TURN_NORMAL; }

    // Returns true if this Clever Zombie holds any weapon ammo
    bool hasWeaponAmmo() const {
        return pistol_ammo > 0 || shotgun_ammo > 0 || grenades > 0 || molotovs > 0 || mines > 0 || warp_ammo > 0;
    }
};

class CleverZombie : public Zombie { public: using Zombie::Zombie; };
class FastZombie : public Zombie { public: using Zombie::Zombie; int getMovesPerTurn() const override { return GameConstants::Zombies::MOVES_PER_TURN_FAST; } };
class ExplodingZombie : public Zombie { public: using Zombie::Zombie; };
class VampireZombie : public Zombie { public: using Zombie::Zombie; };
class SickZombie : public Zombie { public: using Zombie::Zombie; };
class CorruptorZombie : public Zombie { public: using Zombie::Zombie; };

#endif // ENTITIES_H
