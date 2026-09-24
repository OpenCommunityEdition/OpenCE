# hud_nav_points wave-2 ledger (_custom_render_nav_point)

Base tree 931ed8dc. Baseline gate --all (production source): 31 exact, 1 residual
(_custom_render_nav_point 1632 [sha]), 0 unwritten. _hud_render_nav_points is now EXACT.

## Residual (alndiff, production obj)
Only non-naming diff: January +0x58A `push 0` (override_scale 0.0f, arg 8) BEFORE
`mov eax,[ebp-0x10]` (fast_ftol result) and `mov ecx,[ebp-0x14]` (distance); ours loads
both first, then pushes 0,0,0. Streams reconverge at +0x592. Same size/relocs.

## Prior negatives (do not repeat; from ledgers 0828/0830/0901/w2/w3)
s1 decimal expr in arg list; s3 block-local decimal_modulus; s4 HCEA separate meters local;
s5 named real decimal_part; d1-d12: 1.0f, other int consts, NONE/FALSE, constant decimal,
fast_ftol_C for decimal, parenthesised arg, real variable override, x87 (real)short
override (loads interleave inside its push seq); c1 `0`, c2 `0.0`, c3 `0.f`;
explicit/implicit short conversion; literal/local zero.
January-wide scan (w2): `push imm; mov r32,[inline-ftol result]` nowhere else;
push-const/load/push-const/push-reg only in a cross-jumped TIFFReadDirectory tail.
Also recorded negatives: lane C C6 name-count oracle N=0..63 INERT for this fn
(claude-lane-c .../C6-idiom-sweep); w9 (throughput lane): power(10.0f,4.0f), outer-scope
short decimal_value, long decimal_value -> identical schedule.

## /Od readout (0x62fcf0, later build with 5th render_distance param)
Number block order: distance*=3.048; csmemset placement(0x24), numbers(0x54);
placement.corner=0; numbers.colors.color / flash_color = alpha<<24|rgb; digits=3,
fractional_digits=1, number_flags=5; offset.x/.y = (short)(point + 0.33/0.66*scale*extent);
offset.x -= (window.x0 - viewport.x0) [single statement]; pow-helper(10.0f,4)->temp at
[ebp-0x144] (declared after numbers); then ONE call:
hud_draw_numbers(lp, &placement, &numbers, fast_ftol_C(distance),
  fast_ftol(fmodf(fabsf(temp*distance), temp)), 0, 0, 0.0f)  [fast_ftol a real call 0x455400].
/Od pushes 0.0f (push ecx; movss [esp]) FIRST, then 0,0, then evaluates the decimal chain.

## Probes (this wave)
- pushload2.py (broad January scan: push imm; mov r,[m]; ...; push r in same run): 12 rows,
  11 exact elsewhere (action_obey snprintf tails, find_turning_point, TIFF r/w dir, ui_widget,
  create_ghost_effect) - all explained by register reuse (reg busy until a prior push) or
  cross-jump tails; none is an inline-asm-result + free-register case like ours.
- d1 (LAB, semantics changed): decimal moved to arg 7 -> loads (movsx eax; mov ecx) still
  hoisted above P8. => all register loads of the statement are emitted at the region start
  regardless of which push first needs them. January's P8 must sit in an EARLIER region/
  be pinned by a dependency, not a priority tie.
- lab harness (lab.py appends lab fns to v0.c, same TU/flags): lab_a reproduces ours exactly
  (loads, push0 x3). lab_b long-param prototype + no casts; lab_c direct __asm into a long
  local; lab_d no casts in arg list: ALL identical (loads first). lab_e (non-const arg 8 +
  inline call in arg list): whole decimal chain + asm emitted before ANY push => inline body
  is hoisted before the statement (or pushes sunk). d5 (static const real zero arg 8): not
  folded, mov eax,[lab_zero] (+1 reloc) - rejected. d10 repro: x87 (real)short arg ->
  push ecx slot then fild/fstp [esp]; loads still hoisted except the ecx conflict.
- lab5 (stores through a pointer between asm and call): list scheduler interleaves P8..P6
  with the stores; loads still hoisted first. lab6 (address-taken local as push operand):
  load hoisted above pushes -> pushes are NOT treated as aliasing locals.
- KEY DONOR: TIFFReadDirectory (strict exact in OUR build) has the same push-const / load /
  push-const / push-reg shape at +0x701 (MissingRequired(tif,"StripByteCounts") inlined static,
  cross-jump survivor block), while its sibling sites (ImageLength, Colormap) are loads-first.
  => our compiler DOES emit a const push before a free-register load when a block/region
  boundary separates them. Hypothesis H-B: January has an (optimised-away) block boundary
  between arg 8's push and the arg loads.
- TIFF mechanism probe (copy of tif_dirread.c, tiff/tv.py): t1 (drop nstrips reuse) keeps
  push-first; t2 (replace the sibling MissingRequired sites with direct TIFFError) flips the
  StripByteCounts site to loads-first. => the donor shape is a CROSS-JUMP/TAIL-MERGE artefact
  of several identical inlined call sites; needs >=2 identical call tails. January
  custom_render has ONE hud_draw_numbers call and no jump into +0x58C: donor mechanism does not
  transfer directly. lab7/lab8: ?: with equal arms / C2-foldable condition in args folds early,
  no boundary survives.
- TIFF t3 (siblings keep shape but different fmt): survivor becomes `push SBC; push fmt;
  mov ecx,[ebx]; push ecx` - the load sits exactly at the start of the IL-level common
  suffix shared with the siblings. DECODED: an IL tail-merge block boundary; the load is
  hoisted only to that block head; pushes before it stay in the predecessor block; siblings
  later get re-duplicated copies and share only the call. So January's +0x58A implies an
  IL block boundary between arg-8's push and arg 7 (loads hoisted to that boundary).
- lab11/14/15: equal-arm ?: (C1 folds), `cond ? 0 : zero_local` in arg 8/7/6 (C2 folds
  fully): no boundary. lab13 (if/else two full calls with nested fast_ftol_C, join after):
  no IL tail merge (only the final call cross-jumped). lab16 (two calls WITHOUT nested call,
  goto join): IL tail merge DOES happen (survivor: pushes then merged block loads).
- lab17: dead second path (local FALSE flag) is removed BEFORE IL tail merge -> no boundary.
- ORACLE sweep (sweep_locals.py, not landable): 1..12 unused long / 1..9 unused real dummy
  locals at function top and in the numbers block -> all INERT (window unchanged).
- EVIDENCE (same idiom, January): hud_weapon _render_weapon_hud +0x88B..+0x91B uses the SAME
  decimal chain (fld 10.0/4.0; __CIpow; fst; fmul; fabs; fld; __CIfmod; fstp; fld; fistp =
  inline fast_ftol) and TWO hud_draw_numbers calls tail-merged at the value push (if-branch
  jmp 0x90b). Our hud_weapon row is residual 2624!=2656 (separate unit, not this task).
  In that January window the loads after fistp come FIRST (normal) - custom_render remains
  the unique January instance.
- ORACLE flags (not landable): /G5 breaks 19 fns; /G6 /G7 /GB /Ot /Og identical; /Os breaks all.
- lab18 (trivial inline returning 0 as arg 7/6): substituted as expression, no boundary.
- lab19 (assignment / comma expression in arg 7): inert.
- od1 (/Od-attested tail: placement declared before numbers, decimal_modulus =
  power(10.0f, 4.0f), decimal fully in the arg list): 31 exact, identical 8-byte residual.
  Not proposed (zero gain).

## Object-level audit (production obj)
object_audit FAIL(2): _custom_render_nav_point bytes; _nav_point_data storage split 2 /
ours 3. pdb_storage: 2 disagreements - _nav_point_data and _hud_update_nav_point_local_player
are ABSENT from cachebeta publics (=> January file-static) but split objects say external
(symbols.json lacks "static": true). Source already has nav_point_data static;
hud_update_nav_point_local_player is external in source (only caller is in this TU).
- st1: `static` on hud_update_nav_point_local_player (prototype + definition): 31 exact,
  no row change (private ABI unchanged). Zero-credit ownership correction, proposed with
  scratch/w/hud_nav_points/config/symbols.json (+"static": true on lines 3422 and 23038).
- csplit regen into scratch (tl/object_audit_regen.py, splitout/, cfg/ = config + edited
  symbols.json): ONLY source/interface/hud_nav_points.obj changes (2 storage classes 2->3).
  object_audit(st1.obj vs regenerated split): 46 January symbols, 0 differ; FAIL(1) =
  _custom_render_nav_point bytes only. Data: build/report.json .bss/.rdata 100%.
- surplus_identity / provider_link (production obj): _fast_ftol, _arctangent, _square_root
  IDENTICAL/PASS; _object_get_bounding_sphere DIFFERENT vs January's selected copy
  (action_vehicle NODUP hand copy, assert file "..\objects\objects.h"; our objects.h inline
  uses "c:\halo\source\objects\objects.h") -> LNK2005 both orders. January string census:
  "..\objects\objects.h" DEF action_vehicle, REF render_objects; "c:\halo\source\objects\
  objects.h" DEF object_lights. hud_nav_points pulls objects.h via units.h and collisions.h.
  This is the brief-section-7 known conflict; needs the objects.h/action_vehicle provider lane.

## Conclusion (stop rule)
_custom_render_nav_point: NO_PROGRESS. Every natural spelling (prior 30+ plus lab forms here)
gives loads-first. Decoded: VC7 hoists all register loads of a region to the region head;
the only exceptions found are register conflicts (not applicable: eax/ecx free after the asm)
and IL block boundaries (TIFF tail-merge donor). January's single-call, branch-free window
has no partner path for a tail merge, and C2-foldable conditionals / dead paths are removed
before blocks are formed. Reopen only with: (a) evidence of a second hud_draw_numbers call
path or other live IL block boundary in January's custom_render_nav_point source (e.g. an
authentic January source/fragment), or (b) a decoded C2 dag.c rule showing another way a
const push is emitted ahead of region-head loads (live C2 trace of the TIFF survivor vs this
window).
Proposed (zero credit, independent): ownership.patch + config/symbols.json copy.
