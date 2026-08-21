# `path_structure_bsp.obj` Codex salvage

This Codex-owned checkpoint recovers two independently strict-exact functions
from the parked atomic work without importing its unsafe or nonexact bodies.
The unit remains `NonMatching`; no configuration, semantic exception, parked
entry, or pre-existing Markdown changed.

## Scope and provenance

- Authoritative base: `dd371124269b80d25227a42501ae5ef883f618e0`.
- Historical evidence: parked stash commit `fe5ee19e` and the existing atomic
  measurements, used read-only.
- Retained production scope: `structure_surfaces_are_equivalent`,
  `structure_surface_index_from_point`, and the minimal typed declarations in
  `path_structure_bsp.h`.
- The previously exact `clip_empty_interval_by_solid_interval` body is
  unchanged.
- `structure_test_line2d` is declaration-only. Its target symbol remains an
  unresolved relocation from the exact caller, while its unsafe historical
  source and nonexact code section remain absent and uncredited.
- The user-frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`
  sources, objects, configuration, and logs are outside this lane and untouched.

## Strict result

The object advances from 1/6 to 3/6 strict-exact functions and from 104 to 312
meaningful code bytes. This is a gain of two functions and 208 bytes. Padded
code for the accepted set is 336 bytes with five relocations.

| Strict-exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_structure_surfaces_are_equivalent` | 126 | 128 | 4 | `821f618f6425baa324bfbe68c75e8c144d5d0d4bd190dff1606fcd276f781b83` |
| `_clip_empty_interval_by_solid_interval` | 104 | 112 | 0 | `dcef7f55dd07ed3e27a77950d3436383dc2151902aaf5a752fb8b704e4aa1330` |
| `_structure_surface_index_from_point` | 82 | 96 | 1 | `29cfb8955fc45cfc7972505dddb757d4374ca33735024c18aa7f5baac71b8f49` |

The index helper's sole relocation is the target-identical `REL32` reference
to `_structure_test_line2d` at function offset `+0x28`. The full Halo build
accepts the declaration-only form used by this incomplete unit.

No runtime-data credit is claimed. `structure_surfaces_are_equivalent`
naturally emits the target's eight-byte `0.05` constant COMDAT, but the object
still has open data. The historical 64-byte `_rdata_0024ec0c` table had exact
contents, symbol, and zero relocations under a natural declaration, but VC7
gave it eight-byte section alignment while January requires four-byte
alignment. The parked donor reached that flag only with
`__declspec(align(4))`; because the annotation served solely as COFF-shaping,
the table was withdrawn under the campaign's anti-byte-forcing rule.

## Safety audit

The historical exact `_structure_test_ray2d` is deliberately excluded. January
loads `pathfinding_surfaces[enter_surface_index]` at function offset `+0x63`
before its `NONE` comparison at `+0xCC`; the exit path repeats the pattern at
`+0xEC` before `+0x159`. Its breakable-surface test also reads a dword through
the byte-backed flags pointer. Preserving those instructions requires a
sentinel-before-check out-of-bounds read and incompatible typed access. A
defined short-circuit check cannot emit January's order, so the 480-byte body
receives no credit.

The retained source is readable typed C. It contains no assembly, volatile
scheduling device, forced inline, optimizer pragma/barrier, raw-address
reconstruction, incompatible cast, inactive-union punning, byte forcing, or
synthetic anchor. Parameters are one per line and each retained definition has
an explicit final return.

## Regression and repository gates

The clean pre-edit regression manifest reports only the two expected
`NEWLY_EXACT` functions, the new exact floating-constant section and ordinary
symbol inventory, and numeric section-index movement for the unchanged clip
COMDAT. Direct strict comparison proves the clip still has identical padded
bytes, zero relocations, and normalized hash. There are no changed nonexact
functions. A clean post-commit snapshot/rebuild check is recorded after the
new ownership surface is committed.

- Forced `path_structure_bsp.obj`, full Halo, and full libcmt builds pass.
- The strict semantic report scans 470 units and 3,923 functions, finds 3,784
  semantic exact and 3,845 accepted exact functions, and reports zero unit
  errors.
- Campaign progress is 375/833 objects, 3,832/11,060 functions,
  463,956/2,198,102 code bytes, and 1,832,248/4,176,062 data bytes.
- Admission reports zero candidates and zero revocations; the sole inherited
  contradiction remains `source/shell/shell_xbox`.
- Park validation reports three active, zero stale, and zero invalid entries.
- The complete tooling suite passes 179/179 tests.

Final source SHA-256 values are
`58ca289d7362c3a3b7cc52aa2d7786e9261084d79b50b47ec64a7b55ab0b2d72`
for `path_structure_bsp.c` and
`48fed6ab592a71724ddeb3f63ec0b2c4f52ec7444ce0b4a2a205a3fe4f367835`
for `path_structure_bsp.h`. No GitHub push is performed.
