# `saved_game_files.obj` mapfile-mutex release leaf (2026-08-27)

## Result

This wave recovers the target-private `_code_001b31f0` wrapper from strict
cross-function evidence.  `saved_game_files.obj` advances from 5/46 to 6/46
exact functions and from 111 to 124 matched meaningful code bytes.  The object
remains `NonMatching`; no completion label, semantic exception, or admission
override is added.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_001b31f0` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, addends, and resolved ownership:

- `+0x01`, `DIR32`, `_saved_game_files_globals+0x110`, the typed
  `mapfile_mutex` field;
- `+0x07`, `REL32`, `_release_mutex`.

## Evidence and one-shot discipline

The target atlas independently grouped `_code_001b31f0` with the already exact
`_saved_game_files_release_mutex`.  Both target functions have the same
16-byte normalized body, two-relocation schedule, and normalized hash.  The
only semantic difference is the grounded aggregate field: the public wrapper
loads `general_mutex` at `+0x10C`, while `_code_001b31f0` loads
`mapfile_mutex` at `+0x110`.  The existing typed
`struct saved_game_files_globals` definition and its 0x11C size check establish
that field without raw-offset access.

The complete prior Claude/Jonas mutex ledger
`docs/object_matching_logs/saved_game_files_obj_jonas_mutex_pair_20260821.md`
was read before editing.  Its exact public release wrapper supplied the natural
source topology.  A syntax-only `/Zs` check passed, followed by exactly one
normal code-producing compile on the repository's unchanged XDK 3911 Ninja
edge.  The first candidate was strict-exact; there was no retry or tuning wave.

The retained implementation is ordinary typed C.  It contains no cast,
pointer or union pun, raw address/offset access, assembly, `register`,
`volatile`, pragma/barrier, compiler attribute, intrinsic, synthetic anchor,
undefined behavior, or object-byte forcing.  Its no-argument declaration and
definition place `void` on its own line, and the void body ends in an explicit
`return;`.

## Reproducibility and validation

- January target object SHA-256:
  `bd000ef1323c25d9bba631cd0c1a0ef9c6f3bbca7bcc5a634fc9ec13bc8fedf4`.
- First-shot candidate object SHA-256:
  `23b43038c9f0af38d83bb1ecf48fb1128bd204dd92a88a22fd9e1b66d915f834`.
- Retained source SHA-256 before commit:
  `dd80a4c6c2feb53fb0ceea2c6169545785522f24f61562a3e0dc408fece4131f`.
- Direct hardened comparison passes for the new function and all five inherited
  exact functions.
- `halobetacache_build`, `libcmt_build`, semantic progress, and ordinary
  progress all pass.
- Semantic audit: 470 units, 4,339 functions evaluated, 4,193 semantic exact,
  4,253 accepted exact, and zero unit errors.
- Campaign progress: 377/833 complete objects, 4,227/11,060 exact functions,
  510,617/2,198,102 meaningful code bytes.
- Halo progress: 275/468 complete objects, 4,060/7,574 exact functions,
  497,703/1,770,166 meaningful code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

Only `source/saved games/saved_game_files.c` and this new Jonas-owned ledger
change.  Protected Units, Vehicles, Game Engine, and AI sources remain
untouched.  No push is performed.
