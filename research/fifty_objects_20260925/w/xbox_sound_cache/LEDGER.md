# xbox_sound_cache worker ledger (wave 2, base 931ed8dc)

Target: `_sound_cache_debug_render` (368 padded, 14 relocs, park tu-context-optimization).

## Baseline
- gate --all on unmodified source copy (base.c): 17 EXACT / 1 residual (`_sound_cache_debug_render [sha]`, 368).
- Sibling `_texture_cache_debug_render` (784) is EXACT in production; its graph section is the natural donor
  (locals scoped inside predicate; pointer walk counting down from 2; separate x/y statements).
- Prior negatives (w2 20260914, on the point_from_line3d helper sr7): sd1 texture-style restructure left one
  x87 pair (January `fld world_vector.k; fmul forward.k`, ours reversed); sd2 dot_product3d arg swap,
  sd3/4/5 world_vector declaration placement -> identical to sd1. DO NOT repeat.

## Probe sd1 (re-measure of w2 sd1 shape on the current exact add_vectors3d helper)
- Hypothesis: the sibling exact `_texture_cache_debug_render` graph block (locals scoped in the predicate,
  world/screen pointer walk with countdown point_index, per-field screen stores) is January's idiom; the
  January frame (colors at -0x2c, screen_positions at -0x3c, world_positions at -0x54, esi=world+8, edi=screen,
  ebx=2 countdown, page_index homed -0xc, state_index homed -8) is exactly the texture shape.
- Result: 119/119 insns, ONE real diff block: first dot term `T fld [ebp-0x10](wv.k); fmul render+0x24(fwd.k)`
  vs ours `fld fwd.k; fmul wv.k` (plus the cosmetic __real symbol naming row). Same as w2 on sr7 helper.
- Texture January (exact, same shape) loads `fld fwd.k; fmul wv.k` -> the difference is a TU/function
  context key, candidate M8 (IL-local ordinal mod 8).

## Probe lab_sd1_D0..D8 (M8 oracle, LAB ONLY - dead `long lab_mod8_K = 0;` at function top; never landable)
- D=0..4 residual (k pair), D=5,6 STRICT EXACT (18/18), D=7,8 residual with a j-term flip instead.
- Verdict: the residual is purely an IL-local/inline-temp count defect (LAW M8). January's function has a
  different local/inline-temp inventory than sd1. Need the genuine inventory -> /Od.

## /Od readout (halo_cache_symbols.exe, PC build: cache\pc_sound_cache.c, fn 0x51caa0; od_51caa0.txt)
- RTC descriptor names: colors[16], extent[24] (=world positions, real_point3d[2]), line_pt[16]
  (=screen positions, real_point2d[2]), vec[12] (=world vector). (plus buffer/tab_stops/iterator of the
  PC-only stats branch).
- Graph block statement shape (PC /Od):
    rows(short,[ebp-8]) = page_count/640; colors[0..3] = red,green,blue,yellow; page_usage = malloc(PC only);
    lruv_cache_get_page_usage(cache, page_usage);
    for (x=0; x<640; x++) {                              x long [ebp-0x10]
      page_index(short,[ebp-0x2c]) = rows * x;
      for (bit=0; bit<4; bit++) {                         bit long [ebp-0x30]
        if (page_usage[page_index] & (1<<bit)) {
          line_pt[0].x=(real)(x%640); line_pt[0].y=(real)((bit+(x/640)*4)*10);
          line_pt[1].x=(real)(x%640); line_pt[1].y=(real)((bit+(x/640)*4+1)*10);
          for (i=0; i<2; i++) {                           i long [ebp-0x6c]
            render_inverse_transform_screen_point(&line_pt[i], &extent[i], &vec);
            point_from_line3d(&extent[i], &vec,
              (render.camera.z_near + 0.001f) / dot_product3d(&vec, &render.camera.forward), &extent[i]);
          }
          render_debug_line(TRUE, &extent[0], &extent[1], colors[bit]);
    }}}
- KEY FACTS: (1) the scale update is a REAL point_from_line3d call (0x42e0d0, 4 args, result==point);
  current source hand-expands it (rule 6 violation). (2) dot_product3d argument order is (&vec, &forward).
  (3) no `distance`/`scale`/pointer-walk locals: distance expression is inline in the call argument.
  (4) /Od slot order (decl order): rows, page_usage, x, colors, page_index, bit, extent, line_pt, i, vec.

## Probe od1 (/Od-attested shape: rows/page_index shorts, extent/line_pt/vec names, x/bit/i longs,
##   point_from_line3d call with inline distance/dot(&vec,&forward) argument)
- Hypothesis: the /Od statement/local inventory is January's; the hand-expanded scale update + distance/scale
  locals + pointer walk are the inventory defect M8 detects.
- Result: 119/119 insns; same single k-term pair as sd1 (fld fwd.k first). Frame/slots/regs all identical to
  January otherwise. No _point_from_line3d symbol emitted (see below). The /Od shape is at least as close
  as sd1 while being rule-6 compliant (real point_from_line3d call instead of hand expansion).

## Probe lab_od1_D1..D20 (M8 oracle on od1, LAB ONLY)
- D=4 STRICT EXACT (18/18); D=0..3 k-term pair; D=5..9 a 3-block j-term variant; D>=10 4 blocks. NOT period 8
  here. sd1 (12 user locals + no pfl3d temps) closed at D=5,6; od1 (10 user locals + 4 pfl3d temps) at D=4:
  both reach the same total IL-symbol count (~20), consistent with an inventory (count) key.
- Conclusion: January's function carries ~4 more IL-visible locals/inline temps than od1.

## Probe lab_od1_P1..P4 (LAB ONLY): 4 dead longs placed in the innermost block (after vec / before extent),
##   in the x-loop block, in the predicate block -> ALL STRICT EXACT.
- The key is position-insensitive: only the COUNT of IL locals preceding the inline temps matters (inline
  temps are numbered after all user locals). January = od1 + 4 IL-visible locals (in some genuine form).

## /Od readout of the SIBLING (PC texture cache debug render, fn 0x51f1d0; od_51f1d0_texture.txt)
- Same idiom as sound: width/x_offset/y_offset shorts, x/state/i longs, colors, extent, line_pt, vec,
  point_from_line3d(&extent[i], &vec, (z_near+0.001f)/dot_product3d(&vec,&forward), &extent[i]).
  List branch: count(short), iterator, texture ptr, font_index, index, tab_stops, textstring, bounds (+ternary temp).
  /Od user-local inventory = 11 (graph) + 8 (list) = 19. Our EXACT January texture reconstruction instead uses
  25 locals (distance/scale/pointer walk + extra list locals): its exactness does NOT authenticate that inventory.
- Planned calibration: rebuild the texture graph+list in the /Od inventory and see whether January texture
  stays exact (does the /Od inventory transfer to January for this idiom?).

## Calibration results (LAB)
- Texture TU with the graph block in /Od inventory (tex/t_od_graph.c): _texture_cache_debug_render STRICT EXACT
  (25/25). So the /Od graph idiom (point_from_line3d call, dot(&vec,&forward), line_pt/extent/vec, shorts) is
  January-compatible for the sibling.
- Texture oracle D=1..12 on that file: ALWAYS exact -> texture dot order is count-INSENSITIVE (no info).
- Sound oracle is a THRESHOLD, not a mod-8 cycle: as D grows, the dot terms flip one by one from
  "render.forward operand first" to "vec operand first": k flips at D=4, j at D=5, i at D=10 (od1).
  January = k vec-first, j/i render-first  => exactly od1+4 IL symbols (unique D in 0..20).

## Probe lab tu_* (LAB ONLY): 1/2/4/8 dummy externs or prototypes before the function or at the TU top ->
##   all identical residual. The key is IN-FUNCTION only (not TU declaration count / H3).

## Probe lab_od1_P5/P6/P7 (LAB ONLY)
- P5: 4 locals declared uninitialised at top, first ASSIGNED after the point loop -> EXACT.
- P6: 4 locals declared but NEVER referenced -> residual (inert).
- P7: 4 initialised locals in a new block after the point loop -> EXACT.
- => the key counts REFERENCED locals anywhere in the function (unreferenced declarations are inert).
  January has 4 more referenced locals/IL symbols than od1.

## Calibration 2 (LAB): texture TU with BOTH graph and list branch in the /Od inventory (no `bitmap`,
##   no `touched_string` locals; tex/t_od_all.c) -> _texture_cache_debug_render STRICT EXACT (25/25).
- The complete /Od texture inventory transfers to January (sibling side finding; texture is count-insensitive,
  so this is compatibility evidence, not proof).

## Probes v1 / v2 (staging locals; NOT /Od-attested)
- v1 = od1 + named `distance` (hoisted) + `scale` statement: residual, same k pair.
- v2 = v1 + world_position/screen_position pointer walk (the texture-reconstruction inventory, +4 referenced
  locals): residual, same k pair. => plain referenced-local count is not the whole key: moving the dot out of
  the point_from_line3d argument into its own `scale =` statement changes the inline-temp numbering (the dot is
  inlined before point_from_line3d's temps). Model: key = # IL symbols numbered before dot_product3d's param
  temps; od1 = 10 user + ~3 point_from_line3d temps; January ~ +4.

## Probe od1s + oracle (LAB): od1 with dot_product3d(&forward, &vec) -> identical behaviour (D=4 closes only).
- Dot argument order is INERT (confirms w2 sd2); /Od order (&vec, &forward) kept.

## Probe lab_od1_D21..D48 (LAB): monotone, NO periodicity up to 48 (k correct, j and i wrong for every D>=10).
- The key is a pure threshold on the in-function symbol count: January = exactly od1+4 under the /Od
  structure (window width 1). No "remove 8-D locals" alternative exists.
- HCEX.pdb SHIP pc_sound_cache compiland has NO sound_cache_debug_render; HCEX_Release lists only the PC
  stats-branch locals (buffer/tab_stops/iterator): the graph was compiled out in 2011. No PDB local evidence.

## Probe od1_helper_last (definition order: helper defined after debug_render with a static prototype;
##   January emission order helper->debug_render is consistent with both orders by the deferral law)
- Result: identical k-term residual. Definition order is inert for the key.

## Probe od2 = od1 with TEST_FLAG(page_usage[page_index], bit) (rule 18 macro): identical residual (k pair).
## Mechanism probes lab_m1/lab_m2 (LAB ONLY): line_pt stores through set_real_point2d (adds 3-6 inline param
##   temps before the dot) -> NO shift (still k pair). Inline temps are NOT counted by the key; only
##   referenced USER locals are. Under the /Od structure January needs 14 referenced user locals (od1 has 10).

## Oracle characterisation (LAB ONLY, od2 base): what counts toward the key
- 4 scalar locals of any type (real / short / pointer / long) -> EXACT: type-insensitive.
- non-address-taken local aggregates count PER ACCESSED FIELD, 2 units each: 1 vector with 1 field written
  (+2) residual k; 1 vector with 2 fields written (+4) EXACT; 2 vectors x 1 field (+4) EXACT; 3 vectors
  x 1 field (+6) and 4 x 1 (+8) -> j-flip. (consistent with SRA field symbols)
- inline-helper param temps: 0 units (lab_m1/m2).
- So January's function has +4 "units" of IL user symbols over od2: e.g. 4 extra referenced scalar locals,
  or 2 extra accessed fields of non-address-taken local aggregates, or 2 scalars + 1 field.

## Candidate od3 (best credible, house-rule polished): od2 + TU enum XBOX_SOUND_CACHE_PAGE_COUNT = 1024 used for
##   page_usage/rows and sound_cache_new's lruv_new page count.
- gate --all: 17 EXACT + debug_render residual; debug_render normalized sha 6b7a4944... (== od2), 119/119 insns,
  14/14 relocs, ONE differing row (render+36 reloc at 0xe2 vs 0xe5): k-term operand order only.
- Surplus: newly emits `_point_from_line3d` (48 B SELECT_ANY) -> section_infos_equal to January's selected
  copy (action_charge) = True; `_dot_product3d` (already emitted by production) and `_add_vectors3d` also equal.
  provider_link.py od3/od2: SELECTED-PROVIDER LINK PASS (both orders) for every surplus symbol.
- Admission blocker for the COMDAT: ruling 2 requires a STRICT-EXACT caller; debug_render is not exact ->
  od3 is owner-gated (cannot land as a park-body refresh without a ruling), even though it removes the
  production body's rule-6 hand expansion of point_from_line3d.

## Probe lab_counters (LAB, speculative): PC-stats-style dead page-state counters incremented via switch in the
##   graph test -> 384 B / 129 insns (switch dispatch survives DCE): REFUTED as January's source (bytes differ).

## Object-level audit (beyond the function)
- build/report.json: .bss 272 100%, .rdata 952 100% (data already complete); .text 98.04% (17/18).
- Storage: cachebeta publics say 6 symbols are FILE-STATIC (pdb_storage.txt): _render_inverse_transform_screen_point,
  _sound_cache_debug_block_name, _sound_cache_delete_block_proc, _sound_cache_locked_block_proc,
  _sound_cache_start_loading_sound, _xbox_sound_cache_globals. The split marks all 6 external (symbols.json lacks
  "static": true); production source has only 2 static. xref_check: 0 January and 0 of our objects reference
  any of them as UNDEF. HCEX corroborates `static` for locked/delete_block_proc.
- Probe s1 (storage-only: static on the 3 callbacks' prototypes/definitions and on xbox_sound_cache_globals):
  17 EXACT, debug_render sha unchanged (8ae719b8...), .bss unchanged -> BYTE-INERT zero-credit correction.
  Probe s2 (s1 + HCEX `boolean` return for locked_block_proc, cast removed): breaks _sound_cache_locked_block_proc
  -> REJECTED (January returns through a 32-bit long; keep the cast).
- scratch/w/xbox_sound_cache/config/symbols.json: in-place `, "static": true` on lines 7222-7225, 7230, 23289
  (only those 6 lines differ). Integrator must csplit-regen; object_audit then loses the 6 storage DIFFs.
- final.c = od3 + s1: 17 EXACT, debug_render sha 6b7a4944..., provider_link PASS; OBJECT AUDIT would then fail
  only on the debug_render .text row (after the symbols.json regen).

## Side finding (other unit, zero credit): xbox_texture_cache
- tex/texture_od_inventory.patch (git apply --check OK) rewrites texture_cache_debug_render to the /Od-attested
  inventory: point_from_line3d call instead of the hand expansion (rule 6), line_pt/extent/vec, width/x_offset/
  y_offset shorts, no bitmap/touched_string locals. Gate 25/25 EXACT; the new `_point_from_line3d` COMDAT is
  section_infos_equal to January's selected copy (action_charge) and provider_link PASS -> meets ruling 2
  (strict-exact caller). Needs the integrator's full sweep before landing.

## STOP (methodology stop rule)
- Evidence shapes tried: sd1, od1, v1, v2, od1s, od1_helper_last, od2, od3 (+ oracle labs). Remaining machine
  difference in the best candidate (od3/final): ONE x87 commutative operand order in the dot k-term
  (January `fld [ebp-0x10] (vec.k); fmul render+0x24`, ours `fld render+0x24; fmul [ebp-0x10]`), i.e. one
  relocation row (render+36) at 0xe2 vs 0xe5. Everything else (frame 0x454, slots, registers, 14 relocs) equal.
- Decoded cause: a THRESHOLD key on the count of referenced IL user symbols (scalars 1, SRA'd accessed aggregate
  fields 2, inline temps 0, unreferenced declarations 0, TU declarations 0, position/type insensitive). January
  = the /Od-attested inventory + exactly 4 units. No genuine source for those 4 units is attested by /Od (PC 2020
  revision), HCEX (graph compiled out), or January bytes; filler locals are the oracle, never a landing.
- Reopen criteria: (1) authoritative 2002 Xbox source/local record for this function naming the extra referenced
  locals; or (2) an owner ruling admitting a specific attested local inventory. Separately, landing od3 as the
  parked body needs an owner ruling because it newly emits `_point_from_line3d` from a non-exact caller
  (ruling 2 requires a strict-exact caller), although od3 removes production's rule-6 hand expansion.
