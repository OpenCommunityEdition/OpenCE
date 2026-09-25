# source/rasterizer/rasterizer_frame_statistics (canary)

## verdict
RESEARCH_ONLY

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\A_real_math.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\A_matrix_math.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\A_collisions.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\A_cseries_windows.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\A_bitmaps.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\AS_real_math.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\packet\\B1_effects.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\canaries.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\consumer_sweep_list.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\rerun_packet.py",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\canary_frame_statistics\\grid_g1.txt"
]

## production_changes
None. No tracked file was edited. The packet exists only as lab patches: packet/A_*.patch and AS_*.patch, plus B1_effects.c.patch, copied from the round-1 breakable audit. math/real_math is reserved for Codex, so nothing is proposed for landing now.

## evidence
All measurements are in scratch/w/canary_frame_statistics/LEDGER.md (F1-F9). Tools: lab.py, board.py, mkvar.py, netcount.py, window.py, grid.py, scan_i.py, rerun_packet.py. Control: a whole-board shadow sweep of 447 units matched build/base per function (size, sha and relocations) in every unit.

(1) The period is exactly 64 and draw's window is 1 name wide. With K dummy tags at /* public code */ or at the top of the TU, draw is EXACT only at K = 0, 64 and 128 (tested 0-140). P3's finding that names before the globals never help (0-45) was a range artifact. bitmap_copy, units and alpha_bleed also repeat every 64.

(2) The currency is C1's symbol records, wider than declared names:
- 1 record each: declared names, params, locals, each string literal (deduplicated per TU), each compiler label (each `if`; `&&` inferred from spl1 accounting) and each distinct file opened (a new empty #include costs +1).
- 0 records: numeric constants, macros, extra {} blocks, re-including a guarded header.
- /E output compiled as C equals the .c build plus 7 names.

(3) For draw, only the net record count before the .c globals block matters, mod 64. One dummy placed at any of 24 positions (SDK, cseries, real_math.h, rasterizer headers, the .c up to the verify typedefs) always gives the same fingerprint, 7aafc0. So draw's exactness is a coincidence of the whole TU's count, which matches January's mod 64. No single missing declaration explains it.

(4) netcount.py uses draw as an exact mod-64 counter. Net costs in its TU:
- rlr (real_local_random named local): +1
- vp (valid_real_plane3d prototype replaced by an __inline): +1
- stack_walk duplicate removed, either copy: -2
- spl1 (nonuniform_cubic_spline replaced by glow.c's January-exact body): 0
- spl3 (the _vector3d body): -11

(5) Grid g1 ran 36 whole-board sweeps: rlr+vp x {none, swcs, swcw} x {none, bdI, bdB} x spline options. Zero-loss combinations exist only with swcw + bdB (with or without spl1). Grids for rlr alone or vp alone found no zero-loss combination, so the two halves must land together.

(6) Packet PA = rlr + vp + matrix_math plain-definition removal + collisions loop-site call (keeping the /Od function-scope `position`) + swcw + bdB. Results:
- Gains _collision_move_point, 4752 B, sha 8b2f2900.. equal to January. Loses nothing.
- Every data section is identical as a multiset.
- /W3 warning census is unchanged.
- object_audit: collisions PASS; the others match their base audits.
- Every new surplus COMDAT is byte-identical to January's selected copy.
- Packet-object pair links PASS in both orders; pairing with the base matrix_math or base effects objects FAILs, so those halves are required.
- PAE adds the effects B1 half and still loses nothing.
- PAI (drop the bitmaps_internal.h copy instead of bitmaps.h) loses units::_unit_preprocess_node_orientations, 1920 B.

## blockers
1. math/real_math is RESERVED for the Codex packet. Its own real_math.h change will shift every canary again, so this packet has to be re-solved afterwards with rerun_packet.py --refresh.
2. Which duplicate copy to drop is decided by the count; no January evidence settles it. Both copies of each duplicate were added by the campaign:
- stack_walk: cseries.h in 5a89849d (Jan 2026), cseries_windows.h in 5d3444f7 (Sep 2026).
- bitmap_delete: bitmaps_internal.h in c0fcc65f (Aug 31, a campaign-invented header), bitmaps.h in e9a1b0c7 (Sep 2, the HCEX-stub public header).
The zero-loss packet drops the later-added copy in both cases. For bitmap_delete that goes against ownership, which favours keeping bitmaps.h and gives PAI, costing units 1920 B. Owner ruling needed.
3. The parked __rasterizer_model_draw hash moves (89b0d7ea -> cbfa8585 at 5168/348), so its parked.json measurement needs re-baselining.
4. The new surplus COMDATs (_valid_real_plane3d in collisions and render_cameras, _real_local_random_range in effects) need owner COMDAT admission.
5. The fully genuine route is January's missing real_math.h content, and current evidence cannot recover it. The known deviations in draw's TU add up to +17 versus January, so about +17 records (mod 64) are missing. Assert anchors show January had about 114 more lines between Jan line 880 and 1508.

## reopen_criteria
Reopen when all three hold: (a) Codex reports and math/real_math is released; (b) python scratch/w/canary_frame_statistics/rerun_packet.py --refresh still shows 0 lost for PA, PAS or PAE; if not, rerun grid.py and window.py for the lost canaries using canaries.txt and consumer_sweep_list.txt; (c) the owner rules on the duplicate copy choice or first-party evidence of January's header placement appears. Alternatively, recover January's missing real_math.h declarations (the ~17 records mod 64).

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\canary_frame_statistics\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\canary_frame_statistics\LEDGER.md
