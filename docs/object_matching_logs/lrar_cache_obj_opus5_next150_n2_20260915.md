# `lrar_cache.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Baseline and final real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes):
**11 exact / 1 residual / 0 unwritten**, park drift 0, fake scan 0 leads.
`git diff --stat -- source/memory/lrar_cache.c` is empty. No credit is claimed.

## `_lrar_allocate` (parked `tu-context-optimization`): not landed

Target 784 / 27 relocs / sha `a34c14dad0afc2a3`; real file 1,056 / 47.

### Census (required before any shape)

- **Frame.** January `sub esp,0xc` plus `push esi/edi` and a shrink-wrapped `push ebx` at +0x47.
  Slots: `[ebp-4]` adjusted_new_block_address, `[ebp-8]` new_block_index, `[ebp-0xc]` search_address;
  the parameter homes `[ebp+8]` (reused for block_index once cache moves to EDI), `[ebp+0xc]` size,
  `[ebp+0x10]` user_data. **Ours is identical.** `alndiff` aligns 262 vs 345 instructions with three
  differing regions and no frame or slot difference at all, so nothing structural is missing.
- **Relocations by target** (January vs ours): `_lrar_get_block` 2/0, `_lrar_verify_block` 0/2,
  `_lrar_verify_cache` 1/3, `_csprintf` 2/4, `_temporary` 2/4, `_display_assert` 5/9, `_system_exit` 5/9,
  the file-name literal 5/9, "appears to be corrupt" 1/3, the block-index assert literal 1/3.
  Every delta is the expansion of `lrar_get_block` at exactly two call sites: **+0x83** (the `else` arm of
  `if (cache->last_block_index == NONE)`) and **+0xCF** (the pre-loop eviction fetch).
- The only other aligned difference is `mov ax,[edi+0x36]` vs `mov si,...` at +0x70, which is downstream of
  the call at +0x83 owning EAX in January's private ABI.

### New measurement: the inline decision has three tiers, per call site

| tier | what is emitted at the site | January examples |
| --- | --- | --- |
| T3 | `lrar_get_block` + `lrar_verify_cache` + `lrar_verify_block` all inlined | `_lrar_flush`'s loop site; `_lrar_allocate`'s third (in-loop) site |
| T2 | get_block + verify_cache inlined, verify_block called | (our sites 1/2 at baseline) |
| T1 | get_block inlined, verify_cache and verify_block both called | January `_lrar_block_address`, `_lrar_deallocate` — a plain depth-0 statement |
| T0 | no inline | **January's sites 1 and 2** |

So January's two sites are strictly colder than a depth-0 statement, and our baseline puts them one tier
hotter than one. w2 recorded only the two-state picture.

### Shape n1 (`scratch/workers/n2_connected_geometry_actor_stimulus_etc/lr/n1.c`)

Retry loop respelled `found = FALSE; while (!found) { ... }` with the fit test as an if/else — the family
idiom of the sibling `_lra_allocate` (`do { ... } while (!result);`). Result **912 / 37**: both sites drop
exactly ONE tier (T2 -> T1; `_csprintf` and `_temporary` return to January's counts of 2). This is the first
natural construct measured to cool these sites at all. Still one tier short.

### Why no admissible shape supplies the second tier

`alndiff` proves January emits **no branch that our build lacks**, so the extra conditional level must be
code-free, i.e. redundant — which the lane prohibits (w2's exact probe `if (size >= 0)` is exactly that).
Every non-redundant candidate emits a test January does not have: `VALID_INDEX(block_index, cache->block_count)`,
`if (cache->blocks)`, `if (cache->block_count > 0)`, a `block ? ... : ...` NULL phi. The inverted polarity
`if (cache->last_block_index != NONE)` is refuted directly: January's `jne` at +0x78 goes to the getter arm,
so the source tests `== NONE`.

### New sibling evidence (not in any previous lrar ledger)

`source/memory/lra_cache.c` (`_lra_allocate`, a 13/14-exact object) is the same author's allocator and fixes
the family idioms: `result` initialised at the top, `do { ... } while (!result);`, `continue` to retry, an
early `return NULL;` size guard, and `verify_lra_cache_block` called **out of line** at do-loop + `if` depth.
The early-return size guard is refuted for lrar by the tier model — it removes two conditional levels and can
only make sites 1/2 hotter — so it was not spent.

### Reopen criterion (refined)

A natural source construct that cools a call site one tier below a depth-0 statement **without emitting a
branch**, or evidence that `lrar_get_block`'s IL cost differed in January (e.g. a different assert macro
expansion). `lr/n1.c` is the new half-way control: any construct that reproduces its cooling twice is exact.

### Checks

n1 kept all 11 siblings strict EXACT and passed the point guard; parkcheck reports drift 0; the production
file is byte-identical to HEAD. No header, config, ninja, configure, pytest or git action was taken.
