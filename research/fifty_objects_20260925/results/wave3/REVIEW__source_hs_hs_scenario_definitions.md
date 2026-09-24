# REVIEW source/hs/hs_scenario_definitions

approve: False

## per_object
[
 {
  "unit": "source/hs/hs_scenario_definitions",
  "approve": false,
  "issues": "Technically complete: gate EXACT 1/1, object_audit PASS (40/0; .data align 4/4), pdb_storage 0 disagreements, objdiff 3.3.1 1112/1112 with no semantic entry, csplit changes only this object, fake_match_scan 0. Held for an owner ruling on 9 static data names in symbols.json that no first-party source attests. 8 are convention-derived from January's own names (<block>_fields, <name>_bs_codes/_bs_definition). hs_references_block_reference is invented. If the owner approves, land A+B+D (or C1+C2+B+D) as delivered. If not, land the zero-credit fallback F1+F2 from review3_hs_scenario_definitions/fallback/: the first-party static rename only, EXACT, which closes the PDB storage disagreement."
 }
]

## checks
All checks were run independently in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_hs_scenario_definitions\ (see LEDGER.md and REVIEW.md).
(1) git apply --check at 954eebd2. A, B, C1, C2 and D are each clean. A+B+D together and C1+C2+B+D together are clean. The post-images equal deliver/* apart from CRLF, and the patches keep the tree's CRLF. B's post-image is byte-identical to the worker's config/symbols.json. All three JSON files parse. The 9 inserted offsets are 0x2E49C0 + {18, 90, 110, 11c, 17c, 1cc, 204, 218, 240}, none of them already present.
(2) csplit with my own config copies. The control config reproduces build/split (0 of 833 differ). The edited config changes only source/hs/hs_scenario_definitions.obj.
(3) gate:
- production vs build/split: EXACT _hs_syntax_data_byte_swap.
- candidate A vs build/split: UNWRITTEN, because of the rename. Source and symbols.json must land together.
- candidate A vs the review split: EXACT 224 _byte_swap_script_syntax_data, 1/1.
(4) object_audit (split-root copy). Candidate A: PASS. All 22 sections are ok, including .data 628/628 with align 4/4 under section_infos_equal. 40 January symbols, 0 differ. Surplus is .drectve only. Production fails 1 (.data align 4/3), which is the recorded rejection.
(5) objdiff 3.3.1 mini-projects (frozen, sha1 3130e428):
- production reproduces build/report.json exactly (.data 92.773094, 484/1112);
- candidate A: 1112/1112, with .data, .rdata and .text all at 100%;
- the fields-first control ctl_fields_first.c: .data 92.84473.
(6) Section order: candidate A's .rdata literal order equals January's split order, while production reverses 4 block/field pairs. January's leaf_map shows the same block-created-first pattern.
(7) pdb_storage (split-root): 40 symbols, 0 disagreements. The cachebeta publics in 0x2E49C0..0x2E4C34 are exactly the 9 public data names. Nothing is public at 0xBD310.
(8) provider_link: PASS (nothing to link). There is no candidate-only external definition, and the undefined-external set equals January's.
(9) fake_match_scan: 0 leads on A and C2.
(10) /W3 /Zs: A adds 2x C4090. hs.h's `char const *[]` meets tag_groups.h's reconstructed `char **names`, and HCEX types hs_type_names as `const char *[0x31]`. Byte-inert.
(11) Strip test: the plain fields-first spelling is strict-EXACT but loses January's literal order and the objdiff data credit. The forward-declared, block-first order is attested by January's own .rdata, so it passes.
(12) The function body is unchanged apart from name and storage, with no new block scope. The /Od build has neither the function nor its strings (odbuild str: no hit).
(13) Name provenance:
- the function name is in the atlas' 2001-09-25 Bungie .map (tier map), `@byte_swap_script_syntax_data@12`, the only record for this object;
- HCEX has no compiland for this object and no hs tag data;
- the atlas holds code symbols only;
- there is no raw .map file on disk;
- the /Od build and the research-cache reconstructions contain none of the 9 static names.
(14) C packet:
- C1's enum equals HCEX types 1309380-1309386.
- The includers are exactly hs.c, hs_compile.c, hs_runtime.c and hs_scenario_definitions.c.
- A shadow sweep, orig vs mod, gives byte- and relocation-identical non-debug sections: hs 1409, hs_compile 221, hs_runtime 171.
- Per-function strict status is equal for base, orig and mod (447/448, 61/64, 64/66), and objdiff per-function results are identical.
- C2 with C1: EXACT 1/1, object_audit PASS, 1112/1112.
(15) I built and verified a zero-credit fallback, F1+F2 (see issues).

## issues
APPROVE=FALSE. Every technical claim reproduced. The only thing I cannot verify is the names, and that is an owner-policy question.

BLOCKER (owner-gated): packet B adds 9 file-static data names to symbols.json, and no first-party source attests any of them.
- 4 `<block>_fields` and 4 `<name>_bs_codes` / `<name>_bs_definition` follow Bungie naming conventions that HCEX attests, applied to stems that are January's own names.
- `hs_references_block_reference` has no precedent and is invented.
The owner ruled on 2026-09-15 that he prefers no descriptive or invented static names in symbols.json, and held env_fog and models on that basis. review2_rasterizer_xbox withheld a whole-object admission over 9 descriptive static names, and it is held for an owner ruling. The counter-precedent is rasterizer_xbox_shadows, admitted with 2 disclosed descriptive gap statics.
The names cannot be dropped: with A, the function's relocations name them, so B is required to keep its row EXACT.

OWNER QUESTION: admit the object as Matching with these 9 names? If yes, land A+B+D, or C1+C2+B+D, exactly as delivered. No amendment is needed.

FALLBACK if the owner declines (zero credit, not owner-gated, verified by me). Apply both together, INSTEAD of A:
- C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_hs_scenario_definitions\fallback\F1_hs_scenario_definitions_static_rename.patch (full file fallback\hs_scenario_definitions.c)
- C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review3_hs_scenario_definitions\fallback\F2_config_symbols_json_rename_only.patch
F1+F2 contain only the first-party `static byte_swap_script_syntax_data` rename (symbols.json line 3249 only, as in B).
- git apply --check is clean, and csplit changes only this object.
- Gate EXACT 1/1. pdb_storage drops from 1 disagreement to 0.
- The .data alignment rejection stays in force, and objdiff is unchanged.

NON-BLOCKING NOTES:
(a) A adds two byte-inert C4090 warnings. hs.h is correct per HCEX; the mismatch is the reconstructed `char **names` in tag_groups.h. Disclose it.
(b) Optional C1 is a shared-header edit. Consumers to sweep: source/hs/hs.c, hs_compile.c, hs_runtime.c, hs_scenario_definitions.c. It is byte-inert, but hs_runtime's 64 internal $L label names shift by +8 (a name-count effect), so the worker's claim that every symbol is identical is slightly overstated. C2 depends on C1.
(c) Side lead: our admitted leaf_map reverses January's literal order (fields defined before their block).
(d) Environment: another agent's scan left the shared worktree's .git link missing between about 00:43 and 00:57 PDT, and rewrote source/hs/hs.c from CRLF to LF at 00:54 (backup at scratch/hs_scan_backup.c). Both were restored by 00:57, and the tree is clean at 954eebd2. None of my measurements used an affected file.
