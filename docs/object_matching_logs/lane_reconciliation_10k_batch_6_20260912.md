# Sixth 10K lane-reconciliation batch (2026-09-12)

## Canonical result

The frozen published base is
`a565219da15d4c5360c6021714429573b49e95fb`, with 6,828 accepted functions
and 1,103,239 meaningful accepted exact code bytes. The reconciled build has
**6,888 accepted functions and 1,113,556 meaningful bytes**: **+60 functions
and +10,317 bytes**.

The independent fail-closed target-section census records **60 new strict
functions, +10,383 meaningful bytes, +10,880 padded bytes, and zero
regressions**. Both accounting systems therefore cross the user-requested
10,000-byte publication threshold.

| strict packet | functions | meaningful | padded |
|---|---:|---:|---:|
| `path_obstacles` | 2 | 1,228 | 1,248 |
| `xbox_texture_cache` | 1 | 493 | 496 |
| `editor_flying_camera` | 1 | 120 | 128 |
| `observer` | 6 | 1,032 | 1,072 |
| `profile` | 5 | 231 | 288 |
| `devices` | 1 | 288 | 288 |
| `decals` | 6 | 2,004 | 2,048 |
| `input_xbox` | 2 | 262 | 288 |
| `first_person_weapons` | 3 | 688 | 704 |
| `ui_widget_game_data_input_functions` | 2 | 592 | 608 |
| `object_lights` | 3 | 584 | 592 |
| `rasterizer_xbox` | 21 | 257 | 480 |
| `render_debug` | 6 | 2,220 | 2,256 |
| `unit_dialogue` | 1 | 384 | 384 |
| **total** | **60** | **10,383** | **10,880** |

## Exact-accounting reconciliation

The strict and accepted-ledger totals overlap differently; nothing is added
twice:

- `xbox_texture_cache::__texture_cache_bitmap_get_hardware_format` contributes
  493 meaningful bytes to this batch's strict census, but batch 5 had already
  accepted it through ordinary objdiff/COFF-shape evidence. Its semantic-name
  repair therefore adds no new accepted-ledger bytes here.
- `observer_update` is now strict for 292 meaningful / 304 padded bytes. The
  January assertion literal directly proves the original no-space `==`
  spelling that `match_assert` stringizes; both string relocations now agree.
- `render_debug_decals` contributes 427 meaningful accepted bytes in the
  semantic tier, but the strict comparator remains fail-closed. Its 432-byte
  code SHA and all 27 relocation offsets, types, counts, and resolved final
  addresses agree. The sole symbolic difference is an unavoidable same-address
  representation: `_debug_decals` versus `_decal_geometry-24` for the
  authentic predecessor-vertex expression. The public Decals declaration, HS
  registration, and January address establish `debug_decals`; no source trick
  or forced alias is retained.

Thus the accepted delta is exactly
`10,383 - 493 + 427 = 10,317` meaningful bytes, and
`60 - 1 + 1 = 60` functions.

## House-rule and ownership dispositions

- Private target placeholders in Profile, Input, first-person weapons,
  Rasterizer Xbox, and fog wind update now have semantic names and correct
  static/external linkage evidence. The Path Obstacles color table and
  `debug_decals` BSS owner are likewise named instead of left address-derived.
- Public declarations were moved from consumer-local prototype blocks to the
  appropriate camera, editor-camera, Decals, Render Debug, Geometry, and Units
  headers. Full rebuilds preserve the inherited target functions.
- Typed object/tag access uses the established subsystem accessors. The one
  newly found Geometry point/vector representation cast was rejected and
  replaced by a typed `real_vector3d` local before publication.
- First-person weapons no longer emits its six surplus math COMDATs. Narrow
  declaration-only schedules in `real_math.h` preserve the January owner
  boundary without forced inline/noinline or manual arithmetic expansion.
- Decals' exact projection caller naturally emits candidate-only
  `vector_from_points2d` and `cross_product2d` SELECT_ANY bodies. They receive
  no credit and are explicit whole-object admission rejections; Decals is not
  claimed Matching.
- Observer emits no `point_from_line3d`, `sine`, or `cosine` owner. Its
  `observer_collision_test_differential` body remains unwritten because the
  January relocation graph contains no genuine caller; no fake retention is
  introduced.
- `units.obj` still has all 189 target functions exact, but its inherited
  candidate-only `point_from_line3d` COMDAT remains an explicit whole-object
  rejection. This batch does not claim Units as Matching.

The only fake-match scanner leads are Profile's target-proven `RDTSC` helper
and Decals' target-proven x87 color packer. Both fall within the house rule
allowing sparse assembly for low-level/math helpers. No volatile steering,
raw-byte emission, forced inline/noinline, invented address names, or
nonsensical exact source was admitted. The tempting RenderSprite
representation-punning closure and several source-pressure experiments were
restored and receive zero credit.

## Retained fuzzy source

Complete or best-reviewed non-exact bodies are retained and recorded in
`config/parked.json` with zero exact credit. They cover the remaining Bitmap
Extract page builders; three editor-camera routines; eight Observer residuals;
Input rumble flush; first-person weapon update; Object Lights preprocessing;
four Geometry routines; the Rasterizer Lights occlusion test; two Render
Cameras routines; Render Debug player; three Unit Dialogue routines; and the
newly exposed Texture Cache residuals. The fog wind helper is separately
documented as a dormant, exact-when-observable private body that receives zero
board credit until its genuine caller naturally retains it.

## Publication gate

- Full configure/Ninja source build and semantic/progress reports: pass.
- Stable target-section sweep: 60 gains / 10,383 meaningful / 10,880 padded /
  zero regressions.
- Semantic accepted ledger: +60 functions / +10,317 meaningful bytes.
- Units focused gate: 189 exact / 0 residual / 0 unwritten.
- Object-admission audit: no candidates, contradictions, or revocations;
  explicit whole-object rejections remain visible.
- Park-manifest validation: no stale or invalid entries after reconciliation.
- Tool tests: 1,144 passed, 5 skipped, plus 26 subtests passed.
- `git diff --check`: pass.

The canonical treemap is refreshed at `http://127.0.0.1:8765/`. The next
10,000-byte cadence threshold is **1,123,556 meaningful accepted exact bytes**.
