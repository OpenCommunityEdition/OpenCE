# `devices.obj`: change-only-once-more flag setter (2026-08-29)

## Result

`_device_group_change_only_once_more_set` is newly strict-exact on its first
and only production candidate compile. The wave adds one function and 64
padded code bytes. `source/devices/devices.obj` advances from 10/30 to 11/30
strict functions and from 512/4,688 to 576/4,688 strict padded bytes. The
object remains `NonMatching`; no object-completion, data, semantic exception,
park, or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_device_group_change_only_once_more_set` | 64 | 2 | `f40e22ee1dc9c90d2e1f58377e9da5021f7ec0ae1d16370efa3c9a946624480f` |

The exact relocations are a `DIR32` at `+0x0D` to
`_device_groups_data` and a `REL32` at `+0x13` to `_datum_get`. Every
normalized byte, padded byte, relocation address, type, destination, owner,
and addend agrees with the January target.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. The
target loads the group index as a 32-bit value, rejects `NONE`, performs the
typed data-array lookup, conditionally sets or clears bit zero of the group
flags, and then always clears bit one. The target deliberately writes the
flag byte twice.

The independently reconstructed HCEA source at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/device_group_change_only_once_more_set.c`
confirms the same public behavior and names the bits as
`_device_group_can_change_only_once_bit` and
`_device_group_changed_once_bit`. Its foreign addresses and data-access macro
were not imported. The production implementation instead reuses the existing
eight-byte `device_group_datum`, the repository's typed `datum_get` contract,
and the already verified `device_groups_data` owner.

January's `mov eax,[ebp+8]` establishes a 32-bit formal, so the stale
source-local declaration in `source/hs/hs.c` was corrected from `short` to
`long`. The existing `_code_000af070` evaluator remains strict-exact at 64
padded bytes, three relocations, and normalized SHA-256
`c515a88cdfbb7226898a9768ac798384a68c1536622b405ae414797e13b69e00`.

## One-shot and policy boundary

The natural typed body and the declaration correction were frozen before one
normal Ninja build of `devices.obj` and `hs.obj`. The new Devices function was
strict immediately; no alternate spelling, body retry, compiler flag,
adjudication, comparator exception, or object manipulation was used. The ten
inherited Devices functions and the HS evaluator remain exact.

The retained code is C89-compatible readable C with one parameter per line
and an explicit terminal `return;`. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or offset,
pointer/integer reconstruction, representation pun, undefined behavior,
private ABI, synthetic anchor, or byte-forcing expression.

## Validation

- January `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Final rebuilt `devices.obj` SHA-256:
  `222c3696a28d31ff1e519a90a2d08f0abd43709c81e917df22f925cb7479843`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,708/8,246 functions, and
  652,359/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 64 bytes.
- Semantic audit: 470 units, 4,853 functions evaluated, 4,726 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,736 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `facd1df762b70a4b39560e0ede34e19f49af48cf0848fe5057535969fb081ed1`.
- `build/semantic_report.json` SHA-256:
  `0e4ac1faf68f03505a28a22d7d651fd517db6b25896974b228bcce93736f2bfa`.
- `git diff --check`: clean apart from Git's line-ending notices.
