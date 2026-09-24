# review4_hud_draw - adversarial review of hud_stack_check P1 (hud_draw.h) + P2 (hud_draw.c) - LEDGER
Tree 16542e46, git status clean at start. Reviewer slug: scratch/w/review4_hud_draw/.
## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log held table, worker LEDGER (hud_stack_check) read.
- Also read: review3_hud_unit REVIEW (blocker 2 = hand-copied goto sentinel), review4_hud_unit REVIEW (parallel reviewer:
  P1/P2 OK, drop P6, emission-order advisory), hud_draw_argb_inline_owner_20260923, rejections (hud_draw _fast_ftol veto).
## V1 patch applicability (git apply --check, tree 16542e46)
- P1, P2, P3, P4, P5, P6, P7, 01RS each CLEAN alone; P1+P2 CLEAN; P1 P2 01RS P4 P5 P7 CLEAN as a set.
- patch replay onto `git show HEAD:` copies (tree/) reproduces the worker's cand hud_draw.h / hud_draw.c (EOL-normalised) exactly.
## V2 consumer list (own grep -rlE over source+tools, .c/.h/.cpp/.inl, spaces-safe): 23 TUs = worker's list. No header includes
  hud_draw.h, no .c-includes-.c chains. Dirs with spaces (saved films/, saved games/) contain no consumer.
## V3 own harness (sweep.py: gate.py per TU, include line rewritten to a slug header; rawcoff.py: own COFF comparer incl.
   COMDAT selection+checksum, relocations by target name, symbol table, section order; .debug$S skipped)
- prod sweep == worker B0 rows (hud_draw 22/0/1 ... render_sprite 7/3).
- CONTROL (verbatim HEAD header via rewritten include): 23/23 objects 0 differences incl. section order. Harness inert.
- CAND (P1 header; TUs P2, 01RS, P4, P5, P6, P7): gate rows identical to prod except hud_unit 20+2 unwritten (01RS renames,
  needs symbols.json 02 - expected). rawcoff vs prod: 18 non-sentinel consumers 0 differences (models with P7).
  hud_draw: ONLY _check_stack_buffer selection NODUP(1)->ANY(2), checksum 0x4957c551 unchanged, + section ORDER
  (_check_stack_buffer now emitted before _get_return_eip). nav/weapon/motion: only the new ANY _check_stack_buffer + its symbol.
  hud_unit: new ANY COMDAT + 01RS renames (code_000c6590/65a0 -> unit_hud_outline/shield_meter_mapper_tick) + .debug$F relocs.
- hud_draw cand gate: 22 EXACT / 1 UNWRITTEN (_fast_ftol_C), rows identical to production.
## V4 order evidence (secorder.py)
- January split hud_draw: get_return_eip, check_stack_buffer, get_sprite_clip_rect, hud_globals_get_scale, ...
- prod: fast_ftol(ANY), square_root(ANY), real_argb(ANY), real_rgb(ANY), get_return_eip, check_stack_buffer, hud_globals_get_scale ...
- cand: ..., real_rgb(ANY), check_stack_buffer(ANY), get_return_eip, ...
- Sept-2001 map (atlas 7eacac85): get_return_eip 'i', fast_ftol_C 'i', real_argb 'i', real_rgb 'i', real_alpha_intensity 'i'
  all hud_draw.obj; the 'i' bodies sit INTERLEAVED with plain hud_draw functions in the image (argb precedent: landed header
  inline whose January image position is mid-object while ours emits it first). => image order of ANY COMDATs is not
  reproducible from our emission order today; the swapped pair is NOT decisive evidence against P1/P2 and is unscored. Advisory.
## V5 hud_draw admission tooling on cand obj (out_cand/hud_draw.obj)
- object_audit.py: text IDENTICAL to production's (pre-existing FAIL(2) = unwritten _fast_ftol_C; surplus _fast_ftol/_square_root
  + literals unchanged). Data (tinfo --data): identical except .debug$S (gate PID path). /W3 /Zs census (own warn.py):
  identical multisets for hud_draw, hud_unit, nav, weapon, motion, models. fake_match_scan: same 2 pre-existing leads
  (get_return_eip naked/asm), none new.
- _check_stack_buffer in all 5 cand objects: section_infos_equal to January hud_draw (48 B, 0 relocs, sha c7049c31...).
- own plink.py (provider_link's Link.Exe cmd, temp in slug): cand hud_draw + each of cand hud_unit/nav/weapon/motion PASS both
  orders; all five PASS; CONTROL prod hud_draw (NODUP) + each cand FAILS LNK2005 both orders (probe discriminates).
## V6 dependency probes (mode hdronly: P1 header with HEAD TUs)
- hud_draw HEAD + P1: COMPILE FAILS (C2365/C2086 enum redefinition, C2084 check_stack_buffer already has a body) -> P1 needs P2.
- hud_unit HEAD + P1: COMPILE FAILS (C2365/C2086 STACK_BUFFER_LENGTH/FILL redefinition, hud_unit.c:118-119)
  -> P1 ALSO needs P3/01RS in the same commit. The hud_draw result's "P1 and P2 must land together" is INCOMPLETE.
- models HEAD + P1: __rasterizer_model_transparent_geometry_submit EXACT -> residual (12/2 -> 11/3) -> P1 needs P7.
- hud_weapon HEAD + P1: _render_weapon_hud (unparked residual) bytes drift -> P5 keeps it stable.
- nav HEAD + P1, motion HEAD + P1: 0 differences (P4/P6 optional for bytes; P6 fails law (i): all its callers parked residual).
## V7 other evidence
- HCEX.pdb raw bytes: no check_stack_buffer/get_return_eip/stack_buffer strings. /Od build (odbuild str): no "corrupt stack" /
  "corrupt return" strings -> no /Od or HCEX attestation of the macro/helper; no declaration-order evidence for the macro block.
- models uses NO hud_draw.h name (25 names grepped; only the struct tag bitmap_data, declared by bitmap headers). Include came
  from project commit 635bd83d.
- tools/regression_gate.py fingerprints comdat_selection per section; an accepted function whose selection changes fails
  "accepted function evidence changed" unless adjudicated, and the only adjudication recipe is xdk-stock-d3dinline. The lane's
  integrator chain does not run it (Layer 1 provider repairs made the same NODUP->ANY change on exact January-owned functions).
## V8 minimal atomic set (tree_min = HEAD + P1 P2 P3 P5 P7; no P4, no P6, no 01R) - own sweep 'min'
- gate summaries == production for all 23 (hud_unit 22/22 vs PRODUCTION split with P3 alone).
- rawcoff vs prod: hud_draw only selection 1->2; hud_unit + hud_weapon only the new ANY _check_stack_buffer (+symbol);
  the other 20 objects 0 differences (models with P7). min hud_draw == cand hud_draw (0 diffs incl. order).
- plink: min hud_draw + min hud_unit / hud_weapon PASS both orders; all five (prod nav/motion) PASS.
- /W3 warnings identical for hud_unit, hud_weapon, hud_draw, models.
- emitters' callers: hud_unit update_local_player/render EXACT; hud_weapon render_grenade_hud + hud_update_weapon EXACT
  (crosshairs_draw, hud_update_weapon_local_player, render_weapon_hud residual) -> law (i) satisfied.
- git apply --check CLEAN on 16542e46 for {P1,P2,P3,P5,P7}, {P1,P2,01RS,P5,P7}, {P1,P2,01RS,P4,P5,P7}.
  Byte-identical patch copies + SHA256SUMS in patches/ (same hashes as the worker's).
## V9 section order table (order_jan/prod/cand.txt): production ALREADY diverges from January's image order throughout
  (e.g. January get_sprite_clip_rect 3rd, production 15th; header inlines real_argb/real_rgb 9-10th in January, 3-4th ours).
  The cand swap of check_stack_buffer/get_return_eip is within that pre-existing, unscored divergence. Advisory only.
## VERDICT: approve=false AS SUBMITTED; P1/P2 content OK unchanged; amended atomic landing set verified (see REVIEW.md).
