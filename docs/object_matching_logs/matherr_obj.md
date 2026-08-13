# `matherr.obj` exact-match log

## Plain-English behavior

This CRT hook declines to handle a floating-point library error by returning
zero. The translation unit also owns the initialized `matherr_flag` value
`9876`.

## Provenance and reconstruction

- January target: one three-byte cdecl function and one four-byte `.data`
  object.
- Compiler: XDK 3911 CL 13.00.9254.1 with the libcmt project's unchanged
  `/O1 /Gy` flags.
- `_matherr` is deliberately declared with an opaque `_exception` pointer;
  the unused structure's layout cannot affect this function and is not
  invented here.
- `_matherr_flag = 9876` reproduces target bytes `94 26 00 00` and external
  symbol ownership.

## Verification

The object is admitted only if the hardened comparator proves the function's
size, normalized bytes, and relocations, and a section audit proves the exact
`.data` bytes and external symbol identity. No assembly, `volatile`, forced
inlining, undefined behavior, byte patch, or compiler-flag change is used.

## Current-campaign strict result

The older source was treated only as a lead. It was transplanted onto
campaign tip `ff944787`, configured anew, and rebuilt independently with XDK
3911 before admission. The fresh result is exact:

| Section | Target/candidate size | Relocations | SHA-256 |
| --- | ---: | ---: | --- |
| `__matherr` `.text` | 3/3 | 0/0 | `4bc724f3...f755e` |
| `_matherr_flag` `.data` | 4/4 | 0/0 | `ee668c09...3c865` |

Both objects expose `__matherr` as an external function at offset zero with
type `0x20`, and both expose `__matherr_flag` as an external data symbol at
offset zero. The `.text` flags are `0x60101020`; the `.data` flags are
`0xC0300040`. The initialized bytes are exactly `94 26 00 00` (`9876` in
little-endian order). Neither object owns `.rdata` or `.bss`.

## House/Berth audit

- The intentionally unused exception argument keeps its opaque structure
  type; no unsupported field layout was invented.
- The function uses one direct readable return and the data declaration names
  the actual initialized CRT global.
- The decimal value `9876` is proven by the target data, not a codegen magic
  number or guessed engine constant.
- No assembly, `volatile`, undefined behavior, forced inlining, byte forcing,
  alignment override, compiler flag change, or comparator exception exists.
- This three-byte default hook contains no original-game bug requiring a fix
  note.

Admission additionally requires successful full Halo/libcmt builds, zero
semantic unit errors, a clean forced-rebuild regression check, and all 179
project tool tests.
