# bipeds.obj sight-position packet, 2026-10-02

Lane `claude/remaining-frontier-20260926`. **Lane-verified, pending independent canonical reconciliation.** Not pushed.
Owner rulings of 2026-10-02, verbatim in `research/remaining_frontier_20260926/OWNER_PACKET.md`: Q-UP9 (investigate
the two direct parenthesised arguments), Q-UP10 (land form An), Q-UP11 (the four names), Q-UP12 (restorations in
the same packet), Q-UP13 (the third site, narrowly; the `biped_jump` restoration held).
Research record: `research/remaining_frontier_20260926/lead/upstream_punpck/cards/UP-13_biped_get_sight_position_direct_arguments.md`
and `OWNER_BRIEF_UP13_BIPED_SIGHT_POSITION_20261002.md` beside it.

| Function | Meaningful bytes | Padded bytes | Relocations |
| --- | ---: | ---: | ---: |
| `_biped_get_sight_position` (form An: the two approved sites and the coupled jumping correction) | 407 | 416 | 19 |
| `_biped_adjust_placement` (the third site, approved narrowly) | 88 | 96 | 2 |
| Total: 2 functions | 495 | 512 | |

The two `biped_find_ground_surface` restorations are zero-credit fidelity cleanup: the function was exact and its
section is byte-identical before and after.

## What changed

- `source/units/bipeds.c`
  - `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` is removed. In this unit `point_from_line3d` is again the
    shared-header `__inline` of `real_math.h`, which the compiler expands at some sites and calls at others, as
    January's object does (ten relocations to `_point_from_line3d` in four functions, expansions elsewhere).
  - `biped_get_sight_position`: the two hand-written expansions and their block locals `forward_distance` and
    `sideways_distance` become
    `point_from_line3d(sight_position, desired_facing, (desired_gun_offset->i), sight_position);` and
    `point_from_line3d(sight_position, &left, (desired_gun_offset->j), sight_position);`.
  - `biped_adjust_placement`: the hand-written expansion and its local `height_offset` become
    `point_from_line3d(&data->position, &data->up, (definition->biped.collision_radius), &data->position);`.
  - `biped_find_ground_surface`: the two hand-written expansions become
    `point_from_line3d(&origin, global_up3d, 0.4f, &origin);` and
    `point_from_line3d(&origin, &vector, result.t, point);`. The three expansion-only locals (`line_point`,
    `line_vector`, `line_t`) and the comment "Preserve January's inline schedule without owning point_from_line3d
    here" go.
  - `biped_update_jumping`: four constant locals in the jetpack block, `minimum_acceleration = 0.01f`,
    `maximum_acceleration = 0.05f`, `maximum_velocity = 1.4f`, `damping = 0.8f`, used where their values appeared as
    literals (`forward_velocity / maximum_velocity`, `damping - 1.f` twice, the third scale). The crouch brake is
    `else if (...)` with `&biped->object.translational_velocity` passed directly; its `!impulse` test and pointer
    local go.
- `config/parked.json`: the parks of `_biped_get_sight_position` and `_biped_adjust_placement` are retired
  (`tools.campaign.unpark --write`).

No cast is added. No compiler flag, precompiled-header setting, scorer, credit rule, header or `symbols.json` row
changes.

## Evidence

| Claim | Status | Basis |
| --- | --- | --- |
| Two calls of the helper in `biped_get_sight_position`, the scales passed directly, no local | attested by a later first-party build | the unoptimised build (2020), calls at 0x8c04ca and 0x8c04ec; neither scale is stored to a frame slot |
| One call in `biped_adjust_placement`, the scale passed directly | attested by the same build | call at 0x8bcbcd |
| The parentheses around the three scales | **INFERRED from per-site first-party codegen; not authenticated source spelling** | that build loads each scale before reserving its argument slot (`movss xmm0, [reg]; push ecx; movss [esp], xmm0` at 0x8c04b8, 0x8c04d9, 0x8bcbb1), the order a whole-value parenthesised lvalue produces; a macro that parenthesises its argument or a comma expression would leave the same trace |
| January needs them | measured | the second sight scale and the placement scale are defined through the FPU in January; with a bare argument both functions are residual. The first sight scale's parentheses change no byte of January and rest on the later build alone |
| Nothing else produces that definition | measured, sixteen spellings of the second sight scale with January's compiler | cast, unary plus, value-preserving arithmetic, an index, partial parentheses, a `(double)` cast and a local copy all compile to the previous text's section; only whole-value forms are exact |
| The compiler-version gap | **disclosed, not closed** | the unoptimised build was made with MSVC 19.00.24234, which is not on the measuring machine (whole-disk inventory: 13.00.8943, 13.00.9044, 13.00.9210, 13.00.9254, 13.00.9254.1, 19.12.25835 with an x64 target only, 19.51.36252). A 35-spelling lab on 13.00.9254, 19.12 and 19.51 agrees row for row; 19.00 itself is bracketed, not tested |
| Lifting the guard in this unit | owner ruling 2026-09-21 no. 1 | `units/bipeds.obj` is one of the 17 January objects that reference `_point_from_line3d` out of line |
| The new folded copy of `_point_from_line3d` | verified | 48 bytes, no relocations, COMDAT selection ANY, strictly equal to January's selected copy in `ai/action_charge.obj`; all 33 copies on the board are strictly equal to it; it receives no credit |
| Four constant locals in `biped_update_jumping` | attested by the later build, corroborated by January | four stores of 0.01, 0.05, 1.4 and 0.8 into frame slots that are never read (0x8c532c .. 0x8c535b); January pushes 0xbe4ccccc, which is `0.8f - 1.f` in single precision (the literal -0.2f is 0xbe4ccccd), and multiplies by the folded reciprocal of 1.4f |
| Why they matter | measured | in C `local - 1.f` is not a constant expression, so the inliner leaves the second call a call, as January has it; the literal `0.8f - 1.f` is folded by the front end and the call is expanded (the function is lost) |
| `const` on the four locals | inferred | a C++ compile at /Od stores a const-qualified local and folds its uses (lab on test code, MSVC 19.51); non-const locals give the same January bytes |
| The four names | **INFERRED descriptive names, not recovered Bungie identifiers** | no PDB records a local of this block (the demo and 2011 builds compile it out); read off the arithmetic: thrust = (1 - s)(0.01 + 0.05 s) with s = forward speed / 1.4 pinned to 0..1, lateral velocity kept at 0.8 per tick |
| The crouch brake as `else if`, address passed directly | attested by the later build; January cannot tell | the impulse branch jumps past the brake (0x8c5550); the address is computed twice (0x8c556e, 0x8c5583). With the constants as locals every spelling of the brake gives January's bytes; with literals only the previous `if (!impulse && ...)` plus a pointer local did |
| The third scale's expression | unchanged, January-exact; constants spelled by name | the later build computes `s*s*(-0.05) + 0.04*s + 0.01`; that order is not January's bytes, so it is not taken |
| The two `biped_find_ground_surface` calls | attested by the later build; byte-identical | calls at 0x8beb35 and 0x8bebc9; the function's section is identical before and after |

Differences of the later build that are NOT imported: its third-scale term order, its named local for that scale,
its missing rumble block, and its later crouch interpolation in the other branch of `biped_get_sight_position`.

## Not part of this packet

- The parenthesis class in general: still held. This approves three sites, one by one.
- The `biped_jump` restoration: held pending evidence and a ruling, because the genuine call there needs
  `(real_point3d *)&jump_velocity` twice. Its hand-written expansion and the local `velocity_delta` stay.
- The other hand-written expansions of the helper in this unit: `biped_fix_position` (four), `biped_snap_facing`
  (one), `biped_accelerate` (one), `biped_update_physics` (several). Writing the first two as calls loses their
  exactness through an operand-order tie inside a `cross_product3d` expansion elsewhere in the function; nothing
  was steered.
- The avoidance functions, the pill's negation parentheses, the two-local `cross_product3d`, the `ai_debug` casts.

## Lane gate

Measured first in a shadow of the lane HEAD (checkpoint `SH_Q9_AnL_GP` against `SH_Q9BASE`), then gated fresh in
the lane: `R116i_B` against `R115i_merge` (incremental build, objdiff 3.3.1). Zero exact losses.

- Stable diff: gained 2 (512 padded bytes), regressions 0.
- Functions 7,480 -> 7,482 of 7,573. Code 1,601,833 -> 1,602,328 of 1,770,166 (+495 meaningful bytes).
- Data 3,311,591 / 3,922,163 and objects 396 / 468 unchanged.
- Strict section owners 7,653 -> 7,655 of 8,252.
- Parks 61 -> 59, 0 stale, 0 invalid. Admission candidates 9, 0 contradicted, 0 rejected, 0 revoked.
- Tools tests 1,700 passed, 5 skipped. Fake-match scan: the same 26 inherited leads. Whitespace checks pass.
- One object differs on the whole board, `bipeds.obj` (1 of 621). Its changed function sections:
  `_biped_get_sight_position` and `_biped_adjust_placement` (the gains), `_point_from_line3d` (new, uncredited) and
  `_biped_update_physics`, a 5,376-byte residual that is not exact before or after and whose frame slots are
  renumbered (objdiff 88.408554% -> 88.398636%; its five real calls of the helper stay five).
- Identical before and after, and exact: `_biped_find_ground_surface`, `_biped_update_jumping`, `_biped_jump`,
  `_biped_fix_position`, `_biped_snap_facing`.
- Symbols of `bipeds.obj`: one new external definition (`_point_from_line3d`), one undefined reference fewer (the
  same symbol), nothing else: no new data, COMMON, constant or string.
- Real calls of the helper per function equal January's: `_biped_update_airborne` 1, `_biped_update_jumping` 3,
  `_biped_update_moving` 1, `_biped_update_physics` 5.
- Selected-provider pair link in both input orders: PASS for the new copy, and for all 84 surplus external
  definitions of the object. Whole-board diagnostic link before and after: LNK1120 419 = 419, the same 398
  unresolved names, LNK2005 only the inherited `_real_local_random`; no image was produced.
- /W3 4,082 = 4,082, no unit differs. /W4 for `bipeds.c` 101 = 101.
- C1 shift sweep (diagnostic only): the two gains, `_biped_update_jumping`, `_biped_find_ground_surface`,
  `_biped_jump`, `_biped_fix_position` and `_biped_snap_facing` are exact at all 64 shifts, so the result does not
  depend on a declaration count.
