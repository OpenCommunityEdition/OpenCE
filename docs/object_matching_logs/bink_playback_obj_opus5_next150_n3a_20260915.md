# `bink_playback.obj` — next-150K lane, wave n3a (structural park pass, 2026-09-15)

## Scope and provenance

- Translation unit `source/bink/bink_playback.c`; target `build/split/source/bink/bink_playback.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/next-150k-house-clean-20260915`. No header, config, park, build or tooling edit; the real
  file is byte-identical to HEAD (`git diff --stat -- source/bink/bink_playback.c` is empty).
- Ledgers read first: `bink_playback_obj_fable_graph_reconciliation_20260908`,
  `bink_playback_obj_jonas_six_leaf_20260821`, `bink_playback_obj_jonas_stop_render_followup_20260821`,
  `bink_playback_obj_jonas_anonymous_ret_leaf_20260830`, `bink_playback_obj_opus5_150k_w2_20260914`,
  `bink_playback_obj_opus5_next150_n1_20260915`, `fable_bink_ui_network_hs_reconciliation_20260908`,
  all three `config/parked.json` entries, `scratch/workers/n1_bink_playback.md` and the full n1 impl +
  verifier blocks in `scratch/opus5-next150/wave-n1-results.json`.
- Worker notes and every candidate: `scratch/workers/n3_bink_playback.md`,
  `scratch/workers/n3_bink_playback/`.

## Baseline correction

The wave brief quoted `exact 20 / residual 4` with `_bink_free_texture_cache` non-exact. That is stale:
n1's reopen of that row has already landed (commit `3083f800b`). The measured baseline at this HEAD is

    == emitted-symbol guard passed (1 forbidden names checked)
    == exact 21  residual 3  unwritten 0  (of 24 listed)

and **all three remaining residuals are parked**, so nothing in this object can land in the real file
this wave; everything below is a park-reopen proposal measured in scratch.

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline real file (guard passed) | 21 | 3 | 0 |
| final real file (unchanged, guard passed, park drift 0) | 21 | 3 | 0 |
| candidate `reopen_draw_frame_plus_query.c` | 23 | 1 | 0 |

## Two park reopens proposed — 992 padded bytes, both strict EXACT in scratch

Candidate for both: `scratch/workers/n3_bink_playback/reopen_draw_frame_plus_query.c` (the current real
file plus the two reopened bodies and nothing else). Individually:
`reopen_draw_frame.c` (= `d7.c`) and `q3.c`.

### `_bink_draw_frame` (912) — the recorded `[relocs]` gap was never this function's

**Census first.** Frame census: `mov eax,0x1128` on both sides, and every `[ebp±N]` slot is shared
(`[ebp-4]`/`[ebp-0x14]` fild scratch, `[ebp-0xc]` top, `[ebp-0x18]` the homed `long`, `[ebp-0xac]`
`vertices[4]`, `[ebp-0x5c]` bounds, `[ebp-0x112c]` string) — no slot ours lacks or adds. Relocation
census by target: 59 == 59, identical targets and addends. The park's `[relocs 61!=59]` tag belongs to the
*old parked body*; n1's `d0` shape already closed it, and this wave reproduced that on the post-n1 HEAD as
`d0r.c` (912 / 59 / `[sha]`). `d0.c` itself can no longer be gated here — it still spells line 797
`match_dassert` and would regress the landed `_bink_free_texture_cache`.

`alndiff` of `d0r` vs target: 284 vs 284 instructions, all equal except two windows whose start and end
addresses agree on both sides:

| addr | January | `d0r` |
| --- | --- | --- |
| 0x119 | `inc edx` ; `mov [ecx+0xc],0xffffffff` | `mov [ecx+0xc],0xffffffff` ; `inc edx` |
| 0x123 | `inc esi` ; `fxch st(1)` ; `add ecx,0x14` | `add ecx,0x14` ; `fxch st(1)` ; `inc esi` |

n1 classified that as a machine-scheduler tie. **It is not.** Both windows move together, and what moves
them is the identity of the address induction variable: January's vertex cursor ECX is coupled to the
**long** counter (`inc esi`, one instruction, `add ecx,0x14`), ours to the **short** counter. That is the
lvalue-topology class of laws_w3 A33, i.e. a source fact — which variable the array subscript is written
on.

January's two loop variables, read off its own bytes:

- EDX — `xor edx,edx` (0), `cmp dx,1 / jle` twice (the `>1` top/bottom selector), `cmp dx,4 / jl` bound.
  16-bit compares and a signed `jl`: a `short`, 0-based, bounded by a signed literal `4`.
- ESI / `[ebp-0x18]` — `mov dword [ebp-0x18],1`, `and esi,2` (`TEST_FLAG(x, 1)`, bit 1), 32-bit: a `long`,
  1-based, memory-homed because the register file is full (ebx/edi/eax hold left/right/bottom, ecx the
  cursor, edx the index, esi the scratch).
- Quad recovered: bit 1 of {1,2,3,4} = F,T,T,F = left,right,right,left; `>1` of {0,1,2,3} =
  top,top,bottom,bottom. v0=(left,top) v1=(right,top) v2=(right,bottom) v3=(left,bottom).
- `lea ecx,[ebp-0xa8]` = `vertices + 4`, so the cursor is `&vertices[corner_index-1] + 4`.

**Shapes (7 + 4 controls).**

| # | shape | result |
| --- | --- | --- |
| `d0r` | n1's `d0` body rebuilt on this HEAD | 912/59 `[sha]`, the two windows |
| `d1` | explicit `vertex` pointer, `vertex++` last in the clause | January's exact ORDER, but cursor bias +8 (`lea ecx,[ebp-0xa4]`; `fstp [ecx-4]` is 3 B where January's `fstp [ecx]` is 2 B) → +1 byte, `[sha]` |
| `d2` | clause `corner_index++, vertex_index++` | January's exact BYTE POSITIONS, only `inc esi`/`inc edx` swapped |
| `d3` | `corner_index++;` as the last body statement | identical to `d2` |
| `d4` | in-loop `struct dynamic_screen_vertex *vertex= &vertices[vertex_index];` | bias +8 like `d1`, `[sha]` |
| `d5` | `d1` with `vertex++` between the two index increments | identical to `d1` |
| `d6` | initialiser order `corner_index= 1, vertex_index= 0` | byte-identical to `d0r` (init order inert) |
| **`d7`** | **subscript `vertices[corner_index-1]`** | **strict EXACT 912 / 59 / `24aa7c5023629b6b`** |
| `d8` | one long counter, `short vertex_index` derived in the loop | frame 0x1124 (the short loses its own slot), 60 relocs |
| `d10`/`d11`/`d12` | bound moved onto the long (`corner_index<=NUMBEROF(vertices)`, `<5`) | 928 `[size]` |
| `d7n` | `d7` with `vertex_index<NUMBEROF(vertices)` | `[sha]` — `NUMBEROF` is `size_t`, so the compare goes unsigned; January's `jl` is signed, so the literal `4` is byte-proven here |

**What the programmer meant.** A four-corner quad written with a 0-based vertex counter and a 1-based
corner number: the left/right bit comes from the 1-based corner (`TEST_FLAG(corner_index, 1)`), the
top/bottom half from the 0-based vertex (`vertex_index>1`), and the vertex written is the corner's,
`vertices[corner_index-1]`.

**Disclosure.** The two counters run in lockstep, so with `vertex_index` in scope a programmer could
equally have written `vertices[vertex_index]`. The redundancy is a *January* fact (its bytes carry both
counters, at two widths, with two different initial values), and so is which of the two carries the
address (the IV is built from the 32-bit counter, with no `movsx`). `d0r`/`d2`/`d3` measure exactly what
the other choice costs. If the owner rules `corner_index-1` a steering respelling, the fallback is
`d0r.c` — 912/59 `[sha]`, zero credit, R11: do not land it.

### `_bink_query_analog_controller_buttons` (80) — the park premise is refuted

This row is on the laws_w2 section D do-not-repeat table ("register allocation and scheduling", five
shapes) and n1 skipped it. It was re-opened only because a fresh census produced a fact no ledger records.

**Frame census.** January is `push ebx; push esi` and nothing else — **no `push ebp; mov ebp,esp`, no
`sub esp,N`, zero stack slots.** Ours adds `push ebp; mov ebp,esp; push ecx` … `mov esp,ebp; pop ebp` and
one slot `[ebp-1]` for `button_pressed`. Under `/Oy-` VC7 keeps an EBP frame exactly when the function has
stack locals, so the absent frame *proves* January has no stack local here. The two alignment pads ours
carries (`nop` at 0xf, `lea ecx,[ecx]` at 0x2d) exist only because the extra bytes push the two loop heads
onto 0x10/0x30; January's heads at 0x6/0x25 are too far from a boundary to be padded. That is the whole
16-byte gap.

**Relocation census.** 1 == 1, `_input_get_gamepad_state`, same site. Nothing missing, nothing extra.

**The defect.** January holds `button_pressed` in BL (`xor bl,bl` @0x2, `test bl,bl` @0x6 loop head,
`mov bl,1` @0x38, `mov al,bl` @0x42) and rematerialises the comparison constant as an immediate
(`cmp byte ptr [eax+0x1c], 1` ×3). Ours pins the constant 1 in EBX in the preheader (`mov bl,1` @0xd) and
spends the three compares as `cmp byte ptr [...], bl`, one byte cheaper each — which evicts the boolean to
`[ebp-1]` and costs the frame, a store/reload pair and both pads. This is the recurring constant-pinning
family (memory note "zero-register pinning residual"), inverted: January spends the callee-saved byte
register on the live boolean, we spend it on a folded constant.

**Shapes (3).** `q1` declaration-order swap: inert. `q2` `gamepad` hoisted to a function-scope
declaration: inert. **`q3`** — `if (start==1) {p=TRUE;} else if (back==1) {p=TRUE;} else {loop}` — strict
EXACT 80 / 1 / `c88d131ba2ff1ac7`. The third assignment site raises the boolean above the folded constant
in the allocator's ranking; the boolean takes EBX, the constant returns to an immediate, the stack home
and therefore the EBP frame and both pads disappear, and the three sites cross-jump into January's single
`mov bl,1` at 0x38.

**What the programmer meant.** Poll each gamepad; if START was pressed this frame, or BACK was, skip the
movie; otherwise scan the eight analog-button mirrors for one pressed this frame. `buttons[]` is a `byte`
frames-held tick counter (`update_ticks(byte *ticks, boolean down)`, input.h:209), which is why the test is
against the literal 1 rather than a boolean test.

**Disclosure.** Both arms carry the same assignment and `||` is the more idiomatic spelling, so this is
the laws_w3 A26 device (admitted in the w3 newly-admitted list) *without* A26's strongest evidence:
January merges all three `je 0x38` into one physical `mov bl,1` block, so its CFG alone does not
distinguish `||` from `else if`. The discriminating evidence is the allocation — the `||` form does not
reach January's frameless BL-carried shape in any arrangement measured here or in the five shapes of the
earlier waves, and the else-if form reaches it byte-for-byte. It is R2-adjacent and needs a verifier
judgement.

## `_bink_alloc@4` (496) — OWNER-BLOCKED, no new lever

n1 refuted this park in full and produced a strict-EXACT candidate;
`scratch/workers/n1_bink_playback/reopen_alloc_plus.c` still gates `EXACT 496 _bink_alloc@4`
(`== exact 22 residual 2`) unchanged at this HEAD. The single remaining byte is the corpus-unique `int3`
at 0xd1; `__asm { int 3 }` reproduces it and `__debugbreak()` does not.

New this wave, and negative: there is no existing breakpoint construct in the tree to reuse. The only
`__asm` in any header is `fast_ftol` (cseries.h:314); no `__debugbreak`/`DebugBreak`/`debug_break` exists
anywhere under `source/`; and the assert family cannot be the source of the int3, because `match_assert`
lowers to `display_assert(...); system_exit(-1);` (= January's `add esp,0x14` = 0x10 + 0x4) while
`system_exit` and `halt_and_catch_fire` are ordinary out-of-line functions (cseries_windows.c:116,
main.c:2766), not traps. So no non-asm route exists, and the n1 verifier's narrow question — a one-site,
unit-local `__asm { int 3 }` in a non-helper function — remains the only opening. The declined cseries.h
`debug_break()` macro is NOT re-proposed and no body depending on an ungranted item is submitted. Zero
shapes spent.

## Checks

- Final real-file gate: `== exact 21 residual 3 unwritten 0`, emitted-symbol guard passed, identical to
  baseline row for row; `scratch/parkcheck.py` on the real object: parks 3, drift 0.
- Candidate `reopen_draw_frame_plus_query.c`: gate `== exact 23 residual 1`; the row-by-row diff against
  the HEAD `--all` listing moves only the two reopened rows and no baseline-EXACT row changes; point guard
  passed; parkcheck drift 2, each drifting row reaching exactly its target measurement
  ((912, 59, `24aa7c5023629b6b`) and (80, 1, `c88d131ba2ff1ac7`)) with `_bink_alloc@4` unchanged at
  512/51; owner census vs `build/base` and `build/split`: candidate-only `[]`, absent-from-target `[]`,
  base-only-lost `[]` (identical to a HEAD control object built the same `--source` way);
  `tools/fake_match_scan.py` 0 leads, same as HEAD. `q3.c` and `reopen_draw_frame.c` pass the same checks
  individually. CRLF endings preserved throughout (0 bare LF).

## Reusable laws measured here

1. **Address-IV identity is a source fact, not a scheduler tie.** When a loop carries two lockstep
   counters of different widths, the strength-reduced pointer couples to whichever counter the subscript
   is written on, and the coupling fixes the emission slots of *both* increments relative to the body
   stores. Detector: two adjacent-instruction permutations that both involve an induction-variable update,
   inside windows whose start and end addresses agree — that is not a tie, it is the wrong subscript
   variable. Cites `d0r`/`d2`/`d3` vs `d7`.
2. **Subscript spelling sets the cursor bias.** `array[index].field` biases the strength-reduced cursor by
   +4 here; an explicit `T *p` walked with `p++` (whether declared outside the loop or re-derived inside
   it) biases it by +8, which costs one displacement byte on the store that January emits as `fstp [ecx]`.
   So a one-byte size gap in a pointer-walk loop can distinguish the two spellings. Cites `d1`/`d4`/`d5`.
3. **`NUMBEROF` as a loop bound is byte-visible.** `NUMBEROF` is `sizeof/sizeof`, i.e. `size_t`, so a
   `short < NUMBEROF(x)` compare goes unsigned (`jb`). A signed `jl` against a small literal in January
   therefore proves a plain integer bound and forbids the house-style macro at that site. Cites `d7n`.
4. **An absent EBP frame is a structural proof.** Under `/Oy-`, VC7 emits `push ebp; mov ebp,esp` only for
   a function with stack locals. A target prologue of bare `push`es where ours has an EBP frame proves the
   target has *no* stack local, which converts a "register allocation" park into a locatable defect; here
   the defect was a folded constant pinned into the one callee-saved byte register the live boolean needed.
   Alignment `npad`s downstream of such a gap are consequences, not independent differences.
5. **Duplicating an assignment across an else-if chain can unpin a folded constant** (refines laws_w3 A26
   / the zero-register-pinning note): a third source assignment site to a boolean raised it above the
   comparison constant in VC7's ranking for the callee-saved byte register. Recorded with the caveat that
   January's CFG merges the sites, so the evidence is allocational, not control-flow.
