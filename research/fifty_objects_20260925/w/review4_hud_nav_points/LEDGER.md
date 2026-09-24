# review4_hud_nav_points - adversarial review of P4 (hud_nav_points.c shared stack frame; depends on P1)

Tree 16542e46, git status clean at start. Worker slug: scratch/w/hud_stack_check/.

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log held table (hud_unit row: this hud_draw.h __inline packet is the wave-4 reopen path),
  hud_nav_points ledgers (0827..w3), parked.json _custom_render_nav_point (base sha ad77c226..., 1632/95), no hud_nav_points rejection entry,
  earlier-wave scratch/w/hud_nav_points LEDGER (push/load residual; not touched by P4).
## G0 git apply --check (read-only) on 16542e46
- P4 alone CLEAN; P1 CLEAN; P2 CLEAN; P1,P2,01RS,P4,P5,P6,P7 together CLEAN.
## V1 independent reproduction (tree/ = git show HEAD blobs + patch -p1 P1,P2,P4; gtu/ = include line -> slug header)
- tree/hud_nav_points.c, hud_draw.h, hud_draw.c == worker cand/interface/* (EOL-normalised diff: SAME x3).
- gate production (no --source): 31 EXACT / 1 residual (_custom_render_nav_point [sha]).
- gate ctrl (prod TU, include -> verbatim HEAD header copy in slug): 31/1; fullcmp (own strict comparer: raw bytes, relocs by
  target name, flags, COMDAT selection, every symbol incl UNDEF) prod vs ctrl: 0 non-debug differences -> harness inert.
- gate cand: 31 EXACT / 1 residual, same rows. fullcmp prod vs cand: ONLY SEC-ONLY-B _check_stack_buffer (.text 48, sel=2 ANY)
  + its external symbol. Residual _custom_render_nav_point RAW bytes+relocs identical to production => no park drift.
- fullcmp worker h2_out/hud_nav_points.obj vs my cand: 0 non-debug differences (reproduced).
- new COMDAT section_infos_equal to build/split hud_draw.obj _check_stack_buffer (48 B, 0 relocs, sha c7049c31...).
## V2 audit / surplus / links
- object_audit (repo tool) prod vs cand text diff: ONLY `+.text sym:_check_stack_buffer size 48`. Cand audit: 46 January symbols 0 differ,
  FAIL(1) = the parked _custom_render_nav_point residual only (pre-existing, bytes identical).
- surplus_identity (slug copy with CAND_OBJ env; one-line change): 5 candidate-only code COMDATs, 0 not identical;
  _check_stack_buffer IDENTICAL vs build/split hud_draw.
- provider_link (repo) cand vs PRODUCTION hud_draw.obj (NODUP): FAIL LNK2005 both orders (expected control) => P4 is NOT
  standalone; requires P1+P2 in the same atomic packet. provider_link (repo) on prod obj: PASS.
- my hud_draw cand (HEAD+P1+P2): gate 22/0/1unw (= prod); fullcmp vs prod: only _check_stack_buffer sel NODUP(1)->ANY(2).
- provider_link_override (slug copy; PROVIDER_OVERRIDE hud_draw=my draw_cand.obj): all 21 surplus PASS incl _check_stack_buffer.
- 5-object link (draw_cand, nav_cand, worker h2_out hud_unit/hud_weapon/motion_sensor) in 3 orders: 0 LNK2005/1169.
## V3 law (i) check: the new COMDAT and its callers
- Callers in the TU: _hud_update_nav_point_local_player (STRICT EXACT) and _custom_render_nav_point (parked, non-exact).
- reloc-masked byte diff January vs cand _custom_render_nav_point: ONLY +0x58a..+0x591 (the recorded push/load schedule);
  the inlined sentinel check (+0x5bb `mov eax,7Fh` .. end) is byte-identical to January => the non-exact caller's inline use
  is itself byte-attested in January.
- LAB labs/lab_exact_caller_only.c (macro use removed from custom_render, lab only): _hud_update_nav_point_local_player EXACT and
  the _check_stack_buffer COMDAT is still emitted, section_infos_equal to January's hud_draw copy => the emission is caused
  by a strictly exact caller on its own; law (i) is satisfied (not the P1/damage-A class, whose ONLY caller was non-exact).
## V4 warnings, park hash, consumer sweep
- /W3 /Zs (warn_slug.py, temp TU inside slug): prod vs cand warning multisets IDENTICAL (1 C4013 'error', 1 C4146, 11 C4244; pre-existing).
- parked _custom_render_nav_point cand normalized sha ad77c226... == parked.json base sha (1632/95) => no park drift.
- consumer grep (all #include spellings, recursive incl. spaced dirs): 23 TUs, all `#include "interface/hud_draw.h"`, no header
  includes hud_draw.h; list == worker's 23.
- shadow sweep (sweep.py full: HEAD blobs + P2/P3/P4/P5/P6/P7 where applicable, include -> slug P1 header; prod = gate of production):
  gate rows identical in all 23; fullcmp with $L-number normalisation (raw `$Lnnnn` numbers shift by the header's added names;
  invisible to section_infos_equal and objdiff - January split names them fn+offset): RAW bytes+relocs identical for every section
  of all 23 objects; only differences: hud_draw _check_stack_buffer NODUP->ANY; hud_nav_points/hud_unit(P3 on prod)/hud_weapon/
  motion_sensor +ANY _check_stack_buffer. models (P7) 0 differences.
## V5 authenticity / house-rule checks
- /Od build (odbuild str): "corrupt stack at" and "corrupt return address" ABSENT (sanity: `file hud_nav_points.c` finds 5 fns)
  => the later revision removed the sentinel; no /Od frame exists for the macro's block => law (d) cannot be contradicted.
- HCEX.pdb: no check_stack_buffer symbol; custom_render_nav_point present (no sentinel info).
- review3_hud_unit REVIEW (read): S1/S2 goto-free break loops do NOT reproduce January (size change); the inlined-function return merge
  needs a real helper call with the body in scope; map 'i' = inline COMDAT (our own map prints `_fast_ftol f i`). This packet = its path (b).
- STRIP TEST ST1 (lab labs/st1_top_decl.c): hud_update_nav_point_local_player with the macro replaced by the plain expansion and
  `short corrupt_index;` at function top (no block): EXACT, and the whole object fullcmp-identical to cand (0 diffs)
  => the macro's block scope is byte-INERT (not a decoration buying the match); the macro is chosen on January's one-line-per-site
  assert evidence (F2), not for bytes.
- naming: match_assert_stack_frame(file, line) follows the existing match_assert_valid_*(file, line, ...) header-macro convention
  (real_math.h, bitmaps_inlines.h); under MATCH_FILE/MATCH_LINE = __FILE__/__LINE__ the macro naturally gives both asserts one line.
- fake_match_scan (tools/fake_match_scan.py) on tree hud_nav_points.c + hud_draw.h: 0 leads (prod: 0).
- P4 diff scan: no float, no view casts, no consumer-local prototypes, no new names in the TU besides uses of header names;
  removes two hand-expanded check_stack_buffer copies with 4 invented labels/gotos (law (g) hand-expanded header helper class).
- symbols.json: no change needed (_check_stack_buffer stays attributed to hud_draw at file_offset 788720 = 0xC08F0; PUBLIC in cachebeta).
## V6 storage / atlas / scope of the verdict
- pdb_storage (slug copy with CAND_OBJ): prod and cand 46 split symbols, 0 disagreements; _check_stack_buffer and _get_return_eip PUBLIC
  in cachebeta (0xC08F0 / 0xC08E0) -> the candidate-only EXTERNAL copy is storage-consistent.
- atlas 7eacac85 (Sept-2001): `_get_return_eip i hud_draw.obj` 0xcff20, next 0xcff60 _get_sprite_clip_rect (gap 0x40 = 0x10+0x30);
  check_stack_buffer absent from the Sept publics but PUBLIC in January (Sept->Jan storage change; get_sprite_clip_rect went the other
  way, public -> static). Not contradictory to a January extern header __inline. Advisory for P1 (not P4): the 'i' flag suggests
  get_return_eip was also a header inline in 2001; P1 keeps it a plain hud_draw.c naked function (byte-inert either way).
- PRE-EXISTING hud_nav_points admission blockers NOT touched by P4 (law (g)): consumer-local prototypes of other files' functions
  whose owner headers exist (object_get_bounding_sphere -> objects.h, game_engine_render_nav_points -> game_engine.h,
  unit_get_head_position -> units.h, object_try_and_get_and_verify_type, game_engine_get_goal_position, extern hud_globals);
  TU-local struct views with opaque spans (struct hud_globals_definition {...; byte unknown1BC[0x294];},
  struct hud_nav_object_datum {byte unused[0xB6]; word damage_flags;}, local struct scenario_cutscene_flag);
  `float` for real (hud_nav_point_datum.z_offset, several `float vertical_offset` params). => P4 is zero-credit hygiene that removes ONE
  blocker class (hand-expanded helper + invented gotos); hud_nav_points is NOT admission-ready modulo the park.
## VERDICT: APPROVE P4 conditionally (atomic with P1+P2 (+P3/01RS, P5, P6, P7)); not standalone (compile needs P1; link needs P2).
