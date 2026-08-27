# `hud_nav_points.obj`: update-switch recovery (2026-08-27)

## Outcome

`source/interface/hud_nav_points.obj` advances from 29/32 to 30/32
strict-exact functions. `_code_000c61d0`
(`hud_update_nav_point_local_player`) is now exact at 560 padded bytes and 25
relocations. The unit remains honestly `NonMatching`: the 400-byte
`_hud_render_nav_points` body remains non-exact and
`_custom_render_nav_point` remains unwritten.

The measured objects are:

- baseline: `build/audit/hud_nav_points_baseline_20260827.obj`, raw SHA-256
  `ab5c5013ac7a012693538d779f8f0900a67c190282d5cd4dd20d484c4026171e`;
- retained: `build/audit/hud_nav_points_update_exact_20260827.obj`, raw
  SHA-256
  `38ed342f191495ce55052e5dec514f38400e43d347d6312fb6e55d9a5baddc74`;
- January target: `build/split/source/interface/hud_nav_points.obj`, raw
  SHA-256
  `05f6bcce14e467330c57ee1a7bcd0e9ef8862a6f6818bf17fc252a8e67d095fa`.

The source moves from Git blob
`4a634d6ea10b2232679bd23fc59d9eee85a0afb2` to
`5963f46d0291a56586e8ab7cc72bac2e4e6cfbc3`.

## Evidence and recovery

January's `_code_000c61d0` and the inherited reconstruction were already
equal in padded size and relocation count. Their disassemblies were
instruction-identical except for the default edge of the nav-point-type
switch:

- January's type values other than flag, object, or game-engine flag branch
  to the common `position.z += z_offset` and visibility-update tail;
- the inherited C spelling used `default: continue;`, branching directly to
  the next slot.

Changing that default to ordinary `break;` reproduces January exactly:

| Measurement | Target | Baseline | Retained |
| --- | ---: | ---: | ---: |
| padded size | 560 | 560 | 560 |
| relocations | 25 | 25 | 25 |
| normalized SHA-256 | `25a9810cd7147ff55aa9fffce8cb0ee232afea4b20345b0382dad63e74a84a4a` | `bae2ddbc28ddc4eee2282a19aa6e8ea8871ab67e88df0d1fb6caa1c4fd476163` | `25a9810cd7147ff55aa9fffce8cb0ee232afea4b20345b0382dad63e74a84a4a` |

The independently reconstructed build-2276 source in
`research-cache/stian-halo-current-20260827/src/halo/interface/hud_messaging.c`
corroborates the fall-through behavior: its nav-point update switch has cases
0, 1, and 2 and no default-to-next-slot edge. The public HaloCEA reconstruction
at
`src/blam/interface/hud_update_nav_point_local_player.c` independently has
the same switch topology. January remains the admission authority.

The raw object access in this translation unit is now routed through the typed
`hud_nav_object_try_and_get` subsystem macro. It wraps
`object_try_and_get_and_verify_type` with the all-object mask and returns the
translation unit's verified object view. This is a source-policy correction;
the macro expansion preserves the target call and field access.

## Render-loop bounded investigation

`_hud_render_nav_points` remains:

| Measurement | Target | Retained |
| --- | ---: | ---: |
| padded size | 400 | 416 |
| relocations | 17 | 17 |
| normalized SHA-256 | `f2bb689a08030b4b54e28e13307de141eab53b71b0acc334bc2bec2f1dd78391` | `56bf4e1f33a71e475c6a3108db3ca65ad4e44bfc8d23b8443e87d24e31eaa70b` |

January carries the nav-point induction pointer biased to
`reference_index` (+8), then addresses the other fields at -8, -6, and -4.
The inherited typed source keeps the same semantic structure but VC7 retains a
datum-base induction pointer. Stian's machine-code lift corroborates the +8
cursor, while HaloCEA supplies an array-and-field-value reconstruction.

Three evidence-bounded legal-C shapes were measured:

1. a scoped typed pointer to `reference_index`: codegen unchanged;
2. explicit `reference_index` and `type` values: 384 bytes and a different
   register/lifetime topology;
3. only an explicit `reference_index` value: 400 bytes and 17 relocations,
   but a different EBX/ESI/EDI allocation and instruction schedule.

A loop-carried typed member pointer again optimized to the inherited 416-byte
body. These rejected shapes were removed. Reopen the render loop only with
new original-source provenance or a legal typed source topology that explains
the +8 induction pointer; raw pointer arithmetic, container reconstruction,
pointer punning, synthetic anchors, or byte forcing are outside house rules.

## Gates

- `halobetacache_build` and `libcmt_build`: pass, no work remaining;
- report generation and semantic audit: pass; 470 units, 4,398 functions
  evaluated, 4,254 semantic exact, 4,314 accepted exact, zero unit errors;
- ordinary configured headline: 377/833 complete objects, 4,287/11,060
  functions, 515,333/2,198,102 code bytes;
- Halo headline: 275/468 complete objects, 4,120/7,574 functions,
  502,419/1,770,166 code bytes;
- object admission: zero candidates, contradictions, or revocations;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected Units/game-engine/AI source, configuration, semantic ledger,
  comparator, admission rule, parked manifest, compiler setting, or matching
  label changed; no push.

## House-rule audit

The retained source remains C89. Every parameter stays on its own line, no-arg
functions retain `void` on its own line, and every function retains an
explicit terminal return. Typed tag-block access remains through
`TAG_BLOCK_GET_ELEMENT`; typed object access now goes through the local
subsystem macro. The change adds no assembly, `register`, `volatile`,
barrier, pragma, explicit intrinsic, attribute, raw address, pointer/integer
reconstruction, pointer or union pun, undefined behavior, synthetic anchor,
byte forcing, object patch, compiler substitution, or matching exception.
