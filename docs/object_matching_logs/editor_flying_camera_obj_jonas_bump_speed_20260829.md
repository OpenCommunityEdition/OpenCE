# `editor_flying_camera.obj`: speed-step recovery (2026-08-29)

## Result

`_editor_camera_bump_speed` is newly strict-exact on its first and only
production candidate compile. The wave adds one function and 64 padded code
bytes. `source/camera/editor_flying_camera.obj` advances from 8/21 to 9/21
strict functions and from 384/5,184 to 448/5,184 strict padded bytes. The
object remains `NonMatching`; no object-completion, data, semantic exception,
park, or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_editor_camera_bump_speed` | 64 | 7 | `6c6b09e9e88642e4143227ea24f02714700152ac4b88371dcb38e39083ab2966` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with the January target.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show an unsigned increment-and-remainder operation on the dword at
`_bss_0031d438+0x78`, indexing the signed dword table at `_rdata_00256c64`, an
x87 signed-integer load, a real store to `_data_002dcc28`, and a promoted
double passed to `_terminal_printf` with `_global_real_argb_white` and the
literal `"speed is now x%f"`.

The independently reconstructed HCEA sources at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/editor_camera_bump_speed.c`
and `C:/Users/isabe/Documents/Codex/reference/halocea/src/data/multiple.c`
confirm the natural operation and the five signed integer steps
`{ 1, 5, 20, 40, 60 }`. Their foreign addresses were not imported.

The production reconstruction extends the existing typed
`editor_camera_globals` layout through its verified 0x7C-byte boundary and
names the dword at offset 0x78 `speed_step`. Compile-time offset and size
assertions preserve those ABI facts. The implementation uses the existing
typed `editor_camera_data.speed` and `editor_camera_constants.speed_steps`
fields.

## One-shot and policy boundary

The typed layout and natural HCEA-confirmed body were frozen before one normal
Ninja build of `editor_flying_camera.obj`; the function was strict immediately.
No alternate spelling, body retry, compiler flag, adjudication, comparator
exception, or object manipulation was used.

The retained code is C89-compatible readable C with one parameter per line and
an explicit terminal `return;`. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or offset,
pointer/integer reconstruction, representation pun, undefined behavior,
private ABI, synthetic anchor, or byte-forcing expression.

## Validation

- January `editor_flying_camera.obj` SHA-256:
  `5e0066e8f28b6c2c79e9e546075922b263342dea5a50d9ef84e8818792d99945`.
- Final rebuilt `editor_flying_camera.obj` SHA-256:
  `a7a9c2f03be92f8b5c9d9f237d21a1ac40920d40e86b2f35d0d90f6aa137a34c`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,709/8,246 functions, and
  652,423/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 64 bytes.
- Semantic audit: 470 units, 4,854 functions evaluated, 4,727 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,737 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `145ad5599af1da6c77be8790c686ea724e4e77839ecc8d10698a8f5e27f6c38e`.
- `build/semantic_report.json` SHA-256:
  `ea9905134a6347cd83cb34042d2e58683aab74a65b0c01f187fb45f8e2955e59`.
- `git diff --check`: clean apart from Git's line-ending notices.
