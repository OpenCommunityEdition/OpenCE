# source/objects/object_lights

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/lights/cand.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/lights/objects/object_lights_rendering.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/patches/object_lights.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/semantic_data_matches_additions.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/data_gap/fix_lights.txt"
]

## production_changes
(A) Case A credit, +1,541 B. Append the _lights_section entry from scratch/w/data_gap/semantic_data_matches_additions.json to config/semantic_data_matches.json. It is a single-section entry:
  unit source/objects/object_lights
  symbol _lights_section
  allow_incomplete_unit true
  measurements size 1541, relocation_count 1, normalized_sha256 527b43a173f528444fa4423af92d7d58a83f2afe7fc1661b6d297401b2760f71
  plus the reason text in the file.
(B) Optional zero-credit ownership correction, verified:
  - git apply scratch/w/data_gap/patches/object_lights.patch:
    - static on cluster_get_first_light, cluster_get_next_light, light_attenuation and should_render_lights;
    - the should_render_lights prototype moves from object_lights_rendering.h into object_lights.c's prototype block;
    - lights_render_diffuse and lights_render_specular now test `if (should_render_lights())` instead of the inline predicate.
  - Add , "static": true to the symbols.json lines for _should_render_lights (5138), _light_attenuation (5150), _cluster_get_first_light (5152) and _cluster_get_next_light (5153). This is in patches/symbols_json_all.patch.
(C) Keep the status NonMatching.

## evidence
The .data section (1541 B, 1 relocation) resolves identically under coff_compare, with equal flags c0400040.

January's only relocation targets the '?$AA' literal 'render_lights'. January leaves it UNDEF (folded to another object) while we define it. That triggers the objdiff 3.3.1 '$' name defect, and the section scores 99.74%.

The code is 43/43 strict exact. object_audit on the base shows PASS, 71 symbols.

Ownership:
- cachebeta publics lack all four functions, and 0 UNDEF references exist.
- January emits should_render_lights with zero references, so it must be a static that is used inline.
- /Od build:
  - lights_game_globals is at 0xded68c.
  - The only reader of render_lights is should_render_lights at 0x78dab0.
  - Its callers are 0x78c6c0 (lights_render_diffuse, which calls rasterizer_environment_diffuse_lights_begin and then should_render_lights) and 0x78c950 (lights_render_specular).
- Lab results:
  - static without a call leaves should_render_lights UNWRITTEN (42/43).
  - With the /Od call shape it is 43/43 EXACT and all four symbols are emitted static.
- render.c (13/13) and render_objects.c (22/22) are the only other consumers of the header. With the edited shadow header, every function row and every section is unchanged.
- Against a scratch csplit (only this object changes): object_audit PASS, 71/71 symbols, and the _lights_section snapshot is unchanged.

progress_calc: the three Case A entries together add +1,773 data with 0 revoked.

## blockers
The object cannot be marked Matching because of pre-existing surplus. The header-inline _distance3d and _object_get_bounding_sphere COMDATs differ from January's selected action_vehicle copies, and provider_link fails on both (LNK2005 against the NODUP hand copies). A census found 18 base objects emitting this non-identical _distance3d, and none of them is Matching. That is why the data entry needs allow_incomplete_unit.

## reopen_criteria
Drop allow_incomplete_unit and flip to Matching once real_math.h distance3d and object_get_bounding_sphere emit January's selected bytes (the action_vehicle x87 order), or the owner rules on the NODUP providers.

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
