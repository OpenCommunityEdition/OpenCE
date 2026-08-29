# `object_lights.obj`: typed texture-load wrapper (2026-08-29)

## Result

This bounded source-only batch starts from canonical commit
`18a4680f0ad19e97c6d4f8cbfffce78a69c1ac3b`. The ordinary typed-C
implementation of `_texture_cache_bitmap_load` matched on its first compiled
candidate. `object_lights.obj` advances from 9/43 to 10/43 strict functions,
from 459 to 496 meaningful code bytes, and from 512 to 560 padded code bytes.
The object remains `NonMatching`; this batch claims no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_texture_cache_bitmap_load` | 37 | 48 | 3 | `bccf8ad00476da4b23779e1c22823f77cf54510635021b991ff7d754ae43bccc` |

The hardened comparator reports `all_equal: true`. It verifies every padded
byte and the three zero-addend REL32 relocations: `_profile_texture_start` at
`+0x05`, `__texture_cache_bitmap_get_hardware_format` at `+0x12`, and
`_profile_texture_end` at `+0x1C`. The nine inherited exact functions remain
exact; the whole-object gate reports 10 exact, 0 residual, and 33 unwritten.

## Evidence and reconstruction

The target is the January `cachebeta.exe` build 2342 split object. Its
`object_lights.obj` SHA-256 is
`a398a56409b9cf4555ac93df1c208baf328253fc95d2ee6349b0f9bd175d66e9`.
Compilation uses the pinned XDK 3911 VC7 compiler and the translation unit's
unchanged repository flags.

The January code and relocations establish a small public cdecl wrapper. It
starts texture profiling, requests the bitmap's hardware format with both
`block` and `load` set to `TRUE`, preserves the returned pointer across the
profiling-end call, and returns it. Existing typed callers in
`render_contrails.c`, `predicted_resources.c`, and
`rasterizer_xbox_motion_sensor.c` independently confirm the bitmap pointer,
Boolean arguments, and pointer result of
`_texture_cache_bitmap_get_hardware_format`. `profile.c` owns the two profiling
calls. The source declares only the typed interfaces it consumes and does not
define storage.

All reachable object-lights ledgers and repository documentation were searched
before editing. The lifecycle, map-disconnect, and helper-pair ledgers neither
attempt nor reject `_texture_cache_bitmap_load`; repository-wide source and
documentation search found no competing implementation. The earlier
helper-pair ledger's Claude/history/tooling audit remains the provenance record
for this translation unit, while January COFF and the hardened comparator are
the admission authority for this new function.

## Source policy

The retained body is readable typed C with one parameter per line and an
explicit terminal return. It contains no assembly, intrinsic, barrier, pragma,
`volatile`, `register`, attribute, raw address or offset, pointer/integer
reconstruction, union or aliasing pun, undefined behavior, synthetic anchor,
compiler-flag change, object patch, comparator exception, semantic allowlist,
or parked entry. No shared header, protected source, pre-existing ledger, or
configuration file is changed.

## Validation

- Selected and whole-object campaign gates: 1/1 new function exact; 10 exact,
  0 residual, 33 unwritten overall.
- Direct `tools.coff_compare`: `all_equal: true`, including all 48 padded bytes
  and all three relocations.
- Full Halo and libcmt builds: pass.
- Progress: 575,345/2,198,102 meaningful code bytes and 4,588/11,060
  functions overall; Halo is 562,431/1,770,166 bytes and 4,421/7,574
  functions. This batch contributes exactly one function and 37 meaningful
  bytes.
- Semantic audit: 470 units, 4,737 functions evaluated, 4,615 semantic exact,
  4,625 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this batch adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, deleted-path review, protected-source boundary, and
  retained-path review: pass.
