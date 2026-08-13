# `wcslen.obj` exact-match log

## Plain-English behavior

This CRT routine walks a UTF-16 string until its zero terminator and returns
the number of 16-bit characters before it.

## Provenance and verification

- January target: one 22-byte cdecl function, zero relocations, no data.
- Compiler: XDK 3911 CL 13.00.9254.1 with the libcmt project's unchanged
  `/O1 /Gy` flags.
- A pre-tested `while (*end) end++;` shape compiled to 25 bytes and was
  rejected. The original CRT idiom advances the pointer in the condition,
  `while (*end++);`, then returns `end - string - 1`. Exact verification
  requires the target's word load, two byte-wise pointer increments, signed
  divide-by-two shift, and final decrement—not just equivalent behavior.
- No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
  or compiler-flag change is used.

## 2026-08-13 independent salvage audit

The completed local result at commits `f2c0f207` and `8b1ff695` was treated as
untrusted. The source-only change from `f2c0f207` was transplanted without its
history onto authoritative campaign tip `0244dd18`, then rebased and fully
revalidated on the later authoritative tip `b7bde2b6`. It was rebuilt from
scratch and compared against the independently extracted January target
object after that final rebase.

### Target inventory and ownership

- The target owns exactly one runtime section: a 22-byte `.text` COMDAT.
- `_wcslen` is the only function. It is a public cdecl symbol with COFF type
  `0x20` and external storage class `2`.
- There are zero relocations and no target-owned `.rdata`, `.data`, or `.bss`.
- The target instruction sequence is one pointer walk: load a UTF-16 word,
  advance the pointer by two bytes, loop while the word is nonzero, subtract
  the original pointer, divide the byte distance by two, and exclude the
  terminator.

### Measured source shapes

| Shape | Target/candidate size | Relocations | Result |
| --- | ---: | ---: | --- |
| Pretested `while (*end) end++;` | `22/25` | `0/0` | Rejected: different loop topology |
| Post-increment `while (*end++);` | `22/22` | `0/0` | Accepted: strict-exact |

The accepted source is the normal readable CRT idiom, not a code-emission
trick. The hardened comparator reports the same normalized SHA-256 for both
objects: `ab4bab194aae7751eae686a11b9b3777dd65c29138d9a7d064cc8a88cb137299`.
The candidate preserves the target's external function ownership and contains
no extra runtime sections.

### Scope and standards audit

This is an isolated CRT translation unit, so it changes no shared declaration
and has no sibling function to perturb. It uses the canonical XDK 3911
compiler and the generated libcmt `/O1 /Gy /I"xbox/include"` flags unchanged.
The sole parameter is on its own line and the function has one explicit
return. There is no assembly, `volatile`, `__forceinline`, alignment pragma,
undefined behavior, byte forcing, magic offset, compiler flag change, or
comparator exception. The function reproduces no known original bug requiring
a preservation comment.

### Project gates

- A forced target/candidate rebuild and whole-runtime-object audit passed:
  22/22 bytes, 0/0 relocations, exact normalized hash, exact symbol type and
  storage class, and no owned data.
- `ninja halobetacache_build libcmt_build` completed successfully.
- `ninja semantic_progress` and `ninja progress` completed with zero semantic
  unit errors. At the final campaign tip, the libcmt category reports 3/212
  complete objects, 4/476 exact functions, and 142/55,015 exact code bytes.
- A clean post-admission regression snapshot and rebuild/check passed with
  `ok: true`, no failures, no warnings, and `_wcslen` in `still_exact`.
- The admission audit reports no `wcslen` contradiction or revocation.
- The complete tool suite passes 177/177 unit tests, and `git diff --check` is
  clean.
