# rasterizer_xbox_environment_fog object: SDK D3D wrapper symbol storage (split metadata)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\cfg\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\split\\source\\rasterizer\\xbox\\rasterizer_xbox_environment_fog.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\object_audit_split.py"
]

## production_changes
In config/symbols.json, edit these 7 lines IN PLACE, without re-serialising: append `, "static": true` before the closing brace on lines 5766, 5767, 5769, 5774, 5779, 5780 and 5781. By file_offset: 1396688 _D3DDevice_SetRenderState, 1397120 _D3DDevice_SetTextureStageState, 1397232 _IDirect3DDevice8_SetRenderState@12, 1397920 _IDirect3DDevice8_SetTextureStageState@16, 1398464 _IDirect3DDevice8_SetVertexData2s@16, 1398480 _IDirect3DDevice8_Begin@8, 1398496 _IDirect3DDevice8_End@4. Then regenerate the split (csplit only). The exact edited copy is scratch/w/rasterizer_xbox_environment_fog/cfg/symbols.json.

## evidence
object_audit on both the production base and c8 flags these 7 January symbols with storage split 2 vs ours 3. pdb_storage.py shows none of them is a cachebeta.pdb public, so file-static is January's truth. The same entries in other objects already carry "static": true (for example lines 5591-5599 and 5830-5840), and precedent ledger rasterizer_xbox_sdk_names_and_private_reciprocal_20260906 covers the same fix. A scratch csplit (build/tools/csplit.exe -i cachebeta.exe -p cfg -o split) changed only the fog split object out of 833, and all 58 of its sections are section_infos_equal to build/split. With that split, object_audit_split.py reports 78/78 January symbols matching for both the production base and c8, leaving screen_begin as the only failure. Gate rows are unaffected because section bytes and relocations are identical.

## blockers
On its own this does not complete the object. screen_begin is still residual (see the first unit), and the _real_local_random surplus COMDAT still fails the selected-provider link against effects.obj (NODUP). That second item needs an owner decision under the NODUP provider link law.

## reopen_criteria
Apply it together with any future screen_begin closure. If the integrator's full build shows any objdiff or progress regression from the storage change, revert it and record the result.

## task notes
Tools written in the slug:
- probe.py (inline-budget probe) and costlab.py (helper-cost calibration).
- metric.py (slot-normalised distance) and lay.py (/FAsc named layout).
- ft.sh plus p2/ (live C2 frame table, retargeted to this worktree).
- xf.py (source transforms), object_audit_split.py (audit against a scratch split), and cfg/ + split/ (the scratch csplit).

I disassembled C2.Dll as data only, to read the slot-merge rule; nothing was patched. No tracked file was edited, and no ninja, configure or git-mutating command was run. The temporary git init used for patch-apply testing was inside a scratch subdirectory and has been deleted.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_environment_fog\LEDGER.md. Every probe is logged there (P0-P22 and FINAL): hypothesis, the January-byte evidence, the change and the measured result. Main result: the "alpha inline" owner blocker (ruling A) was a .c-local defect. The early return on the near==far density path cost one inline-budget step. Rewriting it as a single-exit if/else gives the same bytes on that path and lets real_alpha_to_pixel32 inline exactly as January does. Four more January-attested fixes landed in the admissible candidate:
- the flags word local;
- the decoded clear_z expression;
- the colour-arm pointer and time local;
- the texture transforms built by this TU's set_real_vector4d.

Also the first-time reseed order and the rotation sign form. Admissible candidate c8: the only relocation difference from January is +5 csmemcpy (ruling D), and slot-normalised unmatched instructions drop from 277 to 105. The diagnostic that adds rulings D and E and the set_real_point2d change reaches [sha]-only at size 3904==3904. Also proposed: an admission fix in symbols.json (7 SDK wrappers marked static), verified by a scratch csplit.
