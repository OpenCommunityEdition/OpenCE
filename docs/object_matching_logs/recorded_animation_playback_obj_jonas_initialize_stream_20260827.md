# `recorded_animation_playback.obj` event-stream initializer

## Result

`recorded_animation_initialize_event_stream` is strict semantic-COFF exact.
The object advances from 1/14 to 2/14 exact functions and from one to 56
matched meaningful code bytes. It remains `NonMatching`; no completion label,
semantic exception, data credit, or parked entry is added.

The matched function is 55 meaningful / 64 padded bytes. Target and rebuilt
COFF have normalized SHA-256
`52ba88be53f2d18549a95432a347c1359c52293915fd67f310cc3543c8c1e824`
and one REL32 relocation at function offset `+0x11` to
`_recorded_animation_initialize_unit_control`, with identical identity, type,
address, and addend. The inherited exact `_byte_swap_recording_stream` remains
exact at one meaningful / 16 padded bytes, zero relocations, and normalized
SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Evidence and source decision

January disassembly fixes the complete topology: call the typed unit-control
initializer, copy three consecutive four-byte controller fields from
`*playback_stream`, advance that stream by 12 bytes, and return. The already
matched v1 sibling independently fixes the four-parameter cdecl call ABI. The
repository implementation of `recorded_animation_initialize_unit_control`
authenticates the natural `struct recorded_unit_control *`, `byte **`, and
`byte` parameter types.

The later-build HCEA donor at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies independent type and
source-shape provenance:

- `src/blam/cutscene/recorded_animation_initialize_event_stream.c`, blob
  `b871602812fc5ec0cb836c3d90c35a02c1f0de5c`, uses ordinary `memcpy` for the
  complete 12-byte controller and then advances the stream by 12;
- `src/headers/animation_playback_controller.h`, blob
  `443ada6133649a6c3bd296719d238ded7c3f1e59`, defines facing, aiming, and
  looking direction controllers at offsets 0, 4, and 8;
- `src/headers/direction_playback_controller.h`, blob
  `09b433c1e72bce9b7c349a020268a3c784fa0398`, defines each direction
  controller as named yaw and pitch shorts.

All reachable Claude object/tooling records were searched before the shot.
They contain only configuration/symbol inventory and residual-frontier entries
for this current-format initializer, with no source candidate, compile
measurement, fixed point, or object-specific ledger. January remains the sole
byte, padding, relocation, and ABI authority.

The first frozen candidate used the natural `memcpy` spelling but inherited
the project-wide `#define memcpy csmemcpy`. It therefore emitted a 48-byte
function with an extra REL32 relocation to `_csmemcpy` and was rejected. That
measurement exposed the exact causal blocker. A new evidence wave then used
the same established local pattern as `path_obstacle_avoidance.c`: undefine
the checked project remap and call the standard C `memcpy` interface. This is
not an explicit intrinsic or compiler directive; VC7 makes its ordinary
optimization decision and emits January's exact three-dword copy.

The retained source uses named, typed controller fields, `sizeof` rather than
a magic layout cast, vertical parameters, and an explicit `return;`. It adds
no raw offset access, pointer/integer reconstruction, representation pun,
union pun, assembly, `register`, `volatile`, pragma, annotation, barrier,
explicit intrinsic, synthetic reference, byte forcing, or compiler-flag
change.

## Reproducibility and gates

- January split object raw SHA-256:
  `3808c85f1eec3cce0269b5ec3c94fab49c21893e49f28fa6f80ffef17274cd49`.
- Frozen pre-shot base object:
  `build/audit/recorded_animation_playback_initialize_baseline_20260827.obj`,
  raw SHA-256
  `8799b9d09f949f30ed954f6cc339bbf4b7b3d6077e4111022ce20d6203321a18`.
- Rejected checked-copy candidate:
  `build/audit/recorded_animation_playback_initialize_checked_memcpy_miss_20260827.obj`,
  raw SHA-256
  `288c5817166b97b7ec6ed9306a1f487343ec85c1669ae03f55dca67d12c0b46a`.
- Retained exact object:
  `build/audit/recorded_animation_playback_initialize_retained_20260827.obj`,
  raw SHA-256
  `fbd3b4e7775c374c19e9c20093a9ce5f2edc1cb394eeb2b77359c657361398ca`.
- Retained source SHA-256:
  `79d048f7516545dc40c8f2ca22c8d459579751d89f021af94e3d16078f22492f`.
- Production-flag XDK 3911 `/Zs` passed before each evidence wave, with the
  corresponding frozen object unchanged.
- Hardened comparison passes both emitted functions with `all_equal: true`.
- Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 4,402 functions evaluated, 4,256 semantic exact,
  4,316 accepted exact, and zero unit errors.
- Campaign progress: 377/833 objects, 4,289/11,060 functions, and
  515,413/2,198,102 code bytes. Halo progress: 275/468 objects,
  4,122/7,574 functions, and 502,499/1,770,166 code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- `git diff --check`: pass; protected source, configuration, semantic ledgers,
  comparator/admission tooling, compiler flags, and object status are
  unchanged.

This checkpoint claims one exact function and 55 meaningful code bytes, not a
completed object or any data ownership.
