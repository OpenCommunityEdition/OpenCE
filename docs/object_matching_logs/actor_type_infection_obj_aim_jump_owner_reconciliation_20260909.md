# `actor_type_infection.obj` aim-jump owner reconciliation (2026-09-09)

## Result

`infection_swarm_aim_jump` is admitted as natural typed C and a strict exact
match: **686 meaningful / 688 padded code bytes**. The isolated object advances
from 3 to 4 exact target functions without changing its other four verdicts.

The reconstruction uses the existing typed actor, swarm, unit, and prop access
macros. A local union gives the ballistic result its actual `real_vector3d`
representation while retaining named horizontal and vertical views; this avoids
raw offset access or repeated pointer casts. The function contains no register,
volatile, pragma, assembly, synthetic branch, or undefined-value steering.

## Declaration ownership

`projectile_aim_ballistic` is implemented by `projectiles.c` and called from
both Projectiles and Infection AI. Its declaration now lives in the owning
`items/projectiles.h`, and the caller includes that header. The duplicate
consumer-local and implementation-local declarations were removed.

All existing consumers of `projectiles.h` were rebuilt with the universal gate.
Their exact-function counts remained unchanged: Units 189, Weapons 47,
Projectiles 17, Players 54, Cinematics 16, Actors 68, Game 24, and Actor Combat
19. Infection advances by one. The pre-existing `point_from_line3d` emission in
Units is unchanged by this packet; Infection and Projectiles both pass the hard
forbidden-symbol guard, and this packet introduces no helper COMDAT.

## Validation

- isolated full-object gates for Infection and Projectiles;
- isolated gate sweep of every existing `projectiles.h` consumer;
- `python tools/fake_match_scan.py` over all touched source/header files: zero
  review leads;
- `git diff --check`.

The repository-wide semantic sweep and published-baseline comparison are
performed once for the containing 10,000-byte publication batch. No fuzzy body
or whole-object completion is credited here.
