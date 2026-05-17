#ifndef ENTITIES_H
#define ENTITIES_H

#include "Types.h"
#include <string>

struct Human {
    Position pos;
    int hp = 5;
    int stamina = 6;
    int pistol_ammo = 12;
    int shotgun_ammo = 6;
    int grenades = 3;
    int mines = 2;
};

class Zombie {
public:
    Position pos;
    int hp;
    std::string name;
    ZombieType type;

    Zombie(Position p, int h, std::string n, ZombieType t) 
        : pos(p), hp(h), name(n), type(t) {}
    virtual ~Zombie() = default;
    virtual int getMovesPerTurn() const { return 1; }
};

class NormalZombie : public Zombie { public: using Zombie::Zombie; };
class FastZombie : public Zombie { public: using Zombie::Zombie; int getMovesPerTurn() const override { return 2; } };
class ExplodingZombie : public Zombie { public: using Zombie::Zombie; };
class VampireZombie : public Zombie { public: using Zombie::Zombie; };

#endif // ENTITIES_H
