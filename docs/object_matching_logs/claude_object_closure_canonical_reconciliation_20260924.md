# Claude object-closure reconciliation (2026-09-24)

Claude's ordered commits `c3277f6d` and `bac75a16`, based on `5e81dabf`,
were integrated into canonical after the later local object-admission batch.
Only `config/config.json` conflicted: the resolution retains canonical's
`action_obey` completion and Claude's `action_flee`/`action_guard` completions.
No Claude source or symbol change had already landed in canonical.

## Accepted result

- `action_flee` 13/13 and `action_guard` 15/15: January COFF storage class 3
  confirms the four newly marked private functions. No new code bytes.
- `aim_assist` 16/16: `compute_composite_attenuation` has January's public
  storage class; `actor_moving` now obtains `arccosine` from the genuine
  shared header instead of a hand-written NODUP copy. No new code bytes.
- `action_vehicle` 17/17: the /Od-attested helper/caller reconstruction
  closes `_action_vehicle_find_destination` for **1,225 meaningful / 1,232
  padded new exact code bytes**. The hand-written `point_from_line2d` copy is
  replaced with the genuine inline helper. Its newly emitted
  `point_from_line3d` and `scale_vector3d` sections are byte-identical to
  January's selected copies and ordinary provider links pass both orders.
  All 18 new 3D/2D view casts are per-site /Od-attested and byte-inert;
  removing them preserves the exact body but produces incompatible-pointer
  warnings. No new fake-match scan lead was found.
- `path_smoothing` stays incomplete: its `_cross_product2d` still collides
  with the hand-written `actor_combat` provider. Claude retained the best
  scratch candidate for `_actor_aim_grenade` at zero credit.

## `distance3d` provider conflict

The merged ordinary link audit found a blocker that neither lane saw in
isolation. January selects exactly one `_distance3d` provider, in
`action_vehicle`, 64 bytes with no relocations (normalized SHA prefix
`4cdb6852`). Our exact ActionVehicle body matches it. `leaf_map` and
`biped_limp_noodle`, however, each emit a 64-byte SELECT_ANY copy with a
different SHA prefix (`01d7809a`), reflecting a different x87 evaluation
order. Both pair links with ActionVehicle fail `LNK2005` in both input orders.
The census found 16 further incompatible copies in units still marked
`NonMatching`; no other `Matching` unit emits one.

The genuine `real_math.h` inline body was tested in an isolated worktree as
`square_root(distance_squared3d(a,b))`, replacing ActionVehicle's hand-written
provider. That makes all three named units' target functions exact and the
selected COMDAT byte-identical; their ordinary pair links then pass. But the
full 8,245-function sweep loses two previously exact functions:
`object_lights::_find_point_lights_for_object_in_cluster` (560 padded bytes)
and `rasterizer_frame_statistics::_rasterizer_frame_statistics_draw` (4,176).
It also stales the `_ai_test_line_of_sight` park, so the global header edit was
**rejected**. The other natural trials either lose ActionVehicle's exact
provider or LeafMap's exact caller. No forced-inline, emission-suppression,
hand expansion, or COMDAT metadata steering was admitted.

Consequently `leaf_map` and `biped_limp_noodle` are now `NonMatching` with
explicit owner rejections. Their 25/25 and 6/6 target function matches remain
credited; LeafMap retains its 1,420 meaningful new code bytes and verified
2,512-byte profile data. This corrects the earlier admission ledgers rather
than erasing useful source progress.

## Merged verification

After both Claude commits and the owner correction: full `ninja` passes;
stable diff against `scratch/ten_more_exact_20260923.json` gains only
`_action_vehicle_find_destination` (1,232 padded bytes) with **zero function
regressions**. The board is **7,577/8,245** strict functions and Halo code
**1,549,953/1,770,166** meaningful bytes, with **439/833** complete objects
(Halo-owned **329/468**). The previous display of 441/833 was two
provider-incompatible labels too high. Parks remain 103 active, zero
stale/invalid; pytest passes 1,154 tests, with five skips and 26 subtests.
The pre-existing 23 data-incomplete admission contradictions remain separate
and are not counted as completed objects.
