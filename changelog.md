# v2.5.0

## Logic:
- Fix bug: `Fast Zombie` runs into `Water` tile in its second step while it should not be able
- Change: except `Fast Zombie`, now when a zombie moves into `Water`, it will not able to attack `Human` after moving in the same turn as usual. This is to compensate the disadvantage of `Human` when moving in `Water`

## UI:
- Fix bug: When `Human` is `Frozen`, the arrows for directional weapons are not displayed


# v2.4.0

## Logic:
- Fix: ice-sliding entity triggered `Mine` to detonate even though Human (who planted that `Mine`) not moving at all and still standing on the same tile
- Update: many `Loot`s now can be dropped on the same tile
- Fix: max HP of `Human` is now not fixed but can be increased when picking loot +HP 

## UI:
- Disable `Mine` button when there is already a `Mine` planted on the tile `Human` is standing
- Icon of loot now has a number to indicate the number of loots on that 
- `Burn` status now appears immediately when entity moving onto `Fire` no matter actively or passively


# v2.3.0

## Logic:
- Fix: `Zombie` in `Burn` now loses 1 HP right at the end of its own turn, not at the end of turns of all `Zombie`
- Fix: `Ice` tiles were not melted immediately when their adjacent tile turns into `Fire`

## UI:
- Now entities in `Burn`, `Fire` tiles and their adjacent tiles as well as things standing/lying on those tiles are also visible in `Dark Clouds` environment
- Add directional arrows for `Human` in `Dark Clouds`


# v2.2.1

## Logic:
- Fix bug in displaying the multikill banners


# v2.2.0

## Logic:
- `Clever Zombie` is now more clever in using the weapons it picked
- Fix: `Grenade` thrown by `Clever Zombie` explode immediately, now it waits until the beginning of the turn of that `Zombie` to explode

## UI:
- Remove the black square icon marking where a `Zombie` died
- Fix: `Human` icon is only a white square when there is `Clouds` without the others decorations
- Add highlights effects for multikills
- Add blinking border to `Zombie` icons when it is in its turn

## Setting:
- Now allow user to choose whether `Stamina` point every turn is fixed or given randomly (also saved as a setting in `.zom` file with values from 1 to 6 for if fixed and 0 if random)
- Reset the limits of scrollbars in Hub

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
