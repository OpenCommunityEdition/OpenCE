# `bipeds.obj` batch-7 exact and fuzzy reconciliation (2026-09-12)

## Result

Against the batch-7 starting `build/report.json` state, `bipeds.obj` advances
from **26 exact functions** to **36 exact / 9 residual / 6 unwritten**. The ten
new strict owners contribute **2,981 meaningful exact code bytes** and **3,040
padded target bytes**. The meaningful figures come from the target function
`size` fields in `build/report.json`; section alignment is not credited as
meaningful code.

| January offset | Semantic owner | Linkage | Meaningful | Padded | Relocations | Normalized SHA-256 |
| ---: | --- | --- | ---: | ---: | ---: | --- |
| `0x190780` | `_biped_make_footstep` | private | 266 | 272 | 20 | `9ca826c266028d2ebf096c8194080858416f7bf4ee4bce1c56ebb25c38f0501a` |
| `0x1916e0` | `_biped_falling_danger` | public | 310 | 320 | 11 | `11b3e4ee1dca0cec06c0850c805f79d9e6a1cbbdecda4318472df09054821f0b` |
| `0x191820` | `_biped_vehicle_speech` | private | 427 | 432 | 21 | `bfe88cc6fa8b4ef188d940377a7c56455b1ac6f60f815a7011d0cd0bf68d7d22` |
| `0x1919d0` | `_biped_apply_rotation` | private | 298 | 304 | 9 | `35d6dd1953a903200ac1aad0076ffc4a92b8ae0a68e3971a1b2d29c9a82ccbe0` |
| `0x191b00` | `_biped_jump` | private | 429 | 432 | 10 | `8b12b750e880a57b0e96c576cf3790219d1f63737eae4b19255c99407f8bb99d` |
| `0x191cb0` | `_biped_try_to_make_footsteps` | private | 408 | 416 | 17 | `cc0fdad80586237445476a1b9607f6f8e3c89a0dd9d444ab67a355dad6c942e6` |
| `0x192170` | `_biped_update_airborne` | public | 345 | 352 | 20 | `c52eb65ddcbb139477ad361a3f1c248a489d4c49462edd250f1e50e20b64d95a` |
| `0x1922d0` | `_biped_update_landing` | private | 169 | 176 | 7 | `0d674d499d2be17aa09451a701a2cca9d665d6e2ee18082744dc9945da7acddc` |
| `0x192380` | `_biped_update_slipping` | private | 126 | 128 | 7 | `d4de48606368472bd8efac12d0c8fa2dfab4361fcd5c053560e7273b9a897f60` |
| `0x195b60` | `_biped_update_dead` | private | 203 | 208 | 12 | `e43c121f3fb32dc716ad097739235aff13d81fd3296624d883b17a9406e6243c` |
| **Total** | **10 owners** |  | **2,981** | **3,040** |  |  |

The semantic names are present in `config/symbols.json`; no definition uses a
`code` plus address identifier. `biped_falling_danger` and
`biped_update_airborne` are declared in the owning `bipeds.h`. The seven
private exact definitions and `biped_make_footstep` remain translation-unit
private because that is the natural linkage that reproduces January's bodies.

## Source and symbol evidence

The same-build January atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
maps the relevant offsets to the semantic Biped names. The HCEA PDB atlas and
the reconstruction files under `_research_cache/halocea/src/blam/units/`
independently corroborate the names, gameplay semantics, field use and control
flow. In particular, the following donor files were consulted as semantic
evidence rather than treated as interchangeable January code:

- `biped_make_footstep.c`;
- `biped_falling_danger.c`;
- `biped_vehicle_speech.c`;
- `biped_apply_rotation.c`;
- `biped_jump.c`;
- `biped_try_to_make_footsteps.c`;
- `biped_update_airborne.c`;
- `biped_update_landing.c`;
- `biped_update_slipping.c`; and
- `biped_update_dead.c`.

January disassembly, target relocations, target string literals and target
COFF sections remain the exactness oracle. The reconstruction uses project
`real` and typed Biped/tag accessors, named flag and enum constants, ordinary
multiline prototypes, and explicit terminal returns. It retains no inline
depth pragma, volatile/register steering, compiler barrier, raw byte emission,
address-derived identifier or implausible dead dependency.

## `_biped_definition` layout proof

The HCEA type-database reconstruction in
`_research_cache/halocea/src/headers/_biped_definition.h` (SHA-256
`24f12c92b7c10dadf05242a2082549cd51f379e53570e293d23a276e918daecb`)
records the complete Biped-specific definition as **0x204 bytes**, with these
post-collision offsets:

| Field | Offset within `_biped_definition` |
| --- | ---: |
| `collision_unused[10]` | `0x140` |
| `autoaim_width` | `0x168` |
| `runtime_cosine_stationary_turning_threshold` | `0x1d8` |
| `runtime_uphill_k1` | `0x1f0` |
| `runtime_pelvis_node_index` | `0x1f4` |
| `runtime_head_node_index` | `0x1f6` |
| `contact_points` | `0x1f8` |

The January outer `biped_definition` places this substructure at `0x2f0`.
Target instructions independently access `autoaim_width` at `0x458` and the
pelvis/head indices at `0x4e4`/`0x4e6`, exactly the sums implied by the HCEA
layout. A compensating tail would incorrectly move the later runtime fields
and contradict the database's 0x204 total. The natural HCEA-shaped
`biped_make_footstep` also becomes a strict 272-byte match only with this
complete layout, corroborating the `contact_points` location.

`fudge_vectors` is likewise restored as the authenticated 27-element
`real_vector3d const` data owner. Target and candidate have the same 324-byte
`.rdata` section, zero relocations and external COFF storage. It earns no code
credit.

## Header blast-radius gate

`biped_definitions.h` has seven direct C consumers and no intervening header
consumer. Every one was compiled with the focused gate after the complete
layout correction. The exact-function sets are supersets of the batch
starting report: **zero inherited exact functions were lost**.

| Consumer | Current gate result | Lost starting exacts |
| --- | ---: | ---: |
| `source/units/bipeds` | 36 exact / 9 residual / 6 unwritten | 0 |
| `source/units/units` | 189 exact / 0 residual / 0 unwritten | 0 |
| `source/units/biped_limp_noodle` | 3 exact / 3 residual / 0 unwritten | 0 |
| `source/ai/actor_perception` | 32 exact / 6 residual / 6 unwritten | 0 |
| `source/game/aim_assist` | 9 exact / 6 residual / 1 unwritten | 0 |
| `source/ai/ai_debug` | 56 exact / 4 residual / 0 unwritten | 0 |
| `source/game/players` | 54 exact / 15 residual / 1 unwritten | 0 |

Some non-Biped rows include unrelated concurrent gains relative to the older
`build/report.json`; none of those gains is claimed by this packet. The
important blast-radius result is the absence of any lost starting exact
owner, with Units still **189/189**.

## Honest fuzzy boundary

The complete, semantically credible residual bodies are retained for future
research and receive **zero exact code credit**:

| Owner | Target padded bytes | Current boundary |
| --- | ---: | --- |
| `_biped_accelerate` | 480 | Pre-existing residual; unchanged by this packet. |
| `_biped_adjust_placement` | 96 | Same 39-instruction envelope; one independent pointer-register allocation differs. |
| `_biped_check_discard` | 176 | Same padded envelope; formatted diagnostic argument evaluation order differs. |
| `_biped_find_nearby_support_surface` | 544 | Same 23 relocations and 175 instructions; one commutative x87 operand-load pair differs. The retained grouping follows the HCEA semantic expression and was not permuted for codegen. |
| `_biped_fix_position` | 1,232 | Same padded and relocation envelope with a narrow load/sign-extension schedule delta. Its discarded pill-base output is a genuine `real_point3d`, not a representation-punned vector. |
| `_biped_get_autoaim_pill` | 336 | Correct target field offsets and identical 131-instruction envelope; only the spherical-branch pointer/value register schedule differs. |
| `_biped_get_sight_position` | 416 | Correct target assertions and same padded envelope; the remaining delta is an x87/local spill choice. |
| `_biped_update_turning` | 1,712 | Complete donor-corroborated gameplay body, but still a normalized-code residual. |
| `_biped_update` | 1,120 | Complete natural body; candidate is 1,104 padded bytes and remains residual. |

No residual is counted as exact merely because it has the correct size,
relocation count or semantics. The 512-byte `_biped_update_jumping` stays
unwritten because the available HCEA donor only reconstructs the short jump
counter prefix and omits January's substantial cheat/jetpack and rumble logic.
The 5,376-byte `_biped_update_physics` is a large, fuzzy owner rather than the
requested small leaf and was not synthesized from incomplete evidence.

## Emitted-helper ownership boundary

The final candidate contains 25 helper code owners absent from the target
object. Seventeen were already present in the built starting candidate and
remain inherited, uncredited helper debt. This packet removes the former
candidate-only `_plane3d_distance_to_point` owner and introduces eight natural
helper COMDATs solely through the retained zero-credit fuzzy bodies:

- `_collision_test_line` from `_biped_fix_position`;
- `_cross_product2d`;
- `_dot_product2d`;
- `_magnitude2d`;
- `_magnitude_squared2d`;
- `_normalize2d`;
- `_rotate_vector2d`; and
- `_scale_vector2d` from the two-dimensional turning path.

These helpers are recorded explicitly so they cannot be mistaken for target
ownership or credited code. The exact-function credit above is based only on
target-owned, strict-equal sections.

## Focused verification

The final alias gate reports `36 exact / 9 residual / 6 unwritten` and passes
the emitted-symbol guard for `_point_from_line3d`. The TU uses the established
external-helper schedule for `point_from_line3d`, so no candidate definition
or COMDAT with that name is emitted. The seven-consumer layout sweep reports
zero inherited exact losses, and `git diff --check` passes.

This is a focused packet only. The integration owner still owns configure,
the full Ninja build, the whole-canonical accepted-ledger comparison, park
validation, test suite and publication verdict.
