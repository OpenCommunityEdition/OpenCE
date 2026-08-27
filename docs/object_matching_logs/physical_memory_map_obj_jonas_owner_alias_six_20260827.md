# `physical_memory_map.obj` Jonas owner-alias six-function wave (2026-08-27)

This ledger records a strict relocation-ownership recovery for six of the seven
functions in `source/cache/physical_memory_map.obj`. The code bytes were already
normalized-equal, but the candidate named its 16-byte tentative global
`_physical_memory_map_globals` while January's relocations name
`_bss_004d1074`.

The wave advances the repository from 4,309 to 4,315 strict functions and from
557,959 to 558,327 strict padded bytes. The object advances from 0/7 to 6/7
functions and 0/528 to 368/528 padded bytes; `_physical_memory_verify` remains
the sole residual, so no whole-object completion is claimed.

## Authority and scope

- Published base: `91f18a4b228ec23d32d4977fca20dd6413742a8a`.
- Baseline source blob: `b09b252e1928addfd8168316ba64e2274465eda7`.
- Retained source blob before commit: `da155fdab9e3604a166f4b1985b4d12d8753d5c2`.
- Retained source SHA-256:
  `5fd816f1ebcdd8697aed6da59ed7e9d77cbe59d973debd460c038aa5aa796bc8`.
- January object SHA-256:
  `d3114e107da5d66cdc5f1cd4d2469d24b7d8f95056b316f167d5fae4a6543e7b`.
- Retained candidate SHA-256:
  `31f964b44c42401ad7d1de4742e403864651c2dae5e64b9eedada528a665b87b`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocations, addends, and owner
identity. The existing source remains the readable behavioral authority. Only
the source file and this ledger change; no header, compiler flag, comparator,
admission rule, semantic allowlist, parked entry, or tool changes.

## Natural ownership recovery

The retained source gives the existing tentative definition its January COFF
name and maps the readable source identifier to that owner with a preprocessor
alias:

```c
struct physical_memory_map_globals bss_004d1074;
#define physical_memory_map_globals bss_004d1074
```

This preserves the readable identifier in `match_assert` stringification while
making emitted references name `_bss_004d1074`. The definition remains an
ordinary 16-byte COFF common symbol; there is no raw address, manual offset,
object patch, synthetic data payload, or linker/compiler substitution.

The six strict functions are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_physical_memory_allocate` | 240 | 24 | `f4f1599f2377cdac026d012e389ff5c873a3a2b476d0e30d0c09f5e0d4e108f4` |
| `_physical_memory_free` | 64 | 8 | `c88041a17c1e11c1e28f9c6df9e1f57fe07d46a8b7d7507bc631c11fb69ba4a6` |
| `_physical_memory_get_game_state_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_tag_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_texture_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_physical_memory_get_sound_cache_base_address` | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |

Every accepted function matches all padded bytes and every relocation address,
type, target, and addend. The retained source also normalizes all seven no-arg
definitions and gives every void function an explicit terminal `return;`.

## Frozen experiments and fail-closed boundary

An initial direct-rename experiment was preserved at
`build/audit/physical_memory_map_ownership_house_first_shot_20260827.obj`. It
proved five functions but changed assertion strings and was rejected. A fresh
macro-owner experiment proved the additional allocation function. The required
house-style replay ran exactly one production edge:

```text
[1/1] CL build\base\source\cache\physical_memory_map.obj
physical_memory_map.c
```

It was preserved immediately at
`build/audit/physical_memory_map_macro_owner_house_first_shot_20260827.obj` and
reconfirmed the same six strict functions. `_physical_memory_verify` remains
160 padded bytes with a different first-loop common-symbol addressing schedule;
it is explicitly not accepted.

The retained source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, pointer/integer reconstruction, type
pun, undefined behavior, byte forcing, or comparator/admission exception.
