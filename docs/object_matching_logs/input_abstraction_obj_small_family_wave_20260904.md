# Input Abstraction small-family wave — 2026-09-04

> Integrated verification: the full dependent build and 8,245-owner sweep
> preserve all inherited exact functions and confirm **9 exact / 1 residual**.
> The update is now a validated park at 67.32703% objdiff. The orchestrator
> corrected the scalar/array symbol boundary to 2,552,736 / 2,552,740 without
> changing the physical 20-byte contribution or claiming data exactness.
> See `small_family_integration_batch_20260904.md` for batch validation;
> pre-integration worker findings below are retained as evidence.

## Result and admission boundary

From canonical starting HEAD `9a1a99f80`, the focused scratch gate advances
`input_abstraction.obj` from **6 exact / 0 residual / 4 unwritten** to
**9 exact / 1 residual / 0 unwritten**. Three small functions add **464 padded
exact code bytes**; all six inherited exact owners remain exact. The previously
undefined, naturally typed 224-byte BSS owner now also compares strictly equal.
The large update is a credible reconstructed residual, not exact credit.

This worker ran no Ninja/configure/full build, commits, pushes, or configuration
edits. Canonical admission still requires the orchestrator's whole dependent
build, exact-set regression sweep, and accounting checks. No object-completion
claim or parked metadata is fabricated here.

## Scope, ownership, and prior evidence

Changed files:

- `source/input/input_abstraction.c`;
- `source/input/input_abstraction.h`: approved owner declarations for implemented
  `input_abstraction_initialize(void)` and `input_abstraction_update(void)`;
- `source/game/player_control.h`: approved owner declarations for existing
  `player_look_yaw_rate` and `player_look_pitch_rate` real arrays;
- this ledger.

Read before implementation: `docs/matching_methodology.md` and the existing
Input Abstraction lifecycle/preferences, typed-update rejection, and residual
freshness ledgers dated 2026-08-21, 2026-08-28, and 2026-08-30. Their old
one-shot/exact-only rejections are not presented as new credit. The substantive
new recovery is the missing actual global owner and its ordinary caller
topology, not a repetition of the old extern-only donor compile.

January split COFF remains sole byte/ABI authority. The frozen Opus source at
`C:\halo-worktrees\opus-small-families-30k-20260902`, HEAD
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, still has only the six inherited
functions. Read-only `scratch/pastudan-halo/src/halo/input/input_abstraction.c`
supplies corroborating control-flow/axis intent, not acceptable source to copy:
its raw addresses, representation casts, and inline-assembly ABI forcing are
not imported. The cached HCEA `src/blam/game/local_player_is_piloting_aircraft.c`
corroborates the aircraft semantics; its later input initialization/update
architecture is materially different and is not transplanted.

## The small exact closures

| Owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_set_default_game_input_preferences` | 80 | 0 | `f8313b9826f3708b86e7ac0118b898342c7b55e51f939a0941c1d543263b62f1` |
| `_input_abstraction_initialize` | 160 | 9 | `b6da87e4cb2be8447d8bbea226c2ca47deada5ffd3e1766bf1c8c3066f76ac06` |
| `_local_player_is_piloting_aircraft` | 224 | 11 | `cf28a96980c7ecab9c98c569b5c1c77b5943b457419aa86ea32445d50270e2f1` |

The ordinary definition
`struct input_abstraction_runtime_globals input_abstraction_globals = {0};`
replaces the inherited extern-only declaration. January owns this 224-byte BSS
in the object itself, not linker COMMON. Candidate storage is external, section
4 `.bss`, flags `0xc0400080`, 224 bytes / zero relocations, normalized SHA-256
`6eb69e26de2a26eda48af77d4cec893aa0cf4748a64cbefcfe11a22c1e680ad9`.
It passes `section_infos_equal`; no duplicate owner exists elsewhere in source.

With the actual array owner known, the natural four-controller initialization
loop reproduces January's independent `input_abstraction_globals+0x68` end
comparison. The ordinary private default helper naturally receives its pointer
in EAX; no calling-convention annotation, assembly, false linkage, forced inline,
or synthetic call is introduced. Defaults use named game-control/gamepad and
joystick enums, 120/60 real look rates, and false inversion flags.

The aircraft predicate uses existing player/unit/vehicle/tag access macros.
January proves player unit index `+0x34`, parent object `+0xCC`, parent seat
`+0x2A0`, vehicle type `+0x2F4`, seat block `+0x2E4`, seat size `0x11C`, and driver
flag bit 2. Vehicle types 3/5 are human plane/alien fighter, independently named
by existing `vehicles.c` and HCEA. The input-local vehicle prefix consists only
of the true `struct unit_definition`, flags, and vehicle type, with a checked
type offset. It is explicitly a partial layout view, not complete Vehicle tag
recovery; it introduces no byte-offset access or padding-based shadow record.

The first aircraft body had identical structure but extracted a boolean bit.
January's final branch instead selects true only for the driver seat. Expressing
that natural branch reproduced its bytes. Its one remaining relocation was the
assertion: January's literal names `MAXIMUM_NUMBER_OF_LOCAL_PLAYERS`, not
`MAXIMUM_GAMEPADS`. Recovering the authenticated owner constant closes the
strict relocation identity as well.

## Update residual: behavior implemented, byte claim withheld

`_input_abstraction_update` is now implemented at **2,416 padded bytes / 114
relocations**, versus January **2,384 / 103**. Candidate normalized SHA-256:
`b313e9082f5a59d752fdaf147b03bb96b8e011b5caf7cfb1129256ea9f44f7c0`.
Target:
`7630e7c9cb2c1abfcce11af3bdf04a7e26ae5bf9a96b1286576b9444cd3c9cbe`.

The body covers controller sampling, rate copies, axis normalization/clamping,
button remapping, legacy diagonal snapping, aircraft inversion, all four joystick
presets, and disconnect UI/keyboard handling. Its initial tag-block lookup is
authenticated by January's first two calls even though the returned pointer
is not subsequently used. The code retains that validation access without an
invented unused pointer variable. Missing gamepads do not spuriously clear old
input state; January only updates their availability here.

Only two complete update formulations were compiled: initial typed reconstruction
(2,256 / 117) and an evidence-driven precision/control-topology correction
(2,416 / 114). No register/declaration-order/allocation/lifetime brute force or
compiler controls were attempted. The residual is not classified as merely a
scheduler tie: frame/selection/layout and relocation differences remain.

The precision pass recovers an important mathematical boundary. January's
10-degree constant is the promoted single-precision value
`0.1745329201221466`; its 35-degree window is promoted rounded 45 degrees minus
that value, `0.6108652651309967`, not a directly rounded 35-degree constant. The
reciprocal is `1.6370221996262229`. The three generated double constants now
match January exactly. Input axes, trigonometric results, and scale locals use
project `real` with the target's observed single-precision boundaries.

### Remaining constant-owner issue

The current target `_stick_direction_angles` label covers a 20-byte `.rdata`
aggregate: first the float `1/32767`, then the four actual diagonal angles
`+45,+135,-45,-135` degrees. January update references the normalization scalar
at owner offset 0 and the logical angle array at owner offset 4. Natural source
declares four angles and lets the compiler emit the separate normalization
literal. It does **not** invent a fifth angle to accommodate the current label.
This old synthetic owner boundary needs separate symbol/data adjudication;
no config rename or data exact credit for that aggregate is claimed here.

## Semantic probe and complete symbol checks

An isolated Unicorn COFF loader executed target and candidate update bodies with
stubbed subsystem APIs; no real game, UI, or network operation occurred.
`scratch/input_abstraction_semantic_probe_20260904.py` records 240 axis/button
cases (four presets, both ordinary inversion states, six d-pad combinations,
five representative stick vectors) and 16 disconnect scenarios (menu/client/
selection/previous availability combinations). Sampled float outputs are
numerically identical, maximum absolute axis difference **0.0**, with matching
button outputs and disconnect event traces. This bounded test is evidence,
not a proof over every input or a claim of byte exactness; aircraft predicates
are independently byte-exact and were not invoked by the sampled inversion mode.

Complete candidate code census: **no candidate-only code owners**. The opt-in
`--forbid-emitted-symbol _point_from_line3d` guard passes against the complete
symbol table, not merely target iteration. Strict before/after exact-set
comparison reports `lost_exact=[]` and exactly the three gains listed above.
`fake_match_scan.py --fail-on-findings` over the three changed C/header files
reports zero review leads. `git diff --check` passes.

## Header blast and final evidence

Direct `input_abstraction.h` consumers: Input Abstraction, Player Control,
Player UI, and UI Widget. The new prototypes precede the existing declarations;
no old declaration order is changed.

Direct pre-existing `player_control.h` consumers: Main, Following Camera,
Bored Camera, Director, HS Library External, Player Effects, Player Control,
Players, Game Engine, First Person Weapons, HUD, and AI Debug. Input Abstraction
now includes it too. The real-array externs are appended in the existing globals
section, using its already-available local-player bound. January update
relocations at +0x50/+0x57 establish the two actual array accesses. All direct
and transitive consumers require the orchestrator's full validation.

- Baseline object: `scratch/input-abstraction-baseline-20260904.obj`, raw SHA-256
  `1b710e17eed22de77f495d19604082e5ff4e00e4c910de9a2545553d6802fca1`.
- Final object: `scratch/input-abstraction-precision-20260904.obj`, raw SHA-256
  `ce885600778865bd6efe852cab2c7a59985950ad552bd48db7f1f84c005bf98f`.
- Final C SHA-256:
  `785ab933641107f954e7e996c6c6ce4fb471950e04028989870a81de8e0eaac0`.
- Input header SHA-256:
  `7883f7067bbf7eb899c13db297425b0b36ee444c75021b95f6ad3fda6a4c4d0a`.
- Player Control header SHA-256:
  `aa80740369b2fdf5fa7150f7df69c85fd8e38e34b57a14e36ef23da03756d40d`.

Reopen the update on new source/precision/ownership evidence; do not chase its
remaining x87 allocation and scheduling at the expense of small-family progress.
