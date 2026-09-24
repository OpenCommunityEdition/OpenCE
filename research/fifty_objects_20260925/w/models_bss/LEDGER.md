# models_bss ledger (wave 3, base 954eebd2)

Task: whole-object admission of source/models/models (12/12 exact). Rejection: .bss
symbol layout (4 default symbols at different offsets). January offsets:
function_values 0 (16), change_colors 16 (48), effect 64 (40), region_permutation_indices 104 (32).
Ours at base: region 0, effect 32, function_values 72, change_colors 88 (name-hash order of
uninitialised file statics).

## Evidence pass (before any probe)

E1. cachebeta publics: none of the four is public (file-static or static local). Also NOT
    public: `_render_model_section` and `_model_geometry_part_build_tangent_matrices`
    (pdb_storage: 2 disagreements; split/ours both external).
E2. HCEX.pdb (SHIP) models compiland, `-sym render_model`: 
    `Data: static, [0171C75C], Static Local, Type: float[0x4], default_function_values`.
    FIRST-PARTY: default_function_values is a STATIC LOCAL of render_model, real[4].
    The other three have NO symbol record anywhere in HCEX.pdb (-g, -p, -sym, -compiland),
    but their storage is there: -ps shows the next named datum (mp_sound_queue) at +0x8C.
    HCEX_Release.pdb identical: default_function_values static local at 0x24A58D0, next
    named datum integer_origin2d at +0x88 = 136 bytes -> the 120 unnamed bytes are the
    other three defaults. HCEX order (VC16 PPC, not transferable): fv, effect, region, change.
E3. HCEX_Release: `static void model_geometry_part_build_tangent_matrices(struct gbxmodel_geometry_part *)`
    (static, wasinlined) -> first-party static, agrees with cachebeta (E1).
E4. halocea (research/halocea/src/data/default_render_model_*.c) is a THIRD-PARTY decompile of
    HCEX_Release.xex; its names for the three unnamed defaults are not in either HCEX PDB, so
    they are not first-party (only default_function_values is).
E5. /Od build render_model = 0x700750 (aligned frame; od_700750.txt). Later revision with profile
    scopes. Default pointer assignments in USE order: region 0xd85fd4, effect 0xd85ffc,
    change_colors 0xd8602c, function_values 0xd86068 (+ a later lighting default 0xd86080).
    January render_model uses them in the same order (relocs 0xa6 region, 0xb4 effect,
    0xc2 change, 0xd0 fv).
E6. OBSERVATION: January's .bss order (fv, change, effect, region) is EXACTLY THE REVERSE of the
    use order (region, effect, change, fv). Hypothesis H1: all four are static locals declared
    at their point of use in render_model and VC7 lays uninitialised static locals out in reverse
    declaration order (needs probe; the landed law covers only file statics / '= 0').

Baseline: gate 12/12 EXACT (base.obj).

## Probes (lab = current tree names; placement/initialiser only; offsets fv/change/effect/region)

January target: fv 0, change 16, effect 64, region 104.
VC7 C static locals are named `?name@?N@??render_model@@9@9` (N = scope ordinal: ?1 function
scope, ?7/?8/?9/?L@ = the four if-blocks in use order). Relocations name the owning symbol, so a
static-local form needs a symbols.json rename to that decorated name (else 1 reloc row differs).

| probe | form | fv | change | effect | region | gate |
|---|---|---|---|---|---|---|
| base | 4 file statics, uninit | 72 | 88 | 32 | 0 | 12/12 |
| L1 | 4 static locals in their if-blocks, uninit | 32 | 48 | 96 | 0 | 11 (layout) |
| L2 | 4 static locals in if-blocks, `= {0}` | 120 | 72 | 32 | 0 | 11 (layout) -> declaration(=use) order region,effect,change,fv = REVERSE of January |
| P1 | 4 file statics `= {0}`, order fv,change,effect,region | **0** | **16** | **64** | **104** | **12/12; object_audit PASS** |
| P2 | fv static local (fn scope, uninit) + 3 file statics `= {0}` | 0 | 16 | 64 | 104 | 11 (only fv reloc NAME differs; symbols.json rename would fix) |
| V1 | fv static local fn-scope uninit + 3 file uninit | 120 | 72 | 32 | 0 | layout |
| V2 | fv static local in its block uninit + 3 file uninit | 32 | 88 | 48 | 0 | layout |
| V3 | 4 static locals fn-scope uninit | 120 | 72 | 0 | 40 | layout |
| V4 | fv fn-scope + 3 in blocks, all uninit (HCEX-shaped) | 120 | 32 | 80 | 0 | layout |
| V5 | V4 with the 3 block ones `= {0}` | 0 | 88 | 48 | 16 | layout (uninit first, then decl order region,effect,change) |

Findings: (1) law confirmed for static locals too: uninit = name(decorated)-hash order first,
`= {0}` = declaration order after. (2) January's order is the exact REVERSE of use order, so
no zero-initialised at-point-of-use static-local form reproduces it; every static-local form
that could reproduce it needs uninitialised members whose (unknown) names hash into place.
(3) The only name-independent reproduction is `= {0}` in declaration order fv,change,effect,region
(file scope, P1; or with fv an uninit static local, P2).

## Pre-registered hypothesis H2 (written BEFORE compiling)

First-party naming convention: HCEX names the fv default `default_function_values` =
"default_" + the render_model PARAMETER name `function_values` (HCEX param names:
region_permutation_indices, change_colors, function_values, model_effect). H2: the other three
follow the same convention: `default_region_permutation_indices`, `default_change_colors`,
`default_model_effect` (types unchanged). Primary form (HCEX-shaped, E2): fv a static local at
render_model function scope, the three static locals inside their `if (!param)` blocks (which is
why HCEX has no record for them), ALL UNINITIALISED (HCEX shows no initialiser; the landed law
says uninit = name-hash order). Controls, reported whatever the outcome: the same four names as
4 file statics / 4 fn-scope static locals / 4 block static locals. ONE name set only; no
alternative spellings will be tried (anything else would be hash steering).

H2 RESULT (pre-registered, one name set): REFUTED in all four forms
(h2_hcex fv120/change32/effect80/region0; h2_file fv48/change0/effect64/region104;
h2_top fv80/change32/effect96/region0; h2_blocks fv32/change48/effect96/region0).
Oracle-only (never landing): block statics renamed a/b/c/d and d/c/b/a reorder -> static-local
order IS decorated-name dependent (L1 == h2_blocks was a coincidence).

## New first-party evidence: the defaults are IN-BLOCK static declarations in later builds

E7. HCEX -lines render_model (hcex_lines_render_model.txt): if/assign line pairs
    region 124/128, effect 134/138, change 144/148, fv 154/158, centroid 164/166.
    Every storage-needing default has TWO extra lines between `if` and the assignment; the
    centroid default (no storage: &node_matrices->position) has none. = `static <type> x;` + blank.
E8. /Od build (0x700750): each default is wrapped in a profile scope whose end-assert line is
    478 (region), 488 (effect), 498 (change), 508 (fv), 516 (centroid), 526 (lighting - a later
    default that ALSO needs storage, 0xd86080). Storage blocks are 10 lines, the centroid block 8:
    the same +2 in an independent build. /Od storage order = use/declaration order.
E9. => later first-party lineage: each default is a static local declared inside its own
    `if (!param)` block, no initialiser visible (HCEX Static Local, no init). With that form VC7
    gives: `= {0}` -> use order region,effect,change,fv (L2; the REVERSE of January) ; uninit ->
    decorated-name hash order (L1/V4/h2_*). January's order therefore = the hash order of the
    AUTHENTIC decorated names `?<name>@?N@??render_model@@9@9`; only fv's name is first-party
    (`default_function_values`, HCEX). The other three names exist in no first-party source
    (HCEX.pdb/HCEX_Release.pdb have no record; cachebeta publics none; 2001-08-15 and 2001-09-25
    linker maps list static FUNCTIONS only - checked, no static data rows).
E10. 2001-08-15 + 2001-09-25 cachebeta.map / cache.map (earlier-map-archives): 
    `_model_geometry_part_build_tangent_matrices` is in the **Static symbols** section (static
    function) in all three maps; `_render_model_section` is in NO map section and not in cachebeta
    publics -> static data. HCEX_Release agrees for the tangent helper (static, wasinlined).
    January emits the 16-byte helper with ZERO references anywhere (no reloc in
    model_build_tangent_matrices, no split object imports it) -> by the VC7 static-COMDAT law
    January's source DEFINED it static AND CALLED it (inlined empty): our loop has the orphan
    `part` fetch with no call -> restore `model_geometry_part_build_tangent_matrices(part);`.

## Status of the .bss question after E7-E10
- P1 (file statics `= {0}` in January order) is exact + audit PASS, but its form contradicts the
  double-attested in-block static-local form (E7/E8) and HCEX's static-local storage for fv
  (E2); the declaration order fv,change,effect,region has no first-party support (it is the
  reverse of use order and of the in-block declaration order). => matching-only construct under
  the rejection's reopen clause; report as owner option only, do not propose as natural.
- The authentic-form reproduction needs the three lost names (hash) -> blocked.

## C1 - zero-credit storage corrections (independent of the .bss question)
Hypothesis (E1/E3/E10 + static-COMDAT law): `render_model_section` is file-static; the tangent
helper is static AND called (inlined empty) from model_build_tangent_matrices' part loop.
Change: cand_C1.c = base + `static` render_model_section; `static` helper with a private
prototype; `model_geometry_part_build_tangent_matrices(part);` in the loop (the orphan `part`
fetch now has its use); hdr/models.h drops the public prototype. config_C1/symbols.json: the two
rows get `"static": true` (in-place line edits).
Measured:
- csplit (config_C1 -> split_C1): 833 objects compared, ONLY models.obj differs.
- lab gate vs split_C1: 12/12 EXACT (--forbid-emitted-symbol _point_from_line3d passes);
  the static helper is still emitted (16 B) exactly as January (static-COMDAT law).
- audit2 (AUDIT_SPLIT_ROOT=split_C1): only the 4 .bss offset DIFFs remain (was 4 as well; the
  two storage rows now agree). pdb_storage_cand: 0 disagreements / 30 (was 2).
- Emission order: model_build_tangent_matrices now defers to the END of .text (it calls a
  later-defined static) = January's order (0x114150 is the last function); base emitted it before
  the helper. Independent corroboration of the restored call (wave-1 definition-order law).
- surplus_identity: _negate_vector3d/_set_real_point3d IDENTICAL; provider_link PASS (11 rows).
- fake_match_scan: 0 leads.
- models.h consumer sweep (shadow header in lab tmpdir, #error control proves it was used for
  every TU): first_person_weapons, model_animations, objects, render_objects, render_sky -
  objeq2 (owner-keyed sections + symbols, $L labels ignored) IDENTICAL vs the same TU compiled
  with the real header. Only compiler $L label numbers shift.

## /Od attestation of the restored call (E11)
/Od 0x6fd230 = model_build_tangent_matrices: frame 0xc = geometry_index [ebp-4] (short),
geometry [ebp-8], part_index [ebp-0xc] (short); inner loop `push 0x84; ...; call tag_block_get_element;
push eax; call 0x6fd530` where 0x6fd530 is a 5-byte empty function (the helper). So the helper IS
called, with the element fetch as a NESTED ARGUMENT (no `part` local). Final C1 uses exactly that
spelling (new/models.c = models_v2): release bytes IDENTICAL to the `part`-local spelling
(objeq2), gate 12/12, pdb 0 disagreements, provider link PASS, fake scan 0.

## FINAL (wave 3)
- production change = C1 only (zero credit, storage/ownership): patches/C1_models.c.patch,
  patches/C1_models.h.patch (SHARED HEADER; consumers first_person_weapons, model_animations,
  objects, render_objects, render_sky + models.c: all IDENTICAL in the shadow sweep, #error
  control), config/symbols.json (2 in-place `"static": true` edits, lines 4771 and 22672;
  diff in patches/C1_symbols.json.diff). git apply --check clean; applying reproduces
  new/models.c and new/models.h byte for byte. csplit emulation: only models.obj changes.
- object_audit after C1: FAIL(4) = the four .bss offsets only (unchanged blocker).
- OWNER OPTION (not proposed as natural): patches/OWNER_OPTION_P1_bss_zero_init.patch on top of
  C1 -> gate 12/12, object_audit PASS 30/30, pdb 0. Strip test: removing the four `= {0}` returns
  to hash order (FAIL 4) - the initialisers are load-bearing, and their form (file scope, order
  fv,change,effect,region = reverse of use) contradicts E7/E8/E2 (in-block static locals).
- Verdict: FUNCTIONS_EXACT_OBJECT_BLOCKED on first-party names of three in-block statics.
- Reopen: (1) first-party names for the region/effect/change-colour defaults (any symbol file
  with static-data or block-scope records of models.c) -> write all four as in-block
  UNINITIALISED static locals (HCEX/Od form), symbols.json decorated names
  `?<name>@?N@??render_model@@9@9` (N from the block ordinal; fv block = ?L@ in our structure),
  and check the offsets; or (2) owner admits the P1 zero-initialised file-scope form.
- Do not repeat: all 24 declaration permutations of uninit file statics (salvage); P1/P2/L1/L2/
  V1-V5/H2 forms above; hash-steering names (forbidden).
