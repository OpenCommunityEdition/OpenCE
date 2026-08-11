# `source/sound/sound_classes.obj` strategy ledger

## Strict result

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/sound/sound_classes.obj`.
- All 12 functions pass `tools/coff_compare.py`: 1,104 padded code bytes and
  64 relocations, including destinations and addends.
- All 38 target-owned non-code sections pass strict comparison: 3,319 logical
  bytes and 51 relocations.

## Final residual and resolution

The final residual was `sound_class_get`. Its target and candidate already had
the same 208-byte padded size and all 18 relocation sites, but the old source
returned the temporary pointer `definition`. That left an ESI/EDI
index-versus-pointer register mirror:

| form | padded size | relocs | normalized SHA-256 | result |
|---|---:|---:|---|---|
| `return definition;` | 208 | 18 | `6b1d112cd08fd123c3ce0cdc72232802444600c5e669463144181f64f4572c00` | rejected |
| `return &sound_classes[class_index];` | 208 | 18 | `94cb85335dd2d7a8df3858d5f19def3472fc2fe6869825370127078f852d65a0` | exact |

Both expressions return the same proven element. Re-forming the return from
`class_index` keeps that value live through the assertion sequence and gives
VC7 the January register/coalescing shape. The old classification as an
uncontrollable compiler tie was therefore wrong; it was a missing source
live-range fact.

## Data ownership

The runtime pointer `sound_class_data` is external to this translation unit;
declaring it `extern` avoids a false COMMON/BSS owner. The sound-class
definition table, names, strings, constants, and their relocation destinations
are all included in the 38-section strict comparison above. No mutable pointer
or data span is accepted by byte similarity alone.

## Preserved negatives and reopen rule

Historical work established that declaration-order and pointer-alias spellings
which still returned `definition` did not remove the mirror. Do not repeat
blind register-name or declaration permutations, and do not use `volatile`,
assembly, byte patches, undefined behavior, or compiler-flag changes.

If this unit changes, require all 12 function sections and all 38 target-owned
non-code sections to pass again; the object must not rely on ordinary objdiff
percentage alone.

## House-rule audit

Parameters are one-per-line, no-argument parameter lists use `void` on its own
line, and every function has one explicit final return. The source contains no
raw typed `tag_get`/`object_get` casts, gratuitous alignment directives, or
prohibited codegen controls.
