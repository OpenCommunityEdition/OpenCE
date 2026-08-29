# `devices.obj`: device-group allocation initializer (2026-08-29)

## Result

`_devices_initialize` is newly strict-exact on its first code-producing
production candidate compile. The wave adds one function and 64 padded code
bytes. `source/devices/devices.obj` advances from 11/30 to 12/30 strict
functions and from 576/4,688 to 640/4,688 strict padded bytes. The object
remains `NonMatching`; no object-completion, data, semantic exception, park,
or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_devices_initialize` | 64 | 7 | `90d04b8c64654785e2e0a5dc1ae6ef7bf63fe68fc3518d3d1b67961ab4cde23d` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with the January target.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show a `game_state_data_new` call with the literal `"device groups"`, maximum
count 1,024, and datum size eight. The returned typed data-array pointer is
stored in `device_groups_data`, checked, and on failure reported with source
line 72 before the normal fatal exit.

The independently reconstructed HCEA source at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/devices_initialize.c`
confirms the allocation and arguments. Its foreign address and declaration
were not imported. The production implementation uses the repository's
existing `game_state_data_new`, typed `device_groups_data`, verified
eight-byte `device_group_datum`, and standard `match_assert` contracts.

The prior
`docs/object_matching_logs/devices_obj_jonas_typed_leaf_wave.md` remains the
fixed authority for the rejected getter and automatic-operation shapes. A
reconnaissance compile repeated the documented natural getter miss before
that ledger was found: both 48-byte functions again placed the zero-real
relocation before the checked-object call and were removed immediately. No
second shape or retained source resulted, and they receive no credit here.

## One-shot and policy boundary

The initializer's natural typed body was frozen before compilation. An
incorrect header path caused preprocessing to fail without emitting an
object; correcting that include path did not change the candidate body. Its
first code-producing compile was strict immediately. No alternate spelling,
body retry, compiler flag, adjudication, comparator exception, or object
manipulation was used for the retained function.

The retained code is C89-compatible readable C with one parameter per line
and an explicit terminal `return;`. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or offset,
pointer/integer reconstruction, representation pun, undefined behavior,
private ABI, synthetic anchor, or byte-forcing expression.

## Validation

- January `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Final rebuilt `devices.obj` SHA-256:
  `5f85db6cc30c6299a333fed3a88aff9a8bf9b96a2251ff9f04ae4243c019b876`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,710/8,246 functions, and
  652,487/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 64 bytes.
- Semantic audit: 470 units, 4,855 functions evaluated, 4,728 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,738 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `87227994cb41f7b44688723bf6a1e84dd6d936abea5646411788acbccf7aeb84`.
- `build/semantic_report.json` SHA-256:
  `c5455554a00201918923dbf092e0c847dc6a077aab19c65c8164b9c9861a27ae`.
- `git diff --check`: clean apart from Git's line-ending notices.
