# `geometry.obj` Claude convex-hull2d exact quintet (2026-08-31)

## Result

Claude/Fable campaign commit
`c8ff585ee941bd8454e991d7512b5b36ba0f161a` contains a bounded, ordinary-C
recovery for the small two-dimensional convex-hull prefix in `geometry.obj`.
Five bodies and the two authenticated initialized globals survive this audit.
The translation unit advances from `7/30` to `12/30` strict functions, with no
previous exact function lost, and remains correctly `NonMatching`.

| Function | Meaningful / padded bytes | Relocations T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_convex_hull2d_verify` | `248/256` | `4/4` | `34b2535697f5138443c71c1940318ef4479c5925876595d81130a547c58fe250` | new strict exact |
| `_convex_hull2d_test_circle` | `207/208` | `2/2` | `7785cf3f04fb5c7f61f3d9449ab75a08e9ef9b34208010754c8f9a8ac4e0bf45` | new strict exact |
| `_convex_hull2d_test_point` | `143/144` | `0/0` | `84761fad8dc9240cb8daa10cd0b6be9cf944a2f43818f90b2a1a92d732d51fed` | new strict exact |
| `_convex_hull2d_test_point_indexed` | `153/160` | `0/0` | `b9502f691fc60f2e201916952a36fbc74ab668c9a6e909789434fdee8b87b494` | new strict exact |
| `_convex_hull2d_test_vector` | `275/288` | `3/3` | `87c9770c978aa8b2a530efe4256257f389b4bb7a0b5b198651ada90997213d55` | new strict exact |

The wave adds `1,026` meaningful code bytes (`1,056` padded bytes).  The three
pre-wave residuals become exact and two previously unwritten target owners are
now emitted.  The seven inherited exact owners remain byte- and
relocation-identical.  The final isolated gate is `12` exact, `0` residual,
and `18` unwritten.

## Provenance and base identity

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `geometry.obj` SHA-256:
  `1afce0545c1e6d766152acf21abd191958962382ec390cfa831e056bae63e451`.
- Claude donor commit:
  `c8ff585ee941bd8454e991d7512b5b36ba0f161a` (parent
  `27b58838c3ed25afd6c8427606c24f977a4336ea`).
- Donor `geometry.c` blob:
  `3ff71f0604818919875dca51d385c94fa95959b7`.
- Donor-parent `geometry.c` blob:
  `6640c797bfbd2537630d442f78b55526b748de35`.
- The canonical pre-wave `geometry.c` was exactly that donor-parent blob; the
  accepted post-wave source is exactly the donor blob.  Thus this lane did not
  replay unrelated portions of the multi-object Claude commit.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86; `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The names of all five functions and both globals were already semantic in
`config/symbols.json`.  No address placeholder was introduced and no shared
symbol-map edit is needed.

## Independent HCEA evidence

The HCEA full-function cache supplies independent signatures and algorithm
intent.  These files were read from
`research-cache/halocea-full-blobs-20260830/src/blam/structures/`:

| HCEA evidence file | SHA-256 |
|---|---|
| `convex_hull2d_verify.c` | `6fff8afdbef5f78cb8f39ae9fb497b93b8857351905f3fc05ca132fefe8d949b` |
| `convex_hull2d_test_circle.c` | `2fc1402bea02a00b393ce7aa94c324cbb61b6af849bca88f746616c38b2b228b` |
| `convex_hull2d_test_point.c` | `5bdb8101c3c327ef8bcd5acd77105b005e16174046815600744372c1a19ac198` |
| `convex_hull2d_test_point_indexed.c` | `c85c42e255e3349c8eff15960a161e295fa7afa3b0c832301a191b6dc9467d32` |
| `convex_hull2d_test_vector.c` | `01098178ac04cf730fab9fa90bd1915f51d632d325b28a2d3c73eacc00367adf` |

That independent source corroborates the left-turn and full-angle verification,
zero-length-edge circle rejection test, direct and indexed point half-space
tests, and Cyrus-Beck-style interval clipping.  It also corroborates the
apparently odd but authentic unused `vertex_count`/`point_count` parameter in
`convex_hull2d_verify`; it is not dead-code invented for a match.

HCEA's reconstructed `test_vector` presents the nearly-parallel half-space
inequality with the opposite sign from January.  It is therefore used only as
signature and algorithm-family evidence for that branch, not as an exact
source oracle.  The retained ordinary typed C follows January's strict bytes
and relocation stream, whose numerator/denominator convention is internally
consistent with its selected polygon winding.  No HCEA decompiler artifact was
copied to force the result.

## Initialized-data proof

The accepted definitions are:

```c
real global_convex_hull3d_delta = 0.01f;
real global_convex_hull3d_epsilon = 0.001f;
```

January and the isolated candidate both emit one eight-byte `.data` section,
zero relocations, symbols at offsets `0` and `4`, raw little-endian bytes
`0ad7233c6f12833a`, and SHA-256
`fb0359fcb85d809ea332a663d32e3ea13efe6ade4f627f8449cb83d2b55530a5`.
This adds eight strict initialized-data bytes.

HCEA independently names and types both owners.  Its evidence files are:

- `research-cache/halocea-full-blobs-20260830/src/data/global_convex_hull3d_delta.c`,
  SHA-256
  `8465478c11525368015dc07c223d0209e3c02e1288c0457f43e41084e6a94d4b`,
  which records `0.01f` and image bytes `3C 23 D7 0A`; and
- `research-cache/halocea-full-blobs-20260830/src/data/global_convex_hull3d_epsilon.c`,
  SHA-256
  `325bbdd2f49692b3a8f5d2e001876588d8601bb14407e34ba78dd45c6265f54c`,
  which records `0.001f` and image bytes `3A 83 12 6F`.

Those image records are written in big-endian display order; their reversed
COFF byte order is exactly the candidate/target byte sequence above.  HCEA's
declaration inventory `src/headers/blam_data_globals.h` also contains both
semantic names (SHA-256
`c708887b567c78b6fd4cc1f0380d1365f3a4eb07bb046998cb0c6bd14d629df7`).

## Source-authenticity audit

The five bodies use repository-native `boolean`, `real`, `real_point2d`, and
`real_vector2d` types and the existing `cross_product2d` and
`angle_between_vectors2d` helpers.  Their loops, wraparound indices, early
rejects, and interval updates are normal geometric operations.  There are no
raw offsets, pointer-layout puns, address constants, unnamed code/BSS owners,
fake dependencies, dead branches, assembly, `volatile`, optimizer barriers,
forced inlining/noinlining, or declaration tricks.

The candidate-only `_cross_product2d` and inherited `_valid_real` helper
COMDATs are both ordinary 32-byte `.text` sections with zero relocations and
COFF selection `2` (`pick any`).  They do not usurp a January owner.
`_point_from_line3d` is absent, preserving the January inline schedule rule.

Only the needed public prototype and the two global extern declarations were
added to `source/math/geometry.h`.  The rest of Claude's header was not
imported, and no prototype was placed in a consumer translation unit.

## Header blast-radius proof

`geometry.h` has exactly two direct external C consumers:
`source/physics/breakable_surfaces.c` and
`source/structures/structure_lens_flares.c`.  Both were independently
recompiled after the owner-header additions with the PID-isolated campaign
gate and compared function-by-function with their pre-wave `build/base`
objects.

| Consumer | Target gate result | Emitted function sections | Changes versus pre-wave base |
|---|---|---:|---|
| `source/physics/breakable_surfaces` | `11` exact, `1` residual, `0` unwritten | `37` | none |
| `source/structures/structure_lens_flares` | `3` exact, `1` residual, `0` unwritten | `16` | none |

This explicitly checks the C2 definition-position hazard: neither an exact
owner nor an existing residual changed size, normalized bytes, relocation
count, or relocation identity.

## Validation boundary

Validation used `tools/campaign/gate.py source/math/geometry --all`, direct
`tools/coff_compare.py` checks of all five accepted owners, raw COFF
initialized-data comparison, candidate-only COMDAT ownership/selection
inspection, and isolated gates plus complete emitted-function comparisons for
both header consumers.  `git diff --check` passes for the source and header.

Per orchestration policy this lane did not run `configure.py`, Ninja, a full
board sweep, commit, or push.  The orchestrator owns those consolidated checks.
