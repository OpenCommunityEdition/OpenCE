# `debug_memory.obj` external realloc-topology rejection (2026-08-26)

## Result

An independent Halo PC reconstruction supplied genuinely new source evidence
for `_debug_realloc`: separate old/new header values, a mutable total-allocation
size, reuse of the `file` and `line` parameters, and a success-or-zero-size
accounting join. One ordinary typed-C adaptation was compiled once. It moved
away from the January object and was inverse-reverted wholesale.

`source/cseries/debug_memory.obj` therefore remains honestly `NonMatching` at
15/18 strict functions. No source, configuration, completion label, semantic
exception, parked record, target artifact, compiler option, protected Units/AI
path, or comparator rule survives this wave.

## Independent provenance

The user-supplied `stianeklund/halo` checkout at commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` contains the PC retail
reconstruction in `src/halo/cseries/errors.c`, raw SHA-256
`3c4475d8b57c7a1870e64851488205fb72cf6da65b197997bcfb047b4219cf9b`.
The same body also appears in the independently checked `pastudan/halo`
checkout at commit `918af885935ec470a31256ecce9a977b12b01f80`.

Only value lifetimes and control topology were adapted. The candidate retained
the project's typed structures, symbols, assertions, one-parameter-per-line
formatting, and explicit terminal return. It imported none of the donors' raw
addresses, offset-based structure access, function-pointer address casts, or
missing house style. It used no assembly, `register`, `volatile`, barrier,
pragma, intrinsic, attribute, pointer-integer reconstruction, pointer/union
pun, undefined behavior, synthetic anchor, byte forcing, object patch, or
compiler/tool substitution.

This is new evidence relative to the rejected predicate-only shot documented
in `debug_memory_obj_jonas_realloc_accounting_20260825.md`. That shot proved
the accounting predicate but left the earlier lifetime/register topology
unchanged. The external donor directly supplied the missing lifetime family,
so one new production wave was justified.

## Frozen candidate and one-shot result

Starting integration commit:
`689bd91e8cb5683c9e540083637be4c817abdf56`. The restored source Git blob is
`fdf84695ba7aa06ed205947425e98f1812dff06f`. The source-identical baseline
object was preserved at ignored path
`build/audit/debug_memory_before_external_realloc_topology_20260826.obj`, raw
SHA-256
`d3626cb65b19dafd6b5f405d1d72e4e94d2d69ca160d9a490a59cd2616c6933d`.
The January split object raw SHA-256 is
`bcf575ff9e4dc3749ba66beb41991f62985262fccc3c643f06251d14ba942244`.

The candidate declared separate `header` and `new_header` values, assigned
`header = NULL` after validation, copied the retained allocation provenance
back into the `file` and `line` parameters, set a named allocation-size value
to zero for a non-null zero-size request, passed that value to
`system_realloc`, and used the donor's success-or-zero-size join before memory
accounting.

A Ninja dry run exposed exactly one normal edge, and exactly that edge compiled
once:

```text
[1/1] CL build\base\source\cseries\debug_memory.obj
debug_memory.c
```

The unchanged first-shot object is preserved at ignored path
`build/audit/debug_memory_external_realloc_topology_first_shot_20260826.obj`,
raw SHA-256
`85f53fef147d27f1f604ac742fbb1c8015218c7cc6989ab69f870bca266365e4`.

| Measurement | January | Baseline | First shot |
| --- | ---: | ---: | ---: |
| Padded bytes | 416 | 400 | 384 |
| Relocations | 25 | 25 | 25 |
| Normalized differing positions | 0 | 315 | 361 |
| Total absolute relocation displacement | 0 | 131 | 214 |
| Normalized SHA-256 | `d812408198d3...` | `9b7027a370ac...` | `b8c79b0d99f9...` |

The candidate did move the first assert relocation from seven bytes after the
target to five bytes before it, but that isolated crossing was not a retained
improvement. The function shrank by another sixteen bytes, normalized distance
worsened by 46 positions, and aggregate relocation displacement worsened by 83
bytes. From the first allocator call onward, the candidate's relocations are
10 to 23 bytes earlier than January. Strict relocation destinations, types,
addends, and order remained the same, but address identity failed.

All 13 externally visible code owners common to the baseline and candidate
were compared with the hardened comparator; `_debug_realloc` was the only
changed function. The already nonexact `_debug_malloc` and `_code_0007cf50`
sections remained byte-for-byte unchanged.

## Closure

The candidate was reverted without a retry. Do not repeat the combined
separate-header/mutable-size/parameter-reuse/goto family, the previously tested
predicate-only family, or equivalent spelling variations. Reopen
`_debug_realloc` only with closer cache-beta source provenance or concrete
evidence for the target's larger pre-call and post-call value lifetimes.

Because no implementation survives, there is no function, byte, object, or
admission credit and no campaign-total change. The next production wave moves
to another object.
