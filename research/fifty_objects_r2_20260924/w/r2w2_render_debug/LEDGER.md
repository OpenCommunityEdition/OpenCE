# LEDGER - r2w2_render_debug (admission packet 05/06 under existing precedent)

Worktree C:\halo-worktrees\claude-fifty-objects-r2-20260924, HEAD 434f0151 (tree = 05255584 + ledger doc).
Claim: source/render/render_debug label r2w2_render_debug (claimed OK).

## Step 0 - state
- git status: only `?? research/fifty_objects_r2_20260924/` (pre-existing untracked), no tracked modification.
- Since round-1 base cdc8ebd3: render_debug.c changed once (7979cf8f: +#include "cache/texture_cache.h");
  vehicle_datum.h unchanged; real_math.h changed in f6d00a8c (lock/unlock_global_random_seed prototype
  formatting only, point_from_line3d untouched); symbols.json changed by several commits (packet needs none).
- 05255584 made hs_runtime emit a _point_from_line3d COMDAT (render_debug_trigger_volumes, ruling #2 class).
- `git apply --check` 05 at HEAD: rc=0.
- Tools: tools/hsweep.py, surplus_identity_emu.py, pdb_storage_emu.py copied from scratch/w/xbox_texture_cache/tools
  (read in full: write only under this slug / read-only).

## Step 1 - /Od attestation of patch 05 helper calls (odbuild fn; outputs od_0x*.txt)
- 0x42e0d0 = point_from_line3d (/Od): result = v*t + p per component, returns result (real_math.h body order).
- 0x84a920 render_debug_vector: asserts at lines 0x184/0x185/0x186 = 388/389/390 (our match_assert lines: zero
  anchor drift), then REAL CALL point_from_line3d(point[ebp+0xc], vector[ebp+0x10], size[ebp+0x14], &end_point),
  then render_debug_line (0x848550). Frame: one 12-byte local.
- 0x84a510 render_debug_tick: two REAL CALLS point_from_line3d(point, tick_vector, tick_size, &point0) and
  (point, tick_vector, -tick_size [xorps sign mask], &point1); frame has only point0/point1 (no negative_tick_size local).
- 0x848640 render_debug_line_offset: REAL CALLS point_from_line3d(p0, global_up3d [0xa31e2c], offset, &point0) and
  (p1, global_up3d, offset, &point1).
=> the hand expansions in production are exactly what rule 6 forbids; the patch-05 calls are the /Od spelling.

## Step 2 - candidate files and shadow-tree full sweep
- files/base = LF HEAD blobs of render_debug.c + vehicle_datum.h; files/c05 = base + round-1 05 (GNU patch, all hunks
  offset +1 from the 7979cf8f include line, no fuzz); files/c06 = c05 + round-1 06.
- Gate path: gate.py cannot shadow the modified shared header (its /I order puts source/ first), so the candidate is
  compiled in shadow trees with tools/hsweep.py (cwd = tree root, same relative path and build.ninja cflags as
  production) and scored with tools/gate_obj.py (gate.py's row logic on a compiled object).
- CONTROL: tree0 (unmodified copy of source/+libs/) compiled 612 TUs, 612/612 IDENTICAL to build/base
  (sweep_control_vs_base.txt) => build/base current at HEAD and the shadow compile is faithful.
- FULL SWEEP (every TU in build.ninja): c05 vs control 611/612 identical, c06 vs control 611/612 identical; the only
  differing object is render_debug.obj (sweep_c05_vs_control.txt, sweep_c06_vs_control.txt). vehicle_datum.h
  includers actor_moving, physics, vehicles byte-identical. c06 vs c05: only the six COMMON symbol rows differ.
- gate: production 36/36; c05 36/36; c06 36/36 (gate_c05.txt, gate_c06.txt); per-function size/relocs/sha rows of
  c06 identical to production (gate_prod_obj.txt). Cross-check with the canonical gate.py on a LAB copy of c06 that
  defines the new macro locally (lab_gatepy_c06.c, never lands): 36/36 (gatepy_lab_c06.txt).

## Step 3 - section-4 audit (candidate c06; symbols.json unchanged -> build/split is the correct split)
- object_audit: PASS (63 January symbols, 0 differ); 27 data sections (26 .rdata + .bss 29708) all ok; the only
  new candidate-only row vs production is `+.text sym:_point_from_line3d size 48` (audit_c06.txt, audit_prod.txt).
- pdb_storage (emu, candidate): 63 split symbols, 0 disagreements. The six toggles are cachebeta PUBLICs
  (0x4EE350..0x4EE355) => external storage, consistent with non-static tentative definitions.
- surplus_identity (emu, candidate): 12 candidate-only code COMDATs, 0 not identical; _point_from_line3d IDENTICAL
  to January's only definer action_charge.obj (size 48, 0 relocs, sha 9b763841...; pfl3d_identity.txt).
- provider_link (candidate, all 39 surplus rows incl. literals/SDK tables): SELECTED-PROVIDER LINK PASS; new-only
  (--baseline production): _point_from_line3d vs action_charge PASS both orders (plink_c06_full.txt, plink_c06_new.txt).
- nodup_census: _point_from_line3d has 26 SELECT_ANY definers in build/base, 0 NODUP; the candidate copy is ANY.
- January: render_debug.obj holds NO _point_from_line3d reference (all-inlined class); single January definer
  action_charge; 17 out-of-line referencers, render_debug not among them (pfl_census_split.txt).
- mini objdiff 3.3.1 (mini/): production, c05, c06 all code 8235/9448 (34/36; the 2 known $L under-credit rows),
  data 30236/30236 => zero scorer delta.
- /W3: production 16 warnings, candidate 17; the one new line is real_math.h:764 C4244 (arctangent's inline body now
  visible because the dead REAL_MATH_EXTERNAL_ARCTANGENT define is gone; same class as the other 11 real_math.h
  C4244 lines every includer gets). C4013 x5 pre-existing (texture_cache_debug_render fixed by 7979cf8f).
- REAL_MATH_EXTERNAL_{ARCTANGENT,DOT_PRODUCT3D,PLANE3D_DISTANCE_TO_POINT}: render_debug.c uses none of the three;
  January, production and candidate objects reference none of _arctangent/_dot_product3d/_plane3d_distance_to_point
  => dead defines (added in d6250fa7); removal emits no COMDAT.

## Step 4 - patch 06 evidence re-verified at HEAD
- census_debug6_split.txt: each toggle is January linker_common .bss record 50..55 (1 byte), referenced only by
  hs_globals_external.obj and render_debug.obj; build/base has NO definer (production links leave them unresolved).
- c06 rows: six external section-0 symbols value 0 -> 1 (COMMON 1 = January record size); nothing else changes.
- linkprobe: c06 + January linker_common.obj both orders 0 LNK2005/LNK1169; c06 + base hs_globals_external both
  orders: 0 duplicates, 0 of 6 unresolved; CONTROL production / c05 + hs_globals_external: 6 unresolved.
- 2001 maps (earlier-map-archives): 08-15 beta 5 toggles (no permanent_decals) and render_debug.obj has no
  render_debug_structure_decals; 09-25 beta 6 toggles incl. _debug_permanent_decals + _render_debug_structure_decals
  (its only user) in render_debug.obj; 09-25 retail cache.map: render_debug.obj 0 rows and all six toggles ABSENT,
  while hs_globals_external.obj is linked AND still carries the "debug_bsp" name literal (??_C@_09LJAGFIKN@) and the
  neighbouring debug_objects/cached_object_render_states/_render records persist => hs_globals_external does not
  define them; they exist iff render_debug.obj is linked.
- /Od (od_hsglobals.txt, od_refs.txt): hs table records point to 0xf0afb0 (permanent_decals), b1 camera, b2 player,
  b3 structure, b6 bsp, b7 input; each read by exactly one render_debug function (0x84a1f0, 0x847660, 0x848d00
  ["riding an elevator"], 0x84a0e0, 0x847210 ["debug_bsp.txt"], 0x8481c0); 0xf0afb4/b8/bc/c0 referenced only by the
  render_debug cache functions 0x845810/0x845c00 ["render debug cache overflow."]/0x846350 => the six interleave with
  render_debug's own .bss in that build.
- Pool (linker_common_44_60.txt): render_sprite (46,47) -> render_objects (48,49) -> the six (50-55) -> render (_render
  56). Supporting only (adjacency is NOT relied on).
- NEW board census (hs_pair_census.txt): 74 January COMMON records are referenced by exactly {hs_globals_external, X}.
  Canonical defines 15 of them, ALL in X, all as bare tentative `boolean name;` in Matching objects (units.c x8,
  structure_visibility.c x2, objects.c x5); 59 have no definer; 0 are defined anywhere other than X. So 06 adds 6
  rows of an existing, uncontradicted canonical form. COMMON symbol-table order is not pool order in the admitted
  precedents either (objects.obj 104,102,100,99,98,...), so the declaration order is kept as in production.

## Step 5 - packaging
- patches/05_render_debug_genuine_helpers_and_vehicle.patch and patches/06_render_debug_tentative_debug_toggles.patch
  regenerated at HEAD (git diff --no-index of LF blobs; index c24b5b33 = HEAD blob). Change lines identical to round-1
  05/06; only hunk line numbers moved (+1 from 7979cf8f).
- git apply --check at repo root: 05 rc=0, 06 rc=0 (each alone). Sequential git apply 05 then 06 into slug copies
  (--directory, LF and CRLF inputs): rc 0/0, results == files/c06 modulo EOL (git writes CRLF under autocrlf=true).
- git status after all steps: only the pre-existing `?? research/fifty_objects_r2_20260924/`.
- Pruned the three shadow source trees (tree0/tree05/tree06; reproducible: `python tools/hsweep.py setup <t>` then
  overlay files/c05 or files/c06). Kept sweep/{control,c05,c06} objects, patches/, files/, all outputs.

## Step 6 - verdict
ADMISSION_FIX_PROPOSED (zero credit): 05 meets every condition of the rule-6 folded-inline exception
(house_rules:33-40, ruling 20260921 #2); 06 meets the objects.c/units.c/structure_visibility.c tentative-definition
precedent with per-symbol evidence (not adjacency). Object completion stays owner-gated on Q2d (D0/D1) + patch 08.
MANIFEST.md written.
