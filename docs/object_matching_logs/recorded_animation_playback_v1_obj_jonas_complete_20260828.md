# `recorded_animation_playback_v1.obj` Jonas complete-object closeout (2026-08-28)

## Result

`source/cutscene/recorded_animation_playback_v1.c` is fully exact under the
strict target-owner campaign rule. The unit advances from 2/13 to 13/13 exact
functions and from 48/2,320 to 2,320/2,320 padded code bytes. This wave
recovers eleven functions, 2,161 meaningful code bytes, and 2,272 padded code
bytes. Every retained function body was exact on the first full compile.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_00082910` | `166 / 176` | 16 | `3d914618e29c41b09a15f8c523442b0d76a404001cc06619170893f233365624` |
| `_code_000829c0` | `166 / 176` | 16 | `dff3320f420eb7dcba45966db5100e2e919a588ed0db5b2e5e1700da0bd31f2b` |
| `_code_00082a70` | `166 / 176` | 16 | `1fcc194d9fba5c773daba3f32ebef30e3691f31004f18c2cda000ab0bc2d9459` |
| `_code_00082b20` | `166 / 176` | 16 | `94a9cef2532da7f00077f53e7b3e9cb0902d1502e4dd90ff946ccbe8e96406f4` |
| `_code_00082bd0` | `180 / 192` | 16 | `4ff6e66647f410f744b7ac02822e8bb4eb9990f53878c594e1f1c070006c2bf5` |
| `_code_00082c90` | `183 / 192` | 16 | `e4a8c0089ae1fd0d9dfa3e566b180660ea6690ea77c0c55e557f6f3785ca2edc` |
| `_code_00082d50` | `183 / 192` | 16 | `8c47d6771a556d605d0f0200e40f496c4ac56944c87c38b07b8e1767eccab201` |
| `_code_00082e10` | `183 / 192` | 16 | `b1fcf7a7bb6b32846b3c3387a839e404ad0e2a9f51354e4d969712b198365304` |
| `_code_00082ed0` | `257 / 272` | 17 | `ca1bcb3d1506ca7cb68b67e93d0fb9eb26fc2269a211798f8f0fe27403b0b7e2` |
| `_code_00082fe0` | `250 / 256` | 16 | `090931aa87530f78cf7b3236df4fc90b39c805b9242f03bfb43194186c4389e5` |
| `_recorded_animation_apply_event_stream_v1` | `261 / 272` | 17 | `597a53095129680467cb397baefdce193a3440985c0188990c7e44a7348b969f` |

The inherited initializer and byte-swap stub remain exact at 32 and 16 padded
bytes. Baseline/final exact-set comparison is 2 to 13, eleven newly exact, and
zero lost exact functions. `config/config.json` index 347 therefore moves from
`NonMatching` to `Matching`; no semantic bridge or comparator exception is
needed.

## Frozen provenance

The isolated branch/worktree is
`jonas/recorded-animation-playback-v1-wave-20260828`, based on published
integration commit `b30d41fb4764e6b6b6be30b76c15ec341a2f0573`. The immutable
and retained artifacts are:

| Artifact | Raw SHA-256 |
| --- | --- |
| `build/split/source/cutscene/recorded_animation_playback_v1.obj` | `d04cd84d0bf9bfa0d05e2bcc32d38974fc308f797162f0d61896d8732b4e2bef` |
| `build/audit/recorded_animation_playback_v1_baseline_20260828.obj` | `a1b70fa1e9e39613af1fd842668f8451e3fc84544edfff4505584530183073b` |
| `build/audit/recorded_animation_playback_v1_full_first_shot_20260828.obj` | `bf041a7532ecb73444e4f5f11758187bee428161324590bc37f84f1a38b57997` |
| `build/audit/recorded_animation_playback_v1_data_exact_20260828.obj` | `bf041a7532ecb73444e4f5f11758187bee428161324590bc37f84f1a38b57997` |
| final `build/base/source/cutscene/recorded_animation_playback_v1.obj` | `bf041a7532ecb73444e4f5f11758187bee428161324590bc37f84f1a38b57997` |

The final source SHA-256 is
`66963bbf45e0c55889e3e1543f74cde11244364a0f6232405bd11769805d6d57`.
Raw whole-object hashes are provenance sentinels only; normalized owner bytes,
resolved relocations, and owner metadata are the admission authority.

The compiler is XDK 3911 `CL.Exe` 13.00.9254.1 under the unchanged repository
`/O2 /Oy- /DDEBUG /Dxbox` edge. No compiler option, split target, symbol
manifest, comparator rule, semantic allowlist, or parked entry changed.

## Typed reconstruction

January's event header is a typed four-byte `{ short type; word time_delta; }`
record. The ten handler bodies use typed scalar, throttle, vector, and angle
event structures and the sibling-proven 64-byte `recorded_unit_control`
layout. The handlers preserve January's assertion expressions and line
numbers, assign the payload to the corresponding unit-control field, and
advance the stream by the typed event size.

The angle family converts its typed `real_euler_angles2d` payload through
`vector3d_from_euler_angles2d`, then copies the result to the vectors selected
by event type. The multi-vector family copies its typed `real_vector3d`
payload with the same three exclusion tests. The dispatcher validates its
three live arguments and stream value, consumes every event whose unsigned
time delta has elapsed, dispatches through the signed event-type index, skips
a null handler by one typed header, subtracts the consumed delta, and returns
false only at the exact end marker. This natural donor-correlated C reproduces
January's loop alignment, signed table index, calls, branches, and epilogues
without tuning.

## Runtime-data ownership

All 21 target-owned non-code owners compare exact: 1,237 logical owner bytes,
or 1,272 bytes with COFF section padding. The 304-byte `_data_002dd030`
aggregate has 35/35 DIR32 relocations and normalized SHA-256
`c17dc398d83da1a719e165e47cd2da24cb6baaa8a0fe02b8b814b152bfe9ee9d`.
Its typed layout is:

| Offset | Field | Logical bytes |
| ---: | --- | ---: |
| `0x00` | `apply_funcs[23]` | 92 |
| `0x5c` | base event codes + definition | 28 |
| `0x78` | animation-state code + definition | 24 |
| `0x90` | aiming-speed code + definition | 24 |
| `0xa8` | control-flags code + definition | 24 |
| `0xc0` | weapon-index code + definition | 24 |
| `0xd8` | throttle codes + definition | 28 |
| `0xf4` | multi-vector codes + definition | 32 |
| `0x114` | angle-vector codes + definition | 28 |

The handler table is null at 0-1 and 7-8, scalar at 2-6, direct vector at
9-11, multi-vector at 12-15, and angle-vector at 16-22. The aggregate's eight
definition-name strings are exact, as are the event/assert/path strings owned
by the object. Every non-code owner has exact bytes, size, flags, storage/type
metadata, relocation count/type/address/destination/addend, and normalized
hash.

## Documentation and donor audit

Before the implementation, the existing v1 initializer ledger,
`docs/matching_methodology.md`, `tools/campaign/README.md`, the applicable
current and Stian `CLAUDE.md` instructions, and the relevant Claude
project/session artifacts were inspected. Claude's active work is Units-only;
all Claude material was read-only and no active Claude worktree was touched.
No v1 source candidate, prior fixed point, or experiment existed beyond the
already exact initializer.

The primary behavior and topology donors were:

- `research-cache/stian-halo-current-20260827/src/halo/cutscene/recorded_animations.c`;
- `work/halocea-reference/src/apply_*.c`;
- `work/halocea-reference/src/recorded_animation_apply_event_stream_v1.c`;
- `work/halocea-reference/src/data/apply_funcs.c` and `src/data/v1_playback.c`;
- `work/halocea-reference/src/data/*_event_v1_bs_definition.c`; and
- the completed sibling `source/cutscene/recorded_animation_playback.c`.

Those sources supplied hypotheses and source-family provenance. January's
target COFF remained the sole authority for every retained byte, relocation,
ABI choice, layout, string, and owner.

## Policy audit

Every parameter is on its own line. Every void function has an explicit
terminal `return;`, and the boolean dispatcher has explicit returns. Event,
payload, stream, control, byte-swap, and aggregate access is typed. The source
contains no assembly, `volatile`, `register`, pragma, intrinsic, barrier,
annotation, raw address/offset access, pointer/integer reconstruction,
pointer/union pun, undefined behavior, synthetic emitter, object-byte patch,
compiler flag change, comparator exception, semantic exception, or parked
credit.

## Validation

- Production-flag XDK 3911 `/Zs`: pass with no diagnostic.
- `tools/campaign/gate.py source/cutscene/recorded_animation_playback_v1 --all`:
  13 exact, zero residual, zero unwritten.
- Baseline/final exact set: 2 to 13, eleven newly exact, zero lost exact.
- Strict runtime-owner census: 21/21 owners, 1,237/1,237 logical bytes, and
  1,272/1,272 section-padded bytes.
- `halobetacache_build`, `libcmt_build`, and `semantic_progress`: pass.
- Semantic audit: 470 units, 4,603 functions evaluated, 4,472 semantic exact,
  4,499 accepted exact, and zero unit errors.
- Object-admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 13 active, zero stale, and zero invalid.
- Tooling tests: 205/205 pass.
- Strict global board: 270/619 objects, 4,453/8,246 functions, and
  596,423/1,922,669 padded code bytes. Parked and asm functions are excluded.

The tracked closeout is limited to the source file, this ledger, and the one
`NonMatching` to `Matching` status change.
