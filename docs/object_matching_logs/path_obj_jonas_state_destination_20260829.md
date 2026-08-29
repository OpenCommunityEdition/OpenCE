# `path.obj`: typed destination setter (2026-08-29)

## Result

This bounded source-only batch starts from canonical commit
`a1e1134309f6f709acf9af9b8085ba1501ccb978`. The ordinary typed-C
implementation of `_path_state_destination` is strict exact. `path.obj`
advances from 7/32 to 8/32 strict functions, from 107 to 155 meaningful code
bytes, and from 176 to 224 padded code bytes. It remains `NonMatching`; this
batch claims no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_path_state_destination` | 48 | 48 | 0 | `1064ebb9794d2f3ff5d6ae7ede65b58c020fc0c186e8d35ebba45c8f9de7aff2` |

The hardened comparator reports `all_equal: true`. It verifies the complete
48-byte function section; the function has no relocations. The seven inherited
exact functions remain exact, and the whole-object gate reports 8 exact,
0 residual, and 24 unwritten.

## Provenance and reconstruction

The target is the January `cachebeta.exe` build 2342 split object. Its
`path.obj` SHA-256 is
`1b989e4dc3bf3d9623cec07fa19164794044eb0bf359cf08480cf0ea8b3a709c`.
Compilation uses the pinned XDK 3911 VC7 compiler and the translation unit's
unchanged repository flags.

The prior path ledger and repository-wide documentation were read before the
edit. They exhaustively record the path-input wave and two rejected siblings,
but neither attempt nor reject `_path_state_destination`. The existing shared
`path.h` already supplies the complete typed `path_state` and
`path_destination` layouts. New compile-time assertions authenticate the
January offsets for `destination_valid` (`+0x4C`), `destination` (`+0x50`),
`surface_index` (`+0x0C` within the destination), and `target_radius` (`+0x10`).

The user-supplied HaloCE archive was cloned read-only at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Its
`src/blam/ai/path_state_destination.c` independently confirms the four-argument
signature and behavior: mark the destination valid, copy the three-real point,
and set its surface index and acceptance radius. Its `path_state.h` and
`path_destination.h` independently confirm the same offsets and scalar widths.

The later-build source orders radius before surface and spells the point copy
component by component. January PC COFF is authoritative for this target: it
uses a natural aggregate point copy and emits the final two typed stores in
structure order, surface before radius. The first compiled candidate followed
the later-build scalar-field order and matched every instruction except those
last two independent loads/stores. Reordering those two semantic assignments
to January's structure order produced the retained exact function. No broader
spelling, type, declaration, scheduling, or compiler-option search was made.

## Source policy

The retained body uses only named typed fields, an ordinary structure copy,
one parameter per line, and an explicit terminal return. It contains no
assembly, intrinsic, barrier, pragma, `volatile`, `register`, attribute, raw
address or offset access, pointer/integer reconstruction, union or aliasing
pun, undefined behavior, synthetic anchor, compiler-flag change, object patch,
comparator exception, semantic allowlist, or parked entry. No shared header,
protected source, pre-existing ledger, or configuration file is changed.

## Validation

- Selected and whole-object campaign gates: 1/1 new function exact; 8 exact,
  0 residual, 24 unwritten overall.
- Direct `tools.coff_compare`: `all_equal: true`, including all 48 bytes and
  the empty relocation schedule.
- Full Halo and libcmt builds: pass.
- Progress: 575,393/2,198,102 meaningful code bytes and 4,589/11,060
  functions overall; Halo is 562,479/1,770,166 bytes and 4,422/7,574
  functions. This batch contributes exactly one function and 48 meaningful
  bytes.
- Semantic audit: 470 units, 4,738 functions evaluated, 4,616 semantic exact,
  4,626 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this batch adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, deleted-path review, protected-source boundary, and
  retained-path review: pass.
