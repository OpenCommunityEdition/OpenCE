# `rasterizer_xbox_profile.obj` Opus reconciliation (2026-09-03)

## Scope and disposition

- Admission base: `e46fd5959`.
- Donor only: Opus commit `bb0d690a3`.
- Audited implementation unit:
  `source/rasterizer/xbox/rasterizer_xbox_profile.c`.
- Cross-unit ABI repair:
  `source/cseries/profile.c` and its owner header
  `source/cseries/profile_rasterizer.h`.
- Public Xbox rasterizer declarations were placed in their owner header,
  `source/rasterizer/xbox/rasterizer_xbox.h`.

The donor was not merged wholesale. It was used as a behavioral hypothesis,
then rewritten around January's function bytes, relocations, storage classes,
data layout, assertion text, and the repository's existing Stian/Pastudan
research. The result closes every function in the profile translation unit:

| State | Exact | Residual | Unwritten | Exact padded code |
| --- | ---: | ---: | ---: | ---: |
| Before | 7 | 0 | 9 | 400 / 2,640 |
| After | 16 | 0 | 0 | 2,640 / 2,640 |
| Delta | **+9** | **0** | **-9** | **+2,240** |

The object remains marked `NonMatching`; this wave claims function credit,
not whole-object admission. Including the stock XDK header naturally emits
linker-discardable D3D select-any lookup-table COMDATs that are not owned by
the January split object. No partial-header facade or code-generation
directive was introduced to suppress them. Objdiff also gives the 132-byte
`.data` section a partial structural score even though the hardened direct
COFF comparison described below proves its bytes and ordered relocation
identities exact.

## Accepted reconstruction

The nine new strict closures are:

| Function | Padded bytes |
| --- | ---: |
| `rasterizer_profile_check` | 128 |
| `rasterizer_profile_callback` | 272 |
| `rasterizer_profile_frame_callback` | 144 |
| `rasterizer_profile_frame_begin` | 336 |
| `rasterizer_profile_begin` | 304 |
| `rasterizer_profile_end` | 320 |
| `rasterizer_profile_query` | 288 |
| `rasterizer_profile_query_pushbuffer` | 224 |
| `rasterizer_profile_frame_end` | 224 |

Defining the private helper and all of its live callers together lets VC7
naturally recover January's ESI/EDI interprocedural contract. No register
annotation, nonstandard calling convention, assembly shim, volatile spill,
barrier, or fake wrapper is used. This resolves the earlier boundary recorded
in `rasterizer_xbox_profile_obj_jonas_remaining_boundary_20260829.md` without
weakening the comparator or importing the old donor's byte-directed controls.

The former address labels now have semantic, TU-private names:

- `rasterizer_profile_check`;
- `rasterizer_profile_callback`;
- `rasterizer_profile_frame_callback`;
- `rasterizer_profile_active`.

These are descriptive reconstructions supported by their callers and target
behavior; they are not represented as recovered PDB spellings. All four are
`static`, and their symbol entries are configured as private. Every recovered
state owner is likewise named by its role rather than `data_<address>` or
`bss_<address>`: the performance-counter frequency, profile-name globals,
callback timestamp arrays, elapsed state, frame state, local enable depth,
profile state, and error count.

The retained `volatile` qualifiers are limited to real asynchronous ownership
boundaries and are explained at their declarations:

- elapsed times are written by a D3D callback and sampled by the CPU query;
- callback error bits are set by D3D callbacks and drained at frame begin;
- start times are initialized by CPU code and consumed by asynchronous D3D
  callbacks.

Callback elapsed/end arrays and CPU-owned frame state remain ordinary objects.
No qualifier is present solely to steer instruction selection.

## Natural data ownership

The 132-byte initialized owner is expressed as two natural static objects:

1. a `LARGE_INTEGER` performance-counter frequency initialized to one; and
2. a naturally aligned 124-byte `rasterizer_profile_globals` state record.

This lets `QueryPerformanceFrequency` receive its real API type directly,
without a representation cast, packed record, or layout pragma. The target and
candidate `.data` owners both have 132 bytes and 29 ordered relocations.
Their normalized SHA-256 is
`b577924a924477a95be3c0a539812cce4cae4eaf93489d580a909a02169d0cf9`,
and `tools.coff_compare.section_infos_equal` returns true.

The reconstructed private BSS objects are explicitly zero-initialized static
state. VC7 retains their source declaration order, which agrees with all
January relocation addends and with the independently attested first four
owners. The resulting `.bss` is 1,122 bytes with no relocations; target and
candidate share normalized SHA-256
`033eeac8e125a5efb66f100fb9ae33c9fd1780f452b92f69a8d6b49ba5e1737d`,
and `section_infos_equal` returns true. The exact offsets are:

| Offset | Owner |
| ---: | --- |
| `+0` | callback elapsed times |
| `+128` | callback end times |
| `+256` | elapsed state |
| `+848` | profile start times |
| `+1080` | frame state |
| `+1104` | local profile enable |
| `+1108` | profile state |
| `+1120` | profile error count |

## Cross-unit statistics ABI

January's frame-end caller pushes a `real` followed by an `__int64` for
`profile_rasterizer_stats`. Its exact callee stores the floating GPU time and
the low 32-bit pushbuffer byte count. The prior `(long, long)` definition and
donor-local conflicting prototype were therefore replaced with one owner
declaration:

```c
void profile_rasterizer_stats(
	real gpu_time,
	__int64 pushbuffer_size);
```

The corresponding `profile_frame` fields are now
`rasterizer_gpu_time` and `rasterizer_pushbuffer_size`. The callee remains
strict exact at 32 padded bytes and two relocations, with normalized SHA-256
`d2a88f26be45877e903a87316d6f952be36c8f04e11a7facf5062f7e448856e4`.
The complete `cseries/profile` gate remains 18 exact, 0 residual, and 26
unwritten.

All twelve public profile declarations and
`rasterizer_set_framebuffer_blend_function` now live in
`rasterizer_xbox.h`. Duplicate consumer-local declarations were not retained.

## Target-proven diagnostic defect

The `profile == NONE` branch in `rasterizer_profile_check` is superficially
wrong: January passes both `profile` and `message` to a format containing only
one `%s`. Direct target disassembly and relocation order prove the same two
vararg pushes in both branches. The retained two-call source reproduces the
target's rare error-path defect transparently rather than changing or hiding
it. The strict function is 128 bytes with 11 relocations and normalized
SHA-256
`edf9b4c390ce6b95ccc9e62cca3149939119ecd18bc0d62622cd1d2693804eb1`.

A cleaner single syntactic call with a conditional format was compiled and
rejected: it produced 96 bytes and 9 relocations, versus January's 128 bytes
and 11 relocations. The difference is therefore target evidence for the two
call sites, not permission to add inert arguments elsewhere.

## Rejected donor material

- Address-derived private/global names were replaced with semantic owners.
- External linkage for TU-private helpers and state was rejected where natural
  static definitions reproduce January.
- `#pragma pack`, representation casts, pointer-punned owners, raw offsets,
  unsupported assembly, forced inline, barriers, dead-load steering, and fake
  linkage anchors were rejected.
- A packed aggregate for the 132-byte initialized state was rejected in favor
  of the naturally aligned `LARGE_INTEGER` plus profile-state objects.
- A consumer-local declaration of `profile_rasterizer_stats` was rejected;
  the ABI is owned by the narrow `profile_rasterizer.h` interface and gated
  at both caller and callee.
- The single-call conditional-format cleanup was rejected by the exact target
  shape described above.

## Verification

- Focused gate, `rasterizer_xbox_profile`: 16 exact, 0 residual, 0 unwritten.
- Protected gate, `units`: 189 exact, 0 residual, 0 unwritten.
- Rename-stable whole-tree snapshots: 5,496 exact before and 5,505 exact
  after across 8,245 target functions; **9 gained / 2,240 padded bytes / 0
  regressions**.
- Full build: `ninja halobetacache_build libcmt_build` completed successfully,
  recompiling the affected header consumers.
- Representative direct-consumer gates retained their prior exact sets,
  including `objects` 121/121, `cseries` 26/26, `render` 13/13,
  `array` 11/11, `console` 13/13, `debug_keys` 12/12, `models` 12/12,
  `marketing` 5/5, and `rasterizer_xbox_debug` 18/18.
- Scoped fake-match scan: 4 files scanned, 0 findings.
- Tool tests: 261 passed.
- `git diff --check`: clean.

## Canonical aggregate reconciliation

The initial canonical admission exposed a VC7 definition-position regression
that the isolated lane could not see: placing the rasterizer-only ABI in the
broad `profile.h` include path changed `units.obj` from 189/189 to 188/189,
even though `units.c` does not call the function. The declaration was moved to
the narrow owner interface `source/cseries/profile_rasterizer.h`, included only
by `profile.c` and `rasterizer_xbox_profile.c`. After a full rebuild and a fresh
target split, `units.obj` returned to 189/189 while
`rasterizer_xbox_profile.obj` remained 16/16. This is the canonical form of the
ABI and supersedes the broad-header wording in the isolated-lane record.
