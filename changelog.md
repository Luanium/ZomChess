# Changes in v1.2.0
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
