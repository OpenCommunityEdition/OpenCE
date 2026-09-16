# `bink_playback.obj` — next-150K lane, wave n5 (LAW Z pin pass, 2026-09-16)

## Scope and provenance

- Translation unit `source/bink/bink_playback.c`; target `build/split/source/bink/bink_playback.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
  `opus/next-150k-house-clean-20260915`, HEAD `3faefe7f6`. The file last changed at `3083f800b`.
- No header, config, park, build or tooling edit. `git diff --stat -- source/bink/bink_playback.c`
  is empty at the end of the wave, as it was at the start.
- Ledgers read first: `bink_playback_obj_opus5_next150_n1_20260915`,
  `..._n3a_20260915`, `..._n4_20260915`, `..._opus5_150k_w2_20260914`,
  `..._fable_graph_reconciliation_20260908`, `..._jonas_six_leaf_20260821`,
  `..._jonas_stop_render_followup_20260821`, `..._jonas_anonymous_ret_leaf_20260830`,
  `fable_bink_ui_network_hs_reconciliation_20260908`; all three `config/parked.json` entries;
  `scratch/WORKER_BRIEF.md`, `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`; worker notes
  `scratch/workers/n1_bink_playback.md`, `n3_bink_playback.md`, `n4_bink_playback.md`.
- Worker notes and every candidate for this wave: `scratch/workers/n5_bink_playback.md`,
  `scratch/workers/n5_bink_playback/`.

## Baseline (the wave prompt's row list is stale, for the third wave running)

    python -B tools/campaign/gate.py source/bink/bink_playback --all --forbid-emitted-symbol _point_from_line3d
    == emitted-symbol guard passed (1 forbidden names checked)
    == exact 21  residual 3  unwritten 0  (of 24 listed)

`_bink_free_texture_cache` (96) is EXACT at HEAD — n1's reopen landed at `3083f800b`. It is not a
residual and was not worked. The three real residuals are `_bink_alloc@4` (496), `_bink_draw_frame`
(912) and `_bink_query_analog_controller_buttons` (80), and **all three are parked**, so nothing in
this object can land in the real file this wave. Everything below is measured in scratch.

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline real file (guard passed, park drift 0) | 21 | 3 | 0 |
| final real file (unchanged, guard passed, park drift 0) | 21 | 3 | 0 |
| candidate `v_a1.c` (alloc only) | 22 | 2 | 0 |
| candidate `v_df.c` (draw_frame only) | 22 | 2 | 0 |
| candidate `v_q3.c` (query only) | 22 | 2 | 0 |
| candidate `n5_clean.c` (draw_frame + query) | 23 | 1 | 0 |
| candidate `n5_all.c` (all three) | **24** | **0** | 0 |

## The wave's assignment: LAW Z on `_bink_alloc@4` — CONFIRMED, and already fixed

The orchestrator assigned this row under LAW Z with the instruction that a pin difference is a source
defect, never a register-allocation tie, and that buying the pin with a redundant store, carrier,
`volatile`, `register`, inline asm, fake dependency or operand permutation is forbidden.

Re-run as instructed:

    python -B scratch/hs_pin/webprofile.py source/bink/bink_playback '_bink_alloc@4'
    JANUARY  67 insns: ebx 0 defs/0 uses ; esi 1 def (xor esi,esi @000a) / 5 uses ; edi 1 def / 1 use
    OURS    137 insns: ebx 1 def (xor ebx,ebx @000b) / 1 use ; esi 4 defs / 8 uses, including the two
                       global loads `mov esi,dword ptr [0]` @002d and @0089 ; edi 2 defs / 3 uses

This wave added `scratch/workers/n5_bink_playback/webprof2.py`, which runs the same profile against an
arbitrary candidate object instead of `build/base`. Against the reopened body:

    CANDIDATE 67 insns: ebx 0 defs/0 uses ; esi 1 def (xor esi,esi @000a) / 5 uses ; edi 1 def / 1 use

**The candidate's callee-saved profile is identical to January's, register for register and def for
def**, and the row is strict EXACT (496 / 52 / `09c916253ce6d4de`). LAW Z is confirmed end to end on a
real target: the pin was fully source-determined, the fix was two source inline-boundary defects, and
no illegitimate store, carrier or permutation was involved. The pinned `esi` is `result` (`void *result=
NULL;`), returned at @00d3 as `mov eax,esi`.

The two defects (found by n1/n4, re-verified here; they are exactly the leverage the orchestrator's
relocation census predicted):

1. **A helper January inlines that we called out of line.** `bink_get_memory_available(const char *)`
   is expanded twice in January — `_csmemset` + `_GlobalMemoryStatus@4` at @0012/@0025 and again at
   @0172/@0185 — while our body inlined only the first site and emitted
   `push "end bink_alloc"; call _bink_get_memory_available; add esp,4`. Function-independent proof
   re-run this wave: `tinfo --data` on the target object contains **zero** occurrences of the string
   `bink_alloc`, while it does contain `begin/end BinkOpen`, `begin/end XPhysicalProtect`,
   `begin/end event_manager` and `begin/end BinkSoundUseDirectSound`. An inlined call whose unused
   `const char *` argument is dropped emits no literal. Our two surplus `mov esi,dword ptr [0]` global
   loads were that out-of-line call's neighbourhood.
2. **A predicate January calls out of line that we inlined.** `is_all_bink_memory_free()` is called
   twice in January (@0047 as the assertion condition, @0070 as the `if`). `match_dassert` is
   `do { match_vassert(...); } while (FALSE)` (cseries.h:109); the `do/while(FALSE)` inflates VC7's
   estimated frequency for the enclosed call and auto-inlines the small static predicate. Spelling the
   site `match_vassert` restores the out-of-line call. `docs/assertions.md` makes `dassert` a project
   convenience wrapper, not a claim about January's text.
3. (n1, retained) a `boolean can_allocate;` holding the three-operand guard chain is required — the
   helper inlines under 0, 1 or 2 `&&` operands and declines at 3 (laws_w2 A13) — and with the boolean
   the assert line anchors 735/759/786/787/799 plus the `ja/jge/jne` polarity put the failure arm first.

Zero new source shapes were spent on this row; the route was already settled by n1/n4 and only the
policy question below is open.

### The one blocker, with new corpus evidence measured this wave

January's failure arm ends:

    00c2  call _display_assert
    00c7  push -1
    00c9  call _system_exit
    00ce  add  esp, 0x14
    00d1  int3
    00d2  pop edi ; mov eax,esi ; pop esi ; mov esp,ebp ; pop ebp ; ret 4

New census over **all 833 January split objects**
(`scratch/workers/n5_bink_playback/int3scan.py`, `sysexit_census.py`, both read-only):

- in-body `int 3` instructions corpus-wide: **2**. One is
  `libs\d3d8\mpintr.obj : ?ServiceGrInterrupt@CMiniport@D3D@@AAEKXZ` @0x130 — a Microsoft vendor
  object. The other is this site. **No other Halo-source function in the image traps.**
- `_system_exit` relocation sites corpus-wide: **230**; `_display_assert`: **253**. Exactly one of the
  230 is followed by `int3`.
- inside `_bink_alloc@4` itself, the `display_assert; system_exit; add esp,0x14` sequence occurs
  **four** times (@0060/@0067, @00c2/@00c9, @0120/@0127, @0149/@0150) and only the second is followed
  by a trap — verified by disassembly: @006c `add esp,0x14` is followed at @006f by
  `call _is_all_bink_memory_free`.

Those four measurements refute every non-site-local explanation simultaneously: a
`__declspec(noreturn)` `system_exit` would trap at 230 sites; an assert-macro trap would trap at four
sites inside this one function; a compiler or flag artifact would not be unique across 833 objects.
n4's vendor-header sweep stands (`DebugBreak()` is a `WINBASEAPI` call with its own relocation;
`_CrtDbgBreak()`'s `__asm { int 3 }` is on the dead `#elif defined(_M_IX86)` arm of
`xbox/include/CrtDbg.h:378`, while the live arm at :376 is `__debugbreak()` for `_MSC_VER >= 1300`),
as does n3a's `source/` sweep (the only `__asm` in any project header is `fast_ftol`, cseries.h:314),
and n1's measurement that the `__debugbreak()` intrinsic is schedulable and sinks into the epilogue.

**Conclusion: the original text at this one site was a literal `__asm { int 3 }`.** House rule 10
admits inline assembly only for an evidenced helper/math routine, so this needs an owner ruling on the
narrow question the n1 verifier asked — a single, unit-local, one-site `__asm { int 3 }` in a
non-helper function, disclosed by the fake scan. The **declined** cseries.h `debug_break()` macro is
not re-proposed. If the ruling is no, keep the park: do not land the body minus the trap, which would
be a zero-credit size-changing body (R11).

## `_bink_draw_frame` (912) — park reopen, no owner ruling needed

Candidate `scratch/workers/n5_bink_playback/v_df.c` (= n4's `n4_draw_frame.c`), re-gated at this HEAD:
strict EXACT 912 / 59 / `24aa7c5023629b6b`, whole-TU 22/2, relocation census 59 == 59.

The source defect is a flattened `rectangle2d`: January copies
`rasterizer_globals.reserved04.screen_bounds` whole as two dwords into a block-scope local
(`mov ecx,[_rasterizer_globals+8]`, `mov eax,[_rasterizer_globals+4]`, `mov [ebp-0xc],eax`,
`mov [ebp-8],ecx`), while the parked body declares four function-scope `short`s and loads +4/+6/+8/+10
individually — the two extra relocations on `_rasterizer_globals+6` and `+8`, and an 8-byte frame gap
(`mov eax,0x1128` vs `0x1130`). Laws_w2 A6 / laws_w3 A33.

New byte evidence recovered this wave on the debug summary threshold:

    JANUARY @02cc  83f91d  cmp ecx, 0x1d      @02d3  7c66  jl  0x33b
    OURS    @02cd  83f91c  cmp ecx, 0x1c      @02d4  7e64  jle 0x33a

January's literal is 29 with a signed `jl`, so the source text is
`bink_globals.rendered_frame_count-bink_globals.frame_count_at_last_summary>=29`, not `>28`. The
spelling is dictated by the encoding, not chosen to steer. A LAW Z cross-check on the same row:
January passes the pinned zero as the NULL `rasterizer_draw_string` argument (`@02ae push esi` where
ours has `push 0`), the Z2 "NULL arguments are spent on the web but do not seed it" case, and the
candidate reproduces January's whole esi/edi/ebx profile def for def.

**Disclosure (carried forward unchanged).** The loop subscript is `vertices[corner_index-1]`.
January's bytes carry two lockstep counters — a 16-bit 0-based `vertex_index` (`xor edx,edx`;
`cmp dx,1`; `cmp dx,4 / jl`) and a 32-bit 1-based `corner_index` homed at `[ebp-0x18]`
(`mov dword [ebp-0x18],1`; `and esi,2`) — and the strength-reduced cursor is built from the 32-bit one
(`lea ecx,[ebp-0xa8]` = `vertices+4`, no `movsx`), which fixes the emission slots of both increments.
`NUMBEROF(vertices)` must be the literal `4` (`NUMBEROF` is `size_t`, giving an unsigned `jb`;
January compares with a signed `jl`). With both counters in scope a programmer could have written
`vertices[vertex_index]`; that alternative is measured (n3a `d0r`/`d2`/`d3`, 912/59 `[sha]`) and must
not be landed as a fallback (R11).

**Reopen criterion: satisfied.** The park's recorded `[relocs 61!=59]` premise is refuted — the gap was
a flattened aggregate local, not a codegen tie.

## `_bink_query_analog_controller_buttons` (80) — park reopen, needs a verifier judgement

Candidate `scratch/workers/n5_bink_playback/v_q3.c` (= n3a's `q3.c`), re-gated at this HEAD: strict
EXACT 80 / 1 / `c88d131ba2ff1ac7`, whole-TU 22/2.

Full disassembly of both sides was re-taken this wave. The CFG is **identical**: same blocks, same
three `je` edges merging into one physical assignment block. The relocation census is 1 == 1. The only
difference is which value owns EBX, the one byte-addressable callee-saved register (LAW Z Z1):

- January carries the live boolean there — `xor bl,bl` @2, `test bl,bl` @6 loop head, `mov bl,1` @0x38,
  `mov al,bl` @0x42 — and rematerialises the comparison constant as an immediate
  (`cmp byte ptr [eax+0x1c], 1` three times, 4 bytes each). It has **no EBP frame and no stack slot**,
  which under `/Oy-` proves it has no stack local.
- Ours pins the constant `1` in BL (`mov bl,1` @0xd) and spends it as `cmp byte ptr [...], bl`
  (3 bytes each), which evicts the boolean to `[ebp-1]` and costs the EBP frame, a store/reload pair
  and two alignment pads (`nop` @0xf, `lea ecx,[ecx]` @0x2d). That is the whole 16-byte gap.

This is the *inverse* of LAW Z's zero-pin family — the pinned value is 1, and the pin is ours, not
January's — so Z2's store-count boundary does not literally govern it; Z1 is what applies.

Two new shapes were run this wave, solely to look for a spelling that avoids `q3`'s duplicated arms:

| # | shape | result |
| --- | --- | --- |
| `qA` | outer condition order `!button_pressed && gamepad_index<MAXIMUM_GAMEPADS` | size 80, relocs 1 — the frame and both pads disappear and the boolean takes BL — but the loop rotation inverts (ours `cmp si,4` at the top and `test bl,bl` at the bottom; January the reverse), plus an extra `jmp` and pad. `[sha]`. |
| `qB` | `short button_index;` hoisted to the `if (gamepad)` block scope | inert, still 96 `[size]` |

`qA` produced a genuinely new and *negative* result that closes the condition-order lever: **VC7
rotates the second `&&` operand of a two-operand loop condition to the loop head and tests the first at
the bottom backedge**, so January's top-of-loop `test bl,bl` with bottom `cmp si,4` proves the source
order is `gamepad_index<MAXIMUM_GAMEPADS && !button_pressed` — the order already in the real file.
Two further candidate levers were refuted from the bytes without spending a shape: an inner
`button_index<N && !button_pressed` condition (January's analog loop contains no `test bl,bl`), and any
CSE-killing respelling of the three compares (all three are literally `==1`).

**Honest risk statement.** `q3`'s two arms are textually identical one-line assignments, `||` is the
more idiomatic spelling, and this wave's disassembly shows the two forms produce the same CFG — the
split changes nothing except which value wins EBX. That is close to the house-rule prohibition on
identical fake branches and on shapes chosen only to steer allocation. Against that: the device is
laws_w3 A26 (in the w3 newly-admitted list), the source stays readable and semantically identical, and
January's frameless BL-carried allocation is not reachable by any non-duplicating spelling measured
across ten shapes now (laws_w2 section D x5, n3a x3, n5 x2). This row is the weakest of the three
reopens and should be decided on its own merits; the other two do not depend on it.

## `_code_001b5850` (16) — exact, and the address-derived name is still the correct one

Already EXACT at baseline, so it is not newly exact. It is a one-byte `ret` plus fifteen pad bytes with
no caller in the TU. `bink_playback_obj_jonas_anonymous_ret_leaf_20260830` records an exhaustive
identity search (1,565 worktrees, 1,834 `bink_playback.obj` artifacts, the preserved public/Claude/Fable
donor trees, Stian, Pastudan, HCEA, xboxrecomp, Baboon, demon, nimbus, halocea, OpenSauce, punpckhdq,
and the January PDB, which has no public symbol at the helper RVA) that produced no name. Re-checked
this wave: nothing in `scratch/atlas/bink_playback.obj.txt` records the offset either. No
EXACT-PENDING-RENAME is filed, because there is no authentic name to file.

## Combined candidates the orchestrator can install

| candidate | bodies | gate | park drift | fake scan | owner census |
| --- | --- | ---: | ---: | ---: | --- |
| `scratch/workers/n5_bink_playback/n5_clean.c` | draw_frame + query | 23 / 1 | 2, each row at exactly its January measurement | 0 leads (= HEAD) | clean |
| `scratch/workers/n5_bink_playback/n5_all.c` | all three | **24 / 0 (whole object)** | 3, each row at exactly its January measurement | 1 lead, the disclosed `__asm { int 3 }` | loses only the `"end bink_alloc"` literal, which January's object also lacks |

Both were produced with `tools/campaign/merge_candidates.py` from the real file plus the individual
bodies, so each is literally "the current real file plus only the reopened bodies". `n5_clean.c` needs
no ruling for `_bink_draw_frame` and only the verifier judgement on the `q3` device; `n5_all.c`
additionally needs the `__asm { int 3 }` policy ruling.

## Checks

- Final real-file gate: `== exact 21 residual 3 unwritten 0`, emitted-symbol guard passed, identical to
  the baseline listing row for row. `scratch/parkcheck.py` on the real object: parks 3, drift 0.
  `tools/fake_match_scan.py source/bink/bink_playback.c`: 0 review leads.
  `git diff --stat -- source/bink/bink_playback.c`: empty.
- Every candidate: point-from-line3d guard passed; park drift rows land exactly on their January
  `(size, relocs, sha)`; owner census vs `build/base` and `build/split` clean except the
  `"end bink_alloc"` literal noted above; CRLF preserved (0 bare LF in every candidate).
- No header, `config/*.json`, build, tooling, test or other-unit file was read for edit or modified.
  No ninja, configure.py, pytest or git write command was run.

## Reusable facts measured this wave

1. **A corpus-wide trap census settles "is this instruction source text?" without any source sweep.**
   Two `int 3` instructions exist across 833 January split objects and one of them is in a Microsoft
   vendor object; with 230 `_system_exit` and 253 `_display_assert` relocation sites corpus-wide, and
   four `display_assert; system_exit; add esp,0x14` sequences inside the one function under study of
   which exactly one traps, every callee-attribute, macro-wide and compiler-flag explanation falls at
   once. Scanners: `scratch/workers/n5_bink_playback/int3scan.py`, `sysexit_census.py`.
2. **LAW Z verified end to end on a real row, with a reusable tool.**
   `scratch/workers/n5_bink_playback/webprof2.py` runs the hs_pin web profile against an arbitrary
   candidate object rather than `build/base`, which turns "did the source fix actually restore
   January's pin?" into a one-line check. Here the candidate's ebx/esi/edi profile became identical to
   January's once two source inline-boundary defects were fixed.
3. **`&&` operand order in a `for` condition is byte-visible through loop rotation.** VC7 rotates the
   second operand of a two-operand loop condition to the loop head and tests the first at the bottom
   backedge, so the (top test, bottom test) pair in the target reads the source operand order directly.
   A candidate that matches the target's register allocation but inverts those two tests has the order
   wrong — a cheap way to turn an "allocation tie" probe into a refutation.
4. **A `>N` versus `>=N+1` threshold is never a tie.** Both the comparison literal and the signed
   condition code are in the encoding (`cmp ecx,0x1d ; jl` against `cmp ecx,0x1c ; jle`), so the target
   dictates which of the two equivalent spellings the source used.
5. **Three consecutive waves received the same stale baseline row list for this object.** n3a, n4 and
   n5 each had to correct `exact 20 / residual 4` with `_bink_free_texture_cache` non-exact to the
   measured `exact 21 / residual 3`. The wave-prompt generator is reading a pre-`3083f800b` snapshot
   for `source/bink/bink_playback`.
