# MANIFEST - source/objects/object_lights admission packet (01 + 02 + 03_NO_ALT), round 2 wave 2

- Base: 05255584 (worktree HEAD 434f0151 = 05255584 + one lane ledger doc; `git diff 05255584 434f0151` touches no
  file in this packet). Tree clean except the pre-existing untracked research/fifty_objects_r2_20260924/.
- Tip: `patches/01_object_lights_static_helpers.patch`, `patches/02_object_lights_symbols_json.patch`,
  `patches/03_NO_ALT_object_lights_semantic_data_entry_incomplete.patch` (LF, git index lines; see
  `patches/APPLY_ORDER.txt`). Post-images: `tree/source/objects/object_lights{.c,_rendering.h}`,
  `cfg_OL/config/{symbols.json,semantic_data_matches.json}`. Hunk bodies are identical to round-1
  owner_queue/objlights_render_debug 01/02/03_NO_ALT; only headers and line numbers were refreshed.
- Claim: `source/objects/object_lights` as r2w2_object_lights (released at the end). The two header consumers
  `source/render/render` and `source/render/render_objects` were claimed for the verification window only and
  released (no source change).

## 1. Precedents (governing rulings, quoted) and their conditions

### P1 - Storage follows cachebeta publics (canonical 05255584)
Quoted from 05255584: "12 typecast converters are static per cachebeta publics (provider link FAIL(12) -> PASS).
The object stays blocked by the retired begin_random tie." and "13 wrapper rows go static". The brief (s3) states
the rule: "cachebeta.pdb publics (January storage truth: public = external, absent = file-static)". The units
admission reason text in config/semantic_data_matches.json records the same form: "all 50 file-statics carry static
storage per cachebeta.pdb publics".

| condition (as applied in 05255584 and the units / hs_runtime packets) | holds? | evidence |
|---|---|---|
| each symbol absent from cachebeta.pdb publics | YES | none of `_should_render_lights`, `_light_attenuation`, `_cluster_get_first_light`, `_cluster_get_next_light` is public. Controls `_lights_initialize`, `_lights_render_diffuse`, `_lights_render_specular` are public. 0x128A10 sits between public `_sample_diffuse_texture` 0x128920 and `_lights_initialize` 0x128A30 with no public row. Production pdb_storage: 4 disagreements; candidate: 0 |
| no January reference outside the object (static is linkable) | YES | refcensus over all 833 build/split objects and all of build/base: each is DEFINED only by object_lights, with 0 UNDEF references elsewhere. Source grep: no other .c/.h uses them except the header prototype that 01 removes |
| storage keyword in source plus in-place `"static": true` symbols.json rows, never re-serialised | YES | 01 adds `static` to 3 prototypes (+1 moved) and 4 definitions. 02 edits 4 rows in place (lines 5138/5150/5152/5153) |
| csplit regen changes only the unit's object | YES | split_ctl (verbatim config) == build/split 833/833; split_OL vs split_ctl: only source/objects/object_lights.obj differs |
| every January function stays strict exact and code bytes are unchanged | YES | 43/43 EXACT vs split_OL; objcmp candidate vs production: every section IDENTICAL, only the 4 storage rows differ |
| object_audit symbol rows match | YES | candidate vs split_OL PASS, 71 January symbols, 0 differ. Controls: production vs split_OL FAIL(4); candidate vs build/split FAIL(4). So 01 and 02 must land together |
| the object may stay blocked (zero-credit storage landing) | YES | 05255584 landed hs_runtime storage with the object still blocked; object_lights stays NonMatching on the held Q1 surplus |
| corroboration where available | YES | HCEX.pdb: cluster_get_first_light / cluster_get_next_light `static function: true`. should_render_lights and light_attenuation have no HCEX symbol (fully inlined) |

Prior ledgers (object_lights_obj_opus_reconciliation_20260903, ..._light_attenuation_20260829) called these
"external" from the SPLIT storage class. That class is generated from config/symbols.json and is not independent
January evidence. The PDB publics are.

Additional construct in 01 (the call sites), justified independently of P1:
- House rule 13 / brief s2 rule 6 say not to hand-expand a helper. The two tests
  `lights_game_globals->render_lights && game_engine_allow_dynamic_lighting()` are a hand expansion of
  should_render_lights.
- /Od attestation (halo_cache_symbols.exe, data only):
  - should_render_lights = 0x78dab0. It reads `[0xded68c]->render_lights`, calls game_engine_allow_dynamic_lighting
    and returns a boolean.
  - Its only callers are lights_render_diffuse 0x78c6c0 (diffuse_lights_begin 0x825320, then the call, then the short
    loop over scene_point_light_count) and lights_render_specular 0x78c950 (0x825a30, then the call). The statement
    order is the same as our source.
- Double-attested against January's bytes:
  - January DEFINES the non-public 32-byte `_should_render_lights` with no referrer anywhere.
  - lights_render_diffuse and lights_render_specular reference `_game_engine_allow_dynamic_lighting` directly (the
    helper is inlined at both sites).
- Strip test (lab/tree_nocall, static without the calls): 42/43, `_should_render_lights` UNWRITTEN. VC7 emits the
  static only because the source uses it (VC7 static COMDAT law). The calls are load-bearing for a January-owned
  section. They are not decoration.

### P2 - leaf_map allow_incomplete_unit single-section data entry (config/semantic_data_matches.json, ae12a3c1)
Quoted entry: `"unit": "source/structures/leaf_map", "symbol": "_leaf_map_initialize_section",
"allow_incomplete_unit": true, "reason": "January and rebuilt profile-section data have identical 2512-byte payload,
section flags and static owner, with all 28 relocations independently resolving to the same image destinations;
objdiff undercredits this sole unmatched section."`. ae12a3c1 context: "Their 25/25 and 6/6 target function matches
remain credited; LeafMap retains its 1,420 meaningful new code bytes and verified 2,512-byte profile data", while
whole-object completion was held on a candidate-only COMDAT. Brief s4.5: a data section that is byte+relocation
identical under coff_compare but <100% in build/report.json is a scorer defect, "handled by a pinned
config/semantic_data_matches.json entry (integrator applies; single-section entries only)".

| condition | holds? | evidence |
|---|---|---|
| single-section entry (no members/group), pinned measurements | YES | symbol `_lights_section`; size 1541, relocation_count 1, normalized_sha256 527b43a1...f71 |
| identical payload, flags, owner storage | YES | object_audit `.data sym:_lights_section 1541/1541 flags eq align 4/4 ok`. Flags c0400040 on both sides. `_lights_section` static @0, four public `_object_light_*` @1528..1540 on both sides |
| relocations independently resolve to the same image destination | YES | semsnap (tools.coff_compare.section_info_resolved, cfg_OL symbols): resolved infos EQUAL. Target imports the `render_lights` literal (selected in hs); ours defines it (the '$'-literal class) |
| objdiff undercredits it, and it is the sole unmatched data | YES | build/report.json: .data 99.7411%, data 1104/2645, so unmatched 1541 == entry size (the verifier enforces equality) |
| all target functions strict exact | YES | 43/43 (leaf_map had 25/25) |
| unit held from Matching for a documented whole-object reason, credit only via explicit opt-in | YES | held on owner question Q1: candidate-only `_object_get_bounding_sphere` differs from action_vehicle's selected copy only in 2 `__FILE__` DIR32 targets. `allow_incomplete_unit: true` is set. Negative control without it fails closed (SemanticProgressError) |
| verifier credit | YES | apply_semantic_data_matches on a mini objdiff 3.3.1 project (cand vs split_OL): +1541, data 2645/2645 |

Difference from the leaf_map commit, disclosed: ae12a3c1 also added an object_admission_rejections entry for
leaf_map, because it DOWNGRADED a Matching label.
- Other allow_incomplete_unit entries on NonMatching units carry no rejection entry: objects (84414a1a) and
  editor_flying_camera (5019c186).
- Adding one here would pre-empt the open owner question Q1, so none is proposed.
- Consequence: tools/audit_object_admission.py goes 9/0/2/0 -> 10/0/2/0. object_lights joins the review queue
  (audit-coff-ownership-before-admission), like the other owner-held units render_debug, rasterizer_xbox and
  game_engine.

## 2. Target vs candidate per function (candidate vs emulated split_OL; full table: per_function_table.md)
- 43/43 EXACT. Total padded 12,608 B. Every row has equal size, relocation count and normalized sha256.
- Changed-storage rows (T/C):

| function | size | relocs | sha256 (12) | storage |
|---|---|---|---|---|
| _should_render_lights | 32/32 | 2/2 | 54bc7d0f092a/54bc7d0f092a | 3/3 |
| _light_attenuation | 32/32 | 1/1 | 3ccf5232cc3c/3ccf5232cc3c | 3/3 |
| _cluster_get_first_light | 32/32 | 2/2 | 5b9b89f6962a/5b9b89f6962a | 3/3 |
| _cluster_get_next_light | 32/32 | 2/2 | e9b3377d3ab4/e9b3377d3ab4 | 3/3 |

- The call-site functions are also exact: _lights_render_diffuse 464/464, 19/19 relocs, feec427cd870; and
  _lights_render_specular 336/336, 16/16 relocs, c33d85c770ff.
- Gate rows: gate_cand_vs_splitOL.txt (tgate shadow tree, build.ninja cflags; needed because 01 edits a header that
  gate.py --source cannot shadow). Controls:
  - gate_prod_vs_buildsplit 43/43;
  - gate_cand_vs_buildsplit 43/43;
  - gate_prod_vs_splitOL 43/43;
  - gate.py --source cand.c 43/43, objcmp IDENTICAL to the shadow-tree object.

## 3. Section-4 audit (emulated split)
| check | production | candidate |
|---|---|---|
| gate --all | 43/43 | 43/43 |
| object_audit vs split_OL | FAIL(4) storage 3/2 | PASS, 71 symbols 0 differ, all January sections ok |
| pdb_storage | 4 disagreements | 0 |
| surplus_identity | 12 COMDATs, 1 different (bounding sphere) | same 12, same 1 (held Q1; _distance3d IDENTICAL to action_vehicle) |
| provider_link, both orders | - | PASS, 33 surplus names; --baseline=production: no new surplus |
| data coff_compare (.data/.bss/.rdata) | ok | ok |
| mini objdiff 3.3.1 | code 12294/12294, 43/43, data 1104/2645 | identical; with 03_NO_ALT 2645/2645 |
| header consumers render / render_objects | - | 13/13, 22/22, objcmp IDENTICAL to build/base |
| fake_match_scan | 0 | 0 |
| /W3, /W4 | C4133 x1 | C4133 x1 (same, line +2) |
| parks / rejections for the unit | none | none |

## 4. House-rule review
- `void` on its own line and one parameter per line in every changed prototype/definition.
- The static prototype sits in the object's own .c prototype block. The public header loses a prototype that
  belongs to no public function (rule 9).
- No casts, dummies, parentheses, pragmas or flags are added. The call replaces a hand-expanded helper (rules 6/13).
- The definition position of should_render_lights in the "public code" block is January's .text order
  (file_offset 1214992). The same holds for the existing statics shade_scalar and shade_vector2d.
- The declaration count before each function is unchanged: one name moves from the header into the .c.
- The strip test does not apply to storage. The call is load-bearing for emission (lab above).

## 5. Credit
- Meaningful/padded code: 0 B new (all 43 were already exact).
- Data: +1,541 B (03_NO_ALT, allow_incomplete_unit).
- Admission: removes 4 pdb_storage disagreements and the 4 storage rows object_audit would report against a
  cachebeta-faithful split. Object stays NonMatching. No status flip, no parks, no new functions.
- Newly exact functions: none.

## 6. Held items / reopen
- Q1 (owner): admit object_lights' per-TU `__FILE__` surplus `_object_get_bounding_sphere` (round-1 RULING.md Q1).
  - On YES, replace 03_NO_ALT with round-1 03_YES and add 04_YES (config.json Matching): +1 object.
  - Reopen the object otherwise only with first-party evidence that removes the rule-6 identity gap.
- Optional round-1 07 (5 tentative COMMON definitions) is not part of this task.
