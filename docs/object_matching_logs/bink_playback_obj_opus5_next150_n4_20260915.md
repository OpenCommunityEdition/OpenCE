# `bink_playback.obj` — next-150K lane, wave n4 (relocation-census pass, 2026-09-15)

## Scope and provenance

- Translation unit `source/bink/bink_playback.c`; target `build/split/source/bink/bink_playback.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/next-150k-house-clean-20260915`, HEAD `927d135ec`; the file last changed at `3083f800b`.
  No header, config, park, build or tooling edit; `git diff --stat -- source/bink/bink_playback.c` is
  empty.
- Ledgers read first: `bink_playback_obj_opus5_next150_n1_20260915`,
  `bink_playback_obj_opus5_next150_n3a_20260915`, `bink_playback_obj_opus5_150k_w2_20260914`,
  `bink_playback_obj_fable_graph_reconciliation_20260908`, `bink_playback_obj_jonas_six_leaf_20260821`,
  `bink_playback_obj_jonas_stop_render_followup_20260821`,
  `bink_playback_obj_jonas_anonymous_ret_leaf_20260830`,
  `fable_bink_ui_network_hs_reconciliation_20260908`, all three `config/parked.json` entries,
  `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, both worker briefs.
- Worker notes and every candidate: `scratch/workers/n4_bink_playback.md`,
  `scratch/workers/n4_bink_playback/`.

## Baseline (the prompt's row list was stale)

The wave prompt quoted `exact 20 / residual 4` with `_bink_free_texture_cache` non-exact; that row
landed at `3083f800b`. Measured here:

    == emitted-symbol guard passed (1 forbidden names checked)
    == exact 21  residual 3  unwritten 0  (of 24 listed)

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline real file (guard passed) | 21 | 3 | 0 |
| final real file (unchanged, guard passed, park drift 0) | 21 | 3 | 0 |
| candidate `n4_draw_frame.c` | 22 | 2 | 0 |
| candidate `a1.c` | 22 | 2 | 0 |
| candidate `n4_combined.c` | 23 | 1 | 0 |

All three residuals are parked, so nothing can land in the real file. `branch_sweep` over 10 historic
blobs: best is `exact 20 residual 4`; HEAD is already the best body anywhere, no donor exists.

## Two park reopens — 1,408 padded bytes, both strict EXACT in scratch

The wave's assignment was a relocation-by-target census delta per target. **Both deltas are real
missing/extra calls, not csplit naming artifacts**, and both go to zero in the reopened bodies.

### `_bink_draw_frame` (912) — a flattened `rectangle2d` local; no owner ruling needed

Candidate `scratch/workers/n4_bink_playback/n4_draw_frame.c` (the current real file plus this one body).

**Census as assigned.** `relocdiff --count-by-target` HEAD vs January: 59 target, 61 candidate. The only
non-artifact rows are `_rasterizer_globals+6` (1 vs 2) and `_rasterizer_globals+8` (1 vs 2) — exactly
the assigned leverage. (The `__real@…` and `_bink_globals` rows that also print carry identical targets
and addends and differ only in `defined-noncode` vs `symbol` classification: the csplit-relocation-alias
class.) Frame census: January `mov eax,0x1128`, ours `mov eax,0x1130` — an **8-byte** gap, structural.

**The source defect.** `rasterizer_globals.reserved04.screen_bounds` is a `rectangle2d` (four shorts at
+4,+6,+8,+10). January's full-screen arm copies the whole rectangle as two dwords into a local:

    d   test byte ptr [_bink_globals+4], 0x10
    14  mov ecx, dword ptr [_rasterizer_globals+8]
    1f  mov eax, dword ptr [_rasterizer_globals+4]
    24  mov dword ptr [ebp-0xc], eax
    27  mov ebx, dword ptr [ebp-0xc]
    2a  mov edi, ecx
    2c  shr edi, 0x10
    2f  mov dword ptr [ebp-8], ecx
    32  mov ax, cx
    35  shr ebx, 0x10

The parked body has no such local: it declares four function-scope scalars
`short top, left, bottom, right;` and assigns each from its own field, so it loads +4,+6,+8,+10
individually (0x42..0x4f) — two extra relocations, and 8 more frame bytes than one 8-byte rectangle.
The fix is the missing block-scope `rectangle2d screen_bounds;` with a struct copy in the full-screen
arm (laws_w2 A6 frame layout; laws_w3 A33 lvalue topology).

**Result.** 912 / 59 / `24aa7c5023629b6b` strict EXACT; `gate --all` 22/2 with every baseline-EXACT row
unchanged. Post-fix census: **59 target, 59 candidate, 0 differing rows**.

**Disclosure (unchanged from n3a, whose `d7` body this is).** The second hunk is the loop subscript
`vertices[corner_index-1]`. January's bytes carry two lockstep counters — a 16-bit 0-based
`vertex_index` (`xor edx,edx`; `cmp dx,1`; `cmp dx,4 / jl`) and a 32-bit 1-based `corner_index` homed at
`[ebp-0x18]` (`mov dword [ebp-0x18],1`; `and esi,2`) — and the strength-reduced cursor is built from the
32-bit one (`lea ecx,[ebp-0xa8]` = `vertices+4`, no `movsx`), which fixes the emission slots of both
increments. `NUMBEROF(vertices)` must be the literal `4`: `NUMBEROF` is `size_t`, giving an unsigned
`jb`, where January compares with a signed `jl`. With both counters in scope a programmer could have
written `vertices[vertex_index]`; that alternative is measured (n3a `d0r`/`d2`/`d3`, 912/59 `[sha]`).

**Reopen criterion.** Satisfied. The park's recorded `[relocs 61!=59]` premise is refuted: the gap was a
flattened `rectangle2d`, not a codegen tie.

### `_bink_alloc@4` (496) — a helper we call out of line and a predicate we inline; one owner ruling left

Candidate `scratch/workers/n4_bink_playback/a1.c` (the current real file plus this one body, minimal:
only hunks inside `bink_alloc`; n1's two byte-inert `dassert`→`vassert` hygiene hunks in
`bink_playback_start` and `bink_free` are dropped and the row is still exact).

**Census as assigned.** 52 target, 51 candidate:

    _csmemset                                     target 2   candidate 1
    _GlobalMemoryStatus@4                         target 2   candidate 1
    _is_all_bink_memory_free                      target 2   candidate 1
    _bink_get_memory_available                    target 0   candidate 1
    .rdata "end bink_alloc"                       target 0   candidate 1

Frame census: identical (`sub esp,0x20`, one `MEMORYSTATUS` home at `[ebp-0x20]`).

**Two source defects, both confirming the leverage.**

1. *A helper January inlines that we call out of line.* `bink_get_memory_available(const char *)` is
   expanded twice in January (`_csmemset` + `_GlobalMemoryStatus@4` at @0012/@0025 and again at
   @0172/@0185 — the "begin" and "end" sites); ours inlines only the first and emits
   `push "end bink_alloc"; call _bink_get_memory_available; add esp,4`. Function-independent proof:
   `tinfo --data` shows January's `.rdata` holds `begin BinkOpen` / `end BinkOpen` but **no**
   `begin bink_alloc` / `end bink_alloc` literal — an inlined call whose unused `const char *` argument
   is dropped emits no literal.
2. *A predicate January calls out of line that we inline.* `is_all_bink_memory_free()` is called twice
   in January (@0047 assertion condition, @0070 the `if`), once in ours:
   `match_dassert` is `do { match_vassert(...); } while (FALSE)` (cseries.h:109) and the
   `do/while(FALSE)` inflates VC7's estimated frequency of the enclosed call, auto-inlining the small
   static predicate. `docs/assertions.md` makes `dassert` a project convenience wrapper, not a claim
   about January's text.

Third fact (n1, reused): a `boolean can_allocate;` holding the three-operand guard chain is required —
the helper inlines under 0, 1 or 2 `&&` operands and declines at 3 (laws_w2 A13) — and with the boolean
the assert line anchors 735/759/786/787/799 plus the `ja/jge/jne` polarity put the failure arm first.

**Result.** 496 / 52 / `09c916253ce6d4de` strict EXACT; `gate --all` 22/2, no baseline-EXACT row
changes. Post-fix census: **52 target, 52 candidate, 0 differing rows**. The owner census correctly
*loses* the `"end bink_alloc"` literal owner, which January's object does not contain either.

**The one blocker — one byte.** January's failure arm ends:

    c2  call _display_assert
    c7  push -1
    c9  call _system_exit
    ce  add esp, 0x14
    d1  int3            <-- site-local trap (the old filtered-corpus count was later superseded)
    d2  pop edi ... d9 ret 4

New this wave: the last non-`__asm` route is refuted **from the vendor headers**, not only from
`source/` (which n3a had already swept):

- `DebugBreak()` (`xbox/include/WinBase.h:912`) is a `WINBASEAPI` function — a 5-byte `call` with its
  own relocation. January's `int3` is one byte with no relocation.
- `_CrtDbgBreak()` (`xbox/include/CrtDbg.h`) *does* expand to `__asm { int 3 }`, but only on the
  `#elif defined(_M_IX86)` arm at line 378. The live arm is line 376,
  `#if _MSC_VER >= 1300 && !defined(_CRT_PORTABLE)` → `__debugbreak()`, and `_MSC_VER` is 1300 for
  VC7 13.00.9254. Without `_DEBUG` (this build uses `/DDEBUG`) the macro is `((void)0)` at line 232.
- `__debugbreak()` was measured in n1 (`pQ.c`): the intrinsic is schedulable and sinks to 0xd4 inside
  the epilogue — positive evidence the original text was inline assembly.
- `__declspec(noreturn) system_exit` is refuted by the local control: `match_assert` lowers to
  `display_assert(...); system_exit(-1);` at thousands of sites image-wide, while only this assertion
  sequence inside `_bink_alloc@4` carries the following `int3`; the trap is site-local source text, not
  an attribute on the callee.
- `halt_and_catch_fire` (main.c:2766) and `system_exit` (cseries_windows.c:116) are ordinary out-of-line
  functions, and `fast_ftol` (cseries.h:314) is the only `__asm` in any project header.

So the only construct that produces January's byte is a literal `__asm { int 3 }` at this one site.
House rule 10 admits inline assembly only for an evidenced helper/math routine, so this needs an owner
ruling. The **declined** cseries.h `debug_break()` macro is not re-proposed; the open question is the
narrower one the n1 verifier asked: a single, unit-local, one-site `__asm { int 3 }` in a non-helper
function, disclosed by the fake scan.

**Reopen criterion.** The park's `tu-context-optimization` premise is refuted (both defects are source
facts, both fixed). The row is strict EXACT and blocked solely on the `__asm { int 3 }` policy ruling.
If the ruling is no, keep the park: do not land `a1.c` minus the trap (a zero-credit size-changing body,
R11).

## `_bink_query_analog_controller_buttons` (80) — not a wave n4 target

Not in this wave's assignment, on the laws_w2 §D do-not-repeat table (5 shapes), and wave n3a has
already filed the park-reopen with a strict-EXACT candidate
(`scratch/workers/n3_bink_playback/q3.c`, 80 / 1 / `c88d131ba2ff1ac7`) and its own frame-census proof
(January emits no EBP frame at all, so it has no stack local). Its relocation census is 1 == 1, i.e. no
leverage of the kind this wave works on. Zero shapes spent; the n3a proposal is not duplicated.

## Checks

- Final real-file gate: `== exact 21 residual 3 unwritten 0`, emitted-symbol guard passed, identical to
  baseline row for row; `scratch/parkcheck.py` on the HEAD object: parks 3, drift 0;
  `git diff --stat -- source/bink/bink_playback.c` empty.
- `n4_draw_frame.c`: gate 22/2; point guard passed; parkcheck drift 1, the drifting row reaching exactly
  the January measurement `(912, 59, '24aa7c5023629b6b')`; owner census vs `build/base` and
  `build/split` — candidate-only `[]`, absent-from-target `[]`, base-only-lost `[]`;
  `tools/fake_match_scan.py` 0 leads, same as HEAD.
- `a1.c`: gate 22/2; point guard passed; parkcheck drift 1 reaching `(496, 52, '09c916253ce6d4de')`;
  owner census candidate-only `[]`, absent-from-target `[]`, base-only-lost the `"end bink_alloc"`
  literal (correct — absent from January too); fake scan 1 lead, the disclosed `__asm { int 3 }`.
- `n4_combined.c` (both bodies): gate 23/1; parkcheck drift 2, each drifting row at exactly its January
  measurement, `_bink_query_analog_controller_buttons` unchanged at 96/1; owner census and fake scan as
  above.
- CRLF preserved throughout (0 bare LF in every candidate).

## Reusable facts measured here

1. **A relocation-by-target census separates two opposite inline defects in one function.** A callee
   January references *more* often than we do can mean either (a) a helper we call out of line that
   January inlines — the extra references are the helper's *own* callees, here `_csmemset` and
   `_GlobalMemoryStatus@4` — or (b) a predicate January calls out of line that we inline. Both appeared
   in `_bink_alloc@4` at once, in opposite directions, and only the by-target multiset separates them.
2. **A dropped string literal is a function-independent inline census.** A helper with an unused
   `const char *` parameter emits no `.rdata` literal when inlined, so `tinfo --data` on the target
   object tells you which call sites January inlined without disassembling anything (confirmed again
   here: `begin/end BinkOpen` present, `begin/end bink_alloc` absent).
3. **A struct-typed local is visible as a relocation count, not just a frame size.** Copying an
   N-short aggregate whole costs ceil(N/2) relocations and one frame object; flattening it into N
   scalars costs N relocations and N slots. A `+2` relocation delta on adjacent addends of one global,
   together with an 8-byte frame gap, localises the defect to exactly one missing aggregate local.
4. **Vendor headers must be swept before declaring an `__asm` requirement.** `xbox/include/CrtDbg.h`
   does contain `#define _CrtDbgBreak() __asm { int 3 }`, but on a dead `#elif` arm at
   `_MSC_VER >= 1300`; the live arm is `__debugbreak()`, which is schedulable and sinks. Checking the
   *arm selection*, not just the presence of the text, is what makes the refutation sound.
