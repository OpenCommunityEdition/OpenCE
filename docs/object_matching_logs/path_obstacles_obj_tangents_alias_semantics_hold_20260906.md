# PathObstacles tangents and alias-safe rotation: integration hold (2026-09-06)

## Result

Baseline is published `b185898cea1a11c7cf78464adbc3c7fb28bcb6e2`.
Two genuine source reconstructions were tested, not a spelling sweep:

| Function | January meaningful / padded | First natural pair | Alias-correct header trial |
| --- | ---: | --- | --- |
| `obstacles_disc_tangents` | 211 / 224 | strict exact | strict exact |
| `circle_tangents` | 124 / 128 | residual, 112 padded | residual, 128 padded |
| `rotate_vector2d` | 42 / 48 | inherited header body, residual | strict exact |

Neither implementation packet is integrated. The genuine public prototype in
`path.h` changes an inherited exact Units function's instruction schedule.
The subsequent semantic correction to `real_math.h` repairs rotation aliasing
and matches the original helper, but its full rebuild loses four other strict
functions. Preserve both complete candidates and their evidence; do not count
their 253 new exact bytes in canonical progress or conceal the regressions.

The independently authenticated `circle_tangents` name and static ownership
can be retained separately in the source symbol comment and `config/symbols.json`.
The implementation is still unwritten in canonical, not complete.

## Original evidence

The January atlas, relative to the outer `i-w` workspace, is
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`.
Lines 553 and 558 authenticate `@circle_tangents@24` at image offset 0x450610
and `_obstacles_disc_tangents` at 0x450C10, both from `path_obstacles.obj`.
The private symbol has file offset 329232 in the selected January image.

`research/tools/DIA2Dump/x64/Release/DIA2Dump.exe -sym circle_tangents` and
`-lines circle_tangents`, against
`research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`,
authenticate the six-parameter static void signature, not specified inline,
in `d:\projects\code\hcex\sources\ai\path_obstacles.c`, lines 73-86,
source MD5 `2E121D7BFA3CC1A475C9AC04A172031A`. DIA also confirms the public
seven-parameter signature. The January public caller proves the optimized
private convention: ECX direction, ESI right output, EDX left output, stack
distance/radius/tangent-distance. No explicit calling-convention annotation
or private export is needed in C.

Later semantic references, relative to the outer workspace:

- `research-cache/halocea-full-blobs-20260830/src/blam/game/obstacles_disc_tangents.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/physics/circle_tangents.c`;
- `research-cache/halocea-full-blobs-20260830/src/blam/math/rotate_vector2d.c`.

Do not copy their later zero-distance guard: January divides radius by
distance unconditionally. Its clamp preserves an unordered ratio, matching
the established `MIN(radius / distance, 1.f)` macro operand order. The public
function uses the real 24-byte disc accessor and its header assertion, forms
center-minus-point, normalizes using the existing helper, and adds the genuine
1/256 radius margin. Normalization's tiny-distance branch sets only the returned
distance to zero; it does not overwrite the small vector components.

## Preserved natural source candidate, NOT integrated

The public declaration belongs in `path.h`'s `PATH_OBSTACLES.C` section; the
private prototype belongs in `path_obstacles.c`. The ordinary definitions were:

```c
void obstacles_disc_tangents(
    struct obstacles const *obstacles,
    short disc_index,
    real_point2d const *point,
    real radius,
    real_vector2d *right_direction,
    real_vector2d *left_direction,
    real *tangent_distance)
{
    struct obstacle_disc const *disc = obstacles_get_disc(obstacles, disc_index);
    real_vector2d direction;
    real distance = normalize2d(vector_from_points2d(point, &disc->center, &direction));

    circle_tangents(&direction, distance, radius + disc->radius + 0.00390625f,
        right_direction, left_direction, tangent_distance);
    return;
}

static void circle_tangents(
    real_vector2d const *direction,
    real distance,
    real radius,
    real_vector2d *right_direction,
    real_vector2d *left_direction,
    real *tangent_distance)
{
    real sine = MIN(radius / distance, 1.f);
    real cosine = square_root(1.f - sine * sine);

    rotate_vector2d(direction, -sine, cosine, right_direction);
    rotate_vector2d(direction, sine, cosine, left_direction);
    *tangent_distance = cosine * distance;
    return;
}
```

The private function's remaining gap includes January spilling `cosine` to a
real-sized stack slot while the candidate retains it on x87. Equal padded size
does not prove equal behavior at extended-precision boundaries. No volatile,
forced spill, register annotation, altered parameter order, fabricated call,
new alias, forced inline/noinline, assembly or comparator waiver was tried.

## Newly proven rotation bug and one correctness trial

The August 26 direct-body rejection at
`path_obstacles_obj_jonas_point_setter_20260826.md` was read in full. It tried the
existing header body once and deferred further work pending new evidence.
The new evidence is semantic: that body stores `result->i`, then rereads
`v->i` while computing j. It is wrong for `result == v`. January computes both
components before either store, preserving in-place rotation. Its real bytes
compute j before i, then store i followed by j. The later decompiler's displayed
j-first stores are also not a trustworthy alias-safe C implementation.

One natural correction to the genuine header definition was tested:

```c
    real j = sine*v->i + cosine*v->j;
    real i = cosine*v->i - sine*v->j;

    result->i = i;
    result->j = j;
    return result;
```

It reproduces all 48 padded target bytes, zero relocations, normalized SHA-256
`9a9864975806d5697fb568f3ea1aadee7f934119b5ebcd0b4aad5edb307aec3a`.
This is a correctness repair, not permission for more source-order variants.
It is not retained in canonical because the full strict regression gate fails.

Independent and root real-byte replays execute ten float-bit cases, in-place
and with separate output, under FPCW 0x027F and 0x037F, for the January,
repaired and genuine pre-repair bodies: 120 executions. Target and repaired
results/write traces/ABI checks agree throughout. The original reconstructed
body fails eight alias cases, including generic and positive/negative quarter
turns under both modes. This is a bounded demonstration, not exhaustive IEEE
behavior or permission to waive the shared-header regressions.

The reusable verifier is `tools/audit/rotate_vector2d_alias_runtime.py`, with
`tools/test_rotate_vector2d_alias_runtime.py`. It requires the locally pinned
original/candidate objects, refuses changed evidence, and refuses to overwrite
reports or write outside scratch. Tests needing original evidence explicitly
skip when it is absent; this does not imply the decompilation is complete.

## Whole-tree boundaries

First public/private trial: a full 51-consumer comparison finds only the
intended PathObstacles additions, the already-fuzzy speech-timer load-order
change, and the inherited exact `_unit_preprocess_node_orientations` change.
The latter remains 1920 padded bytes / 87 relocations; the complete disassembly
diff transposes only two independent loads at +0x269..0x26E:
`mov ecx,[ebp-0xC]` and `mov edx,[ebp-4]`. Even a likely harmless instruction
permutation is still a strict loss and is not waived.

The shared-math correctness trial rebuilds 262 real header consumers. Against
the original baseline it gains the public tangent and rotation functions, but
loses these four inherited strict functions:

- `decals::_decals_delete_permanent_from_cluster`;
- `game_engine_race::_race_touch_flag`;
- `game_engine_race::_race_engine_player_update`;
- `ui_widget_game_data_input_functions::_solo_level_select_list_update_displayed_items`.

Units returns to its original exact form in that trial; this incidental result
does not justify sacrificing the other functions. An attempted all-572-base
deep fingerprint additionally encountered a hardened relocation-resolver order
error; it did not finish, so there is no claim of a passing full COFF audit for
the rejected trial. The complete strict sweep independently establishes the
four losses. No active Opus implementation or donor worktree was edited.

Both production body/header trials are restored. `config/parked.json` is
unchanged: these held source candidates are not approved fuzzy implementations
and the speech-timer evidence must return to its already-pinned baseline.
Reopen for genuine header/owner reconstruction that preserves inherited exact
work, or explicit policy direction; do not shuffle declarations for C2 score.

## Frozen local artifacts

- `scratch/path-obstacles-tangents-before-20260906.json`:
  `1f00a0def09f98cefd9d399a2ee91e662a82a8a793de42eb90ba0f1ce0c15124`.
- `scratch/path-obstacles-tangents-first-20260906.json`:
  `dfe571d5c70d104d2ef90db1cd1f33f9e8ab45e378b270ed40281e56fd12ec3b`.
- `scratch/path-obstacles-tangents-rotate-before-20260906.json` (all 572 bases):
  `1c2108dccba64873f29ed45c0668b7e4daf7d0c048dc68a0514bcff472cc9c5f`.
- `scratch/path-obstacles-tangents-rotate-first-20260906.json` (all 572 bases):
  `fe81aac672fa7cfea243f187943d306960948e07fa6038f486fab37157f2d2a1`.
- `scratch/path-obstacles-tangents-{before,first}-stable-20260906.json` and
  `scratch/path-obstacles-tangents-rotate-first-stable-20260906.json`.
- `scratch/path-obstacles-tangents-full-consumer-first-review-20260906.json`.

The manifests preserve exact C/header/config candidates and compiled objects.
The source excerpts above remain available in Git even though original
binary evidence and temporary compiled objects stay local.

Independent alias report: `scratch/path-obstacles-rotate-alias-runtime-independent-20260906.json`,
SHA-256 `9b246ab5336475610854c2f652eec59fa45816d97fedafb0105e2f395d5e1235`.
Root replay: `scratch/rotate-vector2d-alias-runtime-root-publication-20260906.json`.
Restored capture: `scratch/path-obstacles-tangents-restored-20260906.json`,
SHA-256 `66d6d157b30063999a5fb3808d1a3b307d490766b875c72a9b1be64f0ab648f2`.
The restored full build and strict sweep pass, with 6,271 exact functions,
zero gains and zero regressions. All 572 current base objects are identical
to their original baseline objects apart from the COFF compilation timestamp.
Only the selected PathObstacles object changes among 833 targets: its 28
section headers/payloads and relocation records remain identical, with only
the authenticated helper name and static symbol metadata changed. The two
headers have no Git-content delta; the restored math header has incidental
line-ending normalization only. Parked evidence is unchanged and valid.
The full tooling suite passes: 654 tests plus 26 subtests, including the
new fail-closed loader and original-byte alias regression tests.
