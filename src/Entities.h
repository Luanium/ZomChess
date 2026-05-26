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
    bool is_frozen = false;    // Frozen: standing on water when it freezes
    int frozen_turns = 0;      // Turns remaining frozen (counts down each turn)
    bool loot_spawned = false; // Đã spawn loot khi chết chưa
    bool pending_attack = false; // Delay attack after movement

    Zombie(Position p, int h, std::string n, ZombieType t) 
        : pos(p), hp(h), max_hp(h), name(n), type(t) {}
    virtual ~Zombie() = default;
    virtual int getMovesPerTurn() const { return GameConstants::Zombies::MOVES_PER_TURN_NORMAL; }
};

class NormalZombie : public Zombie { public: using Zombie::Zombie; };
class FastZombie : public Zombie { public: using Zombie::Zombie; int getMovesPerTurn() const override { return GameConstants::Zombies::MOVES_PER_TURN_FAST; } };
class ExplodingZombie : public Zombie { public: using Zombie::Zombie; };
class VampireZombie : public Zombie { public: using Zombie::Zombie; };
class SickZombie : public Zombie { public: using Zombie::Zombie; };

#endif // ENTITIES_H
