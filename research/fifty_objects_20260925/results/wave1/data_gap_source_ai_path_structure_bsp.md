# source/ai/path_structure_bsp

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/pathbsp/cand_wh.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/patches/path_structure_bsp.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/fix_pathbsp.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/pathbsp/cand_hw.c"
]

## production_changes
Case B, a real reconstruction. It needs no semantic entry because ordinary objdiff credit follows.
(1) git apply scratch/w/data_gap/patches/path_structure_bsp.patch. In source/ai/path_structure_bsp.c it adds, under /* ---------- globals */ and in this order:
  static real const quantized_pathfinding_surface_widths[8] = { 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 1.5f, 2.0f, 4.0f };
  static real const quantized_pathfinding_surface_heights[8] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 4.0f, 8.0f };
  It also updates the header symbol comment.
(2) Edit config/symbols.json by line surgery, keeping CRLF and line order:
  - line 13113: rename to { "file_offset": 2419724, "flags": 0, "name": "_quantized_pathfinding_surface_widths", "static": true },
  - insert directly after it: { "file_offset": 2419756, "flags": 0, "name": "_quantized_pathfinding_surface_heights", "static": true },
  These edits are included in patches/symbols_json_all.patch.
(3) Do NOT flip the status to Matching. The pre-existing NODUP provider conflicts below remain.

## evidence
contribs.json shows that module 426 (path_structure_bsp) owns a non-COMDAT 64-byte .rdata at file offset 2419724 with flags 0x40300040. Its bytes are the floats {0.2 .. 4} followed by {0.25 .. 8}.

No January object refers to it, going by a resolved-address scan of all 833 split objects. cachebeta.pdb does not list it as a public.

HCEX.pdb's compiland lists two File Static const float[8] arrays: quantized_pathfinding_surface_widths at 0xC5494 and quantized_pathfinding_surface_heights at 0xC54B4. HCEX_debug.exe holds the big-endian {0.2..4} at exactly 0xC5494 and {0.25..8} at 0xC54B4, which ties each name to its values by address.

Results:
- gate: 6/6 EXACT.
- object_audit against a scratch csplit (only this split object changes): PASS, 13/13 symbols, 64-byte section equal including flags.
- objdiff 3.3.1 mini report: .rdata goes from 70.14% to 100%, and matched_data from None to 200 (+200 B). Code stays 2512/2512.
- Control: defining heights first swaps the halves (DIFF). This non-COMDAT const .rdata follows forward definition order.

The codex salvage ledger withdrew an earlier single 64-byte array because it came out 8-aligned and only __declspec(align(4)) fixed that. The two authentic 32-byte arrays are 4-aligned naturally, so no annotation is needed.

fake_match_scan finds 0 leads, and compiler warnings are unchanged.

## blockers
Whole-object admission is blocked by pre-existing surplus pair-link failures. The 10 helper COMDATs are all identical to January's selected copies, but provider_link fails for _cross_product2d (actor_combat NODUP hand copy) and _distance_squared2d (action_vehicle NODUP hand copy). This is the same in the production base.

## reopen_criteria
Flip to Matching after the NODUP providers are genuinely repaired and provider_link passes in both orders.

## task notes
Expected credit: +1,973 data bytes.
- +1,773 comes from the three Case A single-section entries (periodic 96, king 136, object_lights 1,541); scratch/tools/progress_calc.py takes data from 2,579,142 to 2,580,915, with 38 credits and 0 revoked.
- +200 is path_structure_bsp's ordinary objdiff credit after the Case B reconstruction.
- Code is unchanged and there are no function regressions.

The Case A entries and the ownership/source patches are independent. The entries verify against today's production objects, and I re-checked their snapshots under the proposed symbols/storage changes; they are unchanged.

Integrator order: apply the patches, then csplit plus ninja, then rerun the verifier and progress. The combined symbols.json edits (34 ops, scratch/w/data_gap/fix_all.txt) have been tested with a scratch csplit (cfg_all_split). They change exactly 4 split objects out of 833.

Tool caveat for other workers: tools/campaign/gate.py copies --source to scratch/_gate_PID.c, so a shadow header placed beside the candidate is silently ignored. scratch/w/data_gap/sgate.py compiles in place and honours shadow headers; I confirmed this with a #error test.

Code under-credit is already handled: objdiff scores periodic_function_build_table, transition_function_build_table, bitmap_copy and bitmap_draw_line below 100%, but all four are strict exact and already credited through build/semantic_report.json hidden_exact.

Systemic blocker found: 18 base objects emit a header-inline _distance3d whose bytes differ from January's selected action_vehicle copy. It also fails pair-link against action_vehicle's NODUP hand copy, and none of those objects is Matching. This alone stops king and object_lights from becoming complete objects.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/LEDGER.md
