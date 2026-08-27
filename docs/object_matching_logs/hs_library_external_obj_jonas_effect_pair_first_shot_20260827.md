# `hs_library_external.obj` Jonas effect-pair first emitted object (2026-08-27)

This ledger records one frozen source experiment for `_hs_effect_new` and
`_hs_effect_new_from_object_marker`. Both functions are strict exact in the
first object emitted by the compiler. `hs_library_external.obj` advances from
17/36 to 19/36 accepted functions and from 1,088 to 1,312 padded exact bytes.
The unit remains `NonMatching`; no whole-object or data completion is claimed.

## Authority and frozen inputs

- Cumulative base: `037f7d9498fcc97438e0a3767ba49d4f5e659cde`.
- Baseline source blob: `6f78094eba26652616bd632981d3bca19649499c`.
- Retained source blob: `2b30b4f352e59808c21024aba6f75344f889d7de`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Whole-TU baseline:
  `build/audit/hs_library_external_effect_pair_baseline_20260827.json`,
  SHA-256
  `f37acc78efa1feb000a9b9cd2ff5a5e4b09dce4492d62572f5a21407a1f62a71`.
- First emitted object:
  `build/audit/hs_library_external_effect_pair_first_shot_20260827.obj`,
  SHA-256
  `6c6f7e03008885acdb69fcc82b26baf7d896364a1ef1f033b5b2266e5adf3d3a`.
- Compiler: pinned Microsoft VC7/XDK through the ordinary Ninja edge.

January COFF is the authority for padded bytes, meaningful extents,
relocation address/type/destination/addend, ABI, linkage, and ownership. The
only implementation file changed is `source/hs/hs_library_external.c`; this
ledger is the only other tracked file.

## Documentation and donor screening

All three available Claude guidance files were read before mutation:

- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`;
- `research-cache/stian-halo-current-20260827/CLAUDE.md`;
- `build/audit/refs/stian-halo/CLAUDE.md`.

The latter two are byte-identical. The older copy differs only by the newer
VC71 shape-warning and parallel-regression-harness guidance. The applicable
rules here are binary authority, exact struct offsets, C89 declarations,
call-site verification, small source changes, no inline assembly, and the
effect-marker buffer warning in `docs/lift-learnings.md` section 48.

That warning does not require a workaround in this pair: each effect call has
literal `marker_count == 1` and exactly one real point/forward source. No
named marker can index a second stack slot. The HCEA donors were also screened:

- `build/audit/refs/halocea/src/blam/hs/hs_effect_new.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_effect_new_from_object_marker.c`.

They independently confirm the three-argument public marker wrapper, the
single marker record, the two scale values, the null color/impulse arguments,
and the cutscene-flag position/facing semantics.

## January proof and retained C topology

`_hs_effect_new` uses a 12-byte local forward vector, signed-loads the short
flag index, retrieves a `0x5c` cutscene-flag element from scenario offset
`+0x4e4`, converts facing at `+0x30`, and supplies position at `+0x24` to
`effect_new_unattached_from_markers`. The call uses `NONE`,
`global_zero_vector3d`, one unnamed marker, scales `1.f`, null color and
impulse field, and trailing `TRUE`.

`_hs_effect_new_from_object_marker` has a `0x6c` `struct object_marker` frame.
Its nested definition/object validity tests reproduce January's staggered
EDI/ESI lifetimes. `object_get_marker_by_name` fills one record; the attached
effect receives `marker.node_index`, `&marker_name`,
`&marker.matrix.position`, and `&marker.matrix.forward`. The repository
layout places the matrix forward at frame `-0x30` and position at `-0x0c`,
exactly as January does.

The retained source follows the current house rules: every parameter is on its
own line, declarations precede statements in each scope, and both void
functions end in explicit `return;`. Tag-block access uses
`TAG_BLOCK_GET_ELEMENT`; no raw tag/object cast was introduced. The source has
no assembly, register/volatile scheduling, pragma, intrinsic, attribute,
barrier, raw address, pointer/integer reconstruction, punning, undefined
behavior, synthetic anchor, byte forcing, object patch, comparator exception,
or alternate compiler control.

## First-emission proof

The initial compiler invocation emitted no object because `effects.h` was
included before the repository header that declares `real`. That single
header-order error was new compiler evidence. Moving the include after
`objects.h` changed no function body or codegen hypothesis. The corrective
ordinary edge then emitted the wave's sole candidate object:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

VC7 also reported the known `C4047` declaration warning where the legacy
unattached-effect prototype types its null marker-name table slot as `long`.
The emitted push is the January `0`; no cast or declaration override was used.

Direct hardened comparison gives:

| function | meaningful / padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_effect_new` | 99 / 112 | 5 | `4f51e919fb6d13ae75c0ef5af6085fc853c98865328b478add892d4e3e91e50c` |
| `_hs_effect_new_from_object_marker` | 98 / 112 | 2 | `6df61ea36f25514876a22e65531daab60f3b52d16ce6b455518c53c128486f10` |

Both functions have zero differing padded bytes and identical relocation
count, order, address, type, symbolic destination, and addend.

## Regression adjudication

The fail-closed check reports exactly the two new functions as `NEWLY_EXACT`,
`changed_nonexact: []`, and ten inherited functions as `still_exact`. Seven
later accepted functions were flagged because the inserted COMDATs change
their section indices. Direct hardened comparison proves all seven remain
strict exact: `_hs_damage_new`, `_hs_damage_object`, `_code_000b9330`,
`_hs_sound_get_gain`, `_hs_sound_set_gain`,
`_hs_trigger_volume_test_objects_any`, and
`_hs_object_create_anew_containing`.

The flagged sound diagnostic `.rdata` remains strict exact. The 16-byte
`.debug$F` record retains identical bytes, flags, selection, relocation type,
address, addend, and `_code_000b9330` destination; only that destination's
object-local section number moves. Symbol-inventory comparison has no removal.
Its additions are exactly the two new public functions, their two COMDAT
section symbols, and the five previously unused external references required
by their calls.

## Validation and progress

- Complete Halo and libcmt targets: no work remained after the selected edge.
- Strict board: 260/619 complete objects, 4,291/8,246 functions, and
  555,831/1,922,669 padded bytes. This unit is 19/36 and 1,312/3,824 bytes.
- Semantic audit: 470 units, 4,451 functions evaluated, 4,300 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,360 accepted, zero unit errors.
- `build/report.json` SHA-256:
  `c378913b3d9d3fe554b9b54378623292a2005d760a7349ef9b40a1e92321f9fd`.
- `build/semantic_report.json` SHA-256:
  `9f2b746bdf7fd97fecbf78e4e12812d885c19f1d0bbc30ccc2c2ccc3238e44b9`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable cache warning.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching` in `config/config.json`.
