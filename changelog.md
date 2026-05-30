# Changes in v1.3.0

## Logic
- Fix logic bug: when `Human` step on an `Ice` tile with frozen loot under it, `Human` no longer automatically uses `Ice Pick` to break the ice to get the loot, instead the player can choose to use `Ice Pick` or not
- Increase the accuracy of `Pistol` and its range
- New mechanisms for using `Mine`: `Mine` now can be triggered by shooting `Pistol` or `Shotgun` at it, or by `Lightning`, or by `Fire`, or by explosion

## UI:
- Fix the version info in the splash screen to display the correct version

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
