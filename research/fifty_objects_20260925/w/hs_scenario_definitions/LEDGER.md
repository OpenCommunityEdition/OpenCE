# hs_scenario_definitions - whole-object admission (wave 3, base 954eebd2)

## Inputs read
- WORKER_BRIEF 0-8, campaign_house_rules, claude_fifty_objects_20260925 (held items: none for this unit),
  rejection entry (source-layout-incomplete .data, evidence claude_data_ownership_closeout_20260924.md),
  no parked.json entry, no prior unit ledger, no earlier scratch/w dir.

## P0 baseline (production source copied to prod.c)
- gate: EXACT 224 _hs_syntax_data_byte_swap (1/1).
- object_audit: FAIL(1): .data size 628/628 flags eq but align 4(Jan, 8-byte) vs 3(ours, 4-byte).
  31 January symbols 0 differ (split currently names _hs_syntax_data_byte_swap external - symbols.json lacks static).
- pdb_storage: 1 disagreement: _hs_syntax_data_byte_swap absent from cachebeta publics => file-static.
- cachebeta publics: all 9 named data symbols ARE public (_hs_script_types_enum ... _hs_string_data_definition).

## January .data decode (dumpdata.py on build/split)
Relocation addends expose the unnamed inner objects (csplit attributes to preceding named symbol):
  0x000 hs_script_types_enum (tag_enum_definition, 12)
  0x00c hs_types_enum (tag_enum_definition, 12)
  0x018 [unnamed] 6 tag_fields = 72 B   <- hs_scripts_block fields (ref _hs_types_enum+0xc)
  0x060 hs_scripts_block (tag_block_definition, 44) ends 0x08c
  0x08c 4 bytes zero GAP
  0x090 [unnamed] 7 tag_fields = 84 B   <- hs_globals_block fields (ref _hs_scripts_block+0x30)
  0x0e4 hs_globals_block (44)
  0x110 [unnamed] {0, -1, 0} 12 B       <- tag_reference_definition of "reference*^" (ref _hs_globals_block+0x2c)
  0x11c [unnamed] 3 tag_fields = 36 B   <- hs_references_block fields (ref _hs_globals_block+0x38)
  0x140 hs_references_block (44)
  0x16c hs_source_data_definition (tag_data_definition, 16)
  0x17c [unnamed] 3 tag_fields = 36 B   <- hs_source_files_block fields (ref _hs_source_data_definition+0x10)
  0x1a0 hs_source_files_block (44)
  0x1cc [unnamed] 14 byte_swap_codes = 56 B (data_array_header codes; ref _hs_source_files_block+0x2c)
  0x204 [unnamed] byte_swap_definition "data_array_header" 20 B (code ref _hs_source_files_block+0x64)
  0x218 [unnamed] 10 byte_swap_codes = 40 B (syntax_node codes; ref _hs_source_files_block+0x78)
  0x240 [unnamed] byte_swap_definition "syntax_node" 20 B (code ref _hs_source_files_block+0xa0)
  0x254 hs_syntax_data_definition (16)
  0x264 hs_string_data_definition (16)  end 0x274 = 628
KEY: the only >=64-byte objects are the two field arrays (72 @0x18, 84 @0x90); both sit 8-aligned and the
0x8c..0x90 gap is exactly the 8-alignment pad of an 84-byte object => the "long field_table_alignment" member
is VC7's per-object alignment padding, and the section's 8-byte alignment comes from these >=64 B objects.
The 56-byte codes array (<64) at 0x1cc is only 4-aligned - consistent with the law.

## Naming evidence
- HCEX.pdb has NO hs_scenario_definitions compiland (mods list); /Od build lacks the hs tag-definition strings.
- HCEX Bungie conventions (other TUs' tag defs): `<block>_fields` (bitmap_group_sprite_block_fields,
  recorded_animation_block_fields, map_leaf_face_block_fields...), byte-swap `<name>_bs_codes` + `<name>_bs_definition`
  (packet_header_bs_codes/definition, long_bs_codes/definition, word_bs_*...), tag reference defs `global_<x>_reference`.
- Landed precedent: leaf_map.c / bitmap_group.c use `static struct tag_field <block>_fields[N]` with "static": true
  symbols.json entries (e.g. _map_leaf_face_vertex_block_fields).

## Finding F1 - authentic function name (Sept-2001 Bungie linker map)
- halo-symbol-atlas 2001-09-25 cache_exe (tier "map" = Bungie's own .map ground truth): the ONLY function of
  hs_scenario_definitions.obj is `@byte_swap_script_syntax_data@12` (that build is /Gr fastcall; @12 = 3 args,
  same arity as January's (owner, data, size)). Same map names recorded_animation_definitions' `byte_swap_recording`,
  which our tree already uses (static). January name `_hs_syntax_data_byte_swap` is an invented descriptive name
  (not in cachebeta publics). => rename to `byte_swap_script_syntax_data`, static.

## Finding F2 - .rdata literal order is an oracle for DEFINITION order (lab/ l1-l5)
- January .rdata (split section order) : hs_string, hs_syntax, syntax_node, data_array_header, source,
  hs_source_files_block, hs_source_data_definition, reference*^, hs_references_block, init expr, type*,
  hs_globals_block, root expr, return type*, script type*, name*, hs_scripts_block, [code literals].
- Production .rdata differs in the middle: hs_source_files_block before source, hs_references_block before
  reference*^, hs_globals_block before init/type*, hs_scripts_block before root/return/script/name*.
- LAB (VC7 /O2): literals are emitted in REVERSE CREATION order (creation = first textual use); .data objects
  are placed in FIRST-DECLARATION order, not definition order. l4 (fields defined before block) -> .rdata
  block-literal before field literals (= production); l2/l3/l5 (fields forward-declared, block DEFINED FIRST,
  fields defined after) -> .data unchanged (fields still first) but .rdata field literals before block literal.
  l2 `static T f[3];` tentative, l3 `extern T f[];` then static definition, l5 extern+public: identical bytes.
- => January wrote each block definition BEFORE its field array (field array forward-declared), i.e. the
  Bungie TAG_BLOCK-style shape: declare <block>_fields, define <block> (creates "<block>" literal), define
  <block>_fields. The .data interleave (fields < block) comes from the forward declaration.
  January creation order (reverse of its .rdata): hs_scripts_block, name*, script type*, return type*, root,
  hs_globals_block, type*, init, hs_references_block, reference*^, hs_source_data_definition,
  hs_source_files_block, source, data_array_header, syntax_node, hs_syntax_data_definition, hs_string_data_definition.
- Board corroboration: January leaf_map.obj .rdata shows the SAME idiom (creation order map_leaf_face_vertex_block
  before "vertex*", map_leaf_portal_index_block before "portal index*", map_leaf_face_block before "node index*",
  global_leaf_portal_block before "plane index*"...). Our admitted leaf_map (fields defined first) emits the
  opposite literal order (build/base). So block-before-fields is Bungie's tag-definition idiom, not a one-off.
  (Side note for integrator: leaf_map's literal order differs from January; per-section coff_compare is blind to it.)
- Search for the Sept-2001 cache exe binary (to byte-compare the named function) found nothing on disk; the
  map-tier name rests on the atlas (Bungie's own .map) + one-function-per-object + same arity.

## P1 cand1.c (separate January definitions; block-before-fields forward declarations; static storage; rename)
Hypothesis (F1+F2 + aggregate-vs-scalar law): January had 18 separate objects; the two >=64 B field arrays
(72, 84) naturally take 8-byte alignment (0x8c->0x90 pad = VC7 per-object alignment, not a member) and set the
section to align 8; each tag block is defined BEFORE its forward-declared <block>_fields array.
Change: removed #pragma pack, __declspec(align(4)), the 5 invented wrapper structs and `field_table_alignment`;
separate `static struct tag_field <block>_fields[N];` forward declaration -> public block definition -> static
fields definition; static tag_reference_definition hs_references_block_reference; static
data_array_header_bs_codes/_bs_definition and syntax_node_bs_codes/_bs_definition (HCEX <name>_bs_codes/_bs_definition
convention, byte_swapping.c precedent); `#include "hs/hs.h"` replaces the consumer-local
`extern char *hs_script_type_names[]/hs_type_names[]`; counts spelled NUMBER_OF_HS_SCRIPT_TYPES/NUMBER_OF_HS_TYPES;
function renamed + static `byte_swap_script_syntax_data` (prototype in the TU's prototypes block).
Scratch config (config/symbols.json copy, in-place: rename line 3249 + 9 inserted static entries, symbols_json.diff);
csplit -> scratch split: census 833 objects, ONLY source/hs/hs_scenario_definitions.obj differs from build/split;
every January .data relocation now names its owner at addend 0 (no more `_hs_types_enum+0xc`-style addends).
RESULT: gate (scratch split) EXACT 224 _byte_swap_script_syntax_data 1/1;
object_audit (scratch split) PASS: .data 628/628 align 4/4 section_infos_equal, 40 January symbols 0 differ,
all 21 .rdata ok; surplus only .drectve (as production).
.rdata literal ORDER now identical to January (production's was not) - independent confirmation of the
block-before-fields definition order.
- objdiff 3.3.1 emulation (mkproj.py, frozen build/tools/objdiff-cli.exe): production vs build/split reproduces
  report.json exactly (.data 92.773094%, data 484/1112); cand1 vs scratch split: .data 100%, .rdata 100%,
  .text 100%, data 1112/1112 NATIVELY. Cause: objdiff matches '$'-literal data relocations only by equal
  combined address, and the literal ORDER (hence addresses) now equals January's. No semantic_data_matches
  entry is needed. Net data +628 once the split is regenerated with the symbols.json edit.
- cand1 /W3 /Zs: two new C4090 ('different const qualifiers') at the hs_script_types_enum / hs_types_enum
  initialisers: hs.h's genuine `char const *hs_type_names[]` into tag_groups.h's reconstructed `char **names`.
  Byte-inert. (Production silenced it with a consumer-local `extern char *...[]` redeclaration - a rule-9 defect.)

## Finding F3 - HCEX names the scenario limits (optional header packet B)
- HCEX.pdb types (hcex_types.txt 1309380-1309386) one anonymous enum: MAXIMUM_HS_SCRIPTS_PER_SCENARIO 0x200,
  MAXIMUM_HS_GLOBALS_PER_SCENARIO 0x80, MAXIMUM_HS_REFERENCES_PER_SCENARIO 0x100,
  MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO 0x4A39 (=19001; 56+19001*20 = 380076 = hs_syntax_data_definition max),
  MAXIMUM_HS_STRING_DATA_PER_SCENARIO 0x40000, MAXIMUM_HS_SOURCE_FILES_PER_SCENARIO 8,
  MAXIMUM_HS_SOURCE_DATA_PER_FILE 0x40000 - every literal of this TU's definitions. January's own assert text
  "increase MAXIMUM_HS_STRING_DATA_PER_SCENARIO" (hs_compile) attests the naming in January. hs.c currently
  carries a TU-local `#define MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO 19001`.
- Sweep lab (shadow include root first, shadow_cc.py): header enum added to hs_scenario_definitions.h + hs.c's
  #define removed; consumers hs.c / hs_compile.c / hs_runtime.c (the only includers besides this TU) compiled
  orig-vs-mod: every code/data/rdata section and every symbol identical in order (1410/222/172 sections);
  only .debug$S/.file differ (shadow directory name). Byte-inert for all consumers (+7 declared names is NOT
  a tie-flipper here).

## P2 control ctl_fields_first.c (cand1 with each <block>_fields defined BEFORE its block, no forward decls)
Hypothesis: identical .data/.text bytes and alignment, but literal order reverts to production's.
RESULT: gate EXACT 1/1, object_audit PASS (object_audit is blind to section ORDER), but .rdata order =
production's and objdiff 3.3.1 .data 92.84473% (uncredited). => the block-before-fields definition order is
load-bearing for January's image layout (literal addresses) and is attested by January's own .rdata;
it is source structure (definition order), not decoration.

## P3 cand2.c (= cand1 + HCEX scenario-limit constants; needs header packet B)
Shadow-header compile (sh_mod): object_audit PASS vs scratch split (40 symbols 0 differ, .data align 4/4),
objdiff emulation data 1112/1112 (.data 100%). Same two C4090 as cand1.

## Admission checklist (final A = deliver/hs_scenario_definitions.c == cand1.c; scratch split from config copy)
1. gate (scratch split): EXACT 224 _byte_swap_script_syntax_data, 1/1 (gate_final_A.txt).
2. object_audit (scratch split): PASS - 22 sections ok (.data 628/628 flags eq align 4/4), 40 January symbols
   0 differ, surplus .drectve only (audit_final_A.txt; packet C: audit_final_C.txt PASS).
3. pdb_storage (scratch split + cand): 40 split symbols, 0 disagreements (9 public data names exactly the
   cachebeta publics in 0x2E49C0..0x2E4C34; the 9 new statics and the function are absent => static).
4. surplus_identity: 0 candidate-only code COMDATs; provider_link: nothing to link, PASS.
5. data: objdiff 3.3.1 emulation 1112/1112 native (no semantic entry needed); csplit census: only
   hs_scenario_definitions.obj changes among 833 split objects.
6. fake_match_scan: 0 leads (production also 0). No pragmas, no align/pack, no padding member, no wrapper structs.
NOTE: source and symbols.json MUST land together - production source vs the new split FAILs (function row
renamed; 9 statics missing), and the new source vs the old split names the function _byte_swap_script_syntax_data
which the old split calls _hs_syntax_data_byte_swap.

## Deliverables (deliver/)
- A_hs_scenario_definitions_c.patch (literals; standalone) + full file hs_scenario_definitions.c
- B_config_symbols_json.patch (rename line + 9 inserted static rows, in place) + symbols.json.full_edited_crlf
  (also scratch/w/hs_scenario_definitions/config/symbols.json)
- OPTIONAL shared-header packet: C1_header_scenario_limits.patch (hs_scenario_definitions.h enum + hs.c
  #define removal; consumers hs.c hs_compile.c hs_runtime.c hs_scenario_definitions.c; lab sweep 0 section diffs)
  + C2_hs_scenario_definitions_c_with_limits.patch (use INSTEAD of A).
- D_admission_status.patch (config.json NonMatching->Matching; remove the rejection entry).
- git apply --check: A+B+D together OK; C1+C2+B+D together OK.

## Board lead (not probed; out of scope)
Literal-order census: January .rdata literal order differs from our build in bitmap_group (137 lits),
collision_bsp_definitions, recorded_animation_definitions and leaf_map - all tag-definition TUs whose blocks are
defined AFTER their fields in our source. bitmap_group .data is 83.4% with NO semantic entry (1,424 B uncredited);
the other three lean on semantic_data_matches. The block-before-fields forward-declaration order (F2) is the
candidate native fix.
