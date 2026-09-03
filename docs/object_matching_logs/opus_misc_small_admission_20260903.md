# Opus miscellaneous small-unit admission, 2026-09-03

## Scope and provenance

- Initial admission-audit base:
  `36ec431656b1674265c0986938590e106994b894`.
- Rebased integration base:
  `332a39a56a5d3c5410bcf6743446492006c3de66`.
- Donor: `bb0d690a39b437c487c906e35d8658a0e94d7ba4` from
  `opus/small-families-30k-20260902`.
- Audited units: `progress_bar`, `rasterizer_debug`, `rasterizer_lights`,
  `rasterizer_text`, `render_objects`, `render_sprite`, `leaf_map`, and
  `tiff_file`.
- `rasterizer.c` was touched only to consume the seven Xbox dynavobgeom
  backend declarations from their narrow owner interface.

This was an admission audit, not a bulk cherry-pick. Reconstructed source was
retained only where it was coherent C, used semantic owners and types, and
survived the strict per-function comparison. Residual source is deliberately
parked rather than distorted for instruction selection.

## Strict function accounting

The byte column is the sum of strict-exact target function sizes. `E/R/U`
means exact/residual/unwritten.

| Unit | Base E/R/U | Base bytes | Admitted E/R/U | Admitted bytes | Net |
| --- | ---: | ---: | ---: | ---: | ---: |
| `progress_bar.obj` | 17/0/37 | 1,264 | 51/1/2 | 7,232 | +34 / +5,968 B |
| `rasterizer_debug.obj` | 5/0/7 | 128 | 12/0/0 | 3,136 | +7 / +3,008 B |
| `rasterizer_lights.obj` | 3/0/10 | 80 | 7/3/3 | 1,376 | +4 / +1,296 B |
| `rasterizer_text.obj` | 6/0/8 | 176 | 8/6/0 | 448 | +2 / +272 B |
| `render_objects.obj` | 4/0/18 | 128 | 19/3/0 | 2,928 | +15 / +2,800 B |
| `render_sprite.obj` | 1/0/9 | 144 | 6/4/0 | 1,312 | +5 / +1,168 B |
| `leaf_map.obj` | 1/0/24 | 48 | 7/4/14 | 1,264 | +6 / +1,216 B |
| **Total admitted set** | **37 exact** | **1,968** | **110 exact** | **17,696** | **+73 / +15,728 B** |

The net contains 75 new strict-exact functions totaling 15,792 bytes and two
intentional removals totaling 64 bytes:

- the base `code_000d16d0` was a bare address-named no-op with no authenticated
  semantics; it is again honestly unwritten, not deleted from the inventory;
- the base 48-byte `rasterizer_lights_reset_for_new_map` exact form knowingly
  cleared one complete record past the first array. The admitted implementation
  uses the array's real size and is parked as a semantic residual.

`tiff_file.obj` remains unchanged at 1/2/0 and 112 exact bytes. The donor made
`tiff_export` 1,040 bytes against January's 1,056 and closed nothing, so the
entire TIFF donor change was rejected.

## Source and symbol decisions

- Private owners were given restrained subsystem names. In particular,
  `draw_gravy_layer`, `render_object_list`, `render_object_shadow_end`,
  `object_render_state_refresh`, `leaf_face_get_vertex3d`, and
  `leaf_map_family_mark` now replace target address labels. Relocation evidence
  also corrected `leaf_map_family_mark` to target `0x181630`; the prior
  `0x182400` guess was returned to its unresolved inventory name.
- The cross-build name `lens_flare_evaluate_corona_rotation_function` was
  authenticated from the HCEA source corpus, but its Xbox body remains
  unwritten.
- `render_objects.c` uses a typed cached-render-state accessor rather than
  repeated raw `datum_get` casts. Sprite shader fields and sprite groups use
  typed structures and named enum constants, with size/offset assertions.
- Public declarations introduced by this wave live in dedicated owner headers.
  Adding them to broad `render.h`/`rasterizer.h` interfaces was measured to
  perturb unrelated C2 allocation (`units.obj` and `decals.obj` respectively),
  so the narrow headers preserve both ownership and the January schedule.
- The recovered `render_debug_triangle` interface lives in the narrow
  `render_debug_geometry.h` owner header. On the rebased declaration schedule,
  putting it in broad `render_debug.h` changed
  `unit_preprocess_node_orientations`; the narrow interface keeps Units
  189/189 without leaving the declaration in its consumer TU.
- The seven `_rasterizer_hud_*` / `_rasterizer_dynamic_*` dynavobgeom backend
  declarations requested by the integrator are now in
  `rasterizer_xbox_dynavobgeom.h`, included by both the owner implementation and
  `rasterizer.c`. Broad `rasterizer.h` placement regressed the otherwise exact
  400-byte `decals_delete_permanent_from_cluster` after the canonical rebase;
  the narrow owner interface restores `decals.obj` to 15/1/17 while leaving
  `rasterizer.obj` at 131/131 and dynavobgeom at 17/17.
- `leaf_map_leaf_spans_polygon` initially selected an unwanted
  `point_from_line3d` COMDAT. Its ordinary scalar expansion is now explicit;
  the rebuilt object contains no `point_from_line3d` definition or symbol.
- `progress_bar.c` refers to the named 18,396-byte `compressed_monk_tone`
  payload as an external asset. The split January object co-locates that data,
  but neither the donor nor the available cross-build corpus provides its
  source representation. No zero-filled stand-in, raw extracted byte table, or
  false completion claim was introduced; data ownership remains an explicit
  object-completion follow-up.

## Parked frontier

- `progress_bar`: `draw_gravy_layer` is a coherent 336-byte candidate for the
  352-byte target; `code_000d16d0` and `code_000d1930` remain unwritten.
- `rasterizer_lights`: three coherent residuals and three unwritten functions.
- `rasterizer_text`: six coherent residuals, no unwritten functions.
- `render_objects`: three coherent residuals, no unwritten functions.
- `render_sprite`: four coherent residuals, no unwritten functions. The donor's
  byte-closer for origin transformation wrote a `real_vector3d` through a
  `real_point3d` stack object; that undefined representation overwrite was
  rejected.
- `leaf_map`: four coherent residuals and fourteen unwritten functions.

## Verification

- `tools/fake_match_scan.py --fail-on-findings`: zero findings over admitted
  sources and owner headers.
- volatile-lifetime scan: zero removable or load-bearing sites.
- config inventory preservation: 23,181 base offsets, 23,185 admitted offsets,
  zero missing base offsets.
- direct-header blast: 61 translation units compared against the base with no
  regression; `game_state` is the same gate-path-with-space failure on both
  sides.
- transitive `leaf_map.h`/`structure_bsp_definitions.h` blast: 22 translation
  units compared against the base with no regression.
- sentinels: `units.obj` 189/189; `rasterizer.obj` 131/131.
- tooling tests: 85/85 passed (`fake_match_scan`, `coff_compare`,
  `stable_verdicts`, and `regression_gate`).
- `git diff --check`: clean.

### Post-rebase verification

- Canonical integration-base inventory: 23,189 offsets; rebased lane: 23,193;
  zero base offsets missing.
- Repeated strict gates retain the table above exactly. Additional integration
  sentinels are `rasterizer.obj` 131/131,
  `rasterizer_xbox_dynavobgeom.obj` 17/17, `decals.obj` 15/1/17, and
  `units.obj` 189/189.
- A canonical-versus-lane header blast covered 78 base / 80 admitted direct or
  transitive consumers. It found only the intended audited-unit gains and no
  loss. `game_state` remains the same path-with-space gate-harness failure on
  both sides.
- The fake-match scan again reports zero findings, the volatile scan reports
  zero sites, and the rebuilt `leaf_map.obj` has no `point_from_line3d` symbol.
- Tooling verification remains 85/85: 9 fake-match scanner tests and 76 COFF,
  stable-verdict, and regression-gate unit tests.
