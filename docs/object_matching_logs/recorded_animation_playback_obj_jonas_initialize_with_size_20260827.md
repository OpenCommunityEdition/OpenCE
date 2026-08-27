# `recorded_animation_playback.obj` sized initializer

## Result

The first and only production candidate makes
`recorded_animation_initialize_event_stream_with_size` strict semantic-COFF
exact. The object advances from 2/14 to 3/14 exact functions and from 56 to
115 matched meaningful code bytes. It remains `NonMatching`; no completion
label, semantic exception, data credit, or parked entry is added.

The new function is 59 meaningful / 64 padded bytes, has no relocations, and
has identical target/rebuilt normalized SHA-256
`0013ff996cff7a262daf7e3d233fa7eb084bc82329a3bda99a02a178b49a035d`.
The inherited current initializer and byte-swap stub remain strict exact.

## Evidence and provenance

January's complete relocation-free disassembly performs two ordinary copies:

1. load `*playback_stream`, copy 16 dwords into the unit-control destination
   with `rep movsd`, and advance the stream by `0x40`;
2. copy three dwords into the facing, aiming, and looking controller fields,
   then advance the stream by `0x0C`.

The later-build HCEA donor at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently authenticates this
two-copy topology. Its
`src/blam/cutscene/recorded_animation_initialize_event_stream_with_size.c`
blob is `a99a1c66bea73c7ea060eae5e9c3436b5407c102`. Its typed
`src/headers/unit_control_data.h` blob
`27c7030c76c37c31cf638a2d3fe9c21fe225b6d7` proves the unit-control record is
exactly 64 bytes and names the animation, aiming, control, weapon, grenade,
zoom, throttle, trigger, facing, aiming-vector, and looking-vector content.

The current repository's independently reconstructed
`recorded_animation_initialize.c` supplies the January-side
`struct recorded_unit_control` layout used by the initializer callee. The
retained playback definition mirrors that complete typed layout rather than
using an opaque byte array. Its field sizes sum naturally to 64 bytes, and
`sizeof(*unit_control)` and `sizeof(*animation_state)` drive both copies and
stream advances.

Every reachable Claude branch was searched for the function name before the
shot. The hits are limited to symbol inventories, the original skeleton
comment, and target-only residual-frontier entries on `finish-hs`,
`libcmt-stream`, `near-complete-objects`, and `untried-objects`. No Claude
candidate, compile measurement, source-family exclusion, fixed point, or
object-specific ledger exists for this function.

The retained code uses the already-proven standard C `memcpy` spelling after
the local checked-copy remap is undefined. It contains no explicit intrinsic,
raw offset, pointer/integer reconstruction, representation pun, union pun,
assembly, `register`, `volatile`, pragma, annotation, barrier, synthetic
reference, byte forcing, or compiler-flag change. Every parameter is vertical
and the void function ends with an explicit `return;`.

## Reproducibility and gates

- January split object raw SHA-256:
  `3808c85f1eec3cce0269b5ec3c94fab49c21893e49f28fa6f80ffef17274cd49`.
- Frozen pre-shot object:
  `build/audit/recorded_animation_playback_initialize_with_size_baseline_20260827.obj`,
  raw SHA-256
  `fbd3b4e7775c374c19e9c20093a9ce5f2edc1cb394eeb2b77359c657361398ca`.
- Retained exact object:
  `build/audit/recorded_animation_playback_initialize_with_size_retained_20260827.obj`,
  raw SHA-256
  `644ec65670740dbdc5b3611329fb2a3233c0a9f6b0408dd619a7edb9b8140476`.
- Retained source SHA-256:
  `5621ef03de6b7c23ed41efa294d5b72575c900ac9c9772af78e465ae6183efce`.
- Production-flag XDK 3911 `/Zs`: pass; the frozen object remained unchanged.
- Exactly one normal Ninja compile: strict exact on the first shot.
- Hardened comparison: all three emitted functions exact, `all_equal: true`.
- Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,403 functions evaluated, 4,257 semantic exact,
  4,317 accepted exact, and zero unit errors.
- Campaign progress: 377/833 objects, 4,290/11,060 functions, and
  515,472/2,198,102 code bytes. Halo progress: 275/468 objects,
  4,123/7,574 functions, and 502,558/1,770,166 code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- `git diff --check`: pass; protected source, configuration, semantic ledgers,
  comparator/admission tooling, compiler flags, and object status are
  unchanged.

This checkpoint claims one exact function and 59 meaningful code bytes, not a
completed object or any data ownership.
