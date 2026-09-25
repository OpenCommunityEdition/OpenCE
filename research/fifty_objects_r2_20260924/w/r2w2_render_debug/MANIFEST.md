# MANIFEST - source/render/render_debug, admission packet 05 + 06 (round 2, wave 2)

- Worktree `C:\halo-worktrees\claude-fifty-objects-r2-20260924`. Base HEAD `434f0151` (tree = `05255584` plus a
  ledger doc). No tracked file edited. `git status` stays at the pre-existing `?? research/fifty_objects_r2_20260924/`.
- Claim: `source/render/render_debug`, label `r2w2_render_debug`.
- Packet (apply one at a time, in this order, from the repo root):
  1. `patches/05_render_debug_genuine_helpers_and_vehicle.patch` (render_debug.c + units/vehicle_datum.h)
  2. `patches/06_render_debug_tentative_debug_toggles.patch` (render_debug.c)

  No symbols.json, config.json, parked.json, semantic entry or header other than vehicle_datum.h changes.
- Verdict: **ADMISSION_FIX_PROPOSED, zero credit.**
  - 36/36 functions and all data are already exact in production. The packet removes admission blockers:
    three rule-6 hand expansions, a caller-local vehicle view with opaque spans, and six consumer-local externs
    that no TU defines.
  - The object does **not** become complete. Completion still needs the owner's Q2d ruling (D0 or D1 on the
    invented `render_debug_globals_definition` aggregate) and then patch 08 (status flip). See Held.

## 1. Precedents and their conditions

### 05: folded header-inline COMDAT exception (rule 6), plus genuine owner types

Governing text, `docs/campaign_house_rules.md:33-40` (added by 3c74fa36):

> Rule 6 has a later, narrow owner-approved exception for folded header-inline helpers (see
> `docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md`, section 3). A translation unit may emit a
> shared-header `__inline` COMDAT, including `_point_from_line3d`, only when the emitted copy is byte-identical to
> January's selected copy, its caller is strictly exact, the full ownership and build sweep has zero regressions,
> and the selected-provider link passes. January's per-object folded survivor is not an absolute emission
> boundary. Do not hand-expand or hand-copy the helper to evade this check.

Owner ruling #2, `docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:69-75`:

> **All-inlined header-inline COMDAT class ADMITTED**, on proof: "Require a genuine shared-header __inline,
> byte-identical COMDAT, strict-exact caller, full ownership/build sweep, and zero regressions. No hand expansions."

| condition | holds? | evidence (files in this slug) |
|---|---|---|
| a genuine shared-header `__inline` | YES | `point_from_line3d` is real_math.h's own `__inline` (real_math.h:1080). The packet does not touch real_math.h. The three calls are plain calls, with no copy and no expansion. |
| no hand expansion | YES | The patch **removes** the three hand expansions (render_debug_vector, render_debug_tick with its `negative_tick_size` temporary, render_debug_line_offset). |
| emitted copy byte-identical to January's selected copy | YES | `_point_from_line3d`: candidate size 48, 0 relocs, sha 9b763841... equals January's only definer, action_charge.obj (`section_infos_equal` True; pfl3d_identity.txt). surplus_identity: 12/12 IDENTICAL (surplus_c06.txt). |
| caller strictly exact | YES | Every caller is EXACT: render_debug_vector 192/13 relocs, render_debug_tick 112/1, render_debug_line_offset 128/2 (gate_c06.txt). In fact all 36 rows are EXACT. January render_debug holds no out-of-line reference to the helper (the all-inlined class; pfl_census_split.txt). |
| full ownership and build sweep, zero regressions | YES | Every one of the 612 TUs in build.ninja was compiled from a shadow tree. Control tree 612/612 IDENTICAL to build/base. Candidate 611/612 IDENTICAL; the only differing object is render_debug.obj, which is 36/36 with object_audit PASS. The vehicle_datum.h includers actor_moving, physics and vehicles are byte-identical (sweep_*_vs_control.txt). |
| selected-provider link passes (both orders, per worker brief) | YES | provider_link (candidate): SELECTED-PROVIDER LINK PASS for all 39 surplus rows. The new `_point_from_line3d` passes against action_charge in both orders (plink_c06_full.txt, plink_c06_new.txt). nodup_census: 26 SELECT_ANY definers, 0 NODUP; the candidate copy is ANY. |

Same-class landings already in canonical:
- hs_runtime `_render_debug_trigger_volumes` (05255584, this lane). January hs_runtime has no reference either; the
  base object emits the identical copy.
- rasterizer_lights (the 2026-09-22 reconciliation).

Form: neither landing carries a source comment for the COMDAT, so 05 has none. Disclosure goes in the commit
message (suggested text in section 7).

The vehicle half of 05 is house rules 4/5 and 9 plus the law (j) admission blocker "caller-local struct views with
opaque spans":
- The TU-private `struct vehicle_runtime_datum` / `render_debug_vehicle_data` (`unknown426`, `unknown428[4]`,
  `unknown438[0x40]`) and its four offset checks are deleted.
- `render_debug_player` uses the genuine owner type `struct vehicle_datum` (units/vehicle_datum.h: speed 0x42C,
  slide 0x430, turn 0x434, stuck_mass_point_flags 0x478; the same offsets the deleted checks pinned).
- It uses a new typed accessor `vehicle_datum_try_and_get`, defined in that header next to its sibling
  `vehicle_datum_get`, with the same shape as bipeds.h `biped_try_and_get` and the other `*_try_and_get` macros.

### 06: tentative COMMON definitions (the objects.c precedent)

Precedent: objects.c (Matching) defines its hs debug toggles as bare tentative definitions
(`boolean debug_objects_names;` ... fb0d8145, "Declare .comm data in TUs instead of headers"). units.c and
structure_visibility.c (both Matching) do the same. players.c's COMMON landing (6e3e2d35) used per-symbol pool,
referencer and HCEX evidence.

Limit: the 2026-09-22 reconciliation (`docs/common_pool_ordering_20260922.md`) excluded a 116-definition wave
because pool adjacency alone cannot prove TU ownership, and requires bare uninitialised definitions whose COFF
class is checked against January's.

| condition | holds? | evidence |
|---|---|---|
| January pooled the symbol as COMMON | YES | Each of the six is a January linker_common `.bss` record: sections 50..55, 1 byte each (census_debug6_split.txt). |
| the defining TU is proven per symbol, not by adjacency | YES | See the list after this table. |
| same referencer shape as the precedent | YES | objects.c's toggles have exactly the referencers {hs_globals_external, objects}. Board census over 74 such {hs_globals_external, X} records (hs_pair_census.txt): canonical defines 15 of them, ALL in X (units x8, structure_visibility x2, objects x5, all Matching), 59 have no definer, **0** are defined outside X. |
| bare, uninitialised form (no `= 0`) | YES | `boolean debug_bsp;` etc., the same spelling as objects.c, units.c and structure_visibility.c. No comment, as in those precedents. |
| COFF class/size equals January's record | YES | The only delta vs c05 is six external section-0 symbols going from value 0 to value 1 (COMMON 1 = the record size; common_symbols_c05_c06.txt). All sections are byte-identical (sweep_c06_vs_c05.txt). |
| storage | YES | All six are cachebeta PUBLICs (0x4EE350..0x4EE355), so they are external, not static. |
| links cleanly | YES | c06 + January linker_common.obj, both orders: 0 LNK2005/LNK1169. c06 + base hs_globals_external, both orders: 0 duplicates and all six resolved. Control: production and c05 leave all six unresolved (linkprobe). |

Per-symbol proof that render_debug is the definer:
- (i) render_debug.obj is the only non-hs referencer.
- (ii) In the 2001-09-25 retail cache.map, render_debug.obj is not linked, and all six toggles are absent. Yet
  hs_globals_external.obj is linked, still carries the "debug_bsp" name literal, and neighbouring pool records
  persist. So hs_globals_external never defines them.
- (iii) The toggles appear with render_debug: debug_permanent_decals is new in the 09-25 beta, together with its
  only user, render_debug_structure_decals. The 08-15 beta has 5 toggles and no render_debug_structure_decals.
- (iv) In the later /Od build, the six are interleaved byte by byte with render_debug's own cache variables
  (0xf0afb0..0xf0afc0). Each is read by exactly one render_debug function (od_hsglobals.txt, od_refs.txt).

Pool adjacency (render_objects 48-49 -> the six -> render 56) is consistent with this, but nothing relies on it.

Residual judgement for the integrator (as the precedent audit noted): 06 is optional hygiene. The /Od build is a
later revision, and the evidence standard is per-symbol, not the excluded adjacency standard. Canonical has 15
uncontradicted rows of this exact shape and no counterexample. The rasterizer_frame_statistics counterexample is a
single-referencer, eponymous-struct case, and its owner test pins an import.

## 2. Source evidence per changed construct

| construct | January bytes | /Od (later build, shape only) | other |
|---|---|---|---|
| render_debug_vector: `point_from_line3d(point, vector, size, &end_point)` | EXACT (192, 13 relocs), unchanged sha | 0x84a920: asserts at lines 388/389/390 (zero anchor drift vs our match_assert lines), then a REAL call to 0x42e0d0 (point, vector, size, &end_point) | - |
| render_debug_tick: two calls, the second with `-tick_size`; `negative_tick_size` local deleted | EXACT (112, 1) | 0x84a510: two REAL calls, the second's t negated by xorps; frame holds only point0/point1 | - |
| render_debug_line_offset: `point_from_line3d(p0/p1, global_up3d, offset, &pointN)` | EXACT (128, 2) | 0x848640: two REAL calls with global_up3d (0xa31e2c) | - |
| dead `REAL_MATH_EXTERNAL_{ARCTANGENT,DOT_PRODUCT3D,PLANE3D_DISTANCE_TO_POINT}` removed | January, production and candidate reference none of the 3 symbols; no COMDAT is emitted for them | - | render_debug.c uses none of the three helpers |
| vehicle_datum owner type + `vehicle_datum_try_and_get` | render_debug_player EXACT (272, 16) | - | vehicle_datum.h is the genuine private owner (vehicles.c/physics.c/actor_moving.c) |
| six `boolean debug_*;` tentative definitions | linker_common 50..55 | interleaved with render_debug .bss | 2001 maps; cachebeta publics |

## 3. Target vs candidate per function

- All 36 January functions are EXACT in production, c05 and c06.
- The size/relocs/normalized-sha rows are identical to production for every function (gate_prod_obj.txt ==
  gate_c06.txt).
- Rows of the 4 edited functions (padded size / relocs / sha prefix):
  - `_render_debug_vector` 192 / 13 / 70cfea13919c
  - `_render_debug_tick` 112 / 1 / 7bf04865f13f
  - `_render_debug_line_offset` 128 / 2 / b7c236b7adcb
  - `_render_debug_player` 272 / 16 / a45fe6649ea6
- The canonical gate.py agrees: 36/36 on a lab copy that defines the macro locally (gatepy_lab_c06.txt). That copy
  is needed because gate.py's /I order cannot shadow a modified shared header; it never lands.

## 4. Data, symbols, surplus

- Data (coff_compare via object_audit): all 27 January data sections ok (26 .rdata literals/reals plus .bss
  `_render_debug_globals` 29708) (audit_c06_data_rows.txt). tinfo --data: only section renumbering from the one
  inserted .text COMDAT.
- object_audit (build/split; symbols.json unchanged, so no emulated split is needed): **PASS**, 63 January symbols,
  0 differ. The only new candidate-only row is `+.text _point_from_line3d 48`.
- pdb_storage: 63 split symbols, 0 disagreements.
- surplus_identity: 12 candidate-only code COMDATs, 0 not identical (production has 11).
- provider_link: PASS for all surplus rows, both orders.
- objdiff 3.3.1 mini project: production, c05 and c06 are identical. Code 8235/9448 (34/36; the two known $L
  under-credit rows, `_render_debug_add_cache_entry` 93.6 and `_render_debug` 94.2). Data 30236/30236.

## 5. House-rule review of the changed lines

- Rule 6: genuine helper calls replace hand expansions; the exception conditions are met (section 1).
- Rules 4/5: a typed object-access macro in the owner header replaces a caller-local cast view.
- Rule 9: no prototypes are added.
- Rule 19: `real` throughout.
- Rules 7/8: no invented names except the descriptive accessor macro, which follows the header's own
  `vehicle_datum_get`.
- No filler, no decoration, no dead local. The deleted `negative_tick_size` is also absent in /Od.
- No pragma, volatile, cast trick or flag change.
- Single-line helper calls match render_debug.c's own style (`set_real_point3d(...)`, `cross_product3d(...)`) and
  canonical `point_from_line3d` calls (action_charge.c, action_vehicle.c).
- /W3: +1 C4244 at real_math.h:764 (arctangent's inline body is now visible); same class as the 11 real_math.h C4244
  lines every includer gets. C4013 x5 pre-existing, not introduced.
- fake-matching STRIP TEST: nothing to strip. The patch deletes views, expansions and defines; it adds no
  decoration.

## 6. Tests run

gate_obj on shadow-compiled objects (prod/c05/c06); gate.py (production; lab copy); a 612-TU shadow sweep (control,
c05, c06); object_audit; pdb_storage_emu; surplus_identity_emu; provider_link (full and --baseline); nodup_census;
linkprobe (linker_common, hs_globals_external, both orders); mini objdiff 3.3.1 report; /W3 census; odbuild
(0x84a920, 0x84a510, 0x848640, 0x42e0d0, 0x848d00); /Od hs-table and data-reference census; 2001 map census;
`git apply --check` (05 rc 0, 06 rc 0); sequential `git apply --directory` into LF and CRLF copies (== candidate
modulo EOL).

Not run: ninja or pytest. Nothing tracked changed, so the integrator runs them with the batch. Tests that could see
this: test_campaign_emitted_symbols uses `_point_from_line3d` only as a guard-unit-test name; the render_debug
runtime tests use frozen scratch objects; no test pins the six toggles as imports.

## 7. Credit and landing notes

- Credited: **0** functions, 0 code bytes, 0 data bytes, 0 objects. It is zero-credit admission hygiene; the
  already-exact rows are not re-counted.
- Suggested commit disclosure:

  > render_debug: /Od-attested point_from_line3d calls replace three hand expansions. The new SELECT_ANY
  > `_point_from_line3d` COMDAT is byte-identical to January's action_charge copy, its callers are strict-exact,
  > the sweep shows 0 regressions and the provider link passes (rule-6 folded-inline exception, ruling
  > 20260921 #2). The genuine vehicle_datum owner type and `vehicle_datum_try_and_get` replace the caller-local
  > vehicle view. The dead REAL_MATH_EXTERNAL defines are removed. The six hs toggles become render_debug's bare
  > tentative definitions (January linker_common records 50-55; per-symbol map and /Od evidence; objects.c
  > precedent).

- Interaction: math/real_math is RESERVED for the Codex actor_combat packet. If that packet changes
  `point_from_line3d`'s body, re-run the surplus identity for every emitter, this one included.
- No other in-flight packet touches render_debug.c or vehicle_datum.h. r2w2_vehicles edits only vehicles.c and
  symbols.json.

## 8. Held (object completion) and reopen criteria

- **Q2d (owner):** the invented `render_debug_globals_definition` aggregate with pad members
  `opaque_after_game_time[2]` / `opaque_after_entry_count[2]` (law (j) signature).
  - D1 would give seven `= 0` descriptive statics, with a lab in
    research/fifty_objects_20260925/w/owner_queue/objlights_render_debug/lab/.
  - D0 would admit the aggregate.
  - Still no first-party names:
    - the 2001-08-15 and 2001-09-25 cachebeta.map "Static symbols" sections list only render_debug FUNCTIONS
      (10/11 rows), no data;
    - cachebeta publics, HCEX and the atlas: none.
  - Reopen: an owner answer D0 or D1, then land patch 08 (config status flip), which makes render_debug Matching
    (+1 object). A first-party name source for the cache variables would remove the need for a ruling.
- Advisory (not in the packet):
  - vehicle_datum.h's header comment still says "shared by vehicles.c and physics.c" (already stale: actor_moving).
  - render_debug.c keeps the stale "symbols in this file" placeholder comment.
  - 5 C4013 implicit declarations (ai_debug_render, render_debug_object_damage, render_debug_recording,
    collision_debug_render, render_debug_fog_planes) need owner-header work with a declaration-count sweep.
