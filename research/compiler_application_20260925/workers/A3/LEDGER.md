# A3 (wave 3) ledger: hs_runtime + path_obstacle_avoidance admission

Worker A3, solo, patch-only. Base HEAD 455dffad (canonical cc608036 + batch 1). Started 2026-09-26 00:26:46 -0700.
Assigned units: source/hs/hs_runtime, source/ai/path_obstacle_avoidance.

## 0. Fresh measurements (00:27 -0700)
- `gate.py source/hs/hs_runtime --all`: == exact 66 residual 0 unwritten 0 (of 66).
- `gate.py source/ai/path_obstacle_avoidance --all`: == exact 24 residual 0 unwritten 0 (of 24).
- config.json: both `NonMatching` (hs_runtime index 283, path_obstacle_avoidance index 429).
- `config/object_admission_rejections.json`: NO entry for either unit.
- batch_batch1_admission.json: both are `audit-coff-ownership-before-admission` candidates, data_gap 0
  (hs_runtime 11,940 data B; path_obstacle_avoidance 9,570 data B).

## 1. path_obstacle_avoidance: why it is not admitted (history, sources read)
- 2026-09-08 fable owner reconciliation: data boundary note - January split labels .bss offset 0
  `_current_traverse_index`; source names the snapshots `debug_path`/`debug_obstacles`. "No BSS alias ... until
  that ownership boundary is resolved from stronger symbol evidence."
- 2026-09-14 opus5 w1: proposed symbols.json relabel 3251464 `_current_traverse_index` -> `_debug_path` static +
  3256896 `_debug_obstacles` static. Disclosed: names descriptive, not authentic; uninit .bss order is name-hash
  sensitive.
- 2026-09-14 opus5 w3: evidence search negative (cachebeta publics only `_debug_obstacle_path_on_failure`/
  `_debug_obstacle_path`; punpckhdq `_bss_00319d08`; HCEA corpus different design; h1_tags only hs names).
- 2026-09-20 astra wave17: 2001 MAPs (Aug/Sept beta, Sept cache) have 0 static DATA records; flags spacing
  corroborated, not the split/names.
- 2026-09-20 lane B rejected-hypotheses: both HCEX compilands (SHIP, Release) list NO file statics for
  path_obstacle_avoidance; /Od build copies two distinct typed snapshots (obstacles 0xC08, obstacle_path 0x1534)
  = proof of two objects and types/sizes, not names.
- 2026-09-22 Lane C reconciliation 4f4c8502 LANDED the relabel in symbols.json (plus
  `_global_ai_debug_path_render_id` static at 3251456) -> function credit for `_path_avoid_obstacles` and
  `_render_debug_obstacle_path`.
- 2026-09-23 canonical b0992d0e (`halo_only_object_closures_20260924.md`, "Held, not counted"):
  "`path_obstacle_avoidance` is 24/24 strict but its historical BSS relabel lacks authenticated private
  identities."
- Separately (fifty-objects, 2026-09-23/24): provider link FAIL(1) on `_cross_product2d` vs actor_combat's NODUP
  hand copy. RESOLVED by R3 batch 2 (3ee32a5c, owner ruling Q1 YES): actor_combat.c:1588 now calls the header
  inline. R3-2 applied config 03A only: "03b/03C not applied (path_structure_bsp is reserved;
  path_obstacle_avoidance's .bss relabel is held)".
- Owner class ruling 2026-09-15 (opus5_250k checkpoint section 2): env_fog 21-static and models 13-static
  descriptive .bss splits "neither lands". No later explicit owner ruling on the descriptive-static class.
- Counter-state (disclosed, NOT a precedent I may rely on): collision_debug is Matching (b9a8d587, 2026-09-23 23:53,
  after the hold) with descriptive `_collision_debug_spray_{normals,points,hit_flags}` statics (HCEX empty for
  that unit); rasterizer_xbox_shadows Matching with 2 disclosed descriptive gap statics.

## 2. New evidence search (this wave)
- Halo CE symbol atlas, all 12 jsonl files including January's own map (4cc87b45...): records for
  path_obstacle_avoidance.obj are CODE only (functions + inline COMDATs); zero data records in 0x719D00..0x71BE60
  (January .bss region +0x400000); no record named *debug_obstacle*/*debug_path*/*traverse_index* other than
  functions. NEGATIVE: no authenticated identity for the two snapshots.

## 3. Battery at HEAD (00:30-00:50 -0700), objects = fresh gate --out, keyed-identical to build/base
- gate --all: hs_runtime 66/0/0; path_obstacle_avoidance 24/0/0.
- object_audit: pao PASS (52 January symbols, 0 differ; 17 candidate-only surplus: .drectve, 6 literals, 11 code
  COMDATs); hs_runtime PASS (129 January symbols, 0 differ; 27 candidate-only surplus).
- pdb_storage: pao 52 split symbols 0 disagreements; hs_runtime 129 / 0.
- surplus_identity: pao 11 code COMDATs 0 not identical; hs_runtime 6 / 0.
- provider_link (all surplus, both orders inside the tool): pao PASS (17 symbols); hs_runtime PASS (27 symbols).
  => the old `_cross_product2d` LNK2005 blocker is GONE (actor_combat now emits the header inline).
- protoscan: hs_runtime.c none flagged; pao.c:157 `real normalize2d(real_vector2d *vector);` NOT DEFINED HERE.
  protoscan does NOT flag `extern` data (only parenthesised prototypes) -> manual census below.
- /W3 (A3 copy of W2 w3.py): pao 12 warnings, all in shared headers (cseries.h C4146 x1, real_math.h C4244 x11);
  hs_runtime C4013 x2 (object_list_gc @1155, hs_node_gc @1157), C4028 x64 + C4133 x55 (typecast/inspector
  tables @615-690), C4090 x1 (@1524), C4244 x14 (11 real_math.h + @1578/1586/1597), C4146 x1.

## 4. Source-review findings (evidence)
### path_obstacle_avoidance.c
- F-P1 `#define valid_real_point2d valid_real_point2d_inline` around real_math.h + a TU-defined
  `boolean valid_real_point2d(...)` built from memcpy bit tests (`#undef memcpy`, `<string.h>`). History
  (path_obstacle_avoidance_obj_jonas_valid_point2d_20260826.md): the natural E1 body
  `valid_real(point->x) && valid_real(point->y)` was EXACT on the first shot but was rejected because it emitted
  the `_valid_real` COMDAT and shifted section ordinals; E2 (memcpy) was chosen to avoid that emission = a hand
  expansion "to evade an emission check" (house rule "Preserve January's genuine helper calls..." E/41).
  January census (A3 symscan): `_valid_real_point2d` DEF only path_obstacle_avoidance, REF by nobody =
  emitted-but-unreferenced header-inline COMDAT. `_valid_real` already emitted at HEAD (surplus, identical to
  actor_combat, link PASS), so E1's emission premise no longer applies; gates are name-keyed, not ordinal.
- F-P2 `#define normalize2d normalize2d_inline` + consumer-local `real normalize2d(real_vector2d *vector);`
  (protoscan). January: `_normalize2d` DEF only action_charge; REF by 17 objects incl. path_obstacle_avoidance
  (out-of-line call). actor_combat's identical redirect was REMOVED by owner-approved R3-2 (3ee32a5c).
- F-P3 .bss relabel (debug_path / debug_obstacles descriptive statics) - canonical hold, see section 1.
- F-P4 error_heap block-local anonymous union {real value; long bits;} for the %x print (Fable 09-08 replaced a
  pointer pun). HCEX SHIP has no `error_heap` symbol (query empty). Disclose; precedent class = _wind_variance_get
  block-local real/long transfer (owner override), not identical scope.
### hs_runtime.c
- F-H1 C4013 implicit declarations: object_list_gc (object_lists.c:187, no header prototype), hs_node_gc
  (hs.c:13220, no header prototype; hs.c and hs_compile.c also call it). January: _object_list_gc REF only by
  hs_runtime; _hs_node_gc REF by hs_compile + hs_runtime.
- F-H2 typecast/inspector ABI: TU-defined `union hs_conversion_result` by-value converters/inspectors stored into
  `hs_typecasting_procedure` = `long (*)(long)` and `hs_inspection_procedure` slots (C4133 x55, C4028 x64).
  HCEX SHIP (DIA2Dump -sym): hs_long_to_boolean/hs_long_to_real/hs_short_to_real/hs_real_to_short/
  hs_long_to_short/hs_string_to_boolean/hs_data_to_void/hs_enum_to_real/hs_short_to_boolean/hs_real_to_long all
  `static long f(long)`; several with local `long result` [FFFFFFF0]; hs_object_name_to_object_list(long
  object_name_index) (ours: short); inspectors `static void f(short type, long value, char *buffer)`.
  The 2026-09-04 reconciliation knew HCEX said `long` and chose the union for January identity.
- F-H3 consumer-local extern data in hs_runtime.c: hs_global_data, hs_thread_data, hs_syntax_data,
  hs_external_global_count, hs_type_sizes[], debug_scripting, hs_debug_data[] (547-553);
  _hs_type_{boolean,real,short_integer,long_integer,string}_default (695-699).
- F-H4 hs_inspect_enum supplies the January assert text "enum_value>=0 && enum_value<enum_definition->count" as a
  literal while the source expression is value.short_integer (the text names a variable absent from source).

## 5. Evidence added 00:47-00:58 -0700
- PAO-1/PAO-2/PAO-12/PAO-S outcomes: see CARDS.md (PAO-12 = candidate patch; PAO-S = view casts byte-inert).
- /Od: path_avoid_obstacles 0x4cafb0, path_find 0x4cba00, path_new 0x4cbef0, path_iterate 0x4cbc00,
  render_debug_obstacle_path 0x4cc6b0, error_heap 0x4c9b40 (files scratch/campaign/workers/A3/od_*.txt).
  /Od debug-global layout: 0xae9f80 debug_obstacle_path, +1 on_failure, +2 finishing, +3 ignore_broken_surfaces,
  +4 start_point, +0x10 start_surface_index, +0x14 goal_point, +0x20 goal_surface_index, +0x24 radius,
  0xae9fa8 obstacles snapshot (0xC08), 0xaeabb0 path snapshot (0x1534). No names for the snapshots.
- Consumer-local extern DATA in Matching objects: 51 of 387 Matching .c units have top-level extern data
  declarations (hs_globals_external 164, collision_debug 21, actor_types 12, models 7, ...) -> canonical practice
  tolerates them; dynavobgeom was rejected for INCOMPATIBLE partial views.
- hs_runtime extern owners: hs_global_data/hs_thread_data/hs_syntax_data/debug_scripting/hs_debug_data are January
  COMMON-pool symbols (symbols.json 5842109..5842152) = linker_common, not mine. hs_type_sizes (hs.c:6222
  'short const'), _hs_type_*_default (hs.c:6275-6279, const), hs_external_global_count (hs_globals_external.c:926
  'short const') are declared NON-const in hs_runtime.c. debug_scripting is 'extern byte debug_scripting[]' in
  hs_globals_external.c vs 'extern boolean debug_scripting' in hs_runtime.c. No header declares any of them.
- HCEX SHIP types dump (scratch/campaign/workers/A3/hcex_ship_types.txt, 106 MB, DIA2Dump -t, 3m12s):
  hs_global_runtime {short identifier; unsigned short pad; long value;}; hs_thread {..., long latent_sleep_until,
  hs_stack_frame *stack, long result, uchar stack_data[0x200]}; hs_stack_frame {hs_stack_frame *parent; long
  expression_index; long *child_result; short size; uchar data[0];}; runtime globals {uchar initialized; short
  executing_thread_index;} (no pad member). No hs union type anywhere in HCEX.

## 6. FINAL (2026-09-26 01:31:31 -0700). HEAD moved during the wave: 455dffad -> 09f5208f (decals.c, hud_weapon.c, main.c,
physics.c only; no header or assigned-unit change; both units re-gated 66/66 and 24/24 at 09f5208f).

### Patches (all diff -u against HEAD unless marked; 'git apply --check' OK; review_patch.py 0 gains / 0 losses)
- path_obstacle_avoidance.patch (sha256 00dff72ce33ddc61): PAO-1 genuine header inline
  valid_real_point2d (drops rename, memcpy hand expansion, #undef memcpy, <string.h>); PAO-2 genuine header
  inline normalize2d (drops rename + consumer-local prototype); PAO-3 /Od flag+break instead of two gotos;
  PAO-4 /Od RTC name desired_direction; PAO-5 /Od factor order; PAO-7 comment placeholders -> real names.
  Effect: 24/24 unchanged; +4 surplus COMDATs (_normalize2d 80, _magnitude2d 32, _magnitude_squared2d 32,
  _scale_vector2d 32), each IDENTICAL to January's action_charge copy, provider link PASS both orders.
- path_obstacle_avoidance_with_OPTIONAL_inline.patch (34a491212e6e96f2) =
  above + PAO-6 (__inline path_get_step/path_get_step_index per the Sept-2001 map 'i' tags); ontop:
  path_obstacle_avoidance_OPTIONAL_inline_ontop.patch (d9517a5187db75c7).
- hs_runtime.patch (6fdc74ec10285f7d) LANDABLE: H-1 enum_value local + stringified match_assert; H-1c const
  enum_definition (C4090 gone); H-2 const on 40 externs (definitions const, January .rdata); H-3 HCEX constants
  MAXIMUM_NUMBER_OF_HS_THREADS/GLOBALS + sizeof; H-4b/H-4c /Od hs_can_cast shape (goto removed); H-8 invented
  'byte pad' removed. Effect: keyed-identical to build/base (0/0/0).
- hs_runtime_OWNER_hcex_abi.patch (a78236190cb096f0) OWNER-GATED (includes the landable part;
  ontop version hs_runtime_OWNER_hcex_abi_ontop.patch 2b6ac114b7b617b5): H-5/H-5b S2a HCEX
  converter ABI, H-6 HCEX inspector ABI, H-7 union removed (long + typed views, HCEX local names), 5 BUG comments.
  Effect: keyed-identical to build/base (0/0/0); /W3 from the TU: 0 (with HDR-1).
- LEAD_hs_h_object_lists_h_gc_prototypes.patch (e7924099b36f4be3) HDR-1
  (lead-owned): 'void hs_node_gc(void);' in hs.h after hs_update; 'void object_list_gc(void);' in object_lists.h
  after object_list_remove_reference. 17/17 consumers keyed-identical (control root valid); C4013 hs_runtime 2->0,
  hs_compile 2->1; no other warning change.
- LEAD_HDR2_external_global_definition_owner_type.patch (a2cd6e81be23a592)
  HDR-2 (lead-owned; applies after hs_runtime(.OWNER) + HDR-1): complete struct hs_external_global_definition
  in hs.h; removes the three .c definitions (hs.c partial view {name,type} [B1 owns hs.c this wave],
  hs_globals_external.c, hs_runtime.c). 17/17 consumers keyed-identical; no warning change.
- Apply chains verified on HEAD copies (scratch/campaign/workers/A3/chain_landable, chain_owner): hs_runtime 66/66
  0/0/0; pao 24/24 +4 COMDATs; hs, hs_globals_external, hs_compile 0/0/0.

### Battery (final objects scratch/campaign/workers/A3/final_*.obj)
- gate --all: pao 24/24; hs_runtime 66/66 (both variants).
- object_audit: pao PASS 52/0 (surplus 21); hs_runtime PASS 129/0 (both).
- pdb_storage: pao 52/0; hs_runtime 129/0 (both).
- surplus_identity: pao 15 COMDATs 0 not identical; hs_runtime 6/0 (both).
- provider_link both orders: pao PASS 21 symbols (new: 4 PASS with --baseline); hs_runtime PASS 27 (both).
- protoscan: no consumer-local prototype in any final file (HEAD pao flagged normalize2d).
- /W3: pao 12 header-only (== HEAD); hs_runtime landable+HDR-1 = C4028 64, C4133 55 + 12 header (union ABI);
  owner+HDR-1 = 12 header-only. fake_match_scan: 0 leads on every final file (HEAD 0).

### Failed predictions
- PAO-2 predicted +1 symbol, got +4 (normalize2d's own header-inline callees).
- H-4b predicted NOT exact (single call site), got EXACT.
- H-5 S1 predicted 12/12, got 11/12 (VC7 folds the in-place long_to_short word store).
### Process notes
- Tool heredocs halve '\': two edit groups failed their exactly-once assertion BEFORE any compile (no result
  affected); fixed with the Edit tool.
- PAO-S card and first compile carry the same clock second (card file written first in the same command).

### Owner questions (fully disclosed)
Q-A3-1 path_obstacle_avoidance .bss snapshot names (the only remaining blocker of that object).
- Ask: admit the object (A3 patch applied) while its two uninitialised static snapshots keep the descriptive names
  debug_path (symbols.json 3251464, +0, 0x1534 B) and debug_obstacles (3256896, +0x1538, 0xC08 B), landed for
  function credit in 4f4c8502 (2026-09-22)?
- Hold source: canonical b0992d0e 'historical BSS relabel lacks authenticated private identities'; class ruling
  2026-09-15 (env_fog/models descriptive .bss splits: neither lands).
- The reason still holds: no authentic name exists anywhere searched (cachebeta publics; HCEX SHIP + Release
  compilands; Aug/Sept 2001 cachebeta.map + Sept cache.map; all 12 halo_ce atlas files incl. January's own 4cc87b45;
  /Od build has no symbols).
- Split evidence (not names): January copies 0x1534 B to +0 and 0xC08 B to +0x1538; .bss size 0x2142 and the
  public flags at +0x2140/+0x2141 force path-first plus a 4-byte gap; /Od copies two typed snapshots.
- Disclosure (A3 PAO-D): VC7 orders uninitialised statics by name, so the names are layout-coupled
  (obstacle_path_snapshot/obstacles_snapshot flips the order; debug_* and failed_* keep it; declaration order
  irrelevant).
- Consistency: collision_debug was admitted (b9a8d587, after the hold) with 3 descriptive spray statics (HCEX
  empty); rasterizer_xbox_shadows with 2 disclosed descriptive gap statics.
- YES: +1 Halo object (config flip by the lead), 0 new code/function/data bytes. NO: stays NonMatching; the source
  patch still lands at zero credit.

Q-A3-2 hs_runtime value representation (the only remaining source-review blocker of that object, after the
landable patch + HDR-1 + HDR-2).
- (A) status quo: TU-invented 'union hs_conversion_result' in 12 converter + 6 inspector signatures,
  hs_global_datum.value, 4 result packagings, 3 stack slots; converters stored in 'long (*)(long)' slots and called
  through that type in hs_cast (C4133 x55 + C4028 x64; incompatible function type at the call); union inactive-member
  reads (e.g. result.boolean written, result.long_integer read) = the class rejected in wind except the minimal
  override.
- (B) hs_runtime_OWNER_hcex_abi.patch: HCEX SHIP types throughout (static long f(long); inspectors (short, long,
  char *buffer); hs_global_datum.value long; 'long result'/'result_long' locals named as in HCEX), 0 TU warnings,
  keyed-identical 66/66. It states January's partial writes explicitly with 5 BUG comments:
  hs_string_to_boolean (3 indeterminate bytes; J push ecx / mov [ebp-4],al / mov eax,[ebp-4]), hs_long_to_short
  (2 bytes; J homes result in the argument slot), equality/inequality/logical boolean packaging (3 bytes; J
  equality +0xc5 mov byte [ebp+8],al / mov edx,[ebp+8] / push edx). No UB-free exact form exists
  (initialising adds a store: S2b 32 vs 16 B).
- Either choice carries the same January machine behaviour. YES to (A) or (B): +1 Halo object, 0 new bytes.

Q-A3-3 (low stakes) path_obstacle_avoidance error_heap: block-local anonymous union {real value; long bits;} for
  the January '%3d. %.12g (%x)' diagnostic (/Od: one float local read as bits and as double; indistinguishable from
  a '*(long *)&cost' pun, which the 2026-09-08 reconciliation replaced 'retaining exact code'). Confirm it is within
  the _wind_variance_get block-local real/long bit-transfer precedent.

### Disclosures (no action requested)
- PAO view casts x6 '(real_point2d const *)' on real_point3d: rule-28 conditions met (per-site /Od 0x4cafb0 and
  0x4cc6b0; prefix layout; strip test byte-inert with exactly six C4133; strictly exact callers).
- PAO 4 new surplus COMDATs (rule-42 class): identical to January's action_charge copies; strictly exact caller
  path_add_step; link PASS both orders; January's own object references _normalize2d out of line.
- PAO 'add_step' flag name is inferred (/Od RTC names aggregates only).
- hs_runtime COMMON-pool externs (hs_global_data, hs_thread_data, hs_syntax_data, debug_scripting, hs_debug_data)
  stay consumer-local (linker_common is out of scope); debug_scripting is 'boolean' here vs 'byte []' in the
  Matching hs_globals_external.c.
- hs_runtime names differing from HCEX 2011 kept as descriptive (hs_thread_datum/hs_thread, previous_sleep_until/
  latent_sleep_until, hs_stack_frame previous/result vs parent/child_result, hs_global_datum.unused vs pad,
  _hs_syntax_node_global_bit vs _variable_bit, _hs_thread_sleeping_bit vs _latent_sleep_bit, string vs printbuffer,
  reconcile 'word global_designator' vs HCEX 'short'); _hs_thread_type_console_command is supported by the /Od
  "[console command]" string (HCEX: runtime_evaluate).
- Observation: January's long/short/string -> boolean converters return TRUE for zero/empty (sete); kept as is.
