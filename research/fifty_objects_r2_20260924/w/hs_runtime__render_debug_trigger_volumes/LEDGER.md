# LEDGER r2w1 hs_runtime::_render_debug_trigger_volumes (1088 B)
Worker label: r2w1-hs_runtime::_render_debug_trigger_volumes (1088 B). Base f6d00a8c (worktree HEAD b7c1b7d9 = baseline commit on top).
Slug dir: scratch/w/hs_runtime__render_debug_trigger_volumes/ (':' is illegal in NTFS paths -> sanitized).
Claims: "source/hs/hs_runtime::_render_debug_trigger_volumes (1088 B)" (literal task unit) + source/hs/hs_runtime. Both exit 0.

## Probe 0: baseline
- gate --all on unmodified hs_runtime.c: 64 exact / 2 residual (_hs_evaluate_begin_random sha, _render_debug_trigger_volumes sha). base_gate.txt, base.obj.
- Prior negatives read: opus5_150k_w1 (t1..t7), opus5_250k_w3c, throughput w7 (cand_render_debug_trigger_volumes.c EXACT w/ duplicated arms + hand-expanded helper), triage_game_hs_net LEDGER probe 4.

## Probe 1: /Od RTC descriptor (new, this worker)
- odbuild.rtc(0x606c80,0x607462) -> desc 0x607464, 9 vars: matrix -64/52, local_extent -96/12, world_extent -116/12,
  corner -176/48, sides -208/24, color -240/16, center -260/12, ray -280/12, result -368/80.
  => the 4-point array's first-party name is `corner` (real_point3d corner[4]), not `points`. HCEX_Release.pdb lists
  matrix, local_extent, world_extent, sides[2], result, ray (corner/center/color not homed on PPC).
- /Od scalar slots: volume_index -0x48 (short), scenario -0x4c, volume -0x50 (initialised to 0 before the loop: `mov [ebp-0x50],0`),
  edge_index -0x7c (short), side -0xd8 (short, computed first), axis -0xdc (short).

## Probe 2: candidate c1 (task route, first compile)
- Hypothesis: /Od shape (duplicated tail in both side arms, real point_from_line3d with t=1/1/1/-1 and 0.5 center,
  sides[2]={0}, side before axis, if/else string calls) reproduces January at /O2; *1.0f folds (action_vehicle premise).
- Change: fn_c1.txt spliced into base.c (mk.py) -> c1.c. Local names kept (points) for the first measurement.
- (A first compile had the assert __FILE__ backslashes collapsed by the shell heredoc -> reloc-identity only on the
  .rdata string; fixed the fragment, not a codegen finding.)
- Result: gate --all 65 exact / 1 residual (_hs_evaluate_begin_random only). _render_debug_trigger_volumes EXACT 1088.

## Probe 3: rename points -> corner (/Od RTC name)
- c2 = c1 with `points` -> `corner` (18 sites). gate: 65/1, trigger_volumes EXACT. objeq.py c1 vs c2: only .debug$S differs
  (scratch path), all code/data sections and the symbol table identical -> rename byte-inert.

## Probe 4: admission battery on c2
- object_audit c2.obj: only DIFFs are _hs_evaluate_begin_random (T tie, pre-existing) + the 12 converter storage rows
  (pre-existing; separate ADMISSION claim "source/hs/hs_runtime (storage: 12 converters)"). _render_debug_trigger_volumes ok.
  Surplus gains exactly one new section: +.text sym:_point_from_line3d size 48. (c2_audit.txt)
- symbol diff base.obj -> c2.obj: only $L labels + `_point_from_line3d` (storage 2, .text COMDAT). January hs_runtime.obj
  neither defines nor references it (all-inlined TU class, owner ruling 5 2026-09-21).
- surplus_cand.py (surplus_identity logic on the candidate obj): _point_from_line3d NEW vs source/ai/action_charge.obj IDENTICAL;
  4 pre-existing surplus COMDATs still IDENTICAL. (c2_surplus.txt)
- provider_link c2.obj --baseline=base.obj: _point_from_line3d provider source/ai/action_charge PASS (both orders).
  Unrestricted run: all COMDAT/literal surplus PASS; 12 FAIL = the pre-existing converter storage rows (NO JANUARY PROVIDER). 
- nodup_census _point_from_line3d: 24 definers, NODUP [] -> SELECT_ANY everywhere.
- definers.py: base definers span config index 37 (units) .. 470 (action_charge = January's selected copy); hs_runtime is 283,
  strictly inside the range, so the linker's selected copy cannot change in either link direction.
- cachebeta publics: _point_from_line3d is PUBLIC (line 6312) -> external COMDAT storage agrees with PDB.
- fnsweep.py base.obj vs c2.obj over all 127 January-owned sections: gain 1 (_render_debug_trigger_volumes), regression 0,
  changed-vs-base 1 (only that function). build/base production obj vs my base.obj: changed 0 (baseline compile == production).

## Probe 5: /Od declaration order (reviewer law: groupings must not contradict /Od declaration order)
- /Od frame (first-declared = highest address): matrix, volume_index, scenario, volume, local_extent, world_extent,
  edge_index, corner, sides, side, axis, color, center, ray, result.
- Variants (all full-TU gates): od_order (outer volume_index before scenario; volume-loop block matrix, volume, local_extent,
  world_extent, edge_index, corner, color, center, ray, result), od_order_vfirst, od_inner_only, od_outer_only,
  color_edge (color after axis in the edge-loop block), color_else (`real_argb_color color = *global_real_argb_blue;` in the
  else block = /Od order sides, side, axis, color + house rule 17), bareblock (lab only: center/ray/result in a bare block
  after the edge loop = full /Od text order). ALL 65/1 with trigger_volumes EXACT -> declaration placement is byte-inert here.
- Chosen: color_else (no invented bare block; only center/ray/result precede the edge-loop locals, which C89 forces
  without a bare block; the later /Od build may be a C++-style mid-block declaration, not attestable).
- /Od `volume = NULL` outer initialisation (mov [ebp-0x50],0) NOT reproduced: it is a dead store at /O2, January bytes cannot
  attest it, and it is not needed; recorded only.

## Probe 6: strip test of the duplicated tail + side/axis order
- single_tail.c (color_else with the 3 point_from_line3d calls hoisted once below the if/else): _render_debug_trigger_volumes
  residual [sha]; alndiff 342/342 insns, 53 hunks, 97 differing target insns (frame-slot renames) -> the duplicated tail is
  load-bearing in January's bytes (frame ranking), and /Od shows it as source text: double attestation.
- axis_first.c (axis before side): EXACT -> inert; kept /Od side-first order.

## Probe 7: final candidate cand.c (= color_else.c) battery
- gate --all 65/1/0 (only begin_random). object_audit FAIL(13) = begin_random + 12 converter storage rows (pre-existing).
- surplus_cand: _point_from_line3d NEW IDENTICAL vs action_charge; provider_link --baseline PASS both orders.
- fnsweep base->cand: gain 1, regression 0, changed-vs-base 1. /W3 /WX: no warnings (base and cand).
- production.patch / production_crlf.patch: git apply --check clean; GNU patch simulation reproduces cand.c from both the
  LF HEAD blob and the CRLF working tree; the patched CRLF file gates EXACT (applied_crlf_gate.txt).
- park_retire.patch (+crlf): removes the {source/hs/hs_runtime, _render_debug_trigger_volumes} parked.json entry by line
  deletion; git apply --check clean; JSON validity checked.
- Composition with scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch (the parallel ADMISSION claim): hunk-disjoint,
  order-independent (byte-identical results), composed gate 65/1/0, object_audit FAIL(1) = begin_random only, 129/129
  January symbols match, provider_link PASS (all surplus).

## Verdict
- _render_debug_trigger_volumes: EXACT with the /Od-attested helper form (owner ruling 5 class). Function-level landing
  proposed: production.patch + park_retire.patch. Object blocked by the begin_random tie (retired) -> FUNCTIONS... not all
  exact; storage rows handled by the separate claim.
- Follow-up: re-run surplus_cand/provider_link after the Codex real_math packet lands.
