# `glow.obj` house-rule reconciliation (2026-09-08)

## Result

The complete caller-connected Glow packet is now retained in natural typed C.
It advances `source/objects/widgets/glow` from 10/25 to **23/25 strict-exact
functions** without changing any of the ten inherited exact owners.

- **13 newly exact functions**;
- **3,680 padded / 3,592 meaningful exact code bytes** added;
- **4,832 padded / 4,650 meaningful exact code bytes** in the accepted Glow
  owner set after reconciliation;
- two complete fuzzy functions retained at **zero exact credit**;
- explicit `_point_from_line3d` emitted-owner guard: pass.

This is a function-level admission, not a whole-object exact claim. The
candidate still emits ordinary shared math COMDATs that are not owned by the
January Glow split object. They remain visible rather than being suppressed by
prototype shadowing, forced inlining, renaming, or source distortion.

## Authority and provenance

January's split object remains the byte, relocation, ABI and ownership
authority:

- `build/split/source/objects/widgets/glow.obj`;
- container SHA-256 recorded by the earlier audit:
  `593db0e7fde065d7426231e9e08aaed6360b790033077481e1d70044e54a638a`.

The semantic/source-shape evidence comes from two independent reconstruction
lines:

1. the HCEA Glow packet at
   `C:/Users/isabe/Documents/Codex/reference/halocea/src/` and the clean
   repository packet documented in
   `glow_obj_jonas_complete_hcea_packet_20260830.md`;
2. the complete caller-connected Opus lane at
   `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
   `bd8c008e85a1543877d0843bab3f1c2dffab9a13`, whose Glow source blob is
   `0789afe90403e9acc8d9b12b7444896a1503f2d2`.

The prior admission hold and corrected January control-flow analysis are in
`glow_obj_opus_source_admission_hold_20260906.md` and
`scratch/glow-caller-connected-opus1-evidence-20260906.md`. This pass resolves
the source-policy blockers rather than treating the donor's byte equality as
sufficient evidence.

## Strict exact inventory

The independent gate compiled the full current translation unit with only
diagnostic aliases from the new semantic private names to the old January
split labels. It reported 23 exact, two residual and zero unwritten bodies.

| Newly exact semantic owner | Target padded | Meaningful |
| --- | ---: | ---: |
| `_glow_trailing_particle_update_color` | 128 | 128 |
| `_glow_trailing_particle_update_size` | 96 | 91 |
| `_glow_trailing_particle_update_velocity` | 144 | 138 |
| `_glow_trailing_particle_update_position` | 64 | 61 |
| `_glow_trailing_particle_age` | 96 | 92 |
| `_glow_normal_particle_update_color` | 416 | 406 |
| `_glow_normal_particle_update_size` | 16 | 7 |
| `_glow_particle_new` | 64 | 52 |
| `_glow_normal_particle_new` | 480 | 467 |
| `_glow_normal_particle_update_position` | 560 | 546 |
| `_glow_particles_initialize` | 176 | 170 |
| `_glow_update` | 1,328 | 1,323 |
| `_glow_submit` | 112 | 111 |
| **Total** | **3,680** | **3,592** |

The ten inherited owners remain exact: `_glow_dispose`,
`_glow_initialize_for_new_map`, `_glow_dispose_from_old_map`, `_glow_new`,
`_glow_delete`, `_point_from_parametric_line`, `_glow_render`,
`_nonuniform_cubic_spline`, `_nonuniform_cubic_spline_vector3d`, and
`_glow_initialize`. Their total is 1,152 padded / 1,058 meaningful bytes.

The comparison candidate is
`scratch/glow-house-rules-independent-audit-20260908.obj`. Its container hash
is `8977e763ad876e048ab415290597ceec05652db63985677ef25ec8a45646594b`;
container equality is not claimed because scratch path/timestamp metadata can
differ. Section-aware normalized comparison is the acceptance authority.

## Two honest fuzzy bodies

Both complete bodies are retained for future research and receive no exact
credit. Their measurements are also pinned in `config/parked.json`.

| Function | Target padded / relocs / normalized SHA-256 | Candidate padded / relocs / normalized SHA-256 | Objdiff |
| --- | --- | --- | ---: |
| `_get_particle_world_position` | 1,744 / 43 / `84720a5067d22512436bb45e9b948155fa08a828b9adf592a50c11ddd885596b` | 2,528 / 55 / `fb7bb3c6b7236d625348b35cdfa4175844acc230855ecc85e7ca966dd3b518e0` | 49.07078% |
| `_glow_trailing_particle_new` | 608 / 26 / `dfe35f0bc1c0576d568472c8be04583e4651a2fe14810520891f9608bf86ccd2` | 608 / 26 / `bcf4cefdb82e1d1eb34388f794ca0cdcc98c1d01b635c44e8796e744e9e3e920` | 95.505615% |

`get_particle_world_position` deliberately repairs the shipped machine
behavior where the two-/three-marker paths consume uninitialized side controls
and one three-marker interval consumes an uninitialized knot. The retained
source initializes all spline controls, uses a defined increasing knot
sequence, and copies between named point/vector components instead of pointer
punning between incompatible types. January-confirmed y-base z-component
quirks remain because they are defined expressions and directly present in the
target. The larger candidate is therefore an explicit behavioral repair, not
a false exactness attempt.

`glow_trailing_particle_new` uses the complete natural HCEA-backed algorithm
and has the exact target envelope and relocation census. The Opus donor's
strict version was obtained by adding `t/i/j/k` spill temporaries expressly to
steer x87 stores. Those unauthenticated locals were removed. The resulting
scheduling residual is parked instead of receiving inappropriate credit.

## House-rule reconciliation

- All fourteen recovered private functions have semantic names in both the
  source inventory and `config/symbols.json`; none retain `code + address`
  labels, and each symbol is marked private/static.
- The zero/random and one/uniform particle-distribution values now use the
  correct enum names. The donor's reversed semantic labels were rejected.
- Typed `glow_get`, `glow_particle_get`, and `glow_definition_get` wrappers
  own raw datum/tag access. Repeated call sites no longer cast direct
  `datum_get` or `tag_get` results.
- The redundant local declaration of
  `bitmap_group_get_bitmap_from_sequence` was removed; its prototype remains
  in the owning bitmap header. Glow-private prototypes remain in `glow.c`, and
  the public API remains in `glow.h`.
- Counts, flags, boundary modes and distribution modes use named constants or
  enums; project `real`, point, vector and color types are used throughout.
- Function parameters follow the project multiline convention, and every
  function has an explicit terminal return.
- No volatile/register lever, inline assembly, raw address/layout access,
  point/vector pointer pun, undefined local read, or source-only fake branch
  is retained.

Several exact functions still initialize a local Glow definition even where
the body does not otherwise consume it, and `glow_submit` performs the second
typed Glow datum lookup. These calls are not credited merely because the donor
contains them: January's exact relocation streams independently require the
`tag_get`/`datum_get` calls at those positions. They are retained as
target-authenticated original redundancy, not invented matching anchors.

## Emitted-owner boundary

The independent gate command included
`--forbid-emitted-symbol _point_from_line3d` and passed. This preserves the
January inline schedule's explicit campaign guard.

The translation unit does emit nine other ordinary math COMDATs absent from
the January Glow split owner: `square_root`, `vector_from_points3d`,
`scale_vector3d`, `magnitude_squared3d`, `magnitude3d`, `normalize3d`,
`distance3d`, `cross_product3d`, and `real_local_random_range` (464 padded
bytes in the prior census). That ordinary shared-header behavior is not hidden
or credited as Glow ownership. Consequently this ledger claims only the 23
section-aware exact functions above, never whole-object equality.

## Reproduction

The exact gate uses `tools/campaign/gate.py source/objects/widgets/glow`, the
fourteen semantic-to-January diagnostic aliases listed in
`scratch/glow-caller-connected-opus1-evidence-20260906.md`, `--all`, and the
explicit `_point_from_line3d` guard. The independent run produced:

```text
emitted-symbol guard passed
exact 23  residual 2  unwritten 0  (of 25 listed)
```

A one-unit objdiff report against the same candidate is retained at
`scratch/glow-park-report-20260908/report.json`; it provides the meaningful
sizes and pinned fuzzy percentages above.

Final production verification passed:

- `ninja all_source progress semantic_progress`;
- stable census: 13 gained functions / 3,680 padded bytes / zero regressions
  across 8,245 target functions;
- semantic ledger: 1,035,078 meaningful exact bytes / 6,600 accepted exact
  functions;
- parked ledger: 351 active / zero stale / zero invalid;
- changed-source fake-match scan: zero review leads;
- tooling suite: 1,147 passed, two skipped, 26 subtests passed;
- `git diff --check`.
