# cross_product2d provider unblock (slug scratch/w/cross_product2d)

Base 931ed8dc. Task: find a genuine admissible emission of the header-inline _cross_product2d in
ai/actor_combat so the NODUP hand copy (actor_combat.c ~523) can go, unblocking path_obstacle_avoidance,
path_smoothing, real_math, collision_features, path_structure_bsp.

## Findings / probes

### F0 prior evidence read
- claude_actor_aim_grenade_20260924.md: /Od fn 0x44ebb0 IS _actor_aim_grenade (RTC names aim_vector,
  new_desired_target, horizontal_aim_vector, new_aim_vector); its cross_product2d call is 0x44ed45.
  => the "vehicles worker's 0x44ebb0" is aim_grenade itself, not a second user.
- /Od cross_product2d = fn 0x455250 (thunk 0x408512). `odbuild callers 0x455250`: the ONLY caller in
  the actor_combat address range (0x44c000-0x453000) is 0x44ebb0 (aim_grenade). Next caller 0x4621c0.
- January section order (secorder.py): helper batch 1 [fast_ftol, sine, cosine, tangent, cross_product2d,
  add_vectors3d, random_boolean, valid_real] sits between _actor_get_grenade_definition and
  _actor_combat_find_nearby_target; batch 2 [valid_realcmp, valid_real_normal3d] between
  plan_grenade_trajectory and allow_special_fire_situation. Our build emits all header inlines first.

### F1 January bytes: aim_grenade is the ONLY 2D cross-product site in actor_combat
- disall.py over January actor_combat (jan_all.txt): every fsub/fsubp site checked. 2D pattern
  (fld [h.j]; fmul [f.i]; fld [h.i]; fmul [f.j]; fsubp) occurs ONLY in _actor_aim_grenade 0x105-0x120.
  aim_projectile 0x1d2-0x206 and start_burst 0x3fb-0x43e are 3D cross products (cross_product3d family).
- /Od: only 0x44ebb0 (aim_grenade) in the actor_combat range calls cross_product2d (0x455250).
- No other real_math.h __inline calls cross_product2d (grep), so no transitive genuine reference exists.
=> No EXACT actor_combat function other than the held aim_grenade can genuinely emit _cross_product2d.

### F2 _add_vectors3d / _fast_ftol: NOT hand copies, do not matter
- actor_combat.c has no rename/hand copy for either; base obj emits both SELECT_ANY from the header.
- helper_ident.py: _add_vectors3d 17 board definers, _fast_ftol 25 definers, ALL section_infos_equal to
  January's actor_combat copy; nodup_census: 0 NODUP definers. They block nothing.

### F3 baseline provider_link (production objs): all five consumers FAIL(1) solely on _cross_product2d
  (LNK2005 vs actor_combat NODUP). Every other surplus PASS.

### P1 minimal genuine restore (p1.c / p1.obj) - MEASURED
- Hypothesis: /Od 0x44ed38-0x44ed45 pushes [ebp-0x44] (horizontal_aim_vector, 8 B RTC) then actor+0x140
  (input.facing_vector, 3D) and calls cross_product2d => canonical's hand-expanded
  `aim_vector2d.j*facing.i - aim_vector2d.i*facing.j` IS cross_product2d(&facing, &aim_vector2d); January
  0x105-0x120 is exactly that inline (h.j*f.i - h.i*f.j). Restoring the call (and nothing else) should keep
  every byte and turn _cross_product2d into the header SELECT_ANY copy.
- Change vs production: drop `#define cross_product2d cross_product2d_inline` + `#undef`, drop the hand copy
  (lines 523-528), and in actor_aim_grenade
  `boolean counterclockwise = cross_product2d((real_vector2d const *)&actor->input.facing_vector, &aim_vector2d) > 0.0f;`
- Result: gate 32 EXACT + same 2 pre-existing residuals (aim_grenade size 576!=544, plan_grenade_trajectory sha).
  objeq base.obj vs p1.obj: 81 sections compared, 0 changed, same symbol set. ONLY delta:
  _cross_product2d COMDAT selection NODUP -> ANY (bytes equal January).
- Strip test (p1_strip.c, cast removed): 81/81 sections identical to p1 -> the view cast is byte-inert.
- provlink2 --subst actor_combat:p1.obj on the five consumers: ALL PASS (was FAIL(1) each).
- Rule status: the header copy is emitted from a NON-exact caller (aim_grenade residual 576 vs 544, bytes
  unchanged by this edit) and adds one per-site /Od-attested byte-inert 3D->2D view cast in that caller.
  Brief rule 6 / view-cast rule both say "strictly exact caller" -> OWNER-GATED, report don't land.

### F4 consumers re-verified at 931ed8dc (production source, gate --all + object_audit + pdb_storage + surplus_identity)
- path_obstacle_avoidance 24/24, audit PASS, 0 PDB disagreements, 11 surplus 0 not identical.
- path_smoothing 6/6, audit PASS, 0 PDB, 10 surplus identical.
- real_math 84/84, audit PASS, 0 PDB, 35 surplus identical.
- collision_features 20/20, audit PASS, 0 PDB, 29 surplus identical. Its admission-rejection entry
  (candidate-only _projection_from_vector3d) is STALE: copy IDENTICAL to January decals, provider link PASS.
- path_structure_bsp 6/6 code BUT object_audit FAIL(2): the 64-byte .rdata table _rdata_0024ec0c is
  unwritten (known: path_structure_bsp_codex_salvage.md - natural declaration got 8-byte section alignment
  vs January 4). => path_structure_bsp is NOT unlocked by cross_product2d alone.
- January helper out-of-line refs in actor_combat: only _valid_real_normal3d (aim_projectile, aim_grenade,
  update); every other emitted helper (incl. cross_product2d) is all-inlined.

### P1 warnings + patch
- warn.py (/Zs /W3, real cflags): production and p1.c have IDENTICAL warning multisets (no new warning);
  p1_strip.c adds exactly one C4133 (real_vector3d* -> const real_vector2d*) => the cast is type hygiene,
  byte-inert.
- Patch: actor_combat_cross_product2d.patch (git apply --check OK at 931ed8dc). Full file: p1.c.
- Other _cross_product2d definers after P1: 11 board definers all SELECT_ANY and section_infos_equal to
  January (helper_ident.py) -> 0 NODUP definers left.

### L1 side lead (path_structure_bsp data blocker; NOT in this slug's scope, zero credit)
- January _rdata_0024ec0c: 64 B, 0 relocs, no code reference, not a cachebeta public (file-static), section
  align 4. Values = [0.2,0.4,0.6,0.8,1,1.5,2,4] ++ [0.25,0.5,0.75,1,1.5,2,4,8].
- lab/two.c vs lab/one.c (VC7, /O2 /Oy-): two `static const real t[8]` -> ONE 64-byte .rdata section, align 4
  (= January); one `static const real t[16]` -> align 8 (= the recorded failure). So the 8-byte alignment
  defect is explained by the aggregate-size oracle (size>=64 -> 8), NOT a need for __declspec(align(4)):
  January's storage was >=2 aggregates each <64 B. Exact split + authentic names unknown (/Od exe has no
  copy of either 8-float run: tables are dead by the later build). Needs HCEX/compiland evidence for names.

### PSB1/PSB2 path_structure_bsp data table (closes the 5th object's remaining blocker) - MEASURED
- HCEX.pdb `-compiland ..\..\..\build\x360\SHIP\halo\path_structure_bsp.obj`:
  `Data: static [000C5494] File Static, const float[0x8], quantized_pathfinding_surface_widths` and
  `[000C54B4] ... quantized_pathfinding_surface_heights`.
- Values attested (DATA read, big-endian) in HCEX_debug.exe at exactly 0xC5494 = [0.2,0.4,0.6,0.8,1,1.5,2,4]
  (widths) and 0xC54B4 = [0.25,0.5,0.75,1,1.5,2,4,8] (heights); same adjacency in HCEX_release.exe 0x129e64.
  January 0x24ec0c holds widths then heights = VC7 definition order (lab/two.c: first-defined static at +0).
- psb1.c: two `static const real ...[] = {...}` in the globals section; psb2.c = psb1 + the generated
  header symbol comment updated (byte-inert: 27/27 sections equal psb1).
- gate: 6/6 EXACT (unchanged; +2 file-scope names flip no tie).
- scratch csplit (build/tools/csplit.exe -p scratch/w/cross_product2d/config -o .../split) with symbols.json
  line 13113 replaced by widths@2419724 + heights@2419756 ("static": true): 833 objects, ONLY
  path_structure_bsp.obj differs from build/split. Its .rdata [7] = 64 B, flags 0x40300040, 0 relocs,
  widths@0 static + heights@0x20 static == candidate exactly (bytes, flags, storage, offsets).
- object_audit (SPLIT_ROOT=scratch split): 13 January symbols, 0 differ, OBJECT AUDIT PASS.
- provlink2 psb2.obj + --subst actor_combat:p1.obj: PASS (without p1: FAIL(1) _cross_product2d).
- Patches: path_structure_bsp_tables.patch, symbols_path_structure_bsp.patch (git apply --check OK);
  full symbols copy: scratch/w/cross_product2d/config/symbols.json (one line replaced by two, CRLF kept).

### objdiff 3.3.1 (scratch project scratch/w/cross_product2d/objdiff/, report.json)
- path_structure_bsp: production 2512/2512 code, data 0/200 (the merged .rdata group fails on the missing
  table) -> psb2 + scratch split: 2512/2512 code, data 200/200. Expected gain: +200 data bytes.
- actor_combat: production vs p1 IDENTICAL (6687/12175 code, 31/34 fns, 760/760 data).

## Summary / verdicts
- actor_combat P1 (owner-gated): byte-inert on all 81 sections; only _cross_product2d NODUP->ANY. Removes the
  rule-6 hand copy and the hand expansion at the /Od-attested site. Fails ONLY the "strict-exact caller" clause
  of ruling 2 (COMDAT) and ruling 3 (facing 3D->2D view cast, byte-inert per strip test). Needs owner ruling.
  Reopen (no ruling needed) if _actor_aim_grenade ever goes strict exact with the /Od helper calls.
- With P1: path_obstacle_avoidance, path_smoothing, real_math, collision_features link-clean -> complete
  candidates (audit PASS, data 100%); collision_features' _projection_from_vector3d rejection is stale.
- path_structure_bsp: needs P1 AND the independently landable table fix (psb2 + symbols edit).
- STOP: no genuine EXACT alternative user exists (F1); aim_grenade exactness is held (recorded negatives).
