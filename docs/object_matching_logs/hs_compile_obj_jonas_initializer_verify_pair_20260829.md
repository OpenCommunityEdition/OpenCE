# `hs_compile.obj` initializer and source-offset pair (2026-08-29)

## Outcome

This bounded source-only wave recovers the first two strict functions in
`source/hs/hs_compile.obj`. `_hs_compile_initialize` is exact at 176 padded
bytes with 18 relocations, and `_hs_verify_source_offset` is exact at 48
padded bytes with three relocations. The unit advances from 0/64 to 2/64
strict functions and owns 224/17,072 padded code bytes. It remains incomplete;
no completion label, exception, park, comparator, or compiler configuration
was changed.

The January split object is
`build/split/source/hs/hs_compile.obj`, SHA-256
`463ef9c32e0814d6bbf0239926d85de98fd2cf0ee752abc7fa123368626e2403`.
The integration base is `06b8664f284e36b382cce7e1e4dc9fcfbbe3181d`.

## Recovered behavior and layout

The initializer asserts that compilation is not already active, initializes
the typed compiler state, and records whether it is compiling a Scenario. In
Scenario mode it clears the script, global, and reference blocks, clears the
script string data, and deletes every syntax datum. The January disassembly
and the existing typed `scenario` definition independently agree on the
`hs_string_constants`, `hs_scripts`, `hs_globals`, and `hs_references` fields.

The source-offset verifier accepts offsets in `[0, source_size)`. On failure it
stores January's exact `"bad source offset (you need to recompile.)"` message
and returns false. The local compiler-state prefix names only fields consumed
by these functions. Compile-time checks prove offsets `0x00`, `0x04`, `0x08`,
`0x18`, `0x1C`, and `0x125`; the remaining bytes stay explicitly reserved and
the external BSS owner is not redefined.

Direct hardened comparison proves the complete function sections:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_compile_initialize` | 176 | 18 | `79483f6bab71721cd69aee2ad1920cb3376c833166a27913d1f64405a1bc72fc` |
| `_hs_verify_source_offset` | 48 | 3 | `d331d6a71ac3e8d91dc57faa22d0cb16e9df1ad0f56b2e58cfbddb5d51c3a208` |

The initializer's first candidate already matched every instruction and
relocation site. Its sole hardened mismatch was the assertion COMDAT: the
typed storage symbol stringized as `bss_00453480`, while January contains
`!hs_compile_globals.initialized`. A local source alias restores that
authenticated original expression and makes the complete relocation identity
exact without changing runtime code.

## Boundaries and validation

Before this pair, two small candidates were rejected and removed. The Profile
timestamp leaves require the excluded inline `rdtsc` macro. The HS Runtime
macro evaluator depends on a private same-translation-unit `EAX` calling
convention to its unwritten 448-byte callee. A UI alpha-modulation probe was
also removed because January uses the authenticated inline-assembly
`fast_ftol` dword conversion; ordinary C calls `__ftol2`, and `/QIfist` emits
the wrong-width conversion. No rejected source remains.

Validation on the retained source:

- Complete `hs_compile.obj` gate: 2 exact, zero residual, 62 unwritten.
- Full Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,852 functions evaluated, 4,725 semantic exact,
  4,735 accepted exact, and zero unit errors.
- Progress: 382/833 matched objects, 4,696/11,060 functions, and
  595,912/2,198,102 meaningful code bytes overall.
- Strict board: 276/619 objects, 4,707/8,246 functions, and
  652,295/1,922,669 padded code bytes; parked and assembly credit excluded.
- Tool suite: 205/205 tests pass.

The retained implementation is readable typed C with one parameter per line
and an explicit terminal return in the void function. It adds no assembly,
intrinsic, `volatile`, `register`, pragma, attribute, barrier, raw address or
offset access, pointer pun, representation pun, undefined behavior, object
patch, compiler steering, or matching exception. AI Debug remains paused and
untouched.
