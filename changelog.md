# v2.1.0

## Logic:
- Fix bug: When `Explosing Zombie` detonates in `Water`, adjacent `Wall` is destroyed even though the area of effect of the explosion is only at the tile the zombie is standing
- Fix bug: Explosion now only destroys adjacent `Wall` tiles (assuming those `Wall` tiles is in the area of effect)
- Fix bug: Entity right after `Wall` still gets damage from explosion even though the `Wall` should block
- `Ice` tile right after `Wall` now will not melted into `Water` due to explosions

## UI:
- Fix: The number of available spawn tiles is now get updated when the player adjust the ratio of `Wall` tiles
- `Wall` at the far zone (radius = 2) of the explosion is now not displayed in the blast-cell animation, similarly for `Wall` not right in front of the shotgun direction


# v2.0.0

## New feature: Normal Zombies are no longer "normal", they are "clever" now. Change `Normal` to `Clever` throughout the game and codebase.
- `Clever Zombie` can now pick the loots when it steps on the tiles having the loots (except when the loots are frozen under `Ice` tiles)
- If the loot is extra HP, then HP of `Clever Zombie` increases by that amount (its HP indicator will increases). If the loot is restoring stamina, then it has another full turn right after the end of its current turn.
- If the loot is ammo of the weapons (`Pistol` bullet, `Shotgun` bullet, `Grenade`, `Molotov`, `Mine`), then `Clever Zombie` can use the corresponding weapons
- In every of its turn, `Clever Zombie` can only do one of the actions (randomly): doing nothing, or moving, or using one of the weapons. Of course it can also attack `Human` (bite or scratch) right before its turn end. 
- If `Clever Zombie` is in `Frozen` status, it just cannot move or plant `Mine` on `Ice` tile
- When `Clever Zombie` uses , it uses as if it was `Human` (i.e. same mechanisms), except that it chooses the direction randomly with higher probability for the direction that closest to the position of `Human`
- All the mechanisms of the weapons used by `Clever Zombie` are the same as if they are used by `Human`, the all entities may take damage
- Increase the activeness of `Zombie`

## Logic:
- Fix bug when planting `Mine` in a `Fire` tile but `Mine` does not explode
- Loot in `Fire` tile is now destroyed immediately

## UI:
- If a `Clever Zombie` holds weapon ammo, there will be a small black right-angle triangle at the top-right corner of its square icon to indicate. This indicator will disappear when it does not hold weapon ammo anymore

## SFX:
- New sound effects for events when `Zombie` dies, when `Human` and `Clever Zombie` pick useful loots.


# v1.4.0

## Logic:
- `Frozen` status is now permanent until the `Ice` tile melts into `Water` tile, or it is knocked back by shotgun or explosions, or it is striked by the lightning
- Zombies with `Frozen` status can now bite or scratch if `Human` stays adjacent to their tiles in their turns, they just are not able to move
- Lightning now can melt the `Ice` tile it strikes into `Water`
- Lightning now ignites the `Forest` tile it strikes, regardless there is entity staying on that tile or not
- Range of explosions on `Ice` tile is now the same as in `Dirt` or `Forest`
- Mines now will be deactivated when their tile freezes, reactivated when that tile thaws, and needs extra trigger to explode
- `Shotgun` is now not able to makes `Ice` into `Water`

## UI:
- Fix bug: some button of weapons is not deactivated when not usable


# v1.3.0

## Logic:
- Fix logic bug: when `Human` step on an `Ice` tile with frozen loot under it, `Human` no longer automatically uses `Ice Pick` to break the ice to get the loot, instead the player can choose to use `Ice Pick` or not
- Increase the accuracy of `Pistol` and its range
- New mechanisms for using `Mine`: `Mine` now can be triggered by shooting `Pistol` or `Shotgun` at it, or by `Lightning`, or by `Fire`, or by explosion

## UI:
- Fix the version info in the splash screen to display the correct version


# v1.2.0

## Logic:
- Entities now are resolved from `Frozen` status when pushed back by explosion or by `Shotgun`
- Fix a bug in explosion-blocking logic (some zombies standing in the area of effect of `Grenade` but not be affected)
- `Exploding` zombie drops `Loot` after it explodes to prevent its explosion destroys the `Loot` of itself
- `Windstorm` now makes the `Forest` tiles right after the `Fire` tiles (in the wind direction) become `Fire` tiles
- Now `Fire` doesn't spread after `Environment` phase to decelerate the spreading speed
- Now, `Fire` tiles do not cause extra damage to `Burned` entities, `Fire` tiles only make entities that are not `Burned` to be `Burned`
- Now, `Burned` status will not automatically resolved after 1 turn of that entity, but will last until it is extinguished by stepping on `Water` tile or there is `heavy rain`, entity will lose 1 HP at the end of its turn if it is still `Burned`
- Now only entities on `Fire` tiles will get `Burned`, entities adjacent to `Fire` tiles or tiles with `Burned` entities will not get `Burned`
- `Fire` tiles now also melt the diagonal `Ice` tiles to `Water` tiles, but `Fire` only spreads orthogonally as before

## UI:
- Fix `B = Burn` to `B = Burned` in legend
