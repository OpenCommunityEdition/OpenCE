# `recorded_animation_playback_v1.obj` event-stream initializer

## Result

The first and only production candidate makes
`_recorded_animation_initialize_event_stream_v1` strict semantic-COFF exact.
The object advances from 1/13 to 2/13 exact functions and from 1 to 26 matched
meaningful code bytes. It remains `NonMatching`; its 1,272 target data bytes
remain uncredited.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_recorded_animation_initialize_event_stream_v1` | 25 / 32 | 1 | `8f0458f1d0b08f15c53f92f1ce236ab1008fb861344a3e96fc0dff85e94dbe92` |

The sole relocation is REL32 at function offset `+0x10` to
`_recorded_animation_initialize_unit_control`, with the same destination and
addend on both sides. The inherited exact
`_byte_swap_recording_stream_v1` stub remains exact at 1 meaningful / 16
padded bytes, zero relocations, and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Evidence and provenance

January disassembly proves an ordinary cdecl wrapper. The first parameter is
unused; the function loads parameters four, three, and two as dwords, pushes
them in that order, calls `recorded_animation_initialize_unit_control`, cleans
12 stack bytes, and returns. The current repository implementation of that
callee authenticates its natural types as:

```c
struct recorded_unit_control *unit_control,
byte **stream,
byte unit_control_data_version
```

The later-build HCEA donor at
`build/audit/refs/halocea@570c83fd:src/blam/cutscene/recorded_animation_initialize_event_stream_v1.c`,
blob `8783a7d37e62357d83a20e9564f264e54e752849`, independently authenticates
the unused animation-controller pointer and the same one-call topology.
January remains the byte, padding, relocation, and ABI authority.

All reachable Claude branches and object/tooling records were searched before
the shot. The function occurs only in configuration/symbol inventories and in
the `residual_frontier.json` target-only census on the near-complete,
finish-HS, libcmt-stream, and untried-object branches. No Claude source
candidate, compile measurement, fixed point, or object-specific experiment
exists for this function. The current object has no earlier matching ledger.
The campaign's Claude-derived house rules, verification policy, and hardened
COFF comparison methodology therefore apply without an inherited source-family
exclusion.

The retained source uses incomplete named structure declarations because both
types are passed only by pointer. It introduces no opaque byte layout, raw
offset, pointer/integer reconstruction, pointer or union pun, assembly,
`register`, `volatile`, pragma, intrinsic, annotation, barrier, synthetic
emitter, or byte-forcing expression. Every parameter is on its own line and
the void function ends with an explicit `return;`.

## One-shot and gates

- January split object raw SHA-256:
  `d04cd84d0bf9bfa0d05e2bcc32d38974fc308f797162f0d61896d8732b4e2bef`.
- Frozen pre-shot base object:
  `build/audit/recorded_animation_playback_v1_initialize_baseline_20260827.obj`,
  raw SHA-256
  `a10172255fb86363f629187278b94e41b9cecc0b076975b504ec87ea08452b0f`.
- Retained first-shot object:
  `build/audit/recorded_animation_playback_v1_initialize_retained_20260827.obj`,
  raw SHA-256
  `0116f2fc4f68453361a922b1144e03fcbec2cd21f4af39b8a0a726faf6e34295`.
- Retained source SHA-256:
  `c784034c189cf492665c2c970a9a2a2e136bf442ed17b961ea8150802bb4569d`.
- Production-flag XDK 3911 `/Zs`: pass; the frozen object remained unchanged.
- Exactly one normal Ninja compile: pass and strict exact on first shot.
- Direct hardened comparison: both emitted functions exact, `all_equal: true`.
- Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,401 functions evaluated, 4,255 semantic exact,
  4,315 accepted exact, and zero unit errors.
- Campaign progress: 377/833 objects, 4,288/11,060 functions, and
  515,358/2,198,102 code bytes. Halo progress: 275/468 objects,
  4,121/7,574 functions, and 502,444/1,770,166 code bytes.
- Admission: zero candidates, contradictions, and revocations.
- Parked ledger: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- `git diff --check`: pass; protected paths, configuration, semantic
  exceptions, compiler flags, and object status are unchanged.

This checkpoint claims one exact function and 25 meaningful code bytes, not a
completed object or any data ownership.
