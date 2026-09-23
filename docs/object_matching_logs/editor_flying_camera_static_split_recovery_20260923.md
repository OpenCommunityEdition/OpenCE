# Editor flying camera: private-state split and three exact functions

Base: `d90c9508`. This packet changes only `editor_flying_camera.c`, its
symbol/park/data evidence, and two narrowly tested comparator paths. It does
not claim whole-object completion.

## Credit, kept separate

| Measure | Base | Candidate | Difference |
| --- | ---: | ---: | ---: |
| Halo meaningful exact code | 1,562,154 | 1,564,286 | +2,132 |
| Strict functions across all projects | 7,593 / 8,245 | 7,596 / 8,245 | +3 |
| Editor-camera strict functions | 17 / 21 | 20 / 21 | +3 |
| Halo credited data | 2,572,776 | 2,572,828 | +52 |

The three new target-owned code sections are `_editor_camera_flying_update`
(1,408 padded / 1,407 meaningful), `_editor_camera_set_scripted` (432 /
428), and `_editor_camera_update` (304 / 297). Together they are 2,144
padded bytes. Rename-stable board diff: exactly these three gains, **zero
regressions**.

## State and source provenance

The former 124-byte `editor_camera_globals` aggregate was a reconstruction
convenience, not the original source. The HCEX editor-camera compiland names
separate file statics, in definition order: `is_scripted`, `use_roll`,
`initialized`, `editor_camera_focus`, `editor_camera`, `reset_all`,
`unit_offset`, `camera_mode`, `local_player_index`, `last_scripted`,
`persisted_cameras`, and `speed_step`. The rebuilt BSS offsets are
`0,1,2,4,0x18,0x1c,0x20,0x2c,0x30,0x32,0x38,0x78`, matching the prior
January section census. January's assert text also names bare `camera_mode`.
`is_scripted` is absent from January's PDB publics and is kept `static`;
the symbol map now describes the same private owner rather than externalizing
it to satisfy a comparator.

The flying position update uses the existing shared-header
`point_from_line3d` for point-plus-vector sums, with no new representation
casts. The resulting 48-byte `_point_from_line3d` COMDAT has zero relocations
and normalized SHA `9b763841f8519177`, identical to January's selected
`action_charge.obj` copy. Actual VC7/XDK pair links in both input orders
report no `LNK2005` or `LNK1169`; they stop only at expected unrelated
`LNK1120` unresolved imports. The old `_set_real_point3d` helper emission
disappears; this is the sole new code definition in this TU. This uses the
documented folded-inline exception, not a hand-expanded helper.

`_editor_camera_set_scripted` passes `unit_offset` through the point view
attested by the HCEA source. Stripping the C cast in a memory-only gate leaves
the function byte-exact, showing that the cast itself does not steer the
compiler. It remains a typed, source-attested 3-real view.

## Comparator and data boundary

The target split exposes only its section-origin file-static `_is_scripted`
in `.bss`; MSVC emits that same private origin plus later statics. The former
comparator allowed a private anchor only when it was the *sole* static in the
section, making every correct later reference appear different by name.
`coff_compare` now accepts a unique named static at offset zero as the
section anchor even when later statics exist. It still rejects ambiguous
zero-offset aliases and wrong destination offsets. Target and candidate must
share the anchor name, section and resolved offset. The new focused unit tests
cover all three cases. The full-board stable sweep proves no old exact row
regressed and no unrelated function became exact.

Both camera data sections are independently exact:

| Section / anchor | Raw / padded | Flags | Relocs | SHA | Storage |
| --- | ---: | --- | ---: | --- | ---: |
| `.bss` / `_is_scripted` | 124 / 128 | `c0400080` | 0 | `7b8ec8dd…e9d6fdd9f1` | static 3 |
| `.data` / `_editor_camera_speed` | 52 / 52 | `c0300040` | 9 | `22486dfe…22b880d07b` | external 2 |

Target and candidate share each complete normalized payload and every
relocation destination. The grouped data verifier pins flags, padding, owner
storage and hashes. Its new explicit `credit_raw_size` option is needed
because objdiff reports the BSS as 124 *raw* bytes, while the existing
aligned-rdata group convention scores padded sizes. A test confirms the
raw-size group must exactly cover the report's unmatched section sizes; the
existing padded-size behavior remains unchanged. The split previously credited
the aggregate BSS but not the 52-byte data section. The verified group credits
both for a net **+52 data bytes**, not 176 additional bytes.

## Boundary and verification

`_editor_camera_set_position_and_roll` remains the sole residual:
432-byte target, 464-byte rebuilt body, 30 relocations each; three x87
operand-order regions remain. Its park was remeasured with zero code credit.
The other three parks were retired as exact.

Full Ninja build and progress passed. The rename-stable 8,245-function sweep
found +3/0 regressions. Parks: 96 active, 0 stale, 0 invalid. Admission audit:
0 contradicted and 0 revoked. Changed-source fake scan: 0 leads. Pytest:
1,157 passed, 5 skipped, 26 subtests. `git diff --check` clean.
