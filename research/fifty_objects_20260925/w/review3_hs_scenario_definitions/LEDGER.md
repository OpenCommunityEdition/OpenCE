# review3_hs_scenario_definitions - adversarial review ledger (wave 3, base 954eebd2)

Target claim: OBJECT_COMPLETE_CANDIDATE for source/hs/hs_scenario_definitions (worker: scratch/w/hs_scenario_definitions).

## R0 inputs read
- WORKER_BRIEF 0-8, house rules, lane ledger (no held item for this unit), rejection entry
  (source-layout-incomplete .data; reopen = genuine separate data defs + natural alignment + static byte-swap owner,
  no fabricated aggregates/padding; preserve code+data across full sweep), closeout ledger 20260924 lines 127-141,
  worker LEDGER.md.

## R1 technical re-runs (all independent, my slug)
- Tree clean at 954eebd2. git apply --check: A, B, C1, C2, D each clean. Applied in an out-of-repo mini tree:
  A+B+D together OK, C1+C2+B+D together OK; post-images == deliver/*.c/.h modulo CRLF (tree is CRLF, patches keep CRLF);
  B post-image == worker config/symbols.json byte-for-byte; all 3 JSON files parse after D/B.
- symbols.json B: 9 inserted offsets = 0x2E49C0 + {0x18,0x90,0x110,0x11c,0x17c,0x1cc,0x204,0x218,0x240}; none of the
  offsets already present (no alias precedence risk); rename keeps flags 32 and adds "static": true (same form as line 3097).
- csplit (build/tools/csplit.exe, my config copy): control config -> 0/833 differ from build/split (build/split current);
  edited config -> ONLY source/hs/hs_scenario_definitions.obj differs (833 census). CONFIRMED.
- gate: prod vs build/split EXACT 224 _hs_syntax_data_byte_swap; candA vs build/split UNWRITTEN (rename; must land with B+csplit);
  candA vs review split EXACT 224 _byte_swap_script_syntax_data 1/1. CONFIRMED.
- object_audit (split-root copy): candA vs review split PASS, 22 sections ok incl .data 628/628 align 4/4, 40 January
  symbols 0 differ, surplus .drectve only. prod vs build/split FAIL(1) .data align 4/3 (the recorded rejection).
- section ORDER (secorder.py): candA .rdata literal order == January split order; production differs in 4 block/field
  pairs. leaf_map January literal order shows the same fields-literals-before-block-literal pattern (block created
  first); our admitted leaf_map is reversed. F2 CORROBORATED.
- objdiff 3.3.1 (frozen, sha1 3130e428) mini-project: prod vs build/split reproduces build/report.json exactly
  (.data 92.773094, data 484/1112); candA vs review split data 1112/1112 (.data 100, .rdata 100, .text 100);
  ctl_fields_first vs review split .data 92.84473 (484/1112). CONFIRMED.
- pdb_storage (split-root): 40 split symbols, 0 disagreements. cachebeta publics in 0x2E49C0..0x2E4C34 are exactly the
  9 public data names; nothing public at 0xBD310..0xBD3F0 (function static). CONFIRMED.
- provider_link: nothing to link PASS; candidate has no surplus external definition (30 ext defs = 21 literals + 9 publics);
  undefined externals identical set to January's split.
- fake_match_scan: 0 leads (candA, candC2).
- /W3 /Zs: candA adds 2x C4090 (lines 96/103: hs.h `char const *[]` into tag_groups.h `char **names`); HCEX types
  hs_type_names as `const char *[0x31]`, so hs.h is right and the reconstructed tag_groups.h member type is the mismatch.
  Byte-inert; non-blocking disclosure.

## R2 ENVIRONMENT HAZARD (not caused by this reviewer; report to integrator)
- ~00:43-00:54 PDT: the worktree root `.git` link FILE is missing (`git rev-parse` -> "not a git repository").
  Earlier in this review (00:3x) git worked and HEAD was 954eebd2 with a clean tree.
- 00:54:01: source/hs/hs.c (tracked) was rewritten in place by another agent's scan (scratch/hs_scan_backup.c +
  scratch/volatile_scan2.json written the same second). Content identical modulo CR; the working copy went CRLF->LF.
  C1 still passes `git apply --check` (out-of-repo copy) against the LF bytes.
- All my measurements used build/split, my own config copies and files whose mtimes predate these events
  (hs_scenario_definitions.c/.h, hs.h, tag_groups.h 17:30; symbols.json/config.json/rejections 23:43).

## R3 naming provenance (the owner-gated question)
- Function: atlas 6455066... (2001-09-25 cache_exe, tier "map" = Bungie's own .map): 0x48d500
  `@byte_swap_script_syntax_data@12`, lib hs_scenario_definitions.obj - the only record for that obj; same map gives
  `@byte_swap_recording@12` (recorded_animation_definitions, already used static in our tree). Absent from cachebeta
  publics -> static. FIRST-PARTY. Accept.
- 9 public data names: exactly the cachebeta publics. Accept.
- 9 new static data names: NO first-party source. Checked: HCEX has no hs_scenario_definitions compiland and no
  hs tag-definition data (only hs_syntax_data*, hs_type_names `const char *[0x31]`); atlas records are code-only
  (METHODOLOGY: only code symbols kept; no data); no raw Bungie .map on disk; /Od build has none of the tag strings
  (odbuild str "hs_scripts_block"/"syntax_node"/"hs_scenario_definitions" -> no hit); research-cache reconstructions
  have none of the names.
  * 4x `<block>_fields`: HCEX convention (10 `*_block_fields` statics incl. map_leaf_*, bitmap_group_*); stem = January
    public block name. Convention-derived.
  * 4x `<name>_bs_codes`/`<name>_bs_definition`: HCEX convention (~40 pairs: packet_header_, long_, real_vector3d_ ...);
    stem = January's own byte_swap_definition name string ("data_array_header", "syntax_node"). Convention-derived.
  * `hs_references_block_reference`: NO convention. The only tag_reference_definition names in HCEX/tree are public
    `global_<group>_reference`; this one is a TU-local any-tag {0,NONE,NULL}. INVENTED.
- Policy precedents: owner ruling 2026-09-15 ("prefers no descriptive/invented static names in symbols.json";
  env_fog 21 + models 13 held); review2_rasterizer_xbox withheld admission on 9 descriptive .bss names (lane: held for
  owner policy ruling); bitmap_drawing unnamed static held pending "first-party name and type"; hs_globals_external
  rejected because its 442 names are attested only by a 2011 build. Counter-precedent: batch 3 admitted
  rasterizer_xbox_shadows with 2 disclosed descriptive gap statics following HCEX's `local_` pattern.

## R4 optional packet C (C1 header + C2 source) - independent sweep
- HCEX types 1309380-1309386: anonymous enum MAXIMUM_HS_{SCRIPTS 0x200, GLOBALS 0x80, REFERENCES 0x100,
  SYNTAX_NODES 0x4A39, STRING_DATA 0x40000}_PER_SCENARIO, SOURCE_FILES_PER_SCENARIO 8, SOURCE_DATA_PER_FILE 0x40000.
  C1's enum reproduces names, values and order exactly. CONFIRMED.
- Includers of hs_scenario_definitions.h (grep source/ xbox/, .c and .h): hs.c (bare include, line 2784),
  hs_compile.c, hs_runtime.c, hs_scenario_definitions.c. No header includes it; config.json lists it only in the
  project "headers" list (no /FI). Consumer set CONFIRMED.
- Shadow compile (own sh_orig/sh_mod roots; orig hs.c = CRLF backup identical in content to tree):
  hs 1409 / hs_compile 221 / hs_runtime 171 non-debug$S sections byte+reloc identical orig vs mod.
  Symbols: hs and hs_compile identical; hs_runtime: 64 internal `$L` label NAMES shift by +8 (e.g. $L9179->$L9187),
  same values/sections - a declared-name-count effect. (Worker's "every symbol identical" is slightly overstated.)
  Per-function strict status base == shadow orig == shadow mod (hs 447/448, hs_compile 61/64, hs_runtime 64/66);
  objdiff 3.3.1 per-function fuzzy maps identical orig vs mod for all three. => C1 is byte-inert for its consumers;
  the integrator's full sweep still applies.
- C2 compiled with the C1 header: EXACT 1/1 vs review split, object_audit PASS (40/0), objdiff 1112/1112.
  C2 without C1 fails to compile (dependency: C2 needs C1).
- 00:57:07 `.git` link file reappeared; 00:57:36 source/hs/hs.c restored to CRLF (425021 B). Tree clean at 954eebd2 again.
  Transient; caused by another agent's scan harness in the shared worktree. Reported for the integrator only.

## R5 source review of A (house rules, section-8 blockers, strip tests)
- Removed: #pragma pack, 11x __declspec(align(4)), 5 wrapper structs, `long field_table_alignment`, consumer-local
  `extern char *hs_*_names[]` (rule-9 defect). Added #include "hs/hs.h" (genuine owner; HCEX `const char *[0x31]`).
- Function body byte-identical to production apart from the name/static; no new block scope; early `return;` and the
  block-scoped `syntax_node_count` are inherited and bytes-attested. /Od build has neither the function nor its strings
  (odbuild str "csirtpn"/"data_size>=0"/"data_array_header": no hit), so no /Od declaration-order question arises.
- Prototype of the static function sits in the TU's prototypes block (rule 9 OK). Params one per line, explicit return.
- `(void *)N` pad sizes and `{0, NONE, NULL}` reference follow bitmap_group/tag_groups.h precedent.
- `static byte_swap_code <x>_bs_codes[]` unsized, as byte_swapping.c (byte_/word_/long_/int64_bs_codes).
- STRIP TEST of the only structural construct (forward tentative `static struct tag_field <block>_fields[N];` then block,
  then fields): plain fields-first spelling (ctl_fields_first.c) is strict-EXACT + audit PASS but reverses 4 literal
  pairs vs January's .rdata and objdiff .data drops to 92.84473 (484/1112). The decorated form is independently attested
  by January's own literal order (reverse-creation law) and by January leaf_map's identical pattern. It is ordinary C
  definition order, not decoration. PASS.
- No casts/parens/dummy locals/volatile/asm/pragma. fake_match_scan 0.
- Remaining non-first-party content: the 9 static data names (see R3). hs_references_block_reference is invented.

## R6 fallback F (zero-credit, not owner-gated) - built and verified by the reviewer
- fallback/F1_hs_scenario_definitions_static_rename.patch: production file + ONLY the first-party function
  name/storage (`static void byte_swap_script_syntax_data`, prototype in the prototypes block, definition moved to
  private code, header symbol list updated). fallback/F2_config_symbols_json_rename_only.patch: symbols.json line 3249
  only (identical to worker B's line 3249). git apply --check clean together; post-images byte-identical (CRLF).
- csplit census with F2: only hs_scenario_definitions.obj changes. gate EXACT 224 _byte_swap_script_syntax_data 1/1;
  pdb_storage 0 disagreements (production has 1); object_audit still FAIL(1) on the .data align (the recorded
  rejection, unchanged); objdiff 3.3.1 unchanged (216/216 code, 484/1112 data). fake_match_scan 0.
- Use F only if the owner declines the 9 static data names; it is an alternative to A (not additive).
- .data relocations: coff_compare resolves intra-.data relocations as 'internal'+offset, so the new names matter
  only for the function's cross-section relocations - B's names are required for A's function row to stay EXACT.

## R7 verdict
- sh_orig inputs byte-equal the restored tree (hs.c CRLF 425021, hs_compile.c, hs_runtime.c, header); tree clean at 954eebd2.
- VERDICT approve=false: technical claim fully reproduced; admission blocked only by the owner-policy question on
  9 non-first-party static data names (8 convention-derived, 1 invented). If the owner admits them, A+B+D (or
  C1+C2+B+D) land as delivered. Fallback F1+F2 (zero credit, first-party rename+static only) verified.
  REVIEW.md written.
