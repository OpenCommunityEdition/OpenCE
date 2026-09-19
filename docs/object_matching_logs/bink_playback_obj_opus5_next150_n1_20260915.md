# `bink_playback.obj` — next-150K lane, wave n1 (2026-09-15)

## Scope and provenance

- Translation unit `source/bink/bink_playback.c`; target `build/split/source/bink/bink_playback.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/next-150k-house-clean-20260915`, baseline `ae10935da`. No header, config, park or build edit.
- Ledgers read first: `bink_playback_obj_fable_graph_reconciliation_20260908`,
  `bink_playback_obj_opus5_150k_w2_20260914`, `bink_playback_obj_jonas_six_leaf_20260821`,
  `bink_playback_obj_jonas_stop_render_followup_20260821`,
  `bink_playback_obj_jonas_anonymous_ret_leaf_20260830`, `fable_bink_ui_network_hs_reconciliation_20260908`,
  plus all four `config/parked.json` entries, `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md`.
- Worker notes and every candidate: `scratch/workers/n1_bink_playback.md`,
  `scratch/workers/n1_bink_playback/`.

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 20 | 4 | 0 |
| final real file (unchanged, guard passed) | 20 | 4 | 0 |

All four residuals are parked, so nothing lands in the real file. `git diff --stat` is empty, park drift on
the real object is 0, the owner census equals `build/base`, and the fake scan reports 0 leads.

## Two park reopens proposed (592 padded bytes)

### `_bink_free_texture_cache` (96) — strict EXACT in scratch, no owner ruling needed

Candidate `scratch/workers/n1_bink_playback/reopen_free_texture_cache.c` (the current real file with one
token changed). Whole-TU gate 21/3/0; only this row moves; the other three parks do not drift; owner census
equals base; fake scan 0 leads.

**Census.** January calls `_is_all_bink_memory_free` out of line (`@0013`); the base inlines it, which costs
48 bytes, one extra relocation (`_bink_pointer_block_count` + `_bink_pointer_blocks` instead of the call)
and a `push ebx / push edi` pair. `alndiff` shows every other instruction equal.

**Mechanism.** `match_dassert(file, line, expr, diagnostic)` is
`do { match_vassert(file, line, expr, diagnostic); } while (FALSE)` (cseries.h:109), and `docs/assertions.md`
states that the `dassert` family is a project convenience wrapper, *not* a claim about January's source. The
`do { } while (FALSE)` inflates VC7's estimated frequency of the enclosed call, so the small static predicate
used as the assertion condition is auto-inlined. Written as the two-argument `match_vassert` — the spelling
the supplied CE reference files use — the predicate stays out of line and the row is strict EXACT.

The park's premise ("VC7 chooses a different predicate call/inlining context in the diagnostic path") is
refuted: the context is the wrapper, not a TU-context tie.

### `_bink_alloc@4` (496) — strict EXACT in scratch, blocked on one owner ruling

Candidate `scratch/workers/n1_bink_playback/reopen_alloc_plus.c` (contains the free_texture_cache change as
well). Whole-TU gate 22/2/0; no baseline-exact row changes; the two remaining parks do not drift; owner
census adds nothing and correctly *loses* the `"end bink_alloc"` string owner, which January's object does
not contain either.

**Census.** Frame identical (`sub esp,0x20`, the single MEMORYSTATUS home at `[ebp-0x20]`). The base was
missing one relocation and 16 bytes, all of it the second `bink_get_memory_available` expansion: January
emits `_csmemset`, `_GlobalMemoryStatus@4` and a second `_bink_saved_state` inside the success arm
(0x164..0x19c) where the base emits `push "end bink_alloc"; call ...; add esp,4`. Independent proof: January's
`.rdata` holds `begin/end BinkOpen`, `begin/end BinkSoundUseDirectSound`, `begin/end XPhysicalProtect` and
`begin/end event_manager`, but **no** `begin/end bink_alloc`, `begin/end bink_free` or
`begin bink_playback_start` literal — the inlined sites drop the unused `const char *` argument.

**Three source facts recovered.**

1. The `match_dassert` -> `match_vassert` change at the 735 assertion (same law as above) makes January's
   first 50 instructions byte-identical, with both predicate calls out of line at `@0047` and `@0070`.
2. A `boolean` local holding the guard chain restores the missing inline. Measured cut-off (probes
   `pF`/`pG`/`pK`/`pM`/`pH`): the helper inlines at a call site guarded by zero, one or two `&&` operands and
   declines at three; it is conditional *depth*, not caller size (deleting the 786 vassert, the 787 dassert,
   the 799 assert, the whole first `if` block or the else arm never restores it), and not the callee
   (`static __inline` and January's definition order are both inert). This is laws_w2 A13's "a boolean local
   holding the chain gives identical layout but moves the helper decision", with identical condition bytes.
3. The failure arm is written first (`if (!can_allocate) { assert } else { ... }`). Assert line anchors
   735 / 759 / 786 / 787 / 799 put 759 before the success-arm assertions, and the `ja FAIL / jge FAIL /
   jne SUCCESS` encoding is the negated composite. Without the boolean local the arm order is byte-inert
   (the CFG is canonical); with it, the polarity fixes the block layout and the row reaches 496/52,
   byte-identical to January except one instruction.

**The remaining byte.** The wave-local scanner reported January's `int3` at 0xd1 as unique in its
468-object filtered corpus; the final reconciliation census supersedes that raw count. It lies
between the 759 assertion's `add esp,0x14` and the failure arm's epilogue. `__asm { int 3 }` reproduces it
exactly and closes the function strict EXACT. `__debugbreak()` does not: the intrinsic is schedulable and
sinks into the epilogue at 0xd4 (`pQ.c`), which is positive evidence that the original text was an
inline-assembly breakpoint (or a macro expanding to one), not the intrinsic. `__asm int 3` on the pre-boolean
body changes nothing (`pJ.c`), so it is not a steering device — it is the last instruction.

House rule 10 allows inline assembly only for an evidenced helper or math routine, so this needs an owner
ruling. A cseries.h `debug_break()` macro is proposed (worker notes, ORCHESTRATOR-PROPOSAL B) and was
validated with a TU-local define (`m1.c`, strict EXACT). If both forms are declined, the fallback candidate
`pP.c` is byte-identical to January except for that one int3 and carries zero credit.

## `_bink_draw_frame` (912) — not landed; narrowed to a machine-scheduler tie

`scratch/workers/n1_bink_playback/d0.c` rebuilds the w2 `g2` body on this HEAD: 912 bytes, **59 relocations
(= January)**, frame 0x1128, and `alndiff` reports 284 vs 284 instructions with only two differences left:

- January `inc edx; mov dword [ecx+0xc],0xffffffff` vs ours `mov dword [ecx+0xc],0xffffffff; inc edx`;
- January `inc esi; fxch st(1); add ecx,0x14` vs ours `add ecx,0x14; fxch st(1); inc esi`.

Both are permutations of adjacent independent instructions inside one window (each window starts and ends at
the same address on both sides). Everything else — the vertex layout, the store order (position.x,
position.y, texture.x, texture.y, colour), `short vertex_index` (`inc edx` + `cmp dx,4`), the homed
`long corner_index` at `[ebp-0x18]`, the two-dword `screen_bounds` copy and the `>=29` summary gate — is
identical. Dropping the `(pixel32)` cast on `NONE` is byte-inert; store-statement permutation is on w2's
do-not-repeat list and was not re-run.

Classification: machine-scheduler tie, not structural. **Not** proposed as a park re-baseline (zero strict
credit on a complete body, R11). Narrowed reopen criterion: a source form that emits both `for`-increment
statements one slot earlier in the scheduler's ready list, without store permutation.

## `_bink_query_analog_controller_buttons` (80) — skipped

Out of scope for this wave (orchestrator do-not-spend) and on the laws_w2 section D do-not-repeat table with
five spent shapes and a confirmed allocator tie. No shapes spent.

## Reusable laws measured here

1. **`match_dassert` wrapper law.** The `do { } while (FALSE)` of `match_dassert`/`match_dwarn`/`match_dhalt`
   is not byte-neutral: it inflates VC7's estimated frequency of the enclosed call and auto-inlines a small
   static predicate used as the assertion condition. Detector: a residual whose only defect is an inlined
   predicate in a diagnostic path, where the source uses `dassert`. Fix per site with the attested
   two-argument `vassert` spelling; no header edit required.
2. **`&&`-depth inline cut-off (refines laws_w2 A13).** For a 64-byte static helper, VC7 inlines at a call
   site under zero, one or two `&&` operands and declines at three. The decline is not a caller-size budget
   and not affected by `__inline`, definition order or an early return. A boolean local holding the chain
   restores the inline with identical condition bytes.
3. **Breakpoint form.** `__asm { int 3 }` keeps the int3 where written; `__debugbreak()` sinks into the
   epilogue. A January int3 immediately before an epilogue therefore proves inline assembly, not the
   intrinsic.
4. **Dropped string literals prove inlining.** A helper with an unused `const char *` parameter drops its
   argument literal when inlined, so the set of `.rdata` strings in the target object is a direct, function
   independent census of which call sites January inlined.
