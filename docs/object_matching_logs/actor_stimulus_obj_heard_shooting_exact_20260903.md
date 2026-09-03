# `actor_stimulus.obj` heard-shooting closure (2026-09-03)

## Result

This bounded lane reconstructs the previously unwritten public function
`_actor_stimulus_heard_shooting` in ordinary typed C. The retained function is
a strict January match:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_actor_stimulus_heard_shooting` | 336 | 19 | `6420917f34b2180cf1a8d00ce7bb0b2eae4a19227dc8c6874da233e318237b51` |

The focused object gate advances `source/ai/actor_stimulus.obj` from 18 exact,
zero residual, and four unwritten functions to 19 exact, zero residual, and
three unwritten functions. The target and candidate have identical padded
bytes and identical ordered relocation sites, types, destinations, and
addends.

## Evidence and reconstruction

January disassembly, relocation topology, and existing typed layouts are the
primary evidence. The open HCEA exact-name donor
`src/blam/ai/actor_stimulus_heard_shooting.c` independently authenticates the
behavior:

- an enemy shooting prop immediately enters perceived-enemy combat;
- a friendly shooter enters friend-in-combat handling;
- a player-controlled friend can donate a recent aim-assist enemy as an
  orphan target;
- a non-player friend with sufficiently certain combat status can donate its
  actor target information; and
- every path leaves the actor looking toward the shooting prop.

The retained body uses the canonical typed `prop_get`, `unit_get`,
`player_get`, and `actor_get` accessors. It uses the game-allegiance owner's
`game_team_is_enemy` declaration and the existing public
`actor_stimulus_heard_shooting` declaration in `source/ai/actors.h`; no
use-site prototype or shared-header definition was added.

The first complete implementation was semantically correct and already had
the target's 336-byte padded size, but cached the friendly unit's actor index
in a local. January instead reloads that field after checking the friend's
combat status. Expressing the two genuine source-level uses directly as
`unit->unit.actor_index` removes the unnecessary live range and reproduces the
target. This is a plausible source correction supported by the target's field
reload, not a register hint or fake dependency.

## House-rule audit

The retained source uses project `real`/datum types, semantic enum constants,
typed tag and object accessors, one parameter per line, and an explicit final
`return;`. It contains no address-derived identifier, raw datum cast, manual
byte offset, inline request, pragma, volatile/register forcing, assembly,
undefined behavior, fake dependency, or nonsensical control path. No new
global, private function, prototype, structure definition, or header-owned
type is introduced. The candidate object defines no `point_from_line3d`
symbol or COMDAT.

`actor_stimulus_prop_fleeing` remains at its documented evidence-exhausted
fixed point. `actor_stimulus_weapon_detonation` remains deferred pending its
proper game-team dependency. `actor_stimulus_prop_acknowledged` remains the
next unwritten body but includes a substantially larger January-only AI-debug
diagnostic block and was intentionally kept outside this exact leaf closure.

## Validation

- focused gate: `_actor_stimulus_heard_shooting` strict, 336/336 padded bytes,
  19/19 relocations, and matching normalized SHA-256;
- complete object gate: 19 exact, zero residual, three unwritten;
- all previously accepted functions in `actor_stimulus.obj` remain strict;
- `git diff --check`: clean for the retained source and ledger.

Whole-tree build, stable-verdict comparison, protected `units.obj`, semantic,
parked-function, fake-match, and tooling results are recorded in the
integration checkpoint that admits this function.
