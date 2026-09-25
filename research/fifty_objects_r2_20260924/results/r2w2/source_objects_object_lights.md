# source/objects/object_lights (admit source/objects/object_lights)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\patches\\01_object_lights_static_helpers.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\patches\\02_object_lights_symbols_json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\patches\\03_NO_ALT_object_lights_semantic_data_entry_incomplete.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\patches\\APPLY_ORDER.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\cfg_OL\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\tree\\source\\objects\\object_lights.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_object_lights\\tree\\source\\objects\\object_lights_rendering.h"
]

## production_changes
Three LF git patches, regenerated at HEAD with index lines, in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_object_lights\patches\ (APPLY_ORDER.txt is in the same folder). Their hunk bodies are identical to the round-1 patches.
(01) object_lights.c + object_lights_rendering.h (index 2553c63c..9e77ec23, c863833f..527cc0b3):
- makes should_render_lights, light_attenuation, cluster_get_first_light and cluster_get_next_light static;
- moves the should_render_lights prototype from the public header into the .c prototype block;
- lights_render_diffuse and lights_render_specular now test `if (should_render_lights())`, the call attested in the later /Od build, instead of the hand-expanded predicate.
(02) config/symbols.json (f9abb17d..e6ea96b2): 4 in-place `"static": true` rows at file_offset 1214992, 1216576, 1216768 and 1216800. It must land together with 01, followed by a csplit regen. A full edited copy is at cfg_OL/config/symbols.json.
(03_NO_ALT) config/semantic_data_matches.json (77814b3d..3f1a813e): appends the `_lights_section` single-section entry with allow_incomplete_unit (size 1541, 1 relocation, sha 527b43a1...). Land it after 01+02.
There is no config.json status flip: object_lights stays NonMatching.

## evidence
Base checks:
- git apply --check is clean for each patch alone and for all three in one call.
- Applying the patches one at a time reproduces the measured post-images byte for byte.
- Control split (verbatim config) == build/split, 833/833 objects.
- The emulated split with 02 changes only object_lights.obj.

Gate and object audit:
- Shadow-tree gate against the emulated split: 43/43 EXACT, 12,608 padded bytes.
- Controls: production vs build/split 43/43; candidate vs build/split 43/43; gate.py --source 43/43. The gate.py object is identical to the shadow-tree object.
- objcmp candidate vs production: every section identical; only the 4 storage rows differ.
- object_audit against the emulated split: PASS, 71 January symbols, 0 differ. Controls FAIL(4) either way: production vs emulated split, and candidate vs build/split. This is why 01 and 02 must land together.

Storage and surplus:
- pdb_storage: 4 disagreements in production, 0 in the candidate.
- None of the four helpers is a cachebeta public. No other January or base object references them.
- HCEX.pdb marks cluster_get_first_light and cluster_get_next_light as static.
- surplus_identity: 12 candidate-only code COMDATs. 11 are identical, including _distance3d, which matches action_vehicle. The one different COMDAT is _object_get_bounding_sphere, the held Q1 item, unchanged from production.
- provider_link: PASS in both orders for 33 surplus names; no new surplus vs production.

The should_render_lights() calls:
- In the later /Od build, 0x78dab0 is called only by 0x78c6c0 (diffuse) and 0x78c950 (specular), in our statement order.
- January defines the non-public 32 B _should_render_lights with no referrer anywhere.
- Lab strip test (static helper without the calls): 42/43 with _should_render_lights UNWRITTEN. So the calls are needed for January's section to be emitted.

Header consumers: render 13/13 and render_objects 22/22, both identical to build/base.

Data:
- _lights_section 1541/1541, flags c0400040. Relocations resolve to the same image destination (resolved infos equal).
- build/report.json shows .data at 99.74% and unmatched data 1541, equal to the entry size.
- Mini objdiff 3.3.1 (sha1 3130e428) plus apply_semantic_data_matches: +1541, data 2645/2645.
- Negative control without the opt-in fails closed.

Other scans: fake_match_scan 0 leads. /W3 and /W4 warnings are the same as production (the existing C4133 only).

MANIFEST.md has a precedent section quoting 05255584 and the leaf_map entry, with every condition shown holding. P1 conditions: absent from publics, no outside reference, in-place static rows, csplit regen changes only this unit, all functions exact and bytes unchanged, audit symbol rows match, the object may stay blocked. P2 conditions: single section, pinned measurements, identical payload, flags and owner, relocations resolved equal, sole unmatched data, all functions exact, held unit with an explicit opt-in.

Disclosed side effect: tools/audit_object_admission.py goes from 9/0/2/0 to 10/0/2/0, because object_lights joins the review queue. leaf_map's rejection entry in ae12a3c1 existed only because that commit downgraded leaf_map from Matching. Later allow_incomplete_unit entries on NonMatching units (objects 84414a1a, editor_flying_camera 5019c186) have no rejection entry, so none is proposed here: adding one would pre-empt the owner question Q1.

## blockers
None for this packet. The whole object stays held on owner question Q1: object_lights' surplus _object_get_bounding_sphere COMDAT differs from action_vehicle's selected copy only in its two __FILE__ DIR32 targets (the c:\halo\source\objects\objects.h literal).

## reopen_criteria
If the owner answers YES on Q1, replace 03_NO_ALT with round-1 03_YES (no allow_incomplete_unit) and add 04_YES (config.json Matching): +1 object. Otherwise reopen whole-object completion only with first-party evidence that closes the rule-6 identity gap for the per-TU __FILE__ bounding-sphere copy.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_object_lights\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_object_lights\LEDGER.md
