# cinematics wave-2 worker ledger (slug scratch/w/cinematics)

Base tree 931ed8dc. Unit source/cutscene/cinematics. Target: _cinematic_render (1280, park register-allocation).

## Baseline (gate --all, production source)
- exact 16 / residual 1 (_cinematic_render [sha]) / unwritten 0.
- alndiff: 389/389 insns; only real diff = 4 insns at +0x45d..+0x468 (shadow-color pack):
  - Jan: `mov ecx,[esi+40]; shl eax,18h; and ecx,0FFFFFFh; or eax,ecx; push eax`
  - ours: `mov ecx,[esi+40]; and ecx,0FFFFFFh; shl eax,18h; or ecx,eax; push ecx`
  (all other alndiff rows are __real@ symbol-vs-defined-noncode naming, not byte diffs)

## Prior negatives (do not repeat) - from docs ledgers 20260831 + opus5 w2 20260914
- declaration/scope/barrier/volatile/punning families; HCEA-layout matrices.
- c1 drop (pixel32) cast; c2 swap | operands; c3 RGBA_SETALPHA parenthesisation; c4 PIN inline into shift, no shadow_alpha local; c5 named pixel32 shadow_color local. All inert (same 4-insn residual).

## /Od readout (halo_cache_symbols.exe fn 0x559f00, found via cinematic_globals 0xc4bf64 refs)
- RTC: bounds [ebp-0x20] size 8, text_color [ebp-0x5c] size 16 (HCEX agrees: bounds, text_color).
- Shadow alpha: NO named local. Three PIN evaluations inline in the argument (?: temps -0x98/-0xa8).
  Each evaluation: `mov eax,[title+40]; shr eax,18h; cvtsi2sd + unsigned-correction table; cvtsd2ss; mulss fade; call fast_ftol`
  => source converts an UNSIGNED `title->shadow_color >> 24` (not a (byte)/(long) cast chain) to real.
- Pack: `mov eax,[pin]; shl eax,18h; mov ecx,[title]; mov edx,[ecx+40]; and edx,0FFFFFFh; or eax,edx; push eax`
  => source order (alpha << 24) | (shadow_color & 0x00FFFFFF) (RGBA_SETALPHA shape).

## Probe p1 (/Od-attested spelling) - scratch/w/cinematics/p1.c
- Hypothesis: /Od has no shadow_alpha local, PIN inline in the argument, and converts plain unsigned `shadow_color >> 24`.
- Result: residual unchanged (same 4-insn pack). BUT conversion bytes identical: `(real)(title->shadow_color >> 24)` also
  emits movzx byte [esi+43h] + fild dword, so the production `(real)(long)(byte)` cast chain is a STRIP-TEST-inert decoration.

## Lab findings (scratch/w/cinematics/lab1..lab6.c, labdis.py)
- VC7 puts AND(load field, mask) first for every no-conversion spelling: operand swap, (pixel32) casts, named long/pixel32
  locals, PIN inline, `+` instead of `|`, `*0x1000000`, `(a&0xFF)<<24`, `(a<<24)&0xFF000000`, local copy of the colour,
  pre-masked colour local. 0..9 extra locals before the pack (lab4): never flips.
- SHL goes first ONLY when the shift operand carries a real reg->reg conversion (short/word/byte local or cast: movsx/movzx),
  in either source order (lab_s, lab_srev, lab_sc, lab_bc, lab_w). January has NO movsx/movzx before its shl.
- Board census (pack_census2.py, 33 January sites): every SHL-first site has either a conversion (lens_flares movzx of a
  call result), a heavier shift operand (actors/ai_script `(x&0xff|0x8000)<<16`), or a shared value (winsock/byte_swap
  `x<<16 | x&0xff00` where the AND kills the shared register). cinematic_render is the ONLY plain-register SHL-first site.
- Donor _modulate_pixel32_by_real_alpha (ui_widget, EXACT): `(fast_ftol(a) << 24) | (argb & 0x00FFFFFF)` -> AND first in January too.

## Oracles (not landable) - inert
- TU name-count oracle: 1..32 extern declarations before cinematic_render -> identical residual (2 blocks) every n.
- Unused in-function locals 1..16 in the title block and in the colour block -> identical residual.
=> the pack order is NOT a declaration-count/ordinal tie; it is an expression/DAG-shape property.

## Cross-build evidence (DATA only)
- oct-betaP.xbe: cinematic_render byte-identical to January at file 0x83140 (0 non-reloc byte diffs) -> source stable Oct01->Jan02.
- oct-default.xbe (register-arg/LTCG config) @0x5d00f: same 3-eval PIN, then `mov ecx,[esi+40]; and ecx,0FFFFFFh; shl eax,18h; or ecx,eax; call`
  (argument passed in ECX) -> AND-first there; not discriminating (ECX argument preference).

## KEY FINDING: pack order is CONTEXT-driven (lab7.c, mk_lab7.py; reductions of the p1 body, lab only)
- remove post-pack statements (draw string .. retire)  -> January order `shl eax; and ecx; or eax,ecx`
- remove only the fabs/MIN colour clamp block          -> January order
- remove letterbox block / fade block / letterbox+fade+clamp(+after) -> still AND-first
=> the OR operand order is decided by function-wide state (web/IL numbering or allocation), so a genuine
   source-structure difference elsewhere in January's body (bytes-identical) can explain it.
- finer bisection (lab8.c): removing ONLY the rasterizer_draw_unicode_string(...) call flips to January order; removing
  set_shadow_color(0), time update, retire, any single MIN, or 2 of 3 fabs terms does not.

## Probe p2 (NULL -> 0 in draw call): inert (same 2 blocks).
## Probe p3 (/Od-attested: no title_time local, `(real)active_title->time` at each use): all other bytes identical, pack unchanged.
- lab9.c: draw-call argument bisection. January order appears when title_bounds is NOT used by the draw call
  (dc2: get_string alone; dc3: NULL bounds); stays AND-first when title_bounds is used (dc1, dc4-6), whatever the nested
  call's args. => the trigger is title_bounds (ebx web) being live across the pack.
- HCEX -sym cinematic_render: `bounds` (union rectangle2d) x2 at same offset + text_color; no scalars (as recorded).

## Hypothesis H-nest (from /Od control flow)
/Od: every early check (title_index==NONE, font_index==NONE, help tag==NONE, text_index range) jumps to 0x55a6e3,
the END-OF-BODY `jmp 0x55a1ac` (increment), not to 0x55a1ac itself. If MSVC /Od lowers `continue` as a direct jump to
the increment label, this attests NESTED IFs (single-exit style) rather than `continue`. Verify on a known-continue function.
- VERIFIED (lab_od.c, VC7 /Od): `if (x) continue;` lowers to `jne over; jmp <increment>`; a nested `if (!x) {...}` lowers to
  a single `je <end-of-body jmp>`. The later /Od build has single `je 0x55a6e3` for all four early checks
  => the checks are NESTED IFs (single-exit style), not continue. /Od slot order (decl order):
  active_title -30, font_index -34, title -38, help_text_tag_index -3c, string_list -40, title_bounds -44, fade_amount -48,
  text_color -5c; no title_time, no shadow_alpha locals.

## Probe p4 (nested ifs + /Od decl order + text_color + no title_time) - scratch/w/cinematics/p4_nested.c
- Result: every other byte still identical (nested form is byte-compatible), pack unchanged (2 blocks).

## Working model (from vc7-allocator-decoded + lab9): O = S|A register choice decides both the `or` destination and
## which operand is emitted first; colouring order is priority = sum(refs x block weight), block weight = #webs in block.
## title_bounds live in the pack block adds a web -> shifts A/O vs S(phi) priority order. January must differ in the
## join block's web census or the phi web's references.

## Allocator oracle (lane-A res9 alloc-oracle copied to scratch/w/cinematics/ao, WT retargeted; compiler run under dbg32c)
- oracle.py p1: 20 global-web decisions, model checks R0-R3 all pass (20/20).
- forcesearch.py p1: FLOOR d=4 dblind=2. EVERY single forced chooser decision (all webs x all allowed regs) leaves
  dblind >= 2 (the and/shl order); none EXACT. => the pack order is NOT a global register-allocation tie; it is
  decided pre-allocation / in the block-local (dag.c) emitter for block-local temps. Context sensitivity (title_bounds
  live at the pack) acts through that emitter, not through global colouring.
- lab10.c (lab only): number of `color.x = MIN(color.x, 0.8f)` statements in the clamp block (before the pack):
  0 -> SHL-first(Jan), 1..3 -> AND-first(ours), 4..8 -> SHL-first. => pack order is a function-wide COUNT/numbering
  effect upstream of the pack (?: temps / blocks), not an expression property. January must differ from our source in
  such a count somewhere (bytes elsewhere identical).
- M8 oracle (mod8.py method: D=0..8 `long lab_mod8_K = 0;` at body top): all identical residual -> MOD8-INSENSITIVE.
- lab11.c: MIN count 9..16 all SHL-first (not periodic); `if (x > 0.8f) x = 0.8f;` form behaves identically to MIN
  (0 -> S, 1..3 -> A, >=4 -> S). => a non-periodic context threshold (conditional-store/block count), not an 8-bucket key.

## Probe p5 (/Od letterbox: no letterbox_amount local, MIN/MAX assigned straight into the global): other bytes identical, pack unchanged.
## Probe p6 = p4 + p5 (all /Od-attested structure at once): other bytes identical, pack unchanged.
- oracle v_min4 vs p1: global colouring identical in order/registers; only the PIN-phi web (id38, identified by forcing
  it to edx: exactly the three arm defs + the shl use change) gains refs (terms 6x12 -> 8x12, pos 0x15d -> 0x164).
  => confirms the order flip is not a global colouring change.
- ctx/ matrix (mkvar.py + ctx_edits.json), each on p1 unless noted, all measured: c01 `alpha = alpha*fade`, c02 `fade*alpha`,
  c03 `time = time + (?:)`, c04 `(short)(style-1)`, c06 fade init before title_bounds, c07 realcmp() macro, c09 if-form MIN,
  c10 if/else-if PIN on fade, c11 one-line get_string, c12 braced continue: all 2-block residual (pack unchanged, rest identical);
  c05 swapped bounds compares / c08 swapped retire compare: extra diffs (rejected). s1 text_color at loop-body top, s2 no inner
  block (on p6): pack unchanged.
- t01 non-const title_bounds, t02 const title, t03 const string_list; p_long/p_int/p_ulong shadow-colour parameter type
  (lab prototype): all pack-inert.
- F1 oracle (1..16 unused __inline bodies before the function): inert. H3/name-count (1..32 externs): inert (above).

## Probe p7 (best candidate) - scratch/w/cinematics/best_candidate_cinematics.c (= p7_scoped.c)
- p6 + nested-scope declarations with combined initialisation (house rule 17); declaration nesting order equals the /Od
  slot order exactly (active_title, font_index, title, help_text_tag_index, string_list, title_bounds, fade_amount, text_color).
- gate --all: exact 16 / residual 1. _cinematic_render normalized sha 1a9d49fc4eace03c == production (park stays valid,
  no drift). Whole object vs production: every code/data section identical; only .debug$S differs. No TU-local /W3 warnings.
- Oracle on p7: /Od dead `real seconds_per_tick = 1/30` local (the /Od [ebp-8] dead store) -> inert (and owner-gated anyway).
- optional_fidelity.patch: git apply --check OK; applying to a copy reproduces best_candidate byte-for-byte (CRLF kept).
  ZERO credit; a source-fidelity cleanup only (removes unattested title_time/letterbox_amount/shadow_alpha locals and the
  strip-test-inert (long)(byte) cast chain; continue -> /Od-proven nested ifs; color -> text_color per RTC/HCEX).

## Object-level status (production build, 931ed8dc)
- object_audit: 23/23 January symbols ok; only DIFF = _cinematic_render. Data 140/140 (report.json 100%); pdb_storage 0 disagreements.
- Surplus: _fast_ftol COMDAT IDENTICAL to actor_combat's; provider_link SELECTED-PROVIDER LINK: PASS (all 7 surplus rows PASS).
  => the 20260831 whole-object COMDAT blocker no longer reproduces; the ONLY object blocker is the render pack order.

## VERDICT: NO_PROGRESS (stop rule fired). Remaining machine difference (render +0x45d..+0x468, 4 insns, same size/relocs/frame):
  Jan `shl eax,18h ; and ecx,0FFFFFFh ; or eax,ecx ; push eax`   ours `and ecx,0FFFFFFh ; shl eax,18h ; or ecx,eax ; push ecx`
  Class: dependency-free DAG emission order of the two OR operands (pre-allocation, block-local), NOT a global register tie
  (forcesearch: no chooser decision lowers the register-blind floor) and NOT M8/H3/F1 count keys (all oracles inert).
  It IS context-sensitive (lab7-lab11): removing the draw call's title_bounds use, or having 0 / >=4 conditional colour
  stores instead of 3, yields January's order - so some byte-invisible upstream IL fact differs in January.
## Reopen criteria
  1. A decode of the dag.c operand-order key for commutative integer ops (c2dbg32 breakpoint at the `or` lowering; compare
     p1 vs lab v_min4/min0), naming the IL property that January must have; then find the authentic construct supplying it.
  2. Or a first-party source/listing for cinematic_render's title loop (January-era) showing a structure not in p1..p7.
  Do NOT repeat: pack spellings c1-c5 (opus5), p1..p7, ctx c01-c12, t01-t03, parameter-type labs, name/local/M8/F1 oracles.
