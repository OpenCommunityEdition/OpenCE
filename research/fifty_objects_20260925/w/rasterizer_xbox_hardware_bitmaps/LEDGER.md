# LEDGER rasterizer_xbox_hardware_bitmaps (wave-2 worker, base 931ed8dc)

Target: _rasterizer_bitmap_new (400 B, parked instruction-scheduling). All other 18 fns EXACT.

## Baseline (probe 0)
gate --all on unmodified source copy (base.c): exact 18 residual 1 unwritten 0.
alndiff: 150/150 insns, zero instruction-text differences; sole byte = jne disp at +0x159:
January jne 0x171 (epilogue E1 = fallthrough of error block), ours jne 0x17e (epilogue E2 = fallthrough
of no-device `hardware_format = NULL` block at 0x177). Both epilogues `pop esi; mov al,bl; pop ebx; pop ebp; ret`.
bl holds success; ebx=1 pinned at entry (used as TRUE constant for display_assert halt arg too).

Prior negatives (do not repeat): >50 probes (shared/split returns, predicate polarity, goto topology,
success materialisation, duplicate/single cleanup, declaration/lifetime). w3c: no shape spent (A43).
NEW evidence since the park (2026-08-30): memory law "block-selection-is-source-reachable" (2026-09-21):
returns merge on VALUE; the canonical epilogue owns the $L label; conditional branches target canonical;
changing the VALUE of a return flips the target. Instrument: CL /FAsc. This is the lever to test.

## Evidence review (no compile)
- /Od build (halo_cache_symbols, dx9 later revision) fn 0x7ea7d0 = rasterizer_bitmap_new (dx9): single `boolean success=TRUE`
  ([ebp-1]), asserts L0x68/0x69, `if (global_d3d_device) { if (format ok) { switch {cases: if (FAILED) {success=FALSE; rasterizer_error}}
  if (!hw) success=FALSE; if (!success) hw=NULL; } else hw=NULL; } else hw=NULL; return success;` - SINGLE EXIT, no error() call,
  no `success=TRUE` in case arms. Dump: scratch/w/.../od_7ea7d0.txt. Later revision; shape evidence only.
- Lane C block-selection FINDINGS (claude-lane-c .../scratch/research/block-selection/FINDINGS.md) ~30 probes incl. A3 single exit
  (identical to base), A4 single exit w/o success=TRUE in else (147 insns, loses ebx=1 pin), er/els early guard (384 B), m1 return FALSE
  (jne correct, R2 xor al,al). opus5 crossjump-epilogue notes R3-R8: 38 whole-TU spellings -> 5 bodies, none January; decl-count,
  position, names, case order (24 perms) all inert. => do NOT repeat any of these.
- New angle: corpus has 33 January-exact FIRST-binding instances of this topology (topo.json). Study their SOURCE to find what
  source construct puts the original exit before the region (ours = LAST).

## Probe a1/a2 (hypothesis: /Od dx9 shows case arms `if (FAILED) {success=FALSE; rasterizer_error}` with NO success=TRUE; test if
January matches that): a1 = arms without success=TRUE (two-return tail), a2 = a1 + /Od single exit `else {hw=NULL;} return success;`.
RESULT both: size 432!=400, relocs 28!=26, 161 insns - REGRESSION. The explicit `success = TRUE;` arm (same idiom as exact
rasterizer_xbox.c preinitialize) is load-bearing; January had it. /Od dx9 arm shape is a later-revision difference. Do not repeat.

## Probe a4 (reproduce Lane C A4 single exit w/o success=TRUE in else, to read its bytes): 147 insns, `mov bl,1` (8-bit success)
instead of `mov ebx,1` CSE pin, `push 1` everywhere, jne still LAST (0x183). Confirms the pin needs ND to consume constant 1.

## Hypothesis T7 (layout): January = [err][R orig][ND][clone]. With IL-order layout and "merged return keeps the textually
LAST return's position", January needs ND's `return TRUE` textually BEFORE the final `return success` while ND stays the
else-arm of `if (global_d3d_device)` (je forward to the end). The only natural if/else spelling with that property:
`if (dev) { switch; [if (!hw) success=FALSE;] } else { hw=NULL; return TRUE; } [if (!hw) success=FALSE;] if (!success) {...} return success;`
Not in any recorded probe list (v7/v8 = single exit, ND not returning; A3/A4/g/v4/T5 differ). Test T7 and T7b.
RESULT t7, t7b: both == base (jne 0x17e, 150 insns). /FAsc (p/t7.asm) is informative: VC7 MOVED the textually-earlier
ND else-arm (line 260) to the END of the layout (after err+R1), and the merged canonical (label $L20967 = end label of
`if (!success)`) still lands after ND carrying line 261 `return TRUE`. So ND-at-end is reachable (January's ND position)
but R1/R2 still merge and the canonical follows the layout-last fall-through predecessor (ND). Lane C criterion (a) stands:
ND must reach a DIFFERENT return block while still emitting `mov al,bl`.

## Probe s1 (hypothesis: dx9 /Od shows 3 distinct per-case HRESULT slots => per-case block-scoped `long result`):
RESULT == base (jne 0x17e). Inert. Do not repeat.

## Survivor-rule inference (from recorded probes + t7 listing)
merged R1/R2 canonical sits after the LAYOUT-LAST fall-through predecessor: base (ND last, R2 kept), t7 (ND moved to end,
R2 kept although textually first), er/v1 (ND inline at top, R1 kept). "constant-valued survives" and "fewer preds
survives" both refuted by er. January needs R kept after err with ND laid after it => impossible while ND's return merges
with R1; needs an ND return that is NOT merged (IL-distinct) yet emits identical `pop esi; mov al,bl; pop ebx; pop ebp; ret`.

## Corpus counterexample study (no compile) - the 33 January-exact FIRST-binding instances (opus5 topo.json)
first_scan.py lists them with region/C1 predecessors. The TAIL ones (region ends at function end, like bitmap_new):
_TIFFFindFieldInfo, _oddball_test_trait, _oddball_weapon_pickup, _objects_get_activating_cluster_index,
_rasterizer_preinitialize__..., _scenario_get_fog_region_index, _looping_sound_new, _structure_get_planar_fog_definition_index,
_unit_get_aiming_unit_index, _compute_airborne_ticks, _vehicle_stuck. Read source+bytes of get_next_event, switch_to_pregame,
game_settings_update, oddball_test_trait, preinitialize, unit_get_aiming_unit_index, vehicle_stuck, looping_sound_new:
every FIRST case is either (i) a loop-exit / inlined-loop 'found' block sunk after the return, (ii) a shrink-wrap pop block,
or (iii) the layout-last fall-through predecessor of the return being value-SPECIALISED to a different return text
(`mov al,1` in switch_to_pregame/game_settings_update: `result = TRUE` arms). None has a same-text constant return on the
sunk arm. bitmap_new's ND arm returns constant TRUE, which VC7 materialises as `mov al,bl` from the ebx=1 pin => same text
=> merged => canonical follows ND. No construct in January's bytes (all success defs are constants; no loop; no shrink-wrap
pop in the region) offers a family (i)-(iii) lever.

## STOP (methodology stop rule)
5 new evidence-based shapes this wave (a1, a2, t7, t7b, s1) + a4 reproduction: none moves the binding without regressing;
together with >50 park probes, 38 opus5 whole-TU spellings, ~30 Lane C probes. Remaining difference: one jcc displacement
(+0x15A: January 0x16 -> 0x171, ours 0x23 -> 0x17e) choosing between two byte-identical epilogues.
Best credible candidate = production source unchanged (base.c). Zero credit. No production change.
Reopen criteria (sharpened):
 (a) any natural spelling in which the ND arm's return is NOT merged with `return success` yet still emits `mov al,bl`
     (i.e. ND's return text differs from R1 at merge time but equals it in bytes) - value-specialisation family (iii);
 (b) an original January source/PDB/donor for this function (HCEX has none: 0xE4-byte PPC rewrite, no locals; HCEA
     research/halocea/src/rasterizer_bitmap_new.c is a decompilation of the 2011 rewrite, not source; /Od dx9 build is a
     later revision whose arms lack the load-bearing success=TRUE);
 (c) a decoded C2 layout rule for where a merged return is placed relative to a sunk else-arm (t7 listing shows the
     else-arm sink happens BEFORE the merged return is placed).

## Object audit on baseline (scratch/tools/object_audit.py on base.obj)
All January sections/symbols ok except .text _rasterizer_bitmap_new (400/400, one byte). 32 January symbols, 0 differ.
Candidate-only surplus (would need surplus_identity/provider_link audit if the function ever closes): .drectve,
??_C@_06PCHFJCOP@bitmap, ??_C@_0CC@DECIGGNF (unsupported bitmap type), _D3DPRIMITIVETOVERTEXCOUNT, _D3DSIMPLERENDERSTATEENCODE,
_D3DTEXTUREDIRECTENCODE (XDK header tables). OBJECT AUDIT: FAIL (1) - the sole blocker is the parked function.
FINAL: NO_PROGRESS. No production change; keep the park.
