# source/hs/hs_scenario_definitions

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\hs_scenario_definitions.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\A_hs_scenario_definitions_c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\B_config_symbols_json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\D_admission_status.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\C1_header_scenario_limits.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\C2_hs_scenario_definitions_c_with_limits.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\hs_scenario_definitions.with_limits.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\deliver\\hs_scenario_definitions.with_limits.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hs_scenario_definitions\\LEDGER.md"
]

## production_changes
All artifacts are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hs_scenario_definitions\deliver\. Every patch passes `git apply --check` against 954eebd2. A+B+D apply cleanly together, and so do C1+C2+B+D.

REQUIRED (source and symbols.json must land together):
(A) A_hs_scenario_definitions_c.patch, full file deliver/hs_scenario_definitions.c. It removes `#pragma pack`, every `__declspec(align(4))`, the five wrapper structs and the `long field_table_alignment` member, and writes 18 separate January objects instead:
- public `hs_script_types_enum` and `hs_types_enum`, counted with NUMBER_OF_HS_SCRIPT_TYPES / NUMBER_OF_HS_TYPES;
- for each of the 4 blocks: a forward tentative definition `static struct tag_field <block>_fields[N];`, then the public block definition, then the static fields definition;
- `static struct tag_reference_definition hs_references_block_reference = {0, NONE, NULL}`;
- static `data_array_header_bs_codes` / `data_array_header_bs_definition` and `syntax_node_bs_codes` / `syntax_node_bs_definition`;
- public `hs_source_data_definition`, `hs_syntax_data_definition` and `hs_string_data_definition`.
`#include "hs/hs.h"` replaces the consumer-local `extern char *hs_script_type_names[]` / `hs_type_names[]` declarations. The function is renamed to the static `byte_swap_script_syntax_data`, and its prototype sits in the TU's prototypes block.
(B) B_config_symbols_json.patch edits lines in place. The full edited copy is scratch/w/hs_scenario_definitions/config/symbols.json (CRLF, never re-serialised).
- Line 3249 `_hs_syntax_data_byte_swap` becomes `"_byte_swap_script_syntax_data", "static": true`.
- It inserts 9 static rows in ascending order: 3033560 _hs_scripts_block_fields, 3033680 _hs_globals_block_fields, 3033808 _hs_references_block_reference, 3033820 _hs_references_block_fields, 3033916 _hs_source_files_block_fields, 3033996 _data_array_header_bs_codes, 3034052 _data_array_header_bs_definition, 3034072 _syntax_node_bs_codes, 3034112 _syntax_node_bs_definition.
- A csplit regen then changes only build/split/source/hs/hs_scenario_definitions.obj (census of 833 objects).
(D) D_admission_status.patch sets config.json source/hs/hs_scenario_definitions.c to "Matching" and removes the hs_scenario_definitions entry from config/object_admission_rejections.json.

OPTIONAL shared-header packet (use C2 INSTEAD of A):
(C1) C1_header_scenario_limits.patch adds HCEX's anonymous enum MAXIMUM_HS_{SCRIPTS,GLOBALS,REFERENCES,SYNTAX_NODES,STRING_DATA}_PER_SCENARIO / MAXIMUM_HS_SOURCE_FILES_PER_SCENARIO / MAXIMUM_HS_SOURCE_DATA_PER_FILE to hs_scenario_definitions.h. It also removes hs.c's TU-local `#define MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO 19001`. Consumer TUs to sweep (git grep): source/hs/hs.c, source/hs/hs_compile.c, source/hs/hs_runtime.c, source/hs/hs_scenario_definitions.c.
(C2) C2_hs_scenario_definitions_c_with_limits.patch (full file deliver/hs_scenario_definitions.with_limits.c) is A with those constants. The syntax-data maximum is spelled `sizeof(struct data_array)+MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO*sizeof(struct hs_syntax_node)`, as hs.c spells it.

## evidence
Full record: scratch/w/hs_scenario_definitions/LEDGER.md.

(1) January .data decode. The relocation addends show 18 objects. The only ones of 64 bytes or more are the two field arrays: 72 B at +0x18 and 84 B at +0x90. The 4-byte zero gap at 0x8c..0x90 is VC7's natural 8-byte alignment of the 84-byte array, not a member. The 56-byte codes array sits only 4-aligned at 0x1cc, which is consistent with the alignment law.

(2) Name of the static byte-swap function. The halo-symbol-atlas 2001-09-25 cache_exe map tier (Bungie's own .map) lists `@byte_swap_script_syntax_data@12` as the only function of hs_scenario_definitions.obj: the same arity, and the same map that gives our byte_swap_recording. cachebeta publics lack both it and the 9 new statics, and list exactly the 9 public data names.

(3) Definition order, from a VC7 lab (lab/l1-l5):
- String literals are emitted in reverse creation order.
- .data objects are placed in first-declaration order.
- January's .rdata literal order shows each block-name literal created before its field literals. Production's order differs. So January defined each block before its forward-declared <block>_fields.
- Bungie's January leaf_map shows the same idiom.
- The fields-first control (ctl_fields_first.c) keeps the bytes but reverts the literal order: objdiff .data 92.84%, uncredited.

(4) Results against a scratch csplit from the edited symbols.json:
- gate: EXACT 224 _byte_swap_script_syntax_data (1/1).
- object_audit: PASS. All 22 sections are ok, including .data 628/628 with align 4/4 under section_infos_equal. 40 January symbols, 0 differ. Surplus is .drectve only.
- pdb_storage: 0 disagreements.
- surplus_identity: 0. provider_link: PASS (nothing to link).
- fake_match_scan: 0 leads.
- Frozen objdiff 3.3.1 emulation: data 1112/1112 native (.data 100%, currently 92.773%), so no semantic_data_matches entry is needed. Net data +628.
- Every .data relocation now names its owner at addend 0.

(5) Packet C. A shadow-header lab sweep of hs.c, hs_compile.c and hs_runtime.c (orig vs mod) finds every code, data and rdata section and every symbol identical, in order. Only .debug$S / .file differ (the directory name). cand2 also passes object_audit and gets 1112/1112 data. The HCEX constant values are exactly this TU's literals (0x4A39 = 19001, and 56 + 19001*20 = 380076). January's own assert text 'increase MAXIMUM_HS_STRING_DATA_PER_SCENARIO' attests the naming.

## blockers
No byte or ownership blockers. Disclosures for the reviewer:
(a) Four names are descriptive, not first-party, because HCEX.pdb has no hs_scenario_definitions compiland and the /Od build lacks these definitions: hs_references_block_reference, and the four <name>_bs_codes / <name>_bs_definition names. They follow the HCEX `<block>_fields` / `<name>_bs_codes` + `<name>_bs_definition` conventions, with precedents in byte_swapping.c, leaf_map.c and bitmap_group.c.
(b) Including the genuine owner header hs.h adds two /W3 C4090 'different const qualifiers' warnings. The cause is hs.h's `char const *hs_type_names[]` feeding tag_groups.h's reconstructed `char **names`. They are byte-inert. Production had avoided them with a rule-9 consumer-local redeclaration. HCEX's `enum_definition.strings` is `char **`, so I did not const-qualify the shared header.
(c) The forward declaration is spelled as a standard-C sized tentative definition. The `extern T x[];` + static spelling is byte-identical in the lab.
(d) Source (A or C2) and symbols.json (B) must land together. The old source against the new split fails, and so does the new source against the old split, because the function row is renamed.

## reopen_criteria
Not applicable if admitted. If the owner rejects the descriptive static names, the only open items are first-party names for the reference definition and the byte-swap tables. Their storage, order and layout are proven.

## task notes
A new law, measured in scratch/w/hs_scenario_definitions/lab:
- VC7 places .data objects in first-DECLARATION order, including extern and tentative declarations.
- It emits string literals in reverse creation order.
- So January's .rdata literal order reveals DEFINITION order even when the .data bytes cannot.
- Bungie defined each tag block BEFORE its forward-declared <block>_fields array.

Board lead, not probed because it is out of scope: January's literal order also differs from ours in four sibling tag-definition TUs whose fields are defined first: bitmap_group, collision_bsp_definitions, recorded_animation_definitions and leaf_map.
- bitmap_group .data scores 83.4% with no semantic entry, so 1,424 B are uncredited.
- The other three depend on semantic_data_matches.
- Switching to block-before-fields order may credit them natively.

Integrator follow-through:
- config/symbols.json keeps CRLF in the working tree; the patches are LF and apply with autocrlf.
- The stable-verdict row renames from _hs_syntax_data_byte_swap to _byte_swap_script_syntax_data. The code bytes are unchanged.
- No tracked file was edited. No ninja, configure.py or git-mutating command was run.
- csplit and objdiff-cli were run only into scratch/w/hs_scenario_definitions/.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hs_scenario_definitions\LEDGER.md
