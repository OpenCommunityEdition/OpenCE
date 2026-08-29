# `rasterizer_transparent_geometry.obj` result-lifetime recovery

## Result

`_rasterizer_transparent_geometry_next_group` is strict semantic-COFF exact.
This advances `source/rasterizer/rasterizer_transparent_geometry.obj` from
16/19 to 17/19 exact functions and from 1,808 to 1,968 exact padded code
bytes. The object remains honestly partial: `_code_00174120` is a 176-byte
residual and `_rasterizer_transparent_geometry_initialize` remains unwritten
at 160 bytes.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_transparent_geometry_next_group` | 157 / 160 | 12 | `bb7cb8a6e864760c768872d911609c2ffd98a894e67b681005f3a0c68af9fb7b` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`. All 12
relocation records agree in address, type, destination, and addend. The
relocations cover the group count and array fields, both assertion strings,
the shared source path, and the two `display_assert` / `system_exit` pairs.
The whole-unit campaign gate reports 17 exact, one residual, and one unwritten
function, so all sixteen inherited exact functions remain exact.

## Source evidence and decisive lifetime

The retained function follows the already-established January topology and the
independent Stian lift: it reads a separate 16-bit `group_sorted_index`, forms
the next 16-bit sorted index, validates the full-width group field, and returns
the typed group selected through the sorted-index array.

The final mismatch was not the algorithm. It was the null-result lifetime.
January's null-input edge preserves the parameter already in `EAX`, while the
out-of-range edge separately materializes zero before the common epilogue. The
ordinary source form that expresses that distinction is:

- initialize a typed `next_group` result from `group`;
- overwrite it with the computed successor when one exists;
- overwrite it with `NULL` only for an out-of-range successor; and
- return the result once.

This source causes VC7 to emit January's exact `xor eax,eax; pop edi` ordering
for the out-of-range edge while allowing the null-input branch to join after
the zeroing instruction. No assembly, barrier, qualifier trick, or artificial
reference is involved.

## Adjacent evidence wave

Two newer public donors were reviewed before the exact recovery:

- Stian's transparent-geometry lifecycle lift confirmed the three allocation
  sizes and the initializer's error / auxiliary-buffer semantics. Its readable
  local-result form still compiles to the already-documented diagnostic
  block-placement fixed point, so no initializer body was retained.
- Pastudan's sort lift supplied a genuinely new loop-local group-pointer clue.
  Combined with the previously measured second-pass lifetime correction, it
  reproduced January's two-register prologue and second-pass register layout,
  but the first pass still chose the EAX/ECX allocation instead of January's
  ECX/EDX allocation and loaded the qsort base one byte early. The nonexact
  sort-driver changes were removed.

Those rejected candidates made no production-source, configuration, semantic
ledger, parked-function, comparator, or tooling change.

## Validation

- January split object SHA-256:
  `d1bcf0d85a17c27f0cb9797931187fad486c2ac7b8ce7bcb43e706422c160722`.
- Retained rebuilt object SHA-256:
  `8c3894f4b037f12bff1d3e88ddec3c6feee356e57262ae9947e56711c4b22e62`.
  Raw VC7 object hashes are recorded for provenance; normalized COMDAT bytes
  and ordered relocations are the equality authority.
- Full `halobetacache_build`, `libcmt_build`, and `all_source` graphs: pass.
- Ordinary progress: 573,746 / 2,198,102 code bytes and 4,570 / 11,060
  functions; Halo progress: 560,832 / 1,770,166 bytes and 4,403 / 7,574
  functions.
- Semantic audit: 470 units, 4,719 functions evaluated, 4,597 semantic exact,
  127 hidden exact, 4,607 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked-function validation: 13 active, zero stale, and zero invalid.
- Tooling suite: 205 passed, with only the inherited unwritable pytest-cache
  warning.
- Strict board: 274/619 fully exact objects, 627,687 / 1,922,669 bytes, and
  4,578 / 8,246 functions, excluding parked and assembly-implemented credit.
- `git diff --check`: pass.

## Policy boundary

The retained implementation is readable typed C89-style source. Every
parameter remains on its own line, and the function has one explicit terminal
return. It introduces no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, compiler-option change, raw address or offset,
undefined behavior, representation pun, synthetic caller, byte forcing,
object patch, comparator exception, or admission exception. `matrix_math.obj`,
Vehicles, Units, and all protected AI and game-engine sources were untouched.
