# source/models/models

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\patches\\C1_models.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\patches\\C1_models.h.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\patches\\C1_symbols.json.diff",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\new\\models.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\new\\models.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\patches\\OWNER_OPTION_P1_bss_zero_init.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\new\\models_owner_option_P1.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\hcex_models_compiland.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\hcex_lines_render_model.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\od_700750.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\models_bss\\od_6fd230.txt"
]

## production_changes
C1 is a zero-credit storage/ownership packet. It moves no bytes and does NOT close the .bss blocker. Every file is under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\models_bss\.

(1) patches/C1_models.c.patch (CRLF preserved; git apply --check clean; applying it reproduces new/models.c byte for byte):
- `static struct profile_section render_model_section`.
- A private prototype `static void model_geometry_part_build_tangent_matrices(struct model_geometry_part *part);` after the render_model_parts prototype.
- The static definition.
- The call restored in the inner loop of model_build_tangent_matrices, as `model_geometry_part_build_tangent_matrices(TAG_BLOCK_GET_ELEMENT(&geometry->parts, part_index, struct model_geometry_part));`. This replaces the orphan `part` local, which the /Od build does not have.

(2) patches/C1_models.h.patch. This is a SEPARATE shared-header patch. It removes the public prototype of model_geometry_part_build_tangent_matrices and the forward declaration `struct model_geometry_part;`, which is now unused.
- Consumer TUs to sweep (from grep): source/interface/first_person_weapons.c, source/models/model_animations.c, source/objects/objects.c, source/render/render_objects.c, source/render/render_sky.c and source/models/models.c.
- Lab sweep: a shadow header plus an #error control header. Each of the five other consumers came out IDENTICAL to the same TU compiled with the real header (owner-keyed sections and symbols). Only compiler $L label numbers shift.

(3) config/symbols.json: a full copy with two in-place line edits, never re-serialised. patches/C1_symbols.json.diff shows them:
- line 4771 `{ "file_offset": 1128208, "flags": 32, "name": "_model_geometry_part_build_tangent_matrices", "static": true },`
- line 22672 `{ "file_offset": 3187600, "flags": 0, "name": "_render_model_section", "static": true },`
- csplit emulation (split_C1): of 833 split objects, only models.obj changes.

NOT proposed; owner-gated option: patches/OWNER_OPTION_P1_bss_zero_init.patch, applied on top of C1. It adds `= {0}` to the four file-scope defaults in their current order: fv, change_colors, effect, region. With it, gate is 12/12, object_audit PASSes 30/30 and pdb_storage shows 0 disagreements. See blockers for why it is not natural source.

## evidence
January .bss layout: fv 0, change 16, effect 64, region 104. Base layout: region 0, effect 32, fv 72, change 88 (name-hash order).

First-party evidence:
- E2: HCEX.pdb and HCEX_Release.pdb both record `default_function_values` as a Static Local of render_model, type float[4]. The other 120 bytes of models.obj's 0x88-byte .bss contribution have no symbol record in either PDB (checked -g, -p, -sym, -compiland; no File Static in models.obj).
- E7: HCEX -lines render_model shows each of the four storage defaults with 2 extra lines between the `if` and the assignment (if/assign at 124/128, 134/138, 144/148, 154/158). The centroid default, which needs no storage, has none (164/166).
- E8: the /Od build (0x700750) wraps each default in a profile scope. The scope-end lines are 478/488/498/508/516/526, so storage blocks run 10 lines and the centroid block 8. The later lighting default, which also needs storage, is 10 lines too.
- So in both later builds the defaults are static locals declared inside their if-blocks, with no initialiser.
- The 2001-08-15 and 2001-09-25 linker maps list only static functions, not static data. cachebeta publics has none of the four.
- The three halocea names (default_render_model_*) are third-party and not in any first-party source.

VC7 probes (lab_*, V1-V5, h2_*, table in LEDGER.md):
- Uninitialised statics, file-scope or static-local (decorated `?n@?N@??render_model@@9@9`), are laid out in decorated-name hash order. Oracle renames a/b/c/d prove the order depends on the names.
- `= {0}` statics are laid out in declaration order after all uninitialised ones.
- In-block `= {0}` gives region, effect, change, fv, the exact REVERSE of January.
- Pre-registered H2 (names `default_` + HCEX parameter name, one name set) matches in none of its four forms.
- The only name-independent reproduction is `= {0}` in declaration order fv, change, effect, region (P1 file scope, or P2 with fv as an uninitialised static local).

C1 evidence:
- render_model_section is absent from cachebeta publics and from all three maps.
- The tangent helper is in the Static symbols section of all three 2001 maps, and HCEX_Release lists it as static and wasinlined.
- January emits the 16-byte helper with zero references. By the static-COMDAT law, the source defined it and called it.
- /Od 0x6fd230 calls the empty 0x6fd530 with the element fetch as a nested argument. Its frame is geometry_index/geometry/part_index, with no `part` local.
- With the call restored, model_build_tangent_matrices is deferred to the end of .text, which is January's order (0x114150 is last). Base emitted it earlier.

C1 results:
- Gate vs split_C1: 12/12 EXACT, _point_from_line3d guard passes.
- object_audit: only the 4 .bss offset DIFFs remain.
- pdb_storage: 0 disagreements, down from 2.
- surplus_identity: _negate_vector3d and _set_real_point3d IDENTICAL.
- provider_link: PASS on all 11 rows.
- fake_match_scan: 0 leads.
- Header sweep: IDENTICAL.

## blockers
The three non-fv defaults have no first-party names.

Both later first-party builds agree on the authentic form: static locals inside each `if (!param)` block, with no initialiser. In that form VC7 lays out by decorated-name hash, so January's offsets depend on names that are lost.

With zero initialisers, that same form gives the reverse of January's order.

P1 does reproduce January, but it needs file-scope `= {0}` statics declared in reverse use order. That contradicts the in-block form and HCEX's static-local storage for fv. It is a matching-only construct under the rejection's reopen clause, so it is held for an owner ruling.

Choosing names to steer the hash is forbidden.

The salvage ledger's statement that HaloCEA "independently identifies" the four names is inaccurate: only default_function_values is in the HCEX PDBs.

## reopen_criteria
(1) Recover first-party names for the region-permutation, model-effect and change-colour defaults from any symbol source with static-data or block-scope records for models.c. Then write all four as in-block UNINITIALISED static locals (the HCEX//Od form, fv = `static real default_function_values[MAXIMUM_FUNCTION_VALUES_PER_MODEL];` inside `if (!function_values)`), rename the symbols.json rows to `?<name>@?N@??render_model@@9@9` (our block ordinals: region ?7, effect ?8, change ?9, fv ?L@), emulate csplit and check the offsets. OR (2) the owner admits the P1 file-scope `= {0}` form; the patch is ready and audits PASS on top of C1. Do not repeat: the 24 uninitialised declaration permutations; P1, P2, L1, L2, V1-V5 and the H2 forms; any hash-steering renames.

## task notes
Tools written for this task, all in scratch/w/models_bss/:
- bssdump.py
- mkvar.py, mkvar2.py, mkvar3.py (mkvar3 is oracle-only)
- probe*.sh
- objeq2.py (owner-keyed object equality)
- mk_config.py (in-place symbols.json edits)
- lab_gate.py, audit2.py and pdb_storage_cand.py, copied from rasterizer_xbox_audit

split_C1/ holds the emulated split. No tracked file was edited.

Side findings for the integrator:
- **Held precedent.** The rasterizer_xbox item held for an owner ruling has the same shape: descriptive names plus `= {0}` statics.
- **Reuse for other statics.** The +2-line test worked here: counting line gaps in HCEX -lines and in the /Od profile-scope assert lines shows where a static is declared. It can be reused for other units whose statics lack names.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\models_bss\LEDGER.md
