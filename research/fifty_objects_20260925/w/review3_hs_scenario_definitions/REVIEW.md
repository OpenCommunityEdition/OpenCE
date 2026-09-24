# REVIEW3: source/hs/hs_scenario_definitions (claimed OBJECT_COMPLETE_CANDIDATE)

Reviewer slug: `scratch/w/review3_hs_scenario_definitions/` (base 954eebd2, tree clean). Full probe log: `LEDGER.md`.

## Verdict: approve = false (owner-gated naming). Everything technical reproduces.

I re-ran every technical check independently and each one reproduced. Admission still turns on one policy question:
the packet adds **9 static data names to symbols.json that no first-party source attests**. The owner ruled on
2026-09-15 that he "prefers no descriptive/invented static names in symbols.json" (env_fog and models held). This lane's
review2_rasterizer_xbox withheld a whole-object admission on 9 descriptive static names, and the lane doc now lists it
as "held for owner policy ruling". I cannot verify the names, so the default is approve=false. If the owner accepts
convention-derived names, packet A+B+D (or C1+C2+B+D) is ready as delivered. It needs no amendment.

## Independent re-runs

| Check | Result |
|---|---|
| `git apply --check` at 954eebd2 | A, B, C1, C2 and D are each clean. A+B+D together and C1+C2+B+D together are clean. Post-images match deliver/* (modulo CRLF; patches keep the tree's CRLF). B's post-image is byte-identical to the worker's config/symbols.json. All JSON parses. |
| csplit census (own config copies) | Control config: 0 of 833 differ from build/split. Edited config: only `source/hs/hs_scenario_definitions.obj` differs. |
| gate | production vs build/split: EXACT `_hs_syntax_data_byte_swap`. Candidate A vs build/split: UNWRITTEN, because of the rename. **Candidate A vs review split: EXACT 224 `_byte_swap_script_syntax_data` (1/1).** |
| object_audit (split-root) | **PASS**: 22 sections ok, including .data 628/628 align 4/4 (8-byte) under section_infos_equal. 40 January symbols, 0 differ. Surplus is .drectve only. Production fails 1 (.data align 4/3), which is the recorded rejection. |
| objdiff 3.3.1 (frozen, sha1 3130e428) | Production reproduces build/report.json exactly (.data 92.773094, 484/1112). **Candidate: 1112/1112** (.data/.rdata/.text 100). Fields-first control: .data 92.84473 (484/1112). No semantic_data_matches entry is needed. Net data +628. |
| pdb_storage (split-root) | 40 symbols, 0 disagreements. The cachebeta publics in 0x2E49C0..0x2E4C34 are exactly the 9 public data names. The function at 0xBD310 is not public, so it is static. |
| surplus / provider_link | No candidate-only external definition. The 30 external definitions are 21 literals + 9 publics. The undefined-external set equals January's. provider_link: PASS (nothing to link). |
| fake_match_scan | 0 leads (A, C2). |
| .rdata literal order | Candidate order == January's split order. Production reverses 4 block/field pairs. January's leaf_map shows the same block-first pattern, and our admitted leaf_map is reversed. |
| /W3 | A adds 2x C4090. hs.h's `char const *hs_*_names[]` initialises tag_groups.h's `char **names`. HCEX types `hs_type_names` as `const char *[0x31]`, so hs.h is right and the reconstructed tag_groups.h member type causes the mismatch. Byte-inert. **Disclose; not blocking.** |

## Strip test and house rules

- **Block-before-fields definition order.** The unusual construct is the forward tentative
  `static struct tag_field <block>_fields[N];`, then the block, then the fields.
  - The plain fields-first spelling (ctl_fields_first.c) is strict-EXACT and passes the audit. It loses January's
    literal order and objdiff data credit (92.84473).
  - January's own .rdata attests the decorated order. Under the lab law, literals are emitted in reverse creation
    order. Bungie's leaf_map in January corroborates it.
  - It is ordinary C definition order, not decoration. **PASS.**
- **Removed:** `#pragma pack`, 11 `__declspec(align(4))`, the 5 wrapper structs, the `field_table_alignment` padding
  member, and the consumer-local `extern char *hs_*_names[]` (rule 9). No casts, dummies, volatile, asm or pragma
  remain.
- **The function body is unchanged apart from its name and `static`.** There is no new block scope. The /Od build
  contains neither the function nor its strings, so no /Od declaration-order question arises. The static prototype
  is in the TU's prototypes block. Params are one per line, with an explicit `return;`.
- **Section-8 scan:** no caller-local views, no consumer-local prototypes, no hand-expanded helpers, no `float`, no
  invented aggregates. The only remaining invented content is the static names below.

## Name provenance

| Name(s) | Source | Status |
|---|---|---|
| `byte_swap_script_syntax_data` (static) | Bungie's 2001-09-25 cache_exe .map (atlas 6455066..., tier map): `@byte_swap_script_syntax_data@12`, the only record for hs_scenario_definitions.obj. The same map gives our `byte_swap_recording`. Absent from cachebeta publics. | First-party. Accept. |
| 9 public data names | cachebeta publics | First-party. Accept. |
| `hs_{scripts,globals,references,source_files}_block_fields` (4) | HCEX `<block>_fields` convention (10 statics, e.g. map_leaf_face_block_fields; landed leaf_map/bitmap_group). The stem is January's public block name. | Convention-derived, not attested. |
| `{data_array_header,syntax_node}_bs_{codes,definition}` (4) | HCEX `<name>_bs_codes/_bs_definition` convention (~40 pairs). The stem is January's own byte_swap_definition name string. | Convention-derived, not attested. |
| `hs_references_block_reference` (1) | None. The only precedent is public `global_<group>_reference`. This one is a TU-local any-tag `{0, NONE, NULL}`. | **Invented.** |

Sources checked with no hit:
- HCEX has no hs_scenario_definitions compiland and no hs tag-definition data.
- The atlas is code-only (per its METHODOLOGY).
- There is no raw Bungie .map on disk.
- The /Od build has none of the tag strings.
- The research-cache reconstructions have none of the names.

The names cannot be dropped. With A, the function's cross-section relocations name them, so B is required for the
function row to stay EXACT.

**Owner question.** Admit hs_scenario_definitions as Matching with the 9 file-static data names above?
- 8 of them are derived mechanically from January's own names by HCEX-attested Bungie conventions. 1
  (`hs_references_block_reference`) is descriptive.
- The names do not affect layout. All bytes, alignment and literal order are proven, and the change retires the
  invented aggregates, pragmas and padding member that the rejection cites.
- A yes lands A+B+D as delivered (or C1+C2+B+D).

## Zero-credit fallback (not owner-gated), built and verified here

`fallback/F1_hs_scenario_definitions_static_rename.patch` + `fallback/F2_config_symbols_json_rename_only.patch`:
- **Content:** production source with only the first-party function name and static storage, plus symbols.json line
  3249 only.
- **Applies:** `git apply --check` is clean.
- **Split:** only this object changes.
- **Results:**
  - gate EXACT 1/1;
  - pdb_storage goes from 1 disagreement to 0;
  - object_audit is still FAIL(1) on the .data alignment, as recorded;
  - objdiff is unchanged;
  - fake_match_scan 0.
- **Use:** only if the owner declines the names. It replaces A; it does not add to it.

## Optional packet C (shared header): verified

- **C1's enum:** it equals HCEX's anonymous enum (names, values and order, types 1309380-1309386).
- **Includers:** exactly hs.c (bare include at line 2784), hs_compile.c, hs_runtime.c and hs_scenario_definitions.c.
  No header includes it and there is no /FI.
- **Shadow sweep:** every non-debug section is byte+reloc identical orig vs mod (hs 1409, hs_compile 221,
  hs_runtime 171).
- **Symbols:** hs_runtime's 64 internal `$L` label names shift by +8 (a declared-name count effect; same
  values/sections). The worker's "every symbol identical" slightly overstates it.
- **Per-function status:** strict status is identical across base, orig and mod (447/448, 61/64, 64/66). objdiff 3.3.1
  per-function maps are identical.
- **C2:** needs C1 (it does not compile without it). With C1 it is EXACT 1/1, object_audit PASS, objdiff 1112/1112.
- **Integrator:** run the full sweep over hs, hs_compile, hs_runtime and hs_scenario_definitions.

## Environment note for the integrator (not caused by this review)

- **`.git` went missing.** Between about 00:43 and 00:57 PDT the worktree's `.git` link file was missing, so git
  commands failed.
- **hs.c was rewritten.** At 00:54:01, `source/hs/hs.c` was rewritten from CRLF to LF by another agent's scan
  (`scratch/hs_scan_backup.c`, `scratch/volatile_scan2.json`). Its content was unchanged.
- **Both were restored.** `.git` came back at 00:57:07 and hs.c was back to CRLF at 00:57:36. The tree is now clean at
  954eebd2.
- **My measurements were not affected.** None used files touched in that window, apart from the shadow sweep, which
  used the CRLF backup whose content is identical.
