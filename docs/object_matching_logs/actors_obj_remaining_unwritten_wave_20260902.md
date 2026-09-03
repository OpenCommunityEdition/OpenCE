# `actors.obj` remaining-unwritten wave (2026-09-02)

## Scope and baseline

- Worktree: `worktrees/actors-remaining-wave-20260902`
- Branch: `agent/actors-remaining-wave-20260902`
- Pinned starting commit: `5d663917d602ae8a18ba6e0e0b38fc1a3f2d24f3`
- Owned implementation: `source/ai/actors.c`, plus the smallest owner-header
  corrections needed by the recovered typed source and removal of one now-
  redundant prototype from `source/ai/actions.c`.
- Baseline gate: 65 exact, 7 residual, 4 unwritten of 76 functions.
- Final gate: 67 exact, 9 residual, 0 unwritten of 76 functions.

The four missing functions were reconstructed smallest/high-confidence first.
January's split COFF object and its relocations, assertions, constants, and
diagnostic strings are the byte authority. The independent HCEA lifts in
`halocea-reference/src/actor_died.c`, `actor_new.c`, `actor_place.c`, and
`actor_input_update.c` supplied behavior and naming evidence. The recovered
Claude draft at `C:/halo-worktrees/claude-d3dx-projections-5/src/halo/ai/actors.c`
was treated only as secondary evidence and checked against January.

## Results

| Function | Target bytes | Result | Evidence |
| --- | ---: | --- | --- |
| `_actor_died` | 832 | exact | Full death-wildfire, grenade, weapon-load/ammo, actor deletion, and encounter-status path. |
| `_actor_new` | 944 | exact | Full allocation, actor/meta/state/control/emotion/debug initialization and actor-type initialization. |
| `_actor_place` | 800 | fuzzy parked | Complete typed placement path; 816-byte candidate, 41/41 relocation identities, 90.887215% objdiff. |
| `_actor_input_update` | 2,384 | fuzzy parked | Complete typed swarm/unit/vehicle/encounter/threat/path/orientation/vitality refresh; 2,400-byte candidate, 89/89 relocation identities, 94.84425% objdiff. |

The strict gain is two functions and 1,776 padded bytes. More importantly, all
four formerly unwritten functions now have complete, ordinary, typed C. No
inline assembly, forced inlining, register/volatile steering, barriers,
pragmas, raw-offset access, fake dependencies, or deliberately nonsensical
control flow were retained.

## Honest residuals

`actor_place` retains January's assertions, optional major upgrade, typed tag
lookups, object placement, exact failure diagnostics, squad/start-state
selection, customization, actor creation, cleanup, and activation check. Its
target/candidate section sizes are 800/816 with identical ordered relocation
destinations. The close common prefix diverges across several VC7 allocation,
lifetime, and scheduling choices, including a four-byte-larger local frame.
Natural declaration-lifetime, branch, and local-placement trials did not close
the residual, so it remains fail-closed as `unclassified`.

`actor_input_update` retains the complete January call and diagnostic topology:
swarm center/member sampling, individual and parent-vehicle sampling, AI driver
and gunner roles, fake-encounter migration/restoration, flying versus 3D firing
position diagnostics, attached melee/projectile threat scanning, biped path
state, orientation-frame construction, normal validation, and vitality. Its
target/candidate sizes are 2,384/2,400 with identical ordered relocation
destinations. Several natural condition, declaration-lifetime,
encounter-migration, and vector-expression forms were bounded and reverted.
An equal-size intermediate was also rejected because it represented January's
behavior less faithfully. Since the remainder spans allocation and scheduling
rather than one isolated source construct, the function is conservatively
parked as `unclassified`.

## Owner-header corrections and blast audit

- `actor_placement.h`: corrected `actor_place`'s squad index to the target's
  full-width `long`. It has two direct C includers (`actors.c`, `encounters.c`).
- `actors.h`: placed the recovered `actor_clear_discarded_firing_positions`
  declaration in the closest owning header because its implementation file has
  no header. It has 42 direct C includers. The duplicate declaration formerly
  kept in `actions.c` was removed; that file already includes `actors.h`, and
  its isolated gate remains 58/63 exact with no changed verdict.
- `ai_debug.h`: gave the final debug counter its observed semantic name without
  changing layout. It has 15 direct C includers.
- `ai_scenario_definitions.h`: named the braindead encounter flag and recovered
  the exact 0x1C `actor_starting_location` layout with offset assertions. It has
  13 direct C includers.
- `weapons.h`: placed `weapon_set_current_amount` in its owning header. It has
  16 direct C includers, including `units.c`.
- `projectiles.h` was audited because actors now reads the existing typed
  projectile flags, but no tracked header change was retained. It has seven
  direct C includers, including `units.c`.

A temporary, semantically inert projectile-field offset assertion was removed
before admission. Its declaration position changed `actor_input_update`'s
normalized candidate hash and objdiff score even though the implementation and
layout were unchanged, reproducing the known VC7 header-position sensitivity.
The parking record uses the final no-extra-header-declaration build.

The final all-unit stable-verdict scan compares the pinned baseline and
candidate after full builds, so it covers transitive code-generation effects
from every header above. It records only the two intended `actors.obj` gains
and zero regressions. `units.obj` remains 189/189 exact.

## Verification

- `python tools/campaign/gate.py source/ai/actors --all`:
  67 exact, 9 residual, 0 unwritten.
- `python -m tools.campaign.stable_verdicts diff ...`:
  `_actor_new` +944, `_actor_died` +832; 2 gains / 1,776 bytes; zero
  regressions.
- `python tools/campaign/gate.py source/units/units --all`:
  189 exact, 0 residual, 0 unwritten.
- `ninja all_source`: passed.
- `python -m pytest -q`: 261 passed.
- `python -m tools.parked_functions`: both new parking records active, with no
  stale or invalid measurements.
- `git diff --check`: passed.
