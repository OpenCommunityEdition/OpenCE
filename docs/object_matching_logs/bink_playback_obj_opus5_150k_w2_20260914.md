# `bink_playback.obj` Opus 5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and provenance

- Translation unit `source/bink/bink_playback.c`; target `build/split/source/bink/bink_playback.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, wave w2 worker. No header, config, park or build edits.
- Ledgers read first: `bink_playback_obj_fable_graph_reconciliation_20260908`, `fable_bink_ui_network_hs_reconciliation_20260908`,
  `bink_playback_obj_jonas_six_leaf_20260821`, `bink_playback_obj_jonas_stop_render_followup_20260821`,
  `bink_playback_obj_jonas_anonymous_ret_leaf_20260830`; park entries for all five residuals. `git log --all`
  (5d3444f79, 303c1d55a, 740bbc33a, ...) and `branch_sweep.py` (8 blobs, none better than 19/5/0).
- Worker notes: `scratch/workers/bink_playback_game_sound_etc.md`; candidates in
  `scratch/workers/bink_playback_game_sound_etc/` (`idle/`, `query/`, `draw/`, `alloc_*`, `bk_*`).

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 19 | 5 | 0 |
| final real file (unchanged, guard passed) | 19 | 5 | 0 |

All five residuals are parked, so nothing lands in the real file. `git diff --stat` is empty. Park drift is 0, the
owner census equals `build/base`, and the fake scan reports 0 leads.

## Per-function outcomes

### `_bink_playback_idle` (160) — PARK-REOPEN PROPOSED (strict EXACT in scratch)

- **Candidate:** `scratch/workers/bink_playback_game_sound_etc/bk_reopen_idle.c`. It is the current real file with only
  the idle body changed.
- **Result:** 160 / 14 / `8a3e23da9d6e73e4…`, equal to the park's target hash.
- **Checks:**
  - whole-TU gate 20/4/0; the row diff shows only idle moving from residual to EXACT;
  - point guard passed;
  - the other four parks do not drift;
  - owner census equals base;
  - fake scan 0.

**Evidence.** January `test cl,0x40; je +0x70` jumps straight to the query call. The computed arm keeps
`!filesystem_check_thread_is_active()` in AL, with no stack home and no frame (FPO, tail `jmp _bink_playback_stop`).
The real file's `boolean allow_skipping= TRUE;` followed by a conditional overwrite gives VC7 a dead initial
definition, which it homes at `[ebp-1]`.

**New reusable law: conditional-initializer law.** A boolean declared with a default and then conditionally
overwritten is stack-homed. The same value defined once per arm stays in a register. Both spellings were measured
strict EXACT:
- `idle/s1.c`: if/else assignment;
- `idle/s2.c`: declare and initialize with `?:` (house rule 17). This is the proposed body.

**Park criterion.** The park was classed "register-allocation: VC7 spills the authentic allow_skipping boolean".
Its premise is refuted: the spill comes from the defaulted-declaration spelling.

Rejected shapes:
- s3, a single-exit skipped flag: C89 compile failure;
- s4, an else-if chain duplicating the end check: 192/15.

### `_bink_draw_frame` (912) — NOT LANDED

The scratch body now matches size, relocations and frame. It is not exact.

**January evidence:**
- The frame is `0x1128`; base is `0x1130`. A frame-size gap is structural.
- `tab_stops` and `cursor` are initialized after the `debug_bink` test, one word per element, spending the zero in
  ESI. Those locals live in the debug block, and the initializer is complete (`{250,0,0,0,0,0}`).
- The debug `bounds` and the quad rectangle share `[ebp-0xc]`.
- The full-screen path copies the rasterizer's `screen_bounds` as two dwords. This explains the +2 relocation delta.
- x0, x1 and y1 stay in BX/DI/AX with their stores eliminated. So the quad rectangle is a separate,
  non-address-taken local in a scope disjoint from the debug block.
- `xor edx,edx; mov [ebp-0x18],1` and the increment order show both variables live in the `for` header:
  `for (vertex_index= 0, corner_index= 1; vertex_index<4; vertex_index++, corner_index++)`.
- The loop bound compare is signed: `NUMBEROF` would give `jb`.
- `TEST_FLAG` and `vertex_index>1` are written inline, with no boolean locals.
- All four x87 values are loaded before the first store.
- The summary gate is `>=29` (`cmp ecx,0x1d; jl`).

**Progression** (`draw/`):

| shape | change | size / relocs | structural rows |
| --- | --- | --- | --- |
| base | — | 912 / 61 | 196 |
| d4 | debug-block scope + inline conditions + signed bound | 896 / 62 | — |
| d9 | disjoint-scope quad rectangle | 912 / 59 | — |
| t1 / e1t1 | complete initializer, direct indexing | 912 / 59 | — |
| g2 | `for` header, `>=29` | 912 / 59 / `0be2c080835d3317` | 6 |

The 6 rows left in g2 are the colour store, `add ecx,0x14` and fxch placement at the loop tail.

**Stopped:**
- Store-statement permutations reached 6–10 rows; permutation steering is prohibited, so the search stopped there.
- `set_real_point2d` forms load right to left, which contradicts January.

**Caveat:** g2 uses four `real` temporaries and a bare block. Both follow the evidence but would need review.

**Reopen:** a form that places the colour store after the `vertex_index` increment without permutation steering.
Start from `draw/g2.txt`.

### `_bink_query_analog_controller_buttons` (80) — NOT LANDED; allocator tie confirmed

**Shapes tried:**

| shape | change | result |
| --- | --- | --- |
| q1 | `&& !button_pressed` in the inner loop instead of `break` | 80 bytes; pin and spill gone, but the found edge no longer jumps straight to the shared `mov bl,1` |
| q9 / q10 | condition order swaps | worse |
| q4 | comma initializer | inert |
| q5 | expression assignment (setcc) | worse |

**Conclusion.** January's break form is reproduced only together with the constant-1 pin in BL, so the park premise
holds.

**Reopen:** a lever that keeps the break form without that pin.

### `_bink_alloc@4` (496) and `_bink_free_texture_cache` (96) — NOT LANDED; new evidence

**New evidence:**
- January never inlines `is_all_bink_memory_free`: all 3 call sites are out of line. It does inline both
  `bink_get_memory_available` calls inside `bink_alloc`. This is a per-callee decision, not a caller budget.
- January section numbers follow bottom-up compile order. Reordering our definitions to January's order
  (`bk_ro1.c`) reproduces the emission order of all 20 Halo functions but changes no bytes.
- The "should not fail" path in `bink_alloc` ends in the only `int3` in the whole 468-object split corpus
  (`int3scan.py`), then returns NULL. `__debugbreak()` reproduces the `int3` (`alloc_b.c`), but not the layout or the
  inlining. Without an attested debug-break macro it is not admissible.

**Refuted levers** (all byte-identical):
- nested ifs in `free_texture_cache` (`bk_v1`);
- non-static predicate (`bk_p1`);
- unprototyped predicate declaration (`bk_p2`);
- definition order (`bk_ro1`);
- fail-first early return in `bink_alloc` (`alloc_c`).

**Reopen:** evidence explaining why the predicate is never auto-inlined, plus an attested debug-break macro.

## Proposals

- **park-reopen `_bink_playback_idle`:** candidate `scratch/workers/bink_playback_game_sound_etc/bk_reopen_idle.c`.
- **Reusable law for the brief:** the conditional-initializer law above.
