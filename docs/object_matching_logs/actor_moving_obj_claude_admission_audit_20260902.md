# `actor_moving.obj` Claude admission audit (2026-09-02)

## Result

Starting from canonical commit `429497bbe`, this lane independently audited
the six apparent gains in Claude campaign commit `617703d2e` and admitted five
reconstructed functions after reconciling their source with current canonical
types, interfaces, names, and inline ownership. The donor translation unit was
not copied wholesale.

| January address | Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---:|---|---:|---:|---|
| `0x00018A80` | `_actor_path_input_new` | `188 / 192` | `9` | `71df0b89cef99b79f49e042b6dcb4959aaf77c9a9564d7ecae9ab0bf98fddc38` |
| `0x00019AA0` | `_actor_move_get_avoidance_direction` | `305 / 320` | `17` | `a57433cc690b33e61b3678c7eac3853ec77b15f227692fab27f54dfaf8288c59` |
| `0x00019D30` | `_actor_path_3d_available` | `259 / 272` | `9` | `48ddb7d742f12a4d590091a5f18999102ebe38a2c6264f5a1b21e5ff78c627af` |
| `0x0001D7B0` | `_actor_move_halt` | `132 / 144` | `4` | `b21cabac66d7adefa85bd872fecb12c6f68bec11b07cf216afbd0bd569e1c7e0` |
| `0x0001D840` | `_actor_move_halt_at_firing_position` | `126 / 128` | `5` | `c558be6eb52f3a586c8b2732458839a3a06a32c8283a86698f4b405f7693e870` |
| **Gain** | **five functions** | **`1,010 / 1,056`** | **`44`** | |

`actor_moving.obj` advances from **15 exact / 0 residual / 21 unwritten**
to **20 exact / 0 residual / 16 unwritten**. A whole-tree verdict comparison
against a detached build of the exact base commit reports these five `U -> E`
transitions and no exact loss among 8,245 target functions.

The address-keyed audit also keeps the rejected sixth donor packet visible:

| January address | Function | Before | After |
|---:|---|:---:|:---:|
| `0x00018A80` | `_actor_path_input_new` | `U` | `E` |
| `0x00018B50` | `_midpoint3d` | `U` | `U` |
| `0x00019AA0` | `_actor_move_get_avoidance_direction` | `U` | `E` |
| `0x00019D30` | `_actor_path_3d_available` | `U` | `E` |
| `0x0001D7B0` | `_actor_move_halt` | `U` | `E` |
| `0x0001D840` | `_actor_move_halt_at_firing_position` | `U` | `E` |

## Source reconciliation

The retained bodies are ordinary typed C and were checked against January's
disassembly and relocation identities, the current actor/unit layouts, and the
independent HCEA reconstructions:

- `actor_path_input_new` uses `actor_get`, `actor_definition_get`,
  `vehicle_get`, and `vehicle_specific_definition_get`; the actor definition
  supplies the default pathfinding radius, while a driven vehicle may override
  it and becomes the collision-ignore object. The raw driver test from the
  donor is expressed as `_actor_vehicle_driver_none`.
- `actor_path_3d_available` uses HCEA's recovered vehicle-tag name
  `ai_avoidance_distance` at `+0x388` and the neighboring
  `ai_pathfinding_radius` at `+0x38C`. Because the public vehicle definition is
  intentionally opaque, this translation unit carries only that target-proven
  fragment with compile-time offset checks; it does not retain Claude's guessed
  larger padded structure or its speculative `ai_3d_pathfinding_height` name.
- `actor_move_halt` and `actor_move_halt_at_firing_position` retain the
  existing canonical `boolean` ABI, the semantic `_destination_halt` and
  `_actor_vehicle_driver_directional_flying` constants, and return the actual
  callee result. January preserves `AL` on every exit and HCEA independently
  declares both functions as byte-valued. Claude's byte-coincidental `void`
  spelling is rejected.
- `actor_move_get_avoidance_direction` interpolates the authenticated eight-ray
  table, reports an impossible lookup through the normal error owner, validates
  the angle, and transforms the resulting vector through the existing typed
  helper.

HCEA independently recovers the global name `avoidance_ray_angles` and its
45-degree-step meaning. January's exact eight dwords are:

```text
00000000 3f490fdb 3fc90fdb 4016cbe4
40490fdb 407b53d2 4096cbe4 40afede0
```

Straight `_pi` expressions are retained for the six values they reproduce.
VC7 constant folding makes `5*_pi/4` and `7*_pi/4` one ULP low, as do the
degree-conversion, double-`M_PI`, additive-`_pi`, and all-literal HCEA spellings
tested in bounded scratch builds. The two remaining entries therefore use the
ordinary decimal radians present in the recovered table, `3.9269909f` and
January's `5.4977875f`. They compile to the shipped values without bit casts,
integer aliases, data patches, or optimizer-only source.

No raw object/tag retrieval, address-derived private name, anonymous address
global, raw enum value, representation pun, volatile/register forcing,
barrier, pragma, assembly, fake dependency, forced inline/noinline, dead
branch, or nonsensical source is retained.

## Interface and inline ownership

Cross-translation-unit declarations for the four public movement entry points
now live in the `ACTOR_MOVING.C` section of `ai/actors.h`.
`actor_find_pathfinding_location` is declared in the same header under its
actual `ACTORS.C` owner; its misplaced consumer-local declaration was removed
from `action_fight.c`.

`real_math.h` currently defines `normalize3d` inline, while January's actor
movement compiland imports the out-of-line owner already emitted by
`action_charge.c`. The translation unit therefore follows the established
January schedule used by other canonical objects: it temporarily renames the
header inline and declares the imported typed function locally. The resulting
object has `_normalize3d` undefined, matching January. It has no
`_point_from_line3d` symbol at all and therefore emits no forbidden COMDAT.

## Deliberate exclusion

Claude's `_midpoint3d` is not admitted. The frozen first-shot ledger already
established that the independently preserved later source uses a `void` ABI
and compiles to the wrong three-pointer register assignment. The donor reaches
January's target packet only by selecting the pointer-returning inline ABI
after seeing that target allocation, creating an artificial standalone copy
of the shared real-math helper. No new January source or independent
same-compiler ABI evidence was found, so the prior evidence boundary remains
in force. Its address `0x00018B50`, 64-byte packet, and target hash
`712499e7f9fc6e985d3e3ddfe0b03e70d4a8e59c6eba96567b5cc281a52bc963`
remain unwritten rather than accepting a byte-led source claim.

No fuzzy park or completion exception is added by this lane.

## Validation

- focused PID gate: `actor_moving.obj` is `20 exact / 0 residual /
  16 unwritten`; all five admitted functions independently compare exact;
- protected `units.obj`: `189 exact / 0 residual / 0 unwritten`;
- detached-base whole-tree verdict diff: five gains, zero regressions;
- full Ninja configure/cache/report/semantic/progress build: pass;
- campaign code total: `659,535 / 2,198,102` meaningful bytes and
  `5,020 / 11,060` report functions, an exact `+1,010 / +5` delta over the
  detached base build;
- semantic audit: 472 units, 5,192 functions evaluated, 5,063 accepted exact,
  and zero unit errors;
- parked-function validation: 86 active, zero stale, zero invalid;
- tooling tests: 255/255 pass;
- changed-file fake-match scan: zero review leads;
- `git diff --check`: pass;
- exact angle-table data comparison: all eight dwords equal;
- object symbol audit: `_normalize3d` is undefined and
  `_point_from_line3d` is absent.

No canonical worktree is edited and no push is performed by this audit lane.
