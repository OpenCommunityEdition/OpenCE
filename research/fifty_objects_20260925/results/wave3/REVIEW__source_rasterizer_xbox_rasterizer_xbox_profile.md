# REVIEW source/rasterizer/xbox/rasterizer_xbox_profile

approve: False

## per_object
null

## checks
All checks are my own re-runs. Evidence is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_rasterizer_xbox_profile\LEDGER.md (V1-V7) and REVIEW.md.

(1) git apply --check on the current tree:
- 01..07 each apply alone; 04 also applies alone, because its hunks do not overlap 03. The full series applies.
- The series applied in a scratch git tree (autocrlf) gives post-images byte-identical to the worker's candidate_files.

(2) gate.py --source candidate profile.c with the patched rasterizer_xbox_internal.h on /I: 16/16 EXACT, 0 residual, 0 unwritten.

(3) object_audit.py on that obj:
- PASS: 59 January sections ok, 67 January symbols with 0 differences.
- Surplus: 7 literals, the 3 D3D tables and .drectve.

(4) surplus_identity: 0 candidate-only code COMDATs. provider_link with the candidate obj: 10/10 PASS in both orders. pdb_storage: 0 disagreements with cachebeta publics.

(5) Data:
- data_entry: equal. 132 B, 29 relocations all resolving to image addresses, sha b577924a, flags c0400040, storage 3/3.
- My candidate and shadow objects are resolved-equal to January.
- build/report.json shows .data at 68.686874% (the known '$' defect).

(6) Consumer sweep with my own harness (mini_shadow.py):
- It runs from the shadow root with production cflags and does an owner-keyed coff_compare.section_infos_equal plus flags plus symbol-table comparison against build/base.
- It covers every includer of every changed file (grep: 8 internal.h consumers, no header includes it), plus profile and detail_objects: 10 TUs.
- Pristine control: 10/10 identical. Patched 01..06: 10/10 SECTION+SYMBOL IDENTICAL.

(7) CL /Zs /W3, pristine vs patched: identical warning multisets in all 6 edited TUs. fake_match_scan: 0 leads.

(8) Strip tests:
- The (void *) csmemset casts are inert (still 16/16).
- Stripping all 3 volatile qualifiers gives 12/16 (callback, frame_begin, initialize, query). This reproduces the worker's F6.

(9) /Od 0x8004c0 attests the check parameter order (condition, profile, message) used by patch 04. No new block scopes anywhere.

(10) HCEX DIA2Dump -g: no rasterizer profile TU statics, only a folded rasterizer_profile_enable stub.

(11) January data references (dataref.py) plus a VC7 layout probe (probe_loose_layout.c).

(12) Name-independent comparison (compare_resolved.py) of lab de-aggregations:
- L1 (loose statics): all 16 functions have identical code bytes and .data is resolved-equal. The only .bss difference is error_count at 0x45c instead of January's 0x460.
- L2 (error_count as a static local): same result.

(13) /Od .bss map of the later build.

(14) D3D8.h:325-367 plus January profile imports (_D3DDevice_InsertCallback@12, QueryPerformance*).

## issues
REJECT for admission. The worker's measurements all reproduce and every patch is byte-inert. However, the object source has three issues that block admission under the section-8 and section-2 standards.

B1 (blocker, section 8(g) "invented aggregates"): the TU's private state is four file-static structs with no first-party attestation:
- rasterizer_profile_globals, with pad02/pad06;
- rasterizer_profile_elapsed_state;
- rasterizer_profile_frame_state, with pad12/pad16;
- rasterizer_profile_state, with reserved06[6].

Evidence against them:
- VC7 /O2 pads every file-scope static to 4 bytes, shorts included. So loose statics reproduce January's .data layout exactly (0/8/0xc/0x10) and every .bss offset but one. The pad members only spell out compiler padding, which is the fog-O2 signature of an invented aggregate.
- Lab L1 de-aggregation leaves all code bytes identical in all 16 functions. The only difference is error_count at 0x45c instead of 0x460.
- The later /Od build places elapsed_times (e0e9d0) before callback_start_times (e0eac8). That contradicts elapsed_state's fixed member order.

What remains open:
- January has an unreferenced 4-byte object at .bss +0x45c. No natural spelling reproduces it; making error_count a static local also lands at 0x45c.
- So a filler-free exact de-aggregation is not available. It would also need new descriptive symbols.json names, which is the owner-held class (rasterizer_xbox .bss names; the 20260915 env_fog/models ruling).

B2 (owner-gated): the rasterizer_profile_check NONE branch passes (profile, message) to "### PROFILE: %s -- tell Bernie!". The %s consumes the sign-extended NONE value, so that rare path crashes.
- It is target-proven and /Od-attested (0x8004c0).
- It is the same class as _hs_parse_set ("shorts passed to %s") in the pending original-bug owner packet (claude_structural_owner_packet_20260923.md section 4), and as the held king, glow, fpw-B and dead_camera items.
- The comment lacked the methodology "BUG (preserved for exact matching)" marker. My patch 08 fixes that at zero bytes.

B3 (owner risk): the three volatile qualifiers are load-bearing (stripped: 12/16) with no first-party attestation possible.
- The rationale is only partial: rasterizer_profile_frame_callback is also an asynchronous D3D callback, yet its arrays are not volatile.
- Brief section 2 lists "volatile/aliasing tricks" as fake matching.

Resolved items (the worker is right):
- SDK tables: authentic topology. D3D8.h D3DCONST tables are unconditional selectany definitions, and January's profile object imports _D3DDevice_InsertCallback@12, so it included D3D8.h. The copies are identical and pair-link in both orders.
- Window extern ownership: 01+02+03, with a blast radius of 0.
- Consumer-local Profile API pairs: 06.
- The '$' data gap.

Minor:
- The (void *) csmemset casts are inert decoration.
- Out of scope: the already-Matching rasterizer_xbox_detail_objects still has a caller-local struct detail_objects_debug_options view and a bss_0045e904 name with a #define alias.

INCIDENTS (reviewer-caused, both repaired and verified):
(a) A broken && chain ran `rm -rf .git` in the worktree root and deleted the worktree pointer file.
- I restored it in the exact sibling format (gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925).
- HEAD is claude/fifty-objects-20260925 @ 954eebd2. The bare-repo admin data was untouched.
- Any parallel worker's git command in this worktree during that window would have failed.
(b) tools/campaign/volatile_scan.py has no argparse, so invoking it with --help ran its hard-coded scan and rewrote source/hs/hs.c with LF line endings.
- I restored the file byte-exact from its own pre-run backup; git diff is clean with 0 tracked modifications.
- The run also overwrote the ignored scratch/hs_scan_backup.c and scratch/volatile_scan2.json.

PRODUCTION CHANGES (zero credit; do NOT flip Matching):
1. Apply these from C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_profile\patches\:
   - 01_shared_header_rasterizer_xbox_internal_window_extern.patch together with 02_rasterizer_c_complete_window_type.patch (inseparable);
   - 03_profile_window_owner_include.patch;
   - optional 04_optional_profile_check_parameter_order_od.patch (/Od-attested);
   - optional 05_optional_internal_consumers_redundant_externs.patch;
   - 06_detail_objects_profile_api_owner_header.patch.
   Then apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_rasterizer_xbox_profile\patches\08_profile_none_branch_bug_comment.patch (NONE-branch BUG marker; object identical; applies alone or after the series).
   - SHARED HEADER 01 consumers, for the integrator's full sweep: source/cache/xbox_texture_cache, source/rasterizer/rasterizer, source/rasterizer/rasterizer_transparent_geometry, source/rasterizer/xbox/rasterizer_xbox, rasterizer_xbox_active_camouflage, rasterizer_xbox_environment, rasterizer_xbox_models, rasterizer_xbox_transparent_geometry, plus rasterizer_xbox_profile through 03.
   - My sweep shows all of them, and detail_objects, section- and symbol-identical.
2. Do NOT apply 07_config_profile_admission.patch: no config.json Matching flip, no rejection removal, no semantic_data_matches entry.
   - Instead apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_rasterizer_xbox_profile\patches\07R_config_profile_rejection_update.patch. The full copy is scratch\w\review3_rasterizer_xbox_profile\config\object_admission_rejections.json, edited in place.
   - 07R rewrites the profile rejection: class source-layout-incomplete, symbol _rasterizer_profile_globals. It records the resolved SDK, window and API items and the live blockers B1-B3.
   - It passes tools.object_admission_policy.rejection_index and git apply --check.
   - Its evidence path is research/fifty_objects_20260925/w/review3_rasterizer_xbox_profile/REVIEW.md, so run curate.py first.
3. Owner questions:
   (a) De-aggregate into loose descriptive statics (the L1 shape, exact except +0x45c), and how to represent the unattested 4-byte object at +0x45c?
   (b) Admit the BUG-marked NONE-branch varargs defect?
   (c) Confirm the three volatile qualifiers?
   The pinned 132-byte .data entry (semantic_data_entry.json, verified) lands only together with the Matching flip.
