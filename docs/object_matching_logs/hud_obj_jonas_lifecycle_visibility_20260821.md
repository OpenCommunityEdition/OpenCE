# `hud.obj` lifecycle and visibility leaf recovery

Date: 2026-08-21
Starting commit: `8de9760a24e3109243b4fd4626c0d1feed876725`
Status: partial, `NonMatching`

## Scope

This bounded pass added four ordinary typed C bodies to
`source/interface/hud.c`. It did not attempt the remaining 18 functions and
does not claim any target data. The sole code-producing candidate compile was
strict exact for all four reviewed bodies, so no source-shape tuning was
performed.

## Provenance

The January PC target object is SHA-256
`4fec7af778c957b49525d56739902f8b4023534e2ea3fbd1af7686c9840f0e31`.
Its disassembly and relocations establish the five-call lifecycle order, the
tail calls, byte argument/return widths, and scripted-global offsets `+0` and
`+1`.

Clean local HCEA reference commit
`c168af2e747d3095d9a29418ae401f3a39544863` independently supplies the
corresponding typed `void(void)` lifecycle bodies and `boolean(boolean)`
scripted setters. The source blobs are:

- `hud_dispose.c`: `91c4427485fe668c46b6d6c5ef0fd5ec294bf5e3`
- `hud_dispose_from_old_map.c`: `7811dd5cba17b789c58b578e5d4c319feb5e5dea`
- `scripted_show_hud.c`: `d47d32534674ca3501bd7c392efb6bc55400b16c`
- `scripted_show_hud_help_text.c`: `9298f525dc79b87a8b81c77a522ce6c05cf598e2`

Existing January-exact callers in `interface.c` and `hs.c` corroborate the
same public signatures.

## Strict COFF result

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_hud_dispose` | 25 | 32 | 5 | `41cc33bb9741aa6f1877cb1e39b98dc8ad04cb2028920d37324330cb7356ff0e` |
| `_hud_dispose_from_old_map` | 25 | 32 | 5 | `41cc33bb9741aa6f1877cb1e39b98dc8ad04cb2028920d37324330cb7356ff0e` |
| `_scripted_show_hud` | 23 | 32 | 2 | `3c54201167c1ae1611e1f4cd396a13ca675f2f9c58be47c580272fec753d2510` |
| `_scripted_show_hud_help_text` | 25 | 32 | 2 | `7b804f0807dcc6606e3aabecd940e4a9ce463f4d65c80709efa979f539c8f2ef` |
| **Total** | **98** | **128** | **14** | |

`tools.coff_compare` reports `all_equal: true`: section extents,
normalized bytes, and every relocation address, type, addend, and symbolic
destination match. The object advances from `0/22` to `4/22` exact functions.

## Ownership boundary

The typed four-byte scripted-global pointee view is authenticated only for the
two accessed bytes. The candidate declares `_hud_scripted_globals` as an
external pointer: it is section zero, value zero, storage class external, and
the candidate emits no `.bss`, `.data`, or `.rdata` owner for it. The January
target defines that pointer inside its eight-byte `.bss` aggregate. Therefore
this pass claims zero data bytes and does not synthesize or duplicate storage.

## Policy and validation

The retained source uses ordinary typed calls and byte fields. It contains no
assembly, volatile scheduling, optimizer barriers, force-inlining, raw address
reconstruction, union aliasing, synthetic anchors, or signed-overflow tricks.
All new void functions have explicit terminal returns and the declarations use
house formatting.

The complete Halo and libcmt build graphs pass. Fresh semantic/progress output
reports zero unit errors and credits exactly four functions and 98 meaningful
code bytes. Admission, parked-function, 179-tool-test, diff, scope, and clean
post-commit forced-object regression gates are required before integration
handoff.
