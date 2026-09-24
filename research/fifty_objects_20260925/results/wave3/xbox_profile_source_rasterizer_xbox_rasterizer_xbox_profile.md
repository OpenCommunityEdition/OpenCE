# source/rasterizer/xbox/rasterizer_xbox_profile

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\01_shared_header_rasterizer_xbox_internal_window_extern.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\02_rasterizer_c_complete_window_type.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\03_profile_window_owner_include.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\04_optional_profile_check_parameter_order_od.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\05_optional_internal_consumers_redundant_externs.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\06_detail_objects_profile_api_owner_header.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\patches\\07_config_profile_admission.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\candidate_files\\source\\rasterizer\\xbox\\rasterizer_xbox_profile.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\candidate_files\\source\\rasterizer\\xbox\\rasterizer_xbox_internal.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\candidate_files\\source\\rasterizer\\rasterizer.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\candidate_files\\source\\rasterizer\\xbox\\rasterizer_xbox_detail_objects.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\semantic_data_entry.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\consumers_rasterizer_xbox_internal_h.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_profile\\final_profile.obj"
]

## production_changes
All files are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_profile\. The patches are in patches/ and apply in numeric order. `git apply --check` passes for the whole 01..07 series on the real tree at 954eebd2, and for 01, 02, 03, 05, 06 and 07 applied alone; 04 needs 03. Full post-images are in candidate_files/.

01_shared_header_rasterizer_xbox_internal_window_extern.patch (SHARED HEADER, separate patch). Adds `extern struct rasterizer_window_begin_parameters global_window_parameters;` before #endif in source/rasterizer/xbox/rasterizer_xbox_internal.h, which is the backend header owned by rasterizer_xbox.c.
- Consumers (CL /showIncludes closure, identical to git grep): source/cache/xbox_texture_cache, source/rasterizer/rasterizer, source/rasterizer/rasterizer_transparent_geometry, source/rasterizer/xbox/rasterizer_xbox, rasterizer_xbox_active_camouflage, rasterizer_xbox_environment, rasterizer_xbox_models, rasterizer_xbox_transparent_geometry. rasterizer_xbox_profile becomes a consumer through 03. The list is in consumers_rasterizer_xbox_internal_h.txt.

02_rasterizer_c_complete_window_type.patch (REQUIRED WITH 01: applied alone, 01 gives C2371 in rasterizer.c). Removes rasterizer.c's partial `struct rasterizer_window_parameters` and its local extern. The 6 accesses become `camera.forward` / `camera.position`, whose offsets 0x8 and 0x14 were checked with an offsetof probe.

03_profile_window_owner_include.patch. In rasterizer_xbox_profile.c:
- add `#include "rasterizer_xbox_internal.h"` after `#include "rasterizer_xbox.h"`;
- delete the consumer-local `extern struct rasterizer_window_begin_parameters global_window_parameters;`;
- update the stale symbol-listing comment: `_bss_00465e28` becomes the 8 current symbols.json .bss owners and interior offsets, and the .data listing gains frequency (0000) and globals (0008).

04_optional_profile_check_parameter_order_od.patch (optional, zero bytes). Changes rasterizer_profile_check's parameter order to (condition, profile, message), as in /Od 0x8004c0. This touches the prototype, the definition and 9 call sites.

05_optional_internal_consumers_redundant_externs.patch (optional, zero bytes). Drops the now-redundant compatible local externs in rasterizer_transparent_geometry.c, xbox_active_camouflage.c, xbox_environment.c, xbox_models.c and xbox_transparent_geometry.c.

06_detail_objects_profile_api_owner_header.patch. This is the last of the three consumer-local Profile API pairs; transparent_geometry and models were already migrated. rasterizer_xbox_detail_objects.c now includes rasterizer/xbox/rasterizer_xbox.h and drops the 6 prototypes and 2 externs that header owns (rasterizer_error, rasterizer_profile_end/begin, set_pixel_shader, set_texture, set_vertex_shader_permutation, global_d3d_device, pixel_shader). The local `void rasterizer_set_texture` had the wrong return type; the owner header returns `union point2d *`. The two literal `21` arguments become `_rasterizer_profile_detail_objects`.

07_config_profile_admission.patch. These three edits must land together:
- config/config.json line 487: rasterizer_xbox_profile.c changes from NonMatching to Matching;
- config/object_admission_rejections.json: the profile `_D3DSIMPLERENDERSTATEENCODE` entry is removed;
- config/semantic_data_matches.json: a pinned single-section entry is appended (no allow_incomplete_unit). Unit source/rasterizer/xbox/rasterizer_xbox_profile, symbol _rasterizer_profile_performance_counter_frequency, measurements size 132 / relocation_count 29 / normalized_sha256 b577924a924477a95be3c0a539812cce4cae4eaf93489d580a909a02169d0cf9. The text is also in semantic_data_entry.json.

config/symbols.json: no change. All 8 .bss owners and 2 .data owners are already named and flagged static, with 0 cachebeta disagreements.

## evidence
All ledger files are under scratch/w/rasterizer_xbox_profile/.

1. SDK tables. surplus_all.py compares every candidate-only external definition with every January split definer (raw bytes, flags and section_infos_equal): 10 rows, 0 problems.
- D3DTEXTUREDIRECTENCODE (16) matches headers, D3DSIMPLERENDERSTATEENCODE (328) matches progress_bar, D3DPRIMITIVETOVERTEXCOUNT (88) matches rasterizer_xbox_decals.
- The 7 literals match game_globals, ai_debug, network_connection, rasterizer_xbox, actor_moving, action_charge and actor_firing_position.
- provider_link.py: 10/10 PASS in both orders, including all 3 tables against January's selected providers.
- group_link.py: this object plus all 132 other build/base definers in one link, candidate first and last: 0 LNK2005/LNK1169 naming a table.
- d3d_census.txt: 61 Matching objects already carry all 3 tables as SELECT_ANY surplus (collision_debug, units, files_windows, event_manager and others).
- split_sel_census.txt: csplit writes selection 1 for every one of the split's 176 non-literal .rdata COMDATs, so January's sel=1 is a splitter class encoding, not provenance. D3D8.h:319-325 defines D3DCONST as `extern CONST DECLSPEC_SELECTANY`.

2. Data gap. objdiff 3.3.1 scores .data 132 B at 68.686874%, reproduced in my own one-unit projects in both the complete and incomplete states. coff_compare section_info_resolved on both objects: equal, 29 relocations resolving to image addresses, flags c0400040, owners static. Proof that it is the '$' defect under the frozen scorer: dollar_rename.py replaces only '$' in the ??_C@ names of both objects, and .data goes from 68.69 to 100.0 (2234/2234). objdiff 3.6.0 on the same objects also scores 100 (diagnostic only). Running tools.semantic_progress on the objdiff report with complete=true and only the new entry: +132 credited, not revoked, 2511/2511 code, 2234/2234 data, 16/16 functions. With complete=false the verifier refuses the entry, as intended.

3. Window ownership. January's `_global_window_parameters` is a COMMON record in linker_common's rasterizer_xbox.c cluster (section 65, between pixel_shader and global_frame_parameters). HCEX gives the type rasterizer_window_begin_parameters with size 0x258, which matches our sizeof. Shadow-tree A/B sweeps with shadow_build.py compiled all 466 Halo units; the pristine shadow reproduces build/base with 0 section/symbol diffs.
- Owner rasterizer.h (next to global_frame_parameters, with 6 partial-view TUs converted): regresses rasterizer_frame_statistics `_rasterizer_frame_statistics_draw` (4176, exact at base), the known name-count canary. rasterizer_xbox.h is also included by frame_statistics, so both headers are rejected as landing sites.
- Owner rasterizer_xbox_internal.h, alone and composed with 03-06: 466/466 units section- and symbol-identical, 0 regressions.
- The final profile object: gate-equivalent object_audit PASS (59 January sections, 67 symbols, 0 differ); pdb_storage 0 disagreements; fake_match_scan 0 leads; /W3 adds 0 warnings.

4. detail_objects. P4a, P4c (narrow rasterizer_xbox_state.h) and P4e (enum only) all gate 20/20 exact and are section-identical to production.

5. /Od evidence (later PC dx9 build, rasterizer_dx9_profile.c). Check is (condition, profile, message) with the same NONE-branch extra-argument bug. P5 keeps 16/16 with identical sections.

6. volatile. Stripping any of the 3 volatile qualifiers breaks 2-4 functions. Adding volatile to the frame-callback arrays breaks frame_end or frame_callback. So January's volatile set is exactly the state shared with the per-profile D3D callback. The source is unchanged; this is disclosed for the reviewer.

## blockers
None at byte level. Reviewer and owner judgements needed:

(a) Reopen criterion. The rejection's literal criterion, a successful ordinary canonical link with the real dependency and import closure, is NOT met and cannot be met in this repo. There are no import libraries, and the full split union leaves 171 unresolved externals (20260906 ledger). No Matching object has such a link. The SDK-table item meets exactly the lane's admitted standard (collision_debug in b9a8d587, units, event_manager, and 61 Matching objects carrying the same tables): byte identity with January's selected copies plus clean pair links in both orders, now also a 133-object group link. Retiring the rejection needs the integrator or owner to accept that standard for this object.

(b) Owner-header choice is disclosed. rasterizer.h is the other genuine candidate (type home, next to global_frame_parameters) but costs `_rasterizer_frame_statistics_draw` through the declared-name count. rasterizer_xbox_internal.h is the header of the January COMMON owner rasterizer_xbox.c and already declares its window API.

(c) Previously reviewed and unchanged: the load-bearing volatile qualifiers (semantic, callback-shared); the TU-private struct pads and the opaque rasterizer_profile_state.reserved06[6] (no first-party evidence for January's unreferenced .bss +1114..+1119); the narrow cseries/profile_rasterizer.h ABI header; the four descriptive private function names.

(d) Sweeps were measured on 954eebd2 alone. Isolated header tests do not compose, so after applying, re-sweep on the live tree: full ninja, the stable 8,245-function diff, parks, admission audit. Watch the units and frame_statistics sentinels.

## reopen_criteria
If 01 regresses anything on the live tree, try the one other defensible owner, rasterizer.h. That needs the 6 partial-view conversions in apply_window_owner.py (keys header, debug, lights, decals, dynavobgeom, widgets), and it only becomes viable once the frame_statistics declared-name gap is recovered from genuine evidence. Revisit the reserved06 span or the volatile set only with new first-party evidence: an Xbox-era profile compiland, or January-contemporary source.

## task notes
Tools written in the slug:
- shadow_build.py and ninja_units.py: a shadow-tree harness that compiles and compares all 466 Halo units by section and symbol. It fixes the wrapped `build $` lines that a naive build.ninja parse misses.
- include_closure.py: CL /Zs /showIncludes consumer closure.
- dollar_rename.py: the black-box '$'-defect proof.
- emulate_strict.py: runs the real semantic_progress verifier on a one-unit objdiff 3.3.1 report.
- group_link.py and build_patches.py: patches are generated from edit specs and validated with `git apply --check` in a throwaway repo, and against the real tree with --check only.

The shadow trees and object dirs were deleted after measurement (regenerable), which keeps the slug at 1.9 MB for curate.py.

Unrelated pre-existing link issue seen in passing: `bipeds.obj : error LNK2005: _object_get_type already defined in items.obj`, a NODUP/provider conflict between two other objects.

No tracked file was edited; I did not run ninja, configure or any git-mutating command.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_profile\LEDGER.md
