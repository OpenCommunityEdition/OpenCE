# interface/interface LEDGER (worker started Wed Sep 23 19:31:37 PDT 2026)

## Baseline (2026-09-23)
- gate: exact 17 / residual 1 (_interface_get_weapon_hud_index 448/448, 20/20 relocs). Only diff: final
  `*flashlight_power = flashlight` copy -- January int copy (mov edx,[ebp-4]; mov [eax],edx, pointer loaded FIRST)
  at both duplicated exits; ours fld/fstp with value loaded first. Else-branch assignment into [ebp-4] is int copy in both.
- Prior ledger docs/object_matching_logs/interface_obj_weapon_hud_best_fuzzy_20260912.md: tested /Op, "declaration
  ordering", split init, explicit early return -- all inert. Park reopen: independently attested source that
  naturally selects the integer copy.
- /Od readout (halo_cache_symbols.exe fn 0x63dfa0, saved od_63dfa0.txt): locals player_index[-4], flashlight(real)[-8]
  initialised 0.0 FIRST, weapon_hud_index[-0xc]=NONE SECOND; player[-0x10]; perspective short[-0x14];
  weapon_index[-0x18]; later-revision boolean byte[-0x19] (not in January - Jan exits directly on non-gunner);
  unit[-0x20]; parent[-0x24]; seat[-0x28]; weapon[-0x2c]; weapon_definition[-0x30] (named pointer local).
  Final store: pointer loaded first then value (movss). Single exit.
- HCEX: `static long interface_get_weapon_hud_index(float *)`, param flashlight_power; no locals listed.
- cachebeta publics: _interface_get_weapon_hud_index NOT public -> file-static in January (pdb_storage flags it).

## Probe p1 (flashlight declared+initialised before weapon_hud_index, /Od order) -> INERT (identical residual).

## Lab findings (scratch/w/interface/lab*.c, lab.py = unit cflags standalone compile)
- VC7 float scalar copy matrix (lab7): int-register copy is chosen whenever the SOURCE is an indirection
  (*in) or the DESTINATION is direct (global/local). x87 fld/fstp is chosen exactly when SOURCE is a direct
  operand (local/param/global) AND DEST is an indirection (*out, *gp). Our final `*flashlight_power = flashlight`
  is direct->indirect => x87. January's int copy therefore implies the source is NOT a plain direct scalar
  local at IL level (or dest not an indirection).
- Address-taken float local (lab5 t12) still x87 -> "memory variable" is not the trigger. /Op inert (lab4).
- A 4-byte STRUCT local copied by struct assignment (lab5 t13) reproduces January exactly (pointer first,
  int copy, both tails) -- but that is the rejected aggregate facade unless a genuine January struct exists.

## KEY LAB FINDING (lab15.c): storage class decides the copy template
- `static void k2s(real *a, real *b){ real x=*b; f(1); *a=x; }` -> INT copy `mov edx,[ebp-4]; mov eax,[ebp+8]; mov [eax],edx`
  while the identical extern k2e -> `fld [ebp+0xc]; mov edx,[ebp+8]; fstp [edx]`.
- Corroborated on an EXACT board function: damage.c static object_damage_shield copies `*shield_damage_reference =
  shield_damage` via INT with no null check (dmg/dA stripped variant still INT).
- January evidence that interface_get_weapon_hud_index is FILE-STATIC: absent from cachebeta publics
  (pdb_storage.py flags it: PDB-public False), and HCEX.pdb reports `static long interface_get_weapon_hud_index(float *)`.
- Census (copy_census.txt): other INT copies in exact fns come from null-checked / register-held pointers.
=> Hypothesis H2: declare the function `static` (its genuine January storage), prototype local to interface.c.

## Probe p2 (H2): `static` storage -> EXACT 18/18
- change: definition `static long interface_get_weapon_hud_index(real *flashlight_power)`; static prototype added in
  interface.c "/* ---------- prototypes */" section; public prototype removed from interface.h (probe used a
  modified header copy scratch/w/interface/hdr/interface.h via absolute include path, measurement only).
- result: gate `== exact 18 residual 0 unwritten 0`; alndiff: 144/144 insns, no normalized differences.
- Strip test: no decoration involved; the only change is January's attested storage class (cachebeta publics +
  HCEX `static`). Plain ordinary C.
blast part1 (10 consumers): all IDENTICAL-ROWS (see blast_part1.txt)
blast part2 (8 consumers + terminal): REGRESSION source/rasterizer/rasterizer_frame_statistics
  _rasterizer_frame_statistics_draw (4176) EXACT -> residual [sha] when interface.h loses the prototype.
  All other 17 consumers IDENTICAL-ROWS (rows only; byte-level check pending).
Oracle on rasterizer_frame_statistics_draw (mod header): +1/+2 `extern long` dummies residual, +3 EXACT, +4..+8 residual
  -> pure declaration-count tie with a window of exactly one value; removing the named-param prototype = -3 units.
  Same-shape dummy prototype with NAMED param restores EXACT; unnamed-param prototype does not.
  (dummies are ORACLE ONLY, never landed.)

## Probe p3: p2 + profile_* six data definitions non-static (January PDB-public True for all six) -> EXACT 18/18.
## Probe p4: p3 + render_debug_profile and interface_splitscreen_render `static` (prototype + definition;
   January PDB-public False for both) -> EXACT 18/18.
Weights (oracle, rasterizer_frame_statistics_draw window): `extern long x;`=1, `void f(void);`=2, `long f(real *p);`=3,
  `long f(real *);`(unnamed) != 3. Window = production count exactly (net 0); +1,+2,+3(3 void protos -> +6-3),... residual.
Audit p4: 18/18 strict; object_audit sections all ok; symbols: 3 storage DIFFs vs split (split=2 because symbols.json
  lacks "static":true for _interface_get_weapon_hud_index, _interface_splitscreen_render, _render_debug_profile; January
  PDB-public False for all three -> symbols.json static flags are the correct fix). profile_* data storage now matches (2/2).
  Surplus identical to base (2 COMDATs sine/cosine identical to actor_combat; provider_link PASS all 17).
  Data: .data (profile values) strict EQUAL; all .rdata literals EQUAL.

## Probe p5: p4 but ORIGINAL interface.h (public prototype kept) + static prototype/definition in interface.c
- EXACT 18/18, no header blast radius (header untouched). /W3 silent; /W4 reports C4211 "nonstandard extension used:
  redefined extern to static" -> owner ruling needed (public prototype for a January-static function).
- Wider oracle scan: frame_statistics draw exact ONLY at net 0 (tested net -3..+44 via extern-long dummies).
- Genuine interface.h additions available: interface_dispose(void)=2, interface_dispose_from_old_map(void)=2,
  interface_draw_fullscreen_overlays(void)=2 (January game.obj/render.obj import them; our game.c/render.c carry
  consumer-local prototypes). No genuine weight-3 combination -> -3 cannot be balanced without steering.
- January cross-object imports of interface.c symbols: game.obj (_interface_dispose, _interface_dispose_from_old_map),
  hs.obj (_profile_graph_toggle), render.obj (_interface_draw_fullscreen_overlays). profile_* data: public, unreferenced.
Frame-stat robustness probe: making model_*/transparent_model_* field signedness consistent (both directions, lab
  header copies rfs_X.h / rfs_Y.h) is INERT -- draw exact at production count, residual at -3. Pure count tie. Stopped.
objdiff 3.3.1 mini-project (odproj/): candidate p4 = 17/18 functions 100%, matched_code 5695/5966 (+444 vs production
  5251); remaining <100%: _render_debug_profile_stall_tick 67.27% = PRE-EXISTING under-credit (strict-exact in production;
  our object carries compiler-local $L jump-table labels, split has none) -> semantic_matches.json entry.

## FINAL (2026-09-23)
Candidate: scratch/w/interface/cand_interface.c (== p4 with normal include == p5). gate_after.txt: exact 18/18
(`_point_from_line3d` guard passes); gate_before.txt: 17/18. Gain: _interface_get_weapon_hud_index 448 padded / 444
meaningful, 20 relocs, sha 0673f867... Park config/parked.json source/interface/interface:_interface_get_weapon_hud_index
reopen criterion met (independently attested storage class naturally selects the integer copy).
Patches: production.patch (interface.c: static prototype+definition for get_weapon_hud_index, render_debug_profile,
interface_splitscreen_render; profile_* six data definitions non-static), production_variantA_header.patch (removes the
interface.h prototype), symbols_static.patch (3 in-place "static": true flags). All `git apply --check` clean.
Audit: object_audit sections all ok; symbol DIFFs only the 3 storage rows that symbols_static.patch fixes; surplus
unchanged + identical + provider links PASS; data strict EQUAL; fake_match_scan 0 leads.
BLOCKER (owner ruling): Variant A (clean: header prototype removed) regresses rasterizer_frame_statistics_draw (4176 B,
currently EXACT) through a pure declaration-count tie (window = production count only; no genuine weight-3 compensating
declaration exists; field-type probe inert). Variant B (header untouched: production.patch alone) is exact with zero
blast radius but keeps a public interface.h prototype for a January-static function (extern-then-static linkage
conflict: MSVC C4211 at /W4, silent at /W3; UB in ISO C89 6.1.2.2) -> not self-admissible.
Separately (pre-existing, not caused here): objdiff 3.3.1 under-credits strict-exact _render_debug_profile_stall_tick
(67.27%, compiler-local $L switch labels) -> needs a config/semantic_matches.json entry for 100% report credit.
Verdict: FUNCTIONS_EXACT_OBJECT_BLOCKED.
